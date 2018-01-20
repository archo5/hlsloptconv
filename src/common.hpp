

#pragma once
#include <cassert>
#include <string>
#include <vector>


#ifdef _MSC_VER
#  define FINLINE __forceinline
#else
#  define FINLINE __attribute__((always_inline))
#endif

#define STRLIT_SIZE(s) s, (sizeof(s)-1)


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
	OutStream& operator << (const std::string& v);
};

struct StringStream : OutStream
{
	void Write(const char* str, size_t size) override;
	void Write(const char* str) override;
	void Flush() override {}
	const std::string& str() const { return strbuf; }

	std::string strbuf;
};

struct FILEStream : OutStream
{
	FINLINE FILEStream(FILE* f) : file(f) {}
	void Write(const char* str, size_t size) override;
	void Flush() override { fflush(file); }

	FILE* file = nullptr;
};


double GetTime();
std::string GetFileContents(const char* filename, bool text = false);
void SetFileContents(const char* filename, const std::string& contents, bool text = false);


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

	bool operator == (const Location& o) const { return source == o.source && line == o.line && off == o.off; }
	bool operator != (const Location& o) const { return !(*this == o); }

	uint32_t source;
	uint32_t line;
	uint32_t off;
};


// exceptions
struct NonFatalErrors {};
struct FatalError
{
	std::string msg;
	Location loc;
};


struct Diagnostic
{
	Diagnostic(OutStream* eos, const char* src);
	uint32_t GetSourceID(const std::string& src);
	void PrintMessage(const char* type, const std::string& msg, const Location& loc);
	void EmitError(const std::string& msg, const Location& loc);
	void EmitFatalError(const std::string& msg, const Location& loc);
	void CheckNonFatalErrors();

	void PrintError(const std::string& msg, const Location& loc) { PrintMessage("error", msg, loc); }
	void PrintWarning(const std::string& msg, const Location& loc) { PrintMessage("warning", msg, loc); }

	OutStream* errorOutputStream;
	std::vector<std::string> sourceFiles;
	bool hasErrors = false;
};


#define SWIZZLE_NONE uint32_t(-1)
//#vector:
// generate = (comp[n] << n*2) | ..
// retrieve = (value >> n*2) & 0x3
//#matrix:
// generate = (comp[n] << n*4) | ..
// retrieve = (value >> n*4) & 0xf
#define MATRIX_SWIZZLE(x, y, z, w) (((x)&0xf) | (((y)&0xf)<<4) | (((z)&0xf)<<8) | (((w)&0xf)<<12))

uint8_t InvertVecSwizzleMask(uint8_t mask, int ncomp);
bool IsValidVecSwizzleWriteMask(uint8_t mask, int ncomp);
bool IsValidMtxSwizzleWriteMask(uint16_t mask, int ncomp);
bool IsValidSwizzleWriteMask(uint32_t mask, bool matrix, int ncomp);


enum ShaderStage
{
	ShaderStage_Vertex,
	ShaderStage_Pixel,
};

struct ShaderMacro
{
	const char* name;
	const char* value;
};


// API
// loads include file
// - file is the exact string in #include, requester is the exact string used to load file that contains #include
// - called with file=NULL to free memory pointed to by *outbuf
// - return nonzero for success, zero for failure
typedef int(*LoadIncludeFilePFN)(const char* file, const char* requester, char** outbuf, void* userdata);

