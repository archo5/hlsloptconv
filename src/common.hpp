

#pragma once
#define _HAS_EXCEPTIONS 0
#include <assert.h>
#include <vector>

#define HOC_INTERNAL
#include "hlsloptconv.h"


namespace HOC {


using ShaderStage = HOC_ShaderStage;
using OutputShaderFormat = HOC_OutputShaderFormat;
using ShaderVarType = HOC_ShaderVarType;
using ShaderDataType = HOC_ShaderDataType;
using ShaderVariable = HOC_ShaderVariable;
using ShaderMacro = HOC_ShaderMacro;
using LoadIncludeFilePFN = HOC_LoadIncludeFilePFN;


#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif


#ifdef _MSC_VER
#  define FINLINE __forceinline
#else
#  define FINLINE __attribute__((always_inline))
#endif

#define STRLIT_SIZE(s) s, (sizeof(s)-1)


#define SMALL_STRING_BUFSZ 16
struct String
{
	static const size_t npos = -1;

	char* _str;
	size_t _size = 0;
	size_t _cap = 0;
	char _buf[SMALL_STRING_BUFSZ];

	FINLINE String() : _str(_buf){ _buf[0] = 0; }
	String(const char* s) : _str(_buf){ _buf[0] = 0; append(s); }
	String(const char* s, size_t sz) : _str(_buf){ _buf[0] = 0; append(s, sz); }
	String(const String& s) : _str(_buf){ _buf[0] = 0; append(s._str, s._size); }
	String(String&& s) : _str(s._str != s._buf ? s._str : _buf), _size(s._size), _cap(s._cap)
	{
		if (!_cap)
			memcpy(_buf, s._buf, SMALL_STRING_BUFSZ);
		s._cap = 0;
	}
	~String()
	{
		if (_cap)
			delete [] _str;
	}

	FINLINE String& operator = (const String& o)
	{
		_size = 0;
		append(o._str, o._size);
		return *this;
	}
	String& operator = (String&& o)
	{
		if (_cap)
			delete [] _str;
		_str = o._str != o._buf ? o._str : _buf;
		_size = o._size;
		_cap = o._cap;
		if (!_cap)
			memcpy(_buf, o._buf, SMALL_STRING_BUFSZ);
		o._cap = 0;
		return *this;
	}

	FINLINE char operator [] (size_t i) const { assert(i < _size); return _str[i]; }
	FINLINE char& operator [] (size_t i)      { assert(i < _size); return _str[i]; }
	FINLINE char back() const                 { assert(_size); return _str[_size - 1]; }
	FINLINE char& back()                      { assert(_size); return _str[_size - 1]; }
	FINLINE const char* data() const          { return _str; }
	FINLINE const char* c_str() const         { return _str; }
	FINLINE size_t size() const               { return _size; }
	FINLINE bool empty() const                { return !_size; }
	FINLINE char* begin()                     { return _str; }
	FINLINE const char* begin() const         { return _str; }
	FINLINE char* end()                       { return _str + _size; }
	FINLINE const char* end() const           { return _str + _size; }
	void clear()
	{
		_size = 0;
		_str[0] = 0;
	}
	void reserve(size_t nsz)
	{
		if (nsz < SMALL_STRING_BUFSZ || nsz < _cap)
			return;
		char* nstr = new char[nsz + 1];
		memcpy(nstr, _str, _size + 1);
		_cap = nsz;
		if (_str != _buf)
			delete [] _str;
		_str = nstr;
	}
	void resize(size_t nsz)
	{
		reserve(nsz);
		_size = nsz;
		_str[nsz] = '\0';
	}
	void _resize_loose(size_t nsz)
	{
		if (nsz > _cap)
			reserve(_size + nsz);
		_size = nsz;
		_str[nsz] = '\0';
	}

	String substr(size_t from, size_t num = npos) const
	{
		assert(from <= _size);
		if (num > _size - from)
			num = _size - from;
		return String(_str + from, num);
	}
	void replace(size_t from, size_t num, const String& src)
	{
		assert(from <= _size && num <= _size && from + num <= _size);
		if (num != src._size)
		{
			size_t oldsz = _size;
			_resize_loose(_size - num + src._size);
			if (from + num < oldsz)
				memmove(_str + from + num, _str + from + src._size, oldsz - (from + num));
		}
		memcpy(_str + from, src._str, src._size);
	}
	size_t _find(const char* substr, size_t subsize, size_t from = 0) const
	{
		if (_size < subsize)
			return npos;
		size_t end = _size - subsize;
		for (size_t i = from; i <= end; ++i)
		{
			if (!memcmp(_str + i, substr, subsize))
				return i;
		}
		return npos;
	}
	FINLINE size_t find(const String& substr, size_t from = 0) const
	{
		return _find(substr._str, substr._size, from);
	}
	FINLINE size_t find(const char* substr, size_t from = 0) const
	{
		return _find(substr, strlen(substr), from);
	}
	int compare(const String& o) const
	{
		size_t end = _size < o._size ? _size : o._size;
		for (size_t i = 0; i < end; ++i)
		{
			if (_str[i] != o._str[i])
				return _str[i] - o._str[i];
		}
		if (_size != o._size)
			return _size < o._size ? -1 : 1;
		return 0;
	}
	bool operator == (const String& o) const
	{
		return _size == o._size && !memcmp(_str, o._str, _size);
	}
	bool operator == (const char* o) const
	{
		for (size_t i = 0; i < _size; ++i)
		{
			if (!o[i] || o[i] != _str[i])
				return false;
		}
		return !o[_size];
	}
	FINLINE bool operator != (const String& o) const { return !(*this == o); }
	FINLINE bool operator != (const char* o) const { return !(*this == o); }

	void append(const String& s)
	{
		append(s._str, s._size);
	}
	void append(const char* s)
	{
		append(s, strlen(s));
	}
	void append(const char* s, size_t sz)
	{
		size_t oldsz = _size;
		_resize_loose(oldsz + sz);
		memcpy(_str + oldsz, s, sz);
	}
	String& operator += (const String& o)
	{
		append(o._str, o._size);
		return *this;
	}
	String& operator += (const char* o)
	{
		append(o);
		return *this;
	}
	String& operator += (char o)
	{
		_resize_loose(_size + 1);
		_str[_size - 1] = o;
		return *this;
	}
	String operator + (const String& o) const
	{
		String out;
		out._resize_loose(_size + o._size);
		memcpy(out._str, _str, _size);
		memcpy(out._str + _size, o._str, o._size);
		return out;
	}
	String operator + (const char* o) const
	{
		size_t osz = strlen(o);
		String out;
		out._resize_loose(_size + osz);
		memcpy(out._str, _str, _size);
		memcpy(out._str + _size, o, osz);
		return out;
	}
	friend String operator + (const char* a, const String& b);
};
inline String operator + (const char* a, const String& b)
{
	size_t asz = strlen(a);
	String out;
	out._resize_loose(asz + b._size);
	memcpy(out._str, a, asz);
	memcpy(out._str + asz, b._str, b._size);
	return out;
}
inline String StdToString(int val)
{
	char bfr[16];
	sprintf(bfr, "%d", val);
	return bfr;
}
inline String StdToString(size_t val)
{
	char bfr[32];
	sprintf(bfr, "%zu", val);
	return bfr;
}


} /* namespace HOC */
template<>
struct std::hash<HOC::String>
{
	FINLINE size_t operator () (const HOC::String& s) const
	{
		uint32_t hash = 2166136261U;
		for (size_t i = 0; i < s.size(); ++i)
		{
			hash ^= s[i];
			hash *= 16777619U;
		}
		return hash;
	}
};
namespace HOC {


struct Twine
{
	const char* _cstr  = nullptr;
	const String* _str = nullptr;
	const Twine* _lft  = nullptr;
	const Twine* _rgt  = nullptr;

	FINLINE Twine(){}
	FINLINE Twine(const char* cs) : _cstr(cs) {}
	FINLINE Twine(const String& s) : _str(&s) {}

	size_t size() const
	{
		size_t out = 0;
		if (_cstr)
			out += strlen(_cstr);
		if (_str)
			out += _str->size();
		if (_lft)
			out += _lft->size();
		if (_rgt)
			out += _rgt->size();
		return out;
	}
	String str() const
	{
		String out;
		append_to(out);
		return out;
	}
	void append_to(String& out) const
	{
		out.reserve(out.size() + size());
		_append_to_nsz(out);
	}
	void _append_to_nsz(String& out) const
	{
		if (_cstr)
			out.append(_cstr);
		if (_str)
			out.append(*_str);
		if (_lft)
			_lft->_append_to_nsz(out);
		if (_rgt)
			_rgt->_append_to_nsz(out);
	}
};
FINLINE Twine operator + (const Twine& a, const Twine& b)
{
	Twine out;
	out._lft = &a;
	out._rgt = &b;
	return out;
}


struct OutStream
{
	virtual void Write(const char* str, size_t size) = 0;
	virtual void Write(const char* str) { Write(str, strlen(str)); }
	virtual void Flush() = 0;

	FINLINE OutStream& operator << (const char* str) { Write(str); return *this; }
	FINLINE OutStream& operator << (char v) { Write(&v, 1); return *this; }
	FINLINE OutStream& operator << (unsigned char v) { Write((char*) &v, 1); return *this; }
	OutStream& operator << (short v);
	OutStream& operator << (unsigned short v);
	OutStream& operator << (int v);
	OutStream& operator << (unsigned int v);
	OutStream& operator << (long v);
	OutStream& operator << (unsigned long v);
	OutStream& operator << (long long v);
	OutStream& operator << (unsigned long long v);
	OutStream& operator << (float v);
	OutStream& operator << (double v);
	OutStream& operator << (bool v);
	OutStream& operator << (const void* v);
	OutStream& operator << (const String& v);
	OutStream& operator << (const Twine& v);
};

struct StringStream : OutStream
{
	StringStream(size_t sz = 0) { strbuf.reserve(sz); }
	void Write(const char* str, size_t size) override;
	void Write(const char* str) override;
	void Flush() override {}
	StringStream& Reserve(size_t sz) { strbuf.reserve(sz); return *this; }
	String& str() { return strbuf; }
	const String& str() const { return strbuf; }

	String strbuf;
};

struct FILEStream : OutStream
{
	FINLINE FILEStream(FILE* f) : file(f) {}
	void Write(const char* str, size_t size) override;
	void Flush() override { fflush(file); }

	FILE* file;
};

struct CallbackStream : OutStream
{
	CallbackStream(HOC_TextOutput* to) : textOut(to){}
	void Write(const char* str, size_t size) override;
	void Flush() override {}

	HOC_TextOutput* textOut;
};


double GetTime();
String GetFileContents(const char* filename, bool text = false);
void SetFileContents(const char* filename, const String& contents, bool text = false);


template<class T> struct Cleanup
{
	T fn;
	Cleanup(T&& infn) : fn(std::move(infn)) {}
	~Cleanup()
	{
		fn();
	}
};


struct Location
{
	static Location BAD() { return { 0xffffffffU, 0xffffffffU, 0xffffffffU }; }

	bool operator == (const Location& o) const
	{
		return source == o.source
			&& line == o.line
			&& off == o.off;
	}
	bool operator != (const Location& o) const { return !(*this == o); }

	uint32_t source;
	uint32_t line;
	uint32_t off;
};


struct Diagnostic
{
	Diagnostic(OutStream* eos, const char* src);
	uint32_t GetSourceID(const String& src);
	void PrintMessage(const char* type, const Twine& msg, const Location& loc);
	void EmitError(const Twine& msg, const Location& loc);

	void PrintError(const Twine& msg, const Location& loc) { PrintMessage("error", msg, loc); }
	void PrintWarning(const Twine& msg, const Location& loc) { PrintMessage("warning", msg, loc); }

	OutStream* errorOutputStream;
	std::vector<String> sourceFiles;
	bool hasErrors = false;
	bool hasFatalErrors = false;
};


#define SWIZZLE_NONE uint32_t(-1)
//#vector:
// generate = (comp[n] << n*2) | ..
// retrieve = (value >> n*2) & 0x3
//#matrix:
// generate = (comp[n] << n*4) | ..
// retrieve = (value >> n*4) & 0xf
#define MATRIX_SWIZZLE(x, y, z, w) (((x)&0xf) | (((y)&0xf)<<4) | (((z)&0xf)<<8) | (((w)&0xf)<<12))

bool IsValidSwizzleWriteMask(uint32_t mask, bool matrix, int ncomp);


} /* namespace HOC */

