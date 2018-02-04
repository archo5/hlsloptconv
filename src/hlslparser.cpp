

#include "hlslparser.hpp"

#include <cstring>
#include <cmath>


static inline bool isStr(const char* begin, const char* end, const char* str, size_t sz)
{
	return end - begin == sz && memcmp(begin, str, sz) == 0;
}
template<size_t N> static inline bool isStr(const char* begin, const char* end, const char(&str)[N])
{
	return isStr(begin, end, str, N - 1);
}

static int strtonum_hex(const char** at, int64_t* outi)
{
	int64_t val = 0;
	const char* str = *at + 2;
	while ((*str >= '0' && *str <= '9')
		|| (*str >= 'a' && *str <= 'f')
		|| (*str >= 'A' && *str <= 'F'))
	{
		val *= 16;
		if (*str >= '0' && *str <= '9')
			val += *str - '0';
		else if (*str >= 'a' && *str <= 'f')
			val += *str - 'a' + 10;
		else
			val += *str - 'A' + 10;
		str++;
	}
	*at = str;
	*outi = val;
	return 1;
}

static int strtonum_oct(const char** at, int64_t* outi)
{
	int64_t val = 0;
	const char* str = *at + 2;
	while (*str >= '0' && *str <= '7')
	{
		val *= 8;
		val += *str - '0';
		str++;
	}
	*at = str;
	*outi = val;
	return 1;
}

static int strtonum_bin(const char** at, int64_t* outi)
{
	int64_t val = 0;
	const char* str = *at + 2;
	while (*str == '0' || *str == '1')
	{
		val *= 2;
		val += *str - '0';
		str++;
	}
	*at = str;
	*outi = val;
	return 1;
}

static int strtonum_real(const char** at, double* outf)
{
	double val = 0;
	double vsign = 1;
	const char* str = *at, *teststr;
	if (!*str)
		return 0;

	if (*str == '+') str++;
	else if (*str == '-') { vsign = -1; str++; }

	teststr = str;
	while (*str >= '0' && *str <= '9')
	{
		val *= 10;
		val += *str - '0';
		str++;
	}
	if (str == teststr)
		return 0;
	if (!*str)
		goto done;
	if (*str == '.')
	{
		double mult = 1.0;
		str++;
		while (*str >= '0' && *str <= '9')
		{
			mult /= 10;
			val += (*str - '0') * mult;
			str++;
		}
	}
	if (*str == 'e' || *str == 'E')
	{
		double sign, e = 0;
		str++;
		if (*str != '+' && *str != '-')
			goto done;
		sign = *str++ == '-' ? -1 : 1;
		while (*str >= '0' && *str <= '9')
		{
			e *= 10;
			e += *str - '0';
			str++;
		}
		val *= pow(10, e * sign);
	}
	// TODO double?
	if (*str == 'f')
	{
		str++;
	}

done:
	*outf = val * vsign;
	*at = str;
	return 2;
}

static int strtonum_dec(const char** at, int64_t* outi, double* outf)
{
	const char* str = *at, *teststr;
	if (*str == '+' || *str == '-') str++;
	teststr = str;
	while (*str >= '0' && *str <= '9')
		str++;
	if (str == teststr)
		return 0;
	if (*str == '.' || *str == 'E' || *str == 'e')
		return strtonum_real(at, outf);
	else
	{
		int64_t val = 0;
		int invsign = 0;

		str = *at;
		if (*str == '+') str++;
		else if (*str == '-') { invsign = 1; str++; }

		while (*str >= '0' && *str <= '9')
		{
			val *= 10;
			val += *str - '0';
			str++;
		}
		if (invsign) val = -val;
		*outi = val;
		*at = str;
		return 1;
	}
}

int util_strtonum(const char** at, int64_t* outi, double* outf)
{
	const char* str = *at;
	if (!*str)
		return 0;
	if (*str == '0')
	{
		if (str[1] == 'x') return strtonum_hex(at, outi);
		else if (str[1] == 'o') return strtonum_oct(at, outi);
		else if (str[1] == 'b') return strtonum_bin(at, outi);
	}
	return strtonum_dec(at, outi, outf);
}

struct OperatorInfo
{
	SLTokenType token;
	const char* text;
};
static const OperatorInfo g_allOperatorsResolveOrder[] =
{
	// 3
	{ STT_OP_LshEq, "<<=" },
	{ STT_OP_RshEq, ">>=" },
	// 2
	{ STT_OP_Eq, "==" },
	{ STT_OP_NEq, "!=" },
	{ STT_OP_LEq, "<=" },
	{ STT_OP_GEq, ">=" },
	{ STT_OP_AddEq, "+=" },
	{ STT_OP_SubEq, "-=" },
	{ STT_OP_MulEq, "*=" },
	{ STT_OP_DivEq, "/=" },
	{ STT_OP_ModEq, "%=" },
	{ STT_OP_AndEq, "&=" },
	{ STT_OP_OrEq, "|=" },
	{ STT_OP_XorEq, "^=" },
	{ STT_OP_LogicalAnd, "&&" },
	{ STT_OP_LogicalOr, "||" },
	{ STT_OP_Lsh, "<<" },
	{ STT_OP_Rsh, ">>" },
	{ STT_OP_Inc, "++" },
	{ STT_OP_Dec, "--" },
	// 1
	{ STT_OP_Less, "<" },
	{ STT_OP_Greater, ">" },
	{ STT_OP_Assign, "=" },
	{ STT_OP_Add, "+" },
	{ STT_OP_Sub, "-" },
	{ STT_OP_Mul, "*" },
	{ STT_OP_Div, "/" },
	{ STT_OP_Mod, "%" },
	{ STT_OP_And, "&" },
	{ STT_OP_Or, "|" },
	{ STT_OP_Xor, "^" },
	{ STT_OP_Member, "." },
	{ STT_OP_Not, "!" },
	{ STT_OP_Inv, "~" },
	{ STT_OP_Ternary, "?" },
};

void Parser::ParseCode(const char* text)
{
	ParseTokens(text, 0);
	PreprocessTokens(0);

//	FILEStream err(stderr);
//	for (size_t i = 0; i < tokens.size(); ++i)
//		err << " " << TokenToString(i);
//	err << "\n";

	ast.InitBasicTypes();
	while (curToken < tokens.size())
		ParseDecl();
	diag.CheckNonFatalErrors();

	if (entryPointCount == 0)
	{
		diag.EmitFatalError("entry point '" + entryPointName + "' was not found",
			Location::BAD());
	}
	if (entryPointCount > 1)
	{
		diag.EmitFatalError("too many functions named '" + entryPointName + "', expected one entry point",
			Location::BAD());
	}
}

void Parser::ParseTokens(const char* text, uint32_t source)
{
	uint32_t line = 1;
	const char* lineStart = text;
#define LOC(tp) { 0, line, uint32_t((tp) - lineStart) + 1 }
#define TLOC(tp) LOC(tp), line

	while (*text)
	{
		// space
		if (*text == ' ' || *text == '\t')
		{
			text++;
			continue;
		}
		// newline
		bool isCR = *text == '\r';
		if (isCR || *text == '\n')
		{
			line++;
			text++;
			if (isCR && *text == '\n')
				text++;
			lineStart = text;
			continue;
		}

		// comments
		if (*text == '/' && text[1] == '/')
		{
			text += 2;
			while (*text && *text != '\r' && *text != '\n')
				*text++;
			if (*text)
			{
				line++;
				isCR = *text++ == '\r';
				if (isCR && *text == '\n')
					text++;
				lineStart = text;
			}
			continue;
		}
		if (*text == '/' && text[1] == '*')
		{
			text += 2;
			while (*text && !(*text == '*' && text[1] == '/'))
				text++;
			if (*text)
				text += 2;
			else
				diag.PrintWarning("reached end of file while in a multiline comment", Location::BAD());
			continue;
		}

		if (*text == '\"')
		{
			const char* slStart = text++;
			uint32_t dataOff = tokenData.size();
			tokenData.insert(tokenData.end(), { 0, 0, 0, 0 });
			while (*text)
			{
				if (*text == '\"')
				{
					text++;
					break;
				}
				else if (*text == '\\')
				{
					text++;
					if (*text == '\"') tokenData.push_back('\"');
					else if (!*text)
					{
						tokenData.push_back('\\');
						continue;
					}
					else tokenData.push_back(*text);
				}
				else tokenData.push_back(*text);
				text++;
			}
			tokenData.push_back(0);
			uint32_t realSize = tokenData.size() - dataOff - 4 - 1;
			memcpy(&tokenData[dataOff], &realSize, 4);
			tokens.push_back({ STT_StrLit, TLOC(slStart), dataOff });
			continue;
		}

		// special characters
		SLTokenType specCharToken;
		switch (*text)
		{
		case '(': specCharToken = STT_LParen;    goto addedSpecChar;
		case ')': specCharToken = STT_RParen;    goto addedSpecChar;
		case '[': specCharToken = STT_LBracket;  goto addedSpecChar;
		case ']': specCharToken = STT_RBracket;  goto addedSpecChar;
		case '{': specCharToken = STT_LBrace;    goto addedSpecChar;
		case '}': specCharToken = STT_RBrace;    goto addedSpecChar;
		case ',': specCharToken = STT_Comma;     goto addedSpecChar;
		case ';': specCharToken = STT_Semicolon; goto addedSpecChar;
		case ':': specCharToken = STT_Colon;     goto addedSpecChar;
		case '#': specCharToken = STT_Hash;      goto addedSpecChar;
		default: break;
		addedSpecChar:
			tokens.push_back({ specCharToken, TLOC(text), 0 });
			text++;
			continue;
		}

		// identifier
		if ((*text >= 'a' && *text <= 'z') ||
			(*text >= 'A' && *text <= 'Z') ||
			*text == '_')
		{
			const char* idStart = text++;
			while ((*text >= 'a' && *text <= 'z') ||
				(*text >= 'A' && *text <= 'Z') ||
				(*text >= '0' && *text <= '9') ||
				*text == '_')
				text++;

			if (isStr(idStart, text, "true"))
			{
				tokens.push_back({ STT_BoolLit, TLOC(idStart), 1 });
				continue;
			}
			if (isStr(idStart, text, "false"))
			{
				tokens.push_back({ STT_BoolLit, TLOC(idStart), 0 });
				continue;
			}

			SLTokenType specKwToken;
			if(isStr(idStart, text, "struct"  )){ specKwToken = STT_KW_Struct;   goto addedSpecKw; }
			if(isStr(idStart, text, "return"  )){ specKwToken = STT_KW_Return;   goto addedSpecKw; }
			if(isStr(idStart, text, "discard" )){ specKwToken = STT_KW_Discard;  goto addedSpecKw; }
			if(isStr(idStart, text, "break"   )){ specKwToken = STT_KW_Break;    goto addedSpecKw; }
			if(isStr(idStart, text, "continue")){ specKwToken = STT_KW_Continue; goto addedSpecKw; }
			if(isStr(idStart, text, "if"      )){ specKwToken = STT_KW_If;       goto addedSpecKw; }
			if(isStr(idStart, text, "else"    )){ specKwToken = STT_KW_Else;     goto addedSpecKw; }
			if(isStr(idStart, text, "while"   )){ specKwToken = STT_KW_While;    goto addedSpecKw; }
			if(isStr(idStart, text, "do"      )){ specKwToken = STT_KW_Do;       goto addedSpecKw; }
			if(isStr(idStart, text, "for"     )){ specKwToken = STT_KW_For;      goto addedSpecKw; }
			if(isStr(idStart, text, "in"      )){ specKwToken = STT_KW_In;       goto addedSpecKw; }
			if(isStr(idStart, text, "out"     )){ specKwToken = STT_KW_Out;      goto addedSpecKw; }
			if(isStr(idStart, text, "inout"   )){ specKwToken = STT_KW_InOut;    goto addedSpecKw; }
			if(isStr(idStart, text, "const"   )){ specKwToken = STT_KW_Const;    goto addedSpecKw; }
			if(isStr(idStart, text, "static"  )){ specKwToken = STT_KW_Static;   goto addedSpecKw; }
			if(isStr(idStart, text, "uniform" )){ specKwToken = STT_KW_Uniform;  goto addedSpecKw; }
			if(isStr(idStart, text, "cbuffer" )){ specKwToken = STT_KW_CBuffer;  goto addedSpecKw; }
			if(isStr(idStart, text, "register")){ specKwToken = STT_KW_Register; goto addedSpecKw; }
			if(isStr(idStart, text, "packoffset")){ specKwToken = STT_KW_PackOffset; goto addedSpecKw; }
			if (0) {
			addedSpecKw:
				tokens.push_back({ specKwToken, TLOC(idStart), 0 });
				continue;
			}

			tokens.push_back({ STT_Ident, TLOC(idStart), uint32_t(tokenData.size()) });
			uint32_t length(text - idStart);
			tokenData.insert(tokenData.end(),
				reinterpret_cast<char*>(&length),
				reinterpret_cast<char*>(&length + 1));
			tokenData.insert(tokenData.end(), idStart, text);
			tokenData.push_back(0);
			continue;
		}

		// number
		if (*text >= '0' && *text <= '9')
		{
			int64_t outi;
			double outf;
			int32_t valInt32;
			double valFloat64;

			const char* numStart = text;
			switch (util_strtonum(&text, &outi, &outf))
			{
			case 0: EmitFatalError("failed to parse number", LOC(numStart));
			case 1:
				valInt32 = outi;
				tokens.push_back({ STT_Int32Lit, TLOC(numStart), uint32_t(tokenData.size()) });
				tokenData.insert(tokenData.end(),
					reinterpret_cast<char*>(&valInt32),
					reinterpret_cast<char*>(&valInt32 + 1));
				continue;
			case 2:
				valFloat64 = outf;
				tokens.push_back({ STT_Float32Lit, TLOC(numStart), uint32_t(tokenData.size()) });
				tokenData.insert(tokenData.end(),
					reinterpret_cast<char*>(&valFloat64),
					reinterpret_cast<char*>(&valFloat64 + 1));
				continue;
			}
		}

		// operators
		for (const auto& opInfo : g_allOperatorsResolveOrder)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (opInfo.text[i] == '\0')
					break;
				if (text[i] != opInfo.text[i])
					goto notThisOper;
			}

			tokens.push_back({ opInfo.token, TLOC(text), 0 });
			text += strlen(opInfo.text);
			goto continueParsing;

		notThisOper:;
		}

		EmitFatalError("unexpected character: '" + std::string(text, 1) + "'", LOC(text));

	continueParsing:;
	}
}


struct PPTokenRange
{
	PreprocMacroMap::iterator it;
	SLToken* begin;
	SLToken* end;
};

void Parser::PreprocessTokens(uint32_t source)
{
	std::vector<SLToken> ppTokens, replacedTokens, tokensToReplace;
	ppTokens.reserve(tokens.size());
	int32_t lineOffset = 0;

#define PPOFLAG_ENABLED 0x1
#define PPOFLAG_HASELSE 0x2
#define PPOFLAG_HASSUCC 0x4
	std::vector<uint8_t> ppOutputEnabled;
	ppOutputEnabled.reserve(32);

	auto FindTokenReplaceRange = [this](std::vector<SLToken>& arr, size_t& i) -> PPTokenRange
	{
		if (arr[i].type == STT_Ident)
		{
			auto it = macros.find(TokenStringData(arr[i]));
			if (it != macros.end())
			{
				PreprocMacro& M = it->second;
				if (M.isFunc)
				{
					// function-style macro
					size_t start = i;
					FWD(arr, i);
					EXPECT(arr[i], STT_LParen);
					FWD(arr, i);

					// skip braces
					std::vector<SLTokenType> braceStack;
					braceStack.push_back(STT_RParen);
					while (i < arr.size() && braceStack.empty() == false)
					{
						auto tt = arr[i].type;
						if (tt == STT_LParen)
							braceStack.push_back(STT_RParen);
						else if (tt == STT_LBrace)
							braceStack.push_back(STT_RBrace);
						else if (tt == STT_RParen || tt == STT_RBrace)
						{
							if (braceStack.empty())
								EmitFatalError("brace mismatch (too many endings)", arr[i].loc);
							if (braceStack.back() != tt)
								EmitFatalError("brace mismatch (started with one type, ended with another)", arr[i].loc);
							braceStack.pop_back();
						}

						i++;
					}

					if (braceStack.empty() == false)
					{
						Location loc = Location::BAD();
						if (i < arr.size())
							loc = arr[i].loc;
						EmitFatalError("brace mismatch (too many beginnings)", loc);
					}

					return { it, &arr[start], arr.data() + i };
				}
				else
				{
					return { it, &arr[i], &arr[i] + 1 };
				}
			}
		}
		return { macros.end(), nullptr, nullptr };
	};
	auto ReplaceTokenRangeTo = [this](std::vector<SLToken>& out, PPTokenRange range)
	{
		PreprocMacro& M = range.it->second;
		if (M.isFunc)
		{
			// function-style macro
			out.reserve(M.tokens.size());

			// extract argument sub-ranges from range
			std::vector<PPTokenRange> argRanges;

			std::vector<SLTokenType> braceStack;
			SLToken* argTokens = range.begin;
			for (size_t i = 2; argTokens[i].type != STT_RParen; )
			{
				PPTokenRange arg;
				arg.begin = &argTokens[i];

				// skip braces
				braceStack.clear();
				while (range.begin + i < range.end &&
					((argTokens[i].type != STT_Comma && argTokens[i].type != STT_RParen)
						|| braceStack.empty() == false))
				{
					auto tt = argTokens[i].type;
					if (tt == STT_LParen)
						braceStack.push_back(STT_RParen);
					else if (tt == STT_LBrace)
						braceStack.push_back(STT_RBrace);
					else if (tt == STT_RParen || tt == STT_RBrace)
					{
						assert(braceStack.empty() == false);
						assert(braceStack.back() == tt);
						braceStack.pop_back();
					}

					i++;
				}
				assert(braceStack.empty());

				arg.end = &argTokens[i];
				argRanges.push_back(arg);
				if (argTokens[i].type == STT_Comma)
					i++;
			}

			if (argRanges.size() != M.args.size())
			{
				EmitFatalError("incorrect number of arguments passed to macro");
			}

			for (size_t tid = 0; tid < M.tokens.size(); ++tid)
			{
				bool isArg = false;
				if (M.tokens[tid].type == STT_Ident)
				{
					for (size_t aid = 0; aid < M.args.size(); ++aid)
					{
						if (TokenStringDataEquals(M.tokens[tid], M.args[aid].c_str(), M.args[aid].size()))
						{
							out.insert(out.end(), argRanges[aid].begin, argRanges[aid].end);
							isArg = true;
							break;
						}
					}
				}
				if (isArg == false)
					out.push_back(M.tokens[tid]);
			}

#if 0
			std::cerr << "before:";
			for (auto* t = range.begin; t < range.end; ++t)
				std::cerr << TokenToString(*t) << " ";
			std::cerr << "\nafter:";
			for (auto& t : out)
				std::cerr << TokenToString(t) << " ";
			std::cerr << "\n";
#endif
		}
		else
		{
			out.insert(out.end(), M.tokens.begin(), M.tokens.end());
		}

		// mark identifiers named same as macro unreplaceable
		for (auto& t : out)
			if (t.type == STT_Ident && TokenStringData(t) == range.it->first)
				t.type = STT_IdentPPNoReplace;
	};
	auto EvaluateCondition = [this, &tokensToReplace, &replacedTokens,
		&FindTokenReplaceRange, &ReplaceTokenRangeTo]() -> bool
	{
		// find tokens
		uint32_t logicalLine = T().logicalLine;
		size_t start = ++curToken;
		size_t end = start;

		while (end < tokens.size() && tokens[end].logicalLine == logicalLine)
			end++;

		// perform replacements
		tokensToReplace.clear();
		tokensToReplace.reserve(end - start);
		// - defined(MACRO)
		for (size_t i = start; i < end; ++i)
		{
			if (tokens[i].type == STT_Ident && TokenStringDataEquals(tokens[i], STRLIT_SIZE("defined")))
			{
				if (end - i >= 4 &&
					tokens[i + 1].type == STT_LParen &&
					tokens[i + 2].type == STT_Ident &&
					tokens[i + 3].type == STT_RParen)
				{
					bool def = macros.find(TokenStringData(tokens[i + 2])) != macros.end();
					SLToken nt = RequestIntBoolToken(def);
					nt.loc = tokens[i].loc;
					tokensToReplace.push_back(nt);
					i += 3; // loop incr already +1
				}
				else
				{
					EmitFatalError("expected 'defined(<identifier>)'", tokens[i].loc);
				}
			}
			else
			{
				tokensToReplace.push_back(tokens[i]);
			}
		}
		// - macros
		for (;;)
		{
			PPTokenRange range = { macros.end(), nullptr, nullptr };
			for (size_t i = 0; i < tokensToReplace.size(); ++i)
			{
				range = FindTokenReplaceRange(tokensToReplace, i);
				if (range.begin != range.end)
					break;
			}
			if (range.begin == range.end)
				break;

			ReplaceTokenRangeTo(replacedTokens, range);

			tokensToReplace.clear();
			std::swap(tokensToReplace, replacedTokens);
		}
		// - clean up
		for (auto& t : tokensToReplace)
			if (t.type == STT_IdentPPNoReplace)
				t.type = STT_Ident;

		// parse & evaluate
		curToken = end - 1;
		return EvaluateConstantIntExpr(tokensToReplace, 0, tokensToReplace.size()) != 0;
	};

	while (curToken < tokens.size())
	{
		if (TT() == STT_Hash)
		{
			Location loc = T().loc;
			loc.source = source;
			loc.line += lineOffset;

			if (curToken > 0 && T().logicalLine == tokens[curToken - 1].logicalLine)
			{
				EmitError("unexpected start of preprocessor directive", loc);
				curToken++;
				continue;
			}

			PPFWD();
			if (TT() != STT_Ident && TT() != STT_KW_If && TT() != STT_KW_Else)
				EXPECTERR("preprocessor command identifier");
			std::string cmd = TokenToString();
			if (cmd == "define")
			{
				PPFWD();
				EXPECT(STT_Ident);
				std::string name = TokenStringData();
				uint32_t logicalLine = T().logicalLine;

				PreprocMacro macro;
				curToken++;
				if (curToken < tokens.size() && TT() == STT_LParen)
				{
					// function-style macro, parse arguments
					macro.isFunc = true;
					PPFWD();

					while (TT() != STT_RParen)
					{
						EXPECT(STT_Ident);
						macro.args.push_back(TokenStringData());
						PPFWD();
						if (TT() != STT_RParen)
						{
							EXPECT(STT_Comma);
							PPFWD();
						}
					}

					curToken++;
				}

				while (curToken < tokens.size() && T().logicalLine == logicalLine)
				{
					macro.tokens.push_back(T());
					curToken++;
				}

				macros.insert({ name, macro });
				continue;
			}
			else if (cmd == "undef")
			{
				PPFWD();
				EXPECT(STT_Ident);
				std::string name = TokenStringData();

				if (macros.erase(name) == 0)
				{
					diag.PrintWarning("could not 'undef' macro, it was not defined", T().loc);
				}
			}
			else if (cmd == "if")
			{
				if (ppOutputEnabled.empty() == false && !(ppOutputEnabled.back() & PPOFLAG_ENABLED))
				{
					// do not enable but prevent elif/else from being triggered
					ppOutputEnabled.push_back(PPOFLAG_HASSUCC);
				}
				else
				{
					bool cond = EvaluateCondition();
					ppOutputEnabled.push_back(cond ? (PPOFLAG_ENABLED | PPOFLAG_HASSUCC) : 0);
				}
			}
			else if (cmd == "elif")
			{
				if (ppOutputEnabled.empty())
				{
					EmitFatalError("#elif not inside preprocessor condition", loc);
				}
				else if (ppOutputEnabled.back() & PPOFLAG_HASELSE)
				{
					EmitFatalError("#elif after #else in preprocessor condition", loc);
				}
				else if (ppOutputEnabled.back() & PPOFLAG_HASSUCC)
				{
					ppOutputEnabled.back() &= ~PPOFLAG_ENABLED;
				}
				else if (EvaluateCondition())
				{
					ppOutputEnabled.back() = PPOFLAG_ENABLED | PPOFLAG_HASSUCC;
				}
			}
			else if (cmd == "ifdef")
			{
				PPFWD();
				EXPECT(STT_Ident);
				std::string name = TokenStringData();

				if (ppOutputEnabled.empty() == false && !(ppOutputEnabled.back() & PPOFLAG_ENABLED))
				{
					// do not enable but prevent elif/else from being triggered
					ppOutputEnabled.push_back(PPOFLAG_HASSUCC);
				}
				else
				{
					bool cond = macros.find(name) != macros.end();
					ppOutputEnabled.push_back(cond ? (PPOFLAG_ENABLED | PPOFLAG_HASSUCC) : 0);
				}
			}
			else if (cmd == "ifndef")
			{
				PPFWD();
				EXPECT(STT_Ident);
				std::string name = TokenStringData();

				ppOutputEnabled.push_back(macros.find(name) == macros.end() ? (PPOFLAG_ENABLED | PPOFLAG_HASSUCC) : 0);
			}
			else if (cmd == "else")
			{
				if (ppOutputEnabled.empty())
				{
					EmitFatalError("#else not inside preprocessor condition", loc);
				}
				else if (ppOutputEnabled.back() & PPOFLAG_HASELSE)
				{
					EmitFatalError("#else repeated in preprocessor condition", loc);
				}
				else if (ppOutputEnabled.back() & PPOFLAG_HASSUCC)
				{
					ppOutputEnabled.back() &= ~PPOFLAG_ENABLED;
				}
				else
				{
					ppOutputEnabled.back() = PPOFLAG_ENABLED | PPOFLAG_HASELSE | PPOFLAG_HASSUCC;
				}
			}
			else if (cmd == "endif")
			{
				if (ppOutputEnabled.empty())
				{
					EmitFatalError("#endif not inside preprocessor condition", loc);
				}
				else
				{
					ppOutputEnabled.pop_back();
				}
			}
			else if (cmd == "error")
			{
				PPFWD();
				EXPECT(STT_StrLit);

				if (ppOutputEnabled.empty() || (ppOutputEnabled.back() & PPOFLAG_ENABLED))
				{
					EmitFatalError(TokenStringData(), loc);
				}
			}
			else if (cmd == "line")
			{
				PPFWD();
				EXPECT(STT_Int32Lit);
				lineOffset = TokenInt32Data() - (T().loc.line + 1);

				if (curToken + 1 < tokens.size() &&
					tokens[curToken].logicalLine == tokens[curToken + 1].logicalLine &&
					tokens[curToken + 1].type == STT_StrLit)
				{
					curToken++;
					source = diag.GetSourceID(TokenStringData());
				}
			}
			else if (cmd == "include")
			{
				PPFWD();
				EXPECT(STT_StrLit);

				std::string file = TokenStringData();
				if (curToken + 1 < tokens.size() &&
					tokens[curToken].logicalLine == tokens[curToken + 1].logicalLine)
				{
					EmitFatalError("unexpected tokens on the same line as #include");
				}

				if (ppOutputEnabled.empty() || (ppOutputEnabled.back() & PPOFLAG_ENABLED))
				{
					char* buf = NULL;
					if (loadIncludeFilePFN == nullptr)
					{
						EmitFatalError("#include not supported for this build", loc);
					}
					else if (loadIncludeFilePFN(file.c_str(), diag.sourceFiles[source].c_str(), &buf, loadIncludeFileUD) && buf)
					{
						// parse, preprocess sub-file
						std::vector<SLToken> tmpTokens;
						size_t tmpCurToken = 0;

						std::swap(tmpTokens, tokens);
						std::swap(tmpCurToken, curToken);

						uint32_t subsrc = diag.GetSourceID(file);
						ParseTokens(buf, subsrc);
						PreprocessTokens(subsrc);

						std::swap(tmpTokens, tokens);
						std::swap(tmpCurToken, curToken);

						// integrate generated tokens
						ppTokens.insert(ppTokens.end(), tmpTokens.begin(), tmpTokens.end());

						// free name
						loadIncludeFilePFN(NULL, NULL, &buf, loadIncludeFileUD);
					}
					else
					{
						EmitFatalError("failed to include '" + file + "'", loc);
					}
				}
			}
			else
			{
				EmitFatalError("unknown preprocessor directive: " + cmd, loc);
			}
		}
		else if (ppOutputEnabled.empty() || (ppOutputEnabled.back() & PPOFLAG_ENABLED))
		{
			if (TT() == STT_Ident)
			{
				auto range = FindTokenReplaceRange(tokens, curToken);
				if (range.begin != range.end)
				{
					size_t endPos = range.end - tokens.data();
					while (range.begin != range.end)
					{
						ReplaceTokenRangeTo(replacedTokens, range);

						tokensToReplace.clear();
						std::swap(tokensToReplace, replacedTokens);

						if (tokensToReplace.empty())
							break;
						for (size_t i = 0; i < tokensToReplace.size(); ++i)
						{
							range = FindTokenReplaceRange(tokensToReplace, i);
							if (range.begin != range.end)
								break;
						}
					}
					// clean up
					for (auto& t : tokensToReplace)
					{
						t.loc.source = source;
						t.loc.line += lineOffset;
						if (t.type == STT_IdentPPNoReplace)
							t.type = STT_Ident;
					}
					// commit replacement
					ppTokens.insert(ppTokens.end(), tokensToReplace.begin(), tokensToReplace.end());

					curToken = endPos - 1;
				}
				else
				{
					ppTokens.push_back(T());
					ppTokens.back().loc.source = source;
					ppTokens.back().loc.line += lineOffset;
				}
			}
			else
			{
				ppTokens.push_back(T());
				ppTokens.back().loc.source = source;
				ppTokens.back().loc.line += lineOffset;
			}
		}
		curToken++;
	}

	// replace token stream, reset iterator
	std::swap(tokens, ppTokens);
	curToken = 0;
}


SLToken Parser::RequestIntBoolToken(bool v)
{
	return { STT_Int32Lit, Location::BAD(), 0U, v ? 4U : 0U };
}

int Parser::EvaluateConstantIntExpr(const std::vector<SLToken>& tokenArr, size_t startPos, size_t endPos)
{
	size_t bestSplit;
	int bestScore;
	size_t curPos = startPos;
	FindBestSplit(tokenArr, true, curPos, endPos, STT_NULL, bestSplit, bestScore);

	if (bestSplit == SIZE_MAX)
	{
		if (endPos - startPos >= 2)
		{
			int sub = EvaluateConstantIntExpr(tokenArr, startPos + 1, endPos);
			switch (tokenArr[startPos].type)
			{
			case STT_OP_Add: return sub;
			case STT_OP_Sub: return -sub;
			case STT_OP_Not: return !sub;
			case STT_OP_Inv: return ~sub;
			}
		}
		if (endPos - startPos == 1)
		{
			if (tokenArr[startPos].type == STT_Int32Lit)
			{
				return TokenInt32Data(tokenArr[startPos]);
			}
			if (tokenArr[startPos].type == STT_Ident)
			{
				// previously unreplaced identifier
				return 0;
			}
		}
		EmitError("unexpected token in #if expression: '" + TokenToString(tokenArr[startPos]) + "'");
		return 0;
	}
	else
	{
		int lft = EvaluateConstantIntExpr(tokenArr, startPos, bestSplit);
		int rgt = EvaluateConstantIntExpr(tokenArr, bestSplit + 1, endPos);
		auto tt = tokenArr[bestSplit].type;
		switch (tt)
		{
		case STT_OP_Eq: return lft == rgt;
		case STT_OP_NEq: return lft != rgt;
		case STT_OP_LEq: return lft <= rgt;
		case STT_OP_GEq: return lft >= rgt;
		case STT_OP_Less: return lft < rgt;
		case STT_OP_Greater: return lft > rgt;
		case STT_OP_LogicalAnd: return lft && rgt;
		case STT_OP_LogicalOr: return lft || rgt;
		case STT_OP_Add: return lft + rgt;
		case STT_OP_Sub: return lft - rgt;
		case STT_OP_Mul: return lft * rgt;
		case STT_OP_Div: return rgt ? lft / rgt : 0;
		case STT_OP_Mod: return rgt ? lft % rgt : 0;
		case STT_OP_And: return lft & rgt;
		case STT_OP_Or: return lft | rgt;
		case STT_OP_Xor: return lft ^ rgt;
		case STT_OP_Lsh: return lft << rgt;
		case STT_OP_Rsh: return lft >> rgt;
		default:
			EmitError("unsupported #if operator: '" + TokenTypeToString(tt) + "'");
			return 0;
		}
	}
}


ASTType* Parser::GetType(const std::string& name)
{
	if (auto* t = ast.GetTypeByName(name.c_str()))
		return t;
	EmitFatalError("type not found: " + name);
	return nullptr;
}

ASTType* Parser::ParseType(bool isFuncRet)
{
	EXPECT(STT_Ident);
	std::string name = TokenStringData();
	if (isFuncRet == false && name == "void")
	{
		EmitError("void type can only be used as function return value");
	}
	FWD();

	if (auto* t = ast.GetTypeByName(name.c_str()))
		return t;

	EmitError("unknown type: " + name);
	return ast.GetVoidType();
}

ASTType* Parser::FindMemberType(ASTType* t, const std::string& name, uint32_t& memberID, int& swizzleComp)
{
	switch (t->kind)
	{
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::Float16:
	case ASTType::Float32:
	case ASTType::Vector:
	{
		const ASTType* vecType = ast.CastToVector(t);
		bool xyzwSwizzle = false;
		bool rgbaSwizzle = false;
		bool otherChars = false;
		if (name.size() > 4 || name.size() < 1)
		{
			// cannot have name length < 1 but check anyway so later code isn't bothered
			EmitError("too many components in swizzle");
			return nullptr;
		}
		for (size_t i = 0; i < name.size(); ++i)
		{
			char c = name[i];
			if (c == 'x' || c == 'y' || c == 'z' || c == 'w')
				xyzwSwizzle = true;
			else if (c == 'r' || c == 'g' || c == 'b' || c == 'a')
				rgbaSwizzle = true;
			else
				otherChars = true;
		}
		if (otherChars)
		{
			EmitError("unknown characters in swizzle (expected xyzw or rgba)");
			return nullptr;
		}
		if (xyzwSwizzle && rgbaSwizzle)
		{
			EmitError("cannot mix xyzw/rgba components");
			return nullptr;
		}
		if (xyzwSwizzle || rgbaSwizzle)
		{
			uint32_t swizzleMask = 0;
			for (size_t i = 0; i < name.size(); ++i)
			{
				int comp = 0;
				switch (name[i])
				{
				case 'x': comp = 0; break;
				case 'y': comp = 1; break;
				case 'z': comp = 2; break;
				case 'w': comp = 3; break;
				case 'r': comp = 0; break;
				case 'g': comp = 1; break;
				case 'b': comp = 2; break;
				case 'a': comp = 3; break;
				}
				if (comp >= vecType->sizeX)
				{
					EmitError("vector component out of range");
					return nullptr;
				}
				swizzleMask |= comp << (i * 2);
			}
			memberID = swizzleMask;
			swizzleComp = name.size();
			return ast.GetVectorType(vecType->subType, swizzleComp);
		}
		EmitError("unknown components in swizzle");
		return nullptr;
	}
	case ASTType::Matrix:
	{
		uint32_t swizzleMask = 0;
		int comp = 0;

		for (size_t i = 0; i < name.size(); ++i)
		{
			char c = name[i];
			if (c != '_')
			{
				EmitError("each matrix swizzle component must start with an underscore");
				return nullptr;
			}
			if (i + 2 > name.size() || (name[i + 1] == 'm' && i + 3 > name.size()))
			{
				EmitError("incomplete matrix swizzle component");
				return nullptr;
			}
			if (comp >= 4)
			{
				EmitError("too many matrix swizzle components");
				return nullptr;
			}
			bool onebased = true;
			char n1 = name[++i];
			if (n1 == 'm')
			{
				onebased = false;
				n1 = name[++i];
			}
			char n2 = name[++i];
			char cmin = onebased ? '1' : '0';
			char cmax = onebased ? '4' : '3';
			if (n1 < cmin || n1 > cmax || n2 < cmin || n2 > cmax)
			{
				EmitError("bad matrix swizzle component coordinate");
				return nullptr;
			}
			int rowi = n1 - cmin;
			int coli = n2 - cmin;
			swizzleMask |= (rowi + coli * 4) << (4 * comp++);
		}
		memberID = swizzleMask;
		swizzleComp = comp;
		return ast.GetVectorType(t->subType, comp);
	}
	case ASTType::Structure: {
		auto* sty = static_cast<const ASTStructType*>(t);
		for (size_t i = 0; i < sty->members.size(); ++i)
		{
			if (sty->members[i].name == name)
			{
				memberID = i;
				swizzleComp = 0;
				return sty->members[i].type;
			}
		}
		EmitError("structure member not found");
		return nullptr; }
	case ASTType::Void:
		EmitError("there are no members for 'void'");
		return nullptr;
	default:
		EmitFatalError("trying to get member value of unknown type");
		return nullptr;
	}
}

VoidExpr* Parser::CreateVoidExpr()
{
	auto* e = new VoidExpr;
	e->SetReturnType(ast.GetVoidType());
	ast.unassignedNodes.AppendChild(e);
	return e;
}

void Parser::ParseSemantic(std::string& name, int& index)
{
	if (TT() == STT_Colon)
	{
		FWD();
		EXPECT(STT_Ident);
		name = TokenStringData();
		if (name.back() >= '0' && name.back() <= '9')
		{
			// TODO filter by prefix?
			int num = 0;
			int mult = 1;
			for (size_t i = name.size(); i > 0; )
			{
				i--;
				char c = name[i];
				if (c < '0' || c > '9')
				{
					name.resize(i + 1);
					break;
				}
				num += mult * (c - '0');
				mult *= 10;
			}
			index = num;
		}
		FWD();
	}
}

void Parser::ParseArgList(ASTNode* out)
{
	if (TT() == STT_RParen)
		return;

	for (;;)
	{
		VarDecl* vd = new VarDecl;
		vd->loc = T().loc;
		out->AppendChild(vd);

		switch (TT())
		{
		case STT_KW_In:
			vd->flags = VarDecl::ATTR_In;
			FWD();
			break;
		case STT_KW_Out:
			vd->flags = VarDecl::ATTR_Out;
			FWD();
			break;
		case STT_KW_InOut:
			vd->flags = VarDecl::ATTR_In | VarDecl::ATTR_Out;
			FWD();
			break;
		default:
			vd->flags = VarDecl::ATTR_In;
			break;
		}

		vd->SetType(ParseType());
		assert(vd->GetType());

		EXPECT(STT_Ident);
		vd->name = TokenStringData();
		FWD();

		ParseSemantic(vd->semanticName, vd->semanticIndex);

		if (TT() == STT_RParen)
			break;

		EXPECT(STT_Comma);
		FWD();
	}
}

static bool EffectivelyEqual(ASTType* a, ASTType* b)
{
	if (a == b)
		return true;
	if (a->IsNumericOrVM1() && b->IsNumericOrVM1() && a->GetNVMKind() == b->GetNVMKind())
		return true;
	return false;
}

#define MAX_OVERLOAD 0x7fffffff
int32_t Parser::CalcOverloadMatchFactor(ASTFunction* func, OpExpr* fcall, ASTType** equalArgs, bool err)
{
	if (func->GetArgCount() != fcall->GetArgCount())
	{
		if (err)
			EmitError("incorrect number of arguments passed to function");
		return MAX_OVERLOAD;
	}
	int32_t val = 0;
	ASTNode* fnarg = func->GetFirstArg();
	ASTNode* fcarg = fcall->GetFirstArg();
	for (size_t i = 0; fnarg && fcarg; ++i, fnarg = fnarg->next, fcarg = fcarg->next)
	{
		if (equalArgs && equalArgs[i])
			continue;
		ASTType* inty = fcarg->ToExpr()->GetReturnType();
		ASTType* expty = fnarg->ToVarDecl()->GetType();
		if (EffectivelyEqual(inty, expty))
			val += 1;
		else if (CanCast(inty, expty, false))
			val += 0xffff;
		else
		{
			if (err)
			{
				EmitError("cannot implicitly cast argument " + std::to_string(i + 1)
					+ " from '" + inty->GetName() + "' to '" + expty->GetName() + "'");
			}
			return MAX_OVERLOAD;
		}
	}
	return val;
}

// returns new 'val' (for iteration, to avoid stopping after first cast because 'next' is now null)
static Expr* CastExprTo(Expr* val, ASTType* to)
{
	assert(to);
	if (to != val->GetReturnType())
	{
		CastExpr* cast = new CastExpr;
		cast->loc = val->loc;
		cast->SetReturnType(to);
		val->ReplaceWith(cast);
		cast->SetSource(val);
		return cast;
	}
	return val;
}

static ASTType* ScalableSymmetricIntrin(Parser* parser, OpExpr* fcall,
	OpKind opKind, const char* name, bool alsoInt, int args = 1)
{
	if (fcall->GetArgCount() != args)
	{
		if (args > 1)
			parser->EmitError("'" + std::string(name) + "' requires " + std::to_string(args) + " arguments");
		else
			parser->EmitError("'" + std::string(name) + "' requires 1 argument");
		return nullptr;
	}

	auto arg = fcall->GetFirstArg();
	ASTType* rt0 = fcall->GetFirstArg()->ToExpr()->GetReturnType();
	if (args >= 2)
		rt0 = parser->Promote(rt0, arg->next->ToExpr()->GetReturnType());
	if (args >= 3)
		rt0 = parser->Promote(rt0, arg->next->next->ToExpr()->GetReturnType());
	ASTType* reqty = rt0;
	if (alsoInt)
	{
		if (rt0->IsBoolBased())
		{
			reqty = parser->ast.CastToInt(rt0);
		}
	}
	else
	{
		reqty = parser->ast.CastToFloat(rt0);
	}

	bool notMatch = rt0->IsNumericBased() == false;

	for (ASTNode* arg = fcall->GetFirstArg()->next; arg; arg = arg->next)
	{
		if (parser->CanCast(arg->ToExpr()->GetReturnType(), reqty, false) == false)
		{
			notMatch = true;
			break;
		}
	}

	if (notMatch)
	{
		parser->EmitError("none of '" + std::string(name) + "' overloads matched the argument list");
		return nullptr;
	}
	for (ASTNode* arg = fcall->GetFirstArg(); arg; arg = arg->next)
		arg = CastExprTo(arg->ToExpr(), reqty);
	fcall->opKind = opKind;
	return reqty;
}

#define DEF_INTRIN_SSF(opKind, name) \
	{ #name, [](Parser* parser, OpExpr* fcall) -> ASTType* \
	{ return ScalableSymmetricIntrin(parser, fcall, opKind, #name, false); }}

static ASTType* VectorIntrin(Parser* parser, OpExpr* fcall,
	OpKind opKind, const char* name, bool alsoInt, bool returnScalar, int args = 1)
{
	if (fcall->GetArgCount() != args)
	{
		if (args > 1)
			parser->EmitError("'" + std::string(name) + "' requires " + std::to_string(args) + " arguments");
		else
			parser->EmitError("'" + std::string(name) + "' requires 1 argument");
		return nullptr;
	}

	ASTType* rt0 = nullptr;
	for (ASTNode* arg = fcall->GetFirstArg()->next; arg; arg = arg->next)
	{
		if (arg->ToExpr()->GetReturnType()->kind == ASTType::Vector)
		{
			rt0 = arg->ToExpr()->GetReturnType();
			break;
		}
	}
	if (!rt0)
		rt0 = parser->ast.CastToVector(fcall->GetFirstArg()->ToExpr()->GetReturnType());
	if (!rt0)
		goto unmatched;
	ASTType* reqty = rt0;
	if (alsoInt)
	{
		if (rt0->IsBoolBased())
		{
			reqty = parser->ast.CastToInt(rt0);
		}
	}
	else
	{
		reqty = parser->ast.CastToFloat(rt0);
	}

	bool notMatch = rt0->IsNumericBased() == false;
	for (ASTNode* arg = fcall->GetFirstArg()->next; arg; arg = arg->next)
	{
		if (arg->ToExpr()->GetReturnType()->kind == ASTType::Matrix ||
			parser->CanCast(arg->ToExpr()->GetReturnType(), reqty, false) == false)
		{
			notMatch = true;
			break;
		}
	}

	if (notMatch)
	{
unmatched:
		parser->EmitError("none of '" + std::string(name) + "' overloads matched the argument list");
		return nullptr;
	}
	for (ASTNode* arg = fcall->GetFirstArg(); arg; arg = arg->next)
		arg = CastExprTo(arg->ToExpr(), reqty);
	fcall->opKind = opKind;
	return returnScalar ? parser->ast.CastToScalar(reqty) : reqty;
}

static ASTType* TexSampleIntrin(Parser* parser, OpExpr* fcall, OpKind opKind,
	const char* name, ASTType::Kind smpType, int vecSize, int numArgs)
{
	if (fcall->GetArgCount() != numArgs)
	{
		parser->EmitError("'" + std::string(name) + "' requires " + std::to_string(numArgs) + " arguments");
		return nullptr;
	}
	ASTType* rt0 = fcall->GetFirstArg()->ToExpr()->GetReturnType();
	bool notMatch = rt0->kind != smpType;
	if (!notMatch)
	{
		for (ASTNode* arg = fcall->GetFirstArg()->next; arg; arg = arg->next)
		{
			ASTType* rtN = arg->ToExpr()->GetReturnType();
			if (rtN->IsNumericBased() == false || rtN->kind == ASTType::Matrix)
			{
				notMatch = true;
				break;
			}
			if (vecSize == 1)
			{
				if (rtN->IsNumeric() == false)
				{
					notMatch = true;
					break;
				}
			}
			else if (rtN->kind == ASTType::Vector && !(rtN->sizeX == 1 || rtN->sizeX == vecSize))
			{
				notMatch = true;
				break;
			}

			ASTType* reqty = parser->ast.CastToFloat(rtN);
			if (vecSize > 1)
				reqty = parser->ast.CastToVector(reqty, vecSize);
			arg = CastExprTo(arg->ToExpr(), reqty);
		}
	}

	if (notMatch)
	{
		parser->EmitError("none of '" + std::string(name) + "' overloads matched the argument list");
		return nullptr;
	}

	fcall->opKind = opKind;
	return parser->ast.GetFloat32VecType(4);
}

struct ConstCharEqual
{
	bool operator () (const char* a, const char* b) const
	{
		return !strcmp(a, b);
	}
};

struct ConstCharHash
{
	size_t operator () (const char* s) const
	{
		uint32_t hash = 2166136261U;
		while (*s)
		{
			hash ^= *s++;
			hash *= 16777619U;
		}
		return hash;
	}
};

typedef ASTType* (*IntrinsicValidatorFP)(Parser*, OpExpr*);
std::unordered_map<const char*, IntrinsicValidatorFP, ConstCharHash, ConstCharEqual> g_BuiltinIntrinsics
{
	{ "abs", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Abs, "abs", true); }},
	DEF_INTRIN_SSF(Op_ACos, acos),
	{ "all", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		if (fcall->GetArgCount() != 1)
		{
			parser->EmitError("'all' requires 1 argument");
			return nullptr;
		}
		ASTType* rt0 = fcall->GetFirstArg()->ToExpr()->GetReturnType();
		if (rt0->IsNumericBased() == false)
		{
			parser->EmitError("none of 'all' overloads matched the argument list");
			return nullptr;
		}
		fcall->opKind = Op_All;
		return parser->ast.GetBoolType();
	}},
	{ "any", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		if (fcall->GetArgCount() != 1)
		{
			parser->EmitError("'any' requires 1 argument");
			return nullptr;
		}
		ASTType* rt0 = fcall->GetFirstArg()->ToExpr()->GetReturnType();
		if (rt0->IsNumericBased() == false)
		{
			parser->EmitError("none of 'any' overloads matched the argument list");
			return nullptr;
		}
		fcall->opKind = Op_Any;
		return parser->ast.GetBoolType();
	}},
	DEF_INTRIN_SSF(Op_ASin, asin),
	DEF_INTRIN_SSF(Op_ATan, atan),
	{ "atan2", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_ATan2, "atan2", false, 2); }},
	DEF_INTRIN_SSF(Op_Ceil, ceil),
	{ "clamp", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Clamp, "clamp", true, 3); }},
	{ "clip", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		ASTType* t = ScalableSymmetricIntrin(parser, fcall, Op_Clip, "clip", false, 1);
		return t ? parser->ast.GetVoidType() : t;
	}},
	DEF_INTRIN_SSF(Op_Cos, cos),
	DEF_INTRIN_SSF(Op_CosH, cosh),
	{ "cross", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		if (fcall->GetArgCount() != 2)
		{
			parser->EmitError("'cross' requires 2 arguments");
			return nullptr;
		}
		ASTType* rt0 = fcall->GetFirstArg()->ToExpr()->GetReturnType();
		ASTType* rt1 = fcall->GetFirstArg()->next->ToExpr()->GetReturnType();
		if (rt0->IsNumericOrVM1() == false && (rt0->kind != ASTType::Vector || rt0->sizeX != 3) &&
			rt1->IsNumericOrVM1() == false && (rt1->kind != ASTType::Vector || rt1->sizeX != 3))
		{
			parser->EmitError("none of 'cross' overloads matched the argument list");
			return nullptr;
		}
		ASTType* reqty = parser->ast.CastToVector(parser->ast.CastToFloat(rt0), 3);
		CastExprTo(fcall->GetFirstArg()->ToExpr(), reqty);
		CastExprTo(fcall->GetFirstArg()->next->ToExpr(), reqty);
		fcall->opKind = Op_Cross;
		return reqty;
	}},
	DEF_INTRIN_SSF(Op_DDX, ddx),
	DEF_INTRIN_SSF(Op_DDY, ddy),
	DEF_INTRIN_SSF(Op_Degrees, degrees),
	{ "determinant", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		if (fcall->GetArgCount() != 1)
		{
			parser->EmitError("'determinant' requires 1 argument");
			return nullptr;
		}
		ASTType* rt0 = fcall->GetFirstArg()->ToExpr()->GetReturnType();
		if (rt0->kind != ASTType::Matrix ||
			rt0->subType->IsNumeric() == false ||
			rt0->sizeX != rt0->sizeY)
		{
			parser->EmitError("none of 'determinant' overloads matched the argument list");
			return nullptr;
		}
		ASTType* reqty = parser->ast.CastToFloat(rt0);
		CastExprTo(fcall->GetFirstArg()->ToExpr(), reqty);
		fcall->opKind = Op_Determinant;
		return parser->ast.CastToScalar(reqty);
	}},
	{ "distance", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return VectorIntrin(parser, fcall, Op_Distance, "distance", false, true, 2); }},
	{ "dot", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return VectorIntrin(parser, fcall, Op_Dot, "dot", true, true, 2); }},
	DEF_INTRIN_SSF(Op_Exp, exp),
	DEF_INTRIN_SSF(Op_Exp2, exp2),
	{ "faceforward", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return VectorIntrin(parser, fcall, Op_FaceForward, "faceforward", false, false, 3); }},
	DEF_INTRIN_SSF(Op_Floor, floor),
	{ "fmod", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_FMod, "fmod", false, 2); }},
	DEF_INTRIN_SSF(Op_Frac, frac),
	/// frexp
	DEF_INTRIN_SSF(Op_FWidth, fwidth),
	{ "isfinite", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		ASTType* t = ScalableSymmetricIntrin(parser, fcall, Op_IsFinite, "isfinite", false);
		return t ? parser->ast.CastToBool(t) : t;
	}},
	{ "isinf", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		ASTType* t = ScalableSymmetricIntrin(parser, fcall, Op_IsInf, "isinf", false);
		return t ? parser->ast.CastToBool(t) : t;
	}},
	{ "isnan", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		ASTType* t = ScalableSymmetricIntrin(parser, fcall, Op_IsNaN, "isnan", false);
		return t ? parser->ast.CastToBool(t) : t;
	}},
	{ "ldexp", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_LdExp, "ldexp", false, 2); }},
	{ "length", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return VectorIntrin(parser, fcall, Op_Length, "length", false, true, 1); }},
	{ "lerp", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Lerp, "lerp", false, 3); }},
	DEF_INTRIN_SSF(Op_Log, log),
	DEF_INTRIN_SSF(Op_Log10, log10),
	DEF_INTRIN_SSF(Op_Log2, log2),
	{ "max", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Max, "max", true, 2); }},
	{ "min", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Min, "min", true, 2); }},
	/// modf
	{ "mul", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		if (fcall->GetArgCount() != 2)
		{
			parser->EmitError("'mul' requires 2 arguments");
			return nullptr;
		}
		ASTType* rt0 = fcall->GetFirstArg()->ToExpr()->GetReturnType();
		ASTType* rt1 = fcall->GetFirstArg()->next->ToExpr()->GetReturnType();

		ASTType* retTy = nullptr;
		for (;;)
		{
			// overload 1
			if (rt0->IsNumeric() && rt1->IsNumeric())
			{
				retTy = parser->Promote(rt0, rt1);
				CastExprTo(fcall->GetFirstArg()->ToExpr(), retTy);
				CastExprTo(fcall->GetFirstArg()->next->ToExpr(), retTy);
				fcall->opKind = Op_Multiply;
				break;
			}
			// overload 2
			if (rt0->IsNumeric() && rt1->kind == ASTType::Vector)
			{
				retTy = parser->Promote(rt0, rt1);
				CastExprTo(fcall->GetFirstArg()->ToExpr(), parser->ast.CastToScalar(retTy));
				CastExprTo(fcall->GetFirstArg()->next->ToExpr(), retTy);
				fcall->opKind = Op_Multiply;
				break;
			}
			// overload 3
			if (rt0->IsNumeric() && rt1->kind == ASTType::Matrix)
			{
				retTy = parser->Promote(rt0, rt1);
				CastExprTo(fcall->GetFirstArg()->ToExpr(), parser->ast.CastToScalar(retTy));
				CastExprTo(fcall->GetFirstArg()->next->ToExpr(), retTy);
				fcall->opKind = Op_Multiply;
				break;
			}
			// overload 4
			if (rt0->kind == ASTType::Vector && rt1->IsNumeric())
			{
				retTy = parser->Promote(rt0, rt1);
				CastExprTo(fcall->GetFirstArg()->ToExpr(), retTy);
				CastExprTo(fcall->GetFirstArg()->next->ToExpr(), parser->ast.CastToScalar(retTy));
				fcall->opKind = Op_Multiply;
				break;
			}
			// overload 5
			if (rt0->kind == ASTType::Vector && rt1->kind == ASTType::Vector && rt0->sizeX == rt1->sizeX)
			{
				ASTType* commonType = parser->Promote(rt0, rt1);
				CastExprTo(fcall->GetFirstArg()->ToExpr(), commonType);
				CastExprTo(fcall->GetFirstArg()->next->ToExpr(), commonType);
				retTy = parser->ast.CastToScalar(commonType);
				fcall->opKind = Op_Dot;
				break;
			}
			// overload 6
			if (rt0->kind == ASTType::Vector &&
				rt1->kind == ASTType::Matrix &&
				rt0->sizeX == rt1->sizeX)
			{
				ASTType* commonType = parser->Promote(rt0->subType, rt1->subType);
				retTy = parser->ast.GetVectorType(commonType, rt1->sizeY);
				fcall->opKind = Op_MulVM;
				break;
			}
			// overload 7
			if (rt0->kind == ASTType::Matrix && rt1->IsNumeric())
			{
				retTy = parser->Promote(rt0, rt1);
				CastExprTo(fcall->GetFirstArg()->ToExpr(), retTy);
				CastExprTo(fcall->GetFirstArg()->next->ToExpr(), parser->ast.CastToScalar(retTy));
				fcall->opKind = Op_Multiply;
				break;
			}
			// overload 8
			if (rt0->kind == ASTType::Matrix &&
				rt1->kind == ASTType::Vector &&
				rt0->sizeY == rt1->sizeX)
			{
				ASTType* commonType = parser->Promote(rt0->subType, rt1->subType);
				retTy = parser->ast.GetVectorType(commonType, rt0->sizeX);
				fcall->opKind = Op_MulMV;
				break;
			}
			// overload 9
			if (rt0->kind == ASTType::Matrix &&
				rt1->kind == ASTType::Matrix &&
				rt0->sizeY == rt1->sizeX)
			{
				ASTType* commonType = parser->Promote(rt0->subType, rt1->subType);
				retTy = parser->ast.GetMatrixType(commonType, rt0->sizeX, rt1->sizeY);
				fcall->opKind = Op_MulMM;
				break;
			}
			parser->EmitError("none of 'mul' overloads matched the argument list");
			return nullptr;
		}
		assert(retTy);
		return retTy;
	} },
	{ "normalize", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return VectorIntrin(parser, fcall, Op_Normalize, "normalize", false, false); } },
	{ "pow", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Pow, "pow", false, 2); } },
	DEF_INTRIN_SSF(Op_Radians, radians),
	{ "reflect", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return VectorIntrin(parser, fcall, Op_Reflect, "reflect", false, false, 2); } },
	{ "refract", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		if (fcall->GetArgCount() != 3)
		{
			parser->EmitError("'refract' requires 3 arguments");
			return nullptr;
		}

		ASTType* rt0 = nullptr;
		ASTNode* arg = fcall->GetFirstArg();
		if (arg->ToExpr()->GetReturnType()->kind == ASTType::Vector)
			rt0 = arg->ToExpr()->GetReturnType();
		else if (arg->next->ToExpr()->GetReturnType()->kind == ASTType::Vector)
			rt0 = arg->next->ToExpr()->GetReturnType();
		if (!rt0)
			rt0 = parser->ast.CastToVector(fcall->GetFirstArg()->ToExpr()->GetReturnType());
		if (!rt0)
			goto unmatched;
		ASTType* reqty = parser->ast.CastToFloat(rt0);

		bool notMatch = rt0->IsNumericBased() == false;
		arg = fcall->GetFirstArg();
		if (arg->ToExpr()->GetReturnType()->kind == ASTType::Matrix ||
			parser->CanCast(arg->ToExpr()->GetReturnType(), reqty, false) == false ||
			!(arg = arg->next) ||
			arg->ToExpr()->GetReturnType()->kind == ASTType::Matrix ||
			parser->CanCast(arg->ToExpr()->GetReturnType(), reqty, false) == false ||
			!(arg = arg->next) ||
			arg->ToExpr()->GetReturnType()->IsNumericOrVM1() == false)
			notMatch = true;

		if (notMatch)
		{
	unmatched:
			parser->EmitError("none of 'refract' overloads matched the argument list");
			return nullptr;
		}
		for (ASTNode* arg = fcall->GetFirstArg(); arg; arg = arg->next)
			arg = CastExprTo(arg->ToExpr(), arg->next ? reqty : parser->ast.GetFloat32Type());
		fcall->opKind = Op_Refract;
		return reqty;
	} },
	DEF_INTRIN_SSF(Op_Round, round),
	DEF_INTRIN_SSF(Op_RSqrt, rsqrt),
	{ "saturate", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Saturate, "saturate", false, 1); } },
	{ "sign", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{
		ASTType* t = ScalableSymmetricIntrin(parser, fcall, Op_Sign, "sign", true, 1);
		return t ? parser->ast.CastToInt(t) : t;
	} },
	DEF_INTRIN_SSF(Op_Sin, sin),
	/// sincos
	DEF_INTRIN_SSF(Op_SinH, sinh),
	{ "smoothstep", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_SmoothStep, "smoothstep", false, 2); } },
	DEF_INTRIN_SSF(Op_Sqrt, sqrt),
	{ "step", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return ScalableSymmetricIntrin(parser, fcall, Op_Step, "step", false, 2); } },
	DEF_INTRIN_SSF(Op_Tan, tan),
	DEF_INTRIN_SSF(Op_TanH, tanh),

	{ "tex1D", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex1D, "tex1D", ASTType::Sampler1D, 1, 2); } },
	{ "tex1Dbias", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex1DBias, "tex1Dbias", ASTType::Sampler1D, 4, 2); } },
	{ "tex1Dgrad", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex1DGrad, "tex1Dgrad", ASTType::Sampler1D, 1, 4); } },
	{ "tex1Dlod", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex1DLOD, "tex1Dlod", ASTType::Sampler1D, 4, 2); } },
	{ "tex1Dproj", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex1DProj, "tex1Dproj", ASTType::Sampler1D, 4, 2); } },

	{ "tex2D", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex2D, "tex2D", ASTType::Sampler2D, 2, 2); } },
	{ "tex2Dbias", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex2DBias, "tex2Dbias", ASTType::Sampler2D, 4, 2); } },
	{ "tex2Dgrad", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex2DGrad, "tex2Dgrad", ASTType::Sampler2D, 2, 4); } },
	{ "tex2Dlod", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex2DLOD, "tex2Dlod", ASTType::Sampler2D, 4, 2); } },
	{ "tex2Dproj", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex2DProj, "tex2Dproj", ASTType::Sampler2D, 4, 2); } },

	{ "tex3D", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex3D, "tex3D", ASTType::Sampler3D, 3, 2); } },
	{ "tex3Dbias", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex3DBias, "tex3Dbias", ASTType::Sampler3D, 4, 2); } },
	{ "tex3Dgrad", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex3DGrad, "tex3Dgrad", ASTType::Sampler3D, 3, 4); } },
	{ "tex3Dlod", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex3DLOD, "tex3Dlod", ASTType::Sampler3D, 4, 2); } },
	{ "tex3Dproj", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_Tex3DProj, "tex3Dproj", ASTType::Sampler3D, 4, 2); } },

	{ "texCUBE", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_TexCube, "texCUBE", ASTType::SamplerCUBE, 3, 2); } },
	{ "texCUBEbias", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_TexCubeBias, "texCUBEbias", ASTType::SamplerCUBE, 4, 2); } },
	{ "texCUBEgrad", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_TexCubeGrad, "texCUBEgrad", ASTType::SamplerCUBE, 3, 4); } },
	{ "texCUBElod", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_TexCubeLOD, "texCUBElod", ASTType::SamplerCUBE, 4, 2); } },
	{ "texCUBEproj", [](Parser* parser, OpExpr* fcall) -> ASTType*
	{ return TexSampleIntrin(parser, fcall, Op_TexCubeProj, "texCUBEproj", ASTType::SamplerCUBE, 4, 2); } },

	/// transpose
	DEF_INTRIN_SSF(Op_Trunc, trunc),
};
void Parser::FindFunction(OpExpr* fcall, Expr* fnexpr, const Location& loc)
{
	if (fnexpr)
	{
		if (auto ie = dyn_cast<DeclRefExpr>(fnexpr))
		{
			auto bit = g_BuiltinIntrinsics.find(ie->name.c_str());
			if (bit != g_BuiltinIntrinsics.end())
			{
				ASTType* retType = bit->second(this, fcall);
				if (retType)
				{
					fcall->SetReturnType(retType);
					auto op = fcall->opKind;
					if (op == Op_DDX || op == Op_DDY || op == Op_FWidth)
					{
						ast.usingDerivatives = true;
					}
					if (op == Op_Tex1DLOD || op == Op_Tex2DLOD ||
						op == Op_Tex3DLOD || op == Op_TexCubeLOD)
					{
						ast.usingLODTextureSampling = true;
					}
					if (op == Op_Tex1DGrad || op == Op_Tex2DGrad ||
						op == Op_Tex3DGrad || op == Op_TexCubeGrad)
					{
						ast.usingGradTextureSampling = true;
					}
				}
				// otherwise error already printed
				return;
			}

			auto it = functions.find(ie->name);
			if (it != functions.end())
			{
				// find the right function
				if (it->second.size() == 1)
				{
					ASTFunction* fn = *it->second.begin();
					if (CalcOverloadMatchFactor(fn, fcall, nullptr, true) != MAX_OVERLOAD)
					{
						fcall->resolvedFunc = fn;
						fcall->opKind = Op_FCall;
						fcall->SetReturnType(fn->GetReturnType());
					}
				}
				else
				{
					ASTType* voidTy = ast.GetVoidType();
					std::vector<ASTType*> equalArgs;
					equalArgs.resize(fcall->GetArgCount(), voidTy);

					for (ASTFunction* fn : it->second)
					{
						size_t i = 0;
						for (ASTNode* arg = fn->GetFirstArg(); arg; ++i, arg = arg->next)
						{
							if (equalArgs[i] == voidTy)
								equalArgs[i] = arg->ToVarDecl()->GetType();
							else if (equalArgs[i] != arg->ToVarDecl()->GetType())
								equalArgs[i] = nullptr;
						}
					}

					ASTFunction* lastMF = nullptr;
					int32_t bestOMF = MAX_OVERLOAD;
					int numOverloads = 0;

					for (ASTFunction* fn : it->second)
					{
						int32_t curOMF = CalcOverloadMatchFactor(fn, fcall, equalArgs.data(), false);
						if (curOMF < bestOMF)
						{
							bestOMF = curOMF;
							lastMF = fn;
							numOverloads = 1;
						}
						else if (curOMF == bestOMF)
						{
							numOverloads++;
						}
					}
					if (numOverloads == 0)
					{
						EmitError("none of the overloads for '" + ie->name + "' match the given arguments");
					}
					else if (numOverloads > 1)
					{
						EmitError("ambiguous call to '" + ie->name + "', "
							"multiple overloads match the given arguments equally");
					}
					else
					{
						fcall->resolvedFunc = lastMF;
						fcall->opKind = Op_FCall;
						fcall->SetReturnType(lastMF->GetReturnType());
					}
				}

				// adjust arguments
				if (auto* fn = fcall->resolvedFunc)
				{
					int i = 0;
					for (ASTNode *arg = fcall->GetFirstArg(), *argdecl = fn->GetFirstArg();
						arg && argdecl;
						arg = arg->next, argdecl = argdecl->next)
						arg = CastExprTo(arg->ToExpr(), argdecl->ToVarDecl()->GetType());
				}
			}
			else
			{
				EmitError("failed to find function named '" + ie->name + "'", loc);
			}
			return;
		}
	}
	EmitError("failed to find function", loc);
}

void Parser::FindBestSplit(const std::vector<SLToken>& tokenArr, bool preProcSplit,
	size_t& curPos, size_t endPos, SLTokenType endTokenType, size_t& bestSplit, int& bestScore)
{
	bestSplit = SIZE_MAX;
	bestScore = 0;
	std::vector<SLTokenType> braceStack;

	size_t startPos = curPos;
	while (curPos < endPos
		&& ((tokenArr[curPos].type != STT_Comma && tokenArr[curPos].type != endTokenType)
			|| braceStack.empty() == false))
	{
		if (braceStack.empty())
		{
			int curScore = GetSplitScore(tokenArr, curPos, startPos, preProcSplit);
			bool rtl = (curScore & SPLITSCORE_RTLASSOC) != 0;
			curScore &= ~SPLITSCORE_RTLASSOC;
			if (curScore - (rtl ? 1 : 0) >= bestScore) // ltr: >=, rtl: > (-1)
			{
				bestScore = curScore;
				bestSplit = curPos;
			}
		}

		auto tt = tokenArr[curPos].type;
		if (tt == STT_LParen)
			braceStack.push_back(STT_RParen);
		else if (tt == STT_LBracket)
			braceStack.push_back(STT_RBracket);
		else if (tt == STT_OP_Ternary)
			braceStack.push_back(STT_Colon);
		else if (tt == STT_RParen || tt == STT_RBracket || tt == STT_Colon)
		{
			if (braceStack.empty())
				EmitFatalError("brace mismatch (too many endings)", tokenArr[curPos].loc);
			if (braceStack.back() != tt)
				EmitFatalError("brace mismatch (started with one type, ended with another)", tokenArr[curPos].loc);
			braceStack.pop_back();
		}

		curPos++;
	}

	if (braceStack.empty() == false)
	{
		Location loc = Location::BAD();
		if (curPos < tokenArr.size())
			loc = tokenArr[curPos].loc;
		EmitFatalError("brace mismatch (too many beginnings)", loc);
	}
}

Expr* Parser::ParseExpr(SLTokenType endTokenType, size_t endPos)
{
	if (endPos == SIZE_MAX)
		endPos = tokens.size();

	size_t start = curToken;

	size_t bestSplit;
	int bestScore;
	FindBestSplit(tokens, false, curToken, endPos, endTokenType, bestSplit, bestScore);
#if 0
	if (bestSplit != SIZE_MAX)
	{
		FILEStream err(stderr);
		err << "\n";
		for (size_t i = start; i < curToken; ++i)
		{
			err << (i == bestSplit ? " >>>" : " ");
			err << TokenToString(i);
			if (i == bestSplit) err << "<<<";
		}
		err << "\n";
	}
#endif

	if (bestSplit == SIZE_MAX)
	{
		if (curToken - start == 1)
		{
			// one item long expression
			curToken = start;

			auto tt = TT();
			if (tt == STT_Ident)
			{
				auto* expr = new DeclRefExpr;
				ast.unassignedNodes.AppendChild(expr);
				expr->name = TokenStringData();
				expr->loc = T().loc;

				for (VarDecl* vd = funcInfo.scopeVars; vd; vd = vd->prevScopeDecl)
				{
					if (vd->name == expr->name)
					{
						expr->decl = vd;
						expr->SetReturnType(vd->GetType());
						break;
					}
				}
				if (!expr->GetReturnType())
				{
					if (functions.find(expr->name) != functions.end() ||
						g_BuiltinIntrinsics.find(expr->name.c_str()) != g_BuiltinIntrinsics.end())
					{
						expr->SetReturnType(ast.GetFunctionType());
					}
				}
				if (!expr->GetReturnType())
				{
					EmitError("could not find variable '" + expr->name + "'");
					expr->SetReturnType(ast.GetVoidType());
				}

				FWD();
				return expr;
			}
			else if (tt == STT_BoolLit)
			{
				auto* expr = new BoolExpr;
				ast.unassignedNodes.AppendChild(expr);
				expr->SetReturnType(ast.GetBoolType());
				expr->loc = T().loc;
				expr->value = TokenBoolData();
				FWD();
				return expr;
			}
			else if (tt == STT_Int32Lit)
			{
				auto* expr = new Int32Expr;
				ast.unassignedNodes.AppendChild(expr);
				expr->SetReturnType(ast.GetInt32Type());
				expr->loc = T().loc;
				expr->value = TokenInt32Data();
				FWD();
				return expr;
			}
			else if (tt == STT_Float32Lit)
			{
				auto* expr = new Float32Expr;
				ast.unassignedNodes.AppendChild(expr);
				expr->SetReturnType(ast.GetFloat32Type());
				expr->loc = T().loc;
				expr->value = TokenFloatData();
				FWD();
				return expr;
			}
		}
		else if (tokens[start].type == STT_LParen && tokens[curToken - 1].type == STT_RParen)
		{
			// parenthesized subexpression
			size_t bcp = curToken;

			curToken = start + 1;
			Expr* rtexpr = ParseExpr(STT_RParen, bcp - 1);
			curToken = bcp;
			return rtexpr;
		}

		FILEStream err(stderr);
		err << "UNPARSED TOKENS {";
		for (size_t i = start; i < curToken; ++i)
			err << " " << TokenToString(i);
		err << " }\n";

		EmitError("empty expression");
		return CreateVoidExpr();
	}

	auto ttSplit = tokens[bestSplit].type;
	if (start < bestSplit && ttSplit == STT_LParen)
	{
		size_t bkCur = curToken;

		// constructor
		if (bestSplit - start == 1 && tokens[start].type == STT_Ident)
		{
			if (auto* ty = ast.GetTypeByName(TokenStringC(start)))
			{
				auto* ilist = new InitListExpr;
				ast.unassignedNodes.AppendChild(ilist);
				ilist->SetReturnType(ty);
				ilist->loc = tokens[bestSplit].loc;

				curToken = bestSplit + 1;
				ParseInitList(ilist, ty->GetAccessPointCount(), true);

				if (ty->IsNumericBased() == false)
				{
					curToken = start;
					EmitError("constructors only defined for numeric base types");
				}

				curToken = bkCur;
				return ilist;
			}
		}

		curToken = start;
		Expr* funcName = ParseExpr(endTokenType, bestSplit);

		auto* fcall = new OpExpr;
		ast.unassignedNodes.AppendChild(fcall);
		fcall->SetReturnType(ast.GetVoidType());
		fcall->loc = tokens[bestSplit].loc;

		curToken = bestSplit + 1;
		ParseExprList(fcall, STT_RParen, endPos);

		curToken = bkCur;
		FindFunction(fcall, funcName, tokens[start].loc);

		return fcall;
	}
	else if (start < bestSplit && ttSplit == STT_LBracket)
	{
		size_t bkCur = curToken;

		auto* idx = new IndexExpr;
		ast.unassignedNodes.AppendChild(idx);
		idx->SetReturnType(ast.GetVoidType());
		idx->loc = tokens[bestSplit].loc;

		curToken = start;
		idx->AppendChild(ParseExpr(endTokenType, bestSplit));

		curToken = bestSplit + 1;
		ParseExprList(idx, STT_RBracket, endPos);

		if (idx->childCount != 2)
		{
			EmitError("expected one subexpression as array index");
		}
		else if (idx->GetSource()->GetReturnType()->IsIndexable() == false)
		{
			EmitError("type '" + idx->GetSource()->GetReturnType()->GetName()
				+ "' is not indexable, expected array, vector or matrix");
		}
		else if (idx->GetIndex()->GetReturnType()->IsNumericOrVM1() &&
			TryCastExprTo(idx->GetIndex(),
			idx->GetIndex()->GetReturnType()->IsFloatBased()
				? ast.CastToScalar(idx->GetIndex()->GetReturnType())
				: ast.GetInt32Type(),
			"index"))
		{
			idx->SetReturnType(idx->GetSource()->GetReturnType()->subType);

			// TODO validate constant indices
		}
		else
		{
			EmitError("type '" + idx->GetIndex()->GetReturnType()->GetName()
				+ "' is not a valid index type");
		}

		curToken = bkCur;
		return idx;
	}
	else if (start < bestSplit && ttSplit == STT_OP_Ternary)
	{
		size_t bkCur = curToken;

		auto* tnop = new TernaryOpExpr;
		ast.unassignedNodes.AppendChild(tnop);
		tnop->SetReturnType(ast.GetVoidType());
		tnop->loc = tokens[bestSplit].loc;

		curToken = start;
		tnop->AppendChild(ParseExpr(STT_OP_Ternary, bestSplit));

		curToken = bestSplit + 1;
		tnop->AppendChild(ParseExpr(STT_Colon, bkCur));

		EXPECT(STT_Colon);
		curToken++;
		tnop->AppendChild(ParseExpr(endTokenType, bkCur));

		TryCastExprTo(tnop->GetCond(), ast.GetBoolType(), "ternary operator condition");

		ASTType* rt = Promote(tnop->GetTrueExpr()->GetReturnType(), tnop->GetFalseExpr()->GetReturnType());
		if (rt)
		{
			if (TryCastExprTo(tnop->GetTrueExpr(), rt, "ternary operator first choice") &&
				TryCastExprTo(tnop->GetTrueExpr(), rt, "ternary operator second choice"))
			{
				tnop->SetReturnType(rt);
			}
		}
		else
		{
			EmitError("cannot find a common type for ternary operator", tokens[bestSplit].loc);
		}

		curToken = bkCur;
		return tnop;
	}
	else // operator
	{
		if (bestSplit == start)
		{
			if (ttSplit == STT_OP_Add ||
				ttSplit == STT_OP_Sub ||
				ttSplit == STT_OP_Not ||
				ttSplit == STT_OP_Inv ||
				ttSplit == STT_OP_Inc ||
				ttSplit == STT_OP_Dec)
			{
				// unary operators
				auto tt = ttSplit;
				size_t bcp = curToken;

				curToken = start + 1;
				Expr* rtexpr = ParseExpr(STT_NULL, bcp);
				curToken = bcp;

				if (tt == STT_OP_Add)
					return rtexpr;

				if (tt == STT_OP_Inc ||
					tt == STT_OP_Dec)
				{
					auto* idop = new IncDecOpExpr;
					ast.unassignedNodes.AppendChild(idop);
					idop->loc = tokens[bestSplit].loc;
					idop->dec = tt == STT_OP_Dec;
					idop->post = false;
					idop->SetSource(rtexpr);
					idop->SetReturnType(idop->GetSource()->GetReturnType());
					return idop;
				}

				auto* unop = new UnaryOpExpr;
				ast.unassignedNodes.AppendChild(unop);
				unop->loc = tokens[bestSplit].loc;
				unop->opType = tt;
				unop->SetSource(rtexpr);
				unop->SetReturnType(unop->GetSource()->GetReturnType());

				if (tt == STT_OP_Not)
					unop->SetReturnType(ast.CastToBool(unop->GetReturnType()));
				else if (tt == STT_OP_Inv)
					unop->SetReturnType(ast.CastToInt(unop->GetReturnType()));

				CastExprTo(unop->GetSource(), unop->GetReturnType());

				return unop;
			}
			if (ttSplit == STT_LParen)
			{
				// explicit cast
				size_t bcp = curToken;

				curToken = bestSplit + 1;
				ASTType* tgtType = ParseType();
				EXPECT(STT_RParen);
				curToken++;

				auto* cast = new CastExpr;
				ast.unassignedNodes.AppendChild(cast);
				cast->loc = tokens[bestSplit].loc;
				cast->SetReturnType(tgtType);
				cast->SetSource(ParseExpr(endTokenType, bcp));
				curToken = bcp;

				if (CanCast(cast->GetSource()->GetReturnType(), tgtType, true) == false)
				{
					EmitError("cannot cast from '" + cast->GetSource()->GetReturnType()->GetName()
						+ "' to '" + tgtType->GetName() + "'");
				}

				return cast;
			}
		}
		else if (ttSplit == STT_OP_Inc
			|| ttSplit == STT_OP_Dec)
		{
			size_t bcp = curToken;

			curToken = start;
			Expr* rtexpr = ParseExpr(STT_NULL, bestSplit);
			curToken = bcp;

			auto* idop = new IncDecOpExpr;
			ast.unassignedNodes.AppendChild(idop);
			idop->loc = tokens[bestSplit].loc;
			idop->dec = tokens[bestSplit].type == STT_OP_Dec;
			idop->post = true;
			idop->SetSource(rtexpr);
			idop->SetReturnType(idop->GetSource()->GetReturnType());
			return idop;
		}

		size_t bkCur = curToken;
		curToken = start;
		bool prevIsWriteCtx = isWriteCtx;
		if (TokenIsOpAssign(ttSplit))
		{
			isWriteCtx = true;
		}
		Expr* lft = ParseExpr(endTokenType, bestSplit);
		isWriteCtx = prevIsWriteCtx;
		curToken = bkCur;

		if (ttSplit == STT_OP_Member)
		{
			if (bkCur - bestSplit == 2 && tokens[bestSplit + 1].type == STT_Ident)
			{
				std::string memberName = TokenStringData(bestSplit + 1);
				uint32_t memberID = 0;
				int swizzleComp = 0;
				if (auto* mmbTy = FindMemberType(lft->GetReturnType(), memberName, memberID, swizzleComp))
				{
					if (isWriteCtx && swizzleComp &&
						IsValidSwizzleWriteMask(memberID, lft->GetReturnType()->kind == ASTType::Matrix, mmbTy->GetElementCount()) == false)
					{
						EmitError("swizzle '" + memberName + "' is not valid for writing, cannot repeat components");
						return CreateVoidExpr();
					}

					auto* mmb = new MemberExpr;
					ast.unassignedNodes.AppendChild(mmb);
					mmb->loc = tokens[bestSplit].loc;
					mmb->SetSource(lft);
					mmb->memberName = memberName;
					mmb->memberID = memberID;
					mmb->swizzleComp = swizzleComp;
					mmb->SetReturnType(mmbTy);
					return mmb;
				}
				else
				{
					// error already printed
				//	EmitError( "member '" + memberName + "' for variable of type '" + lft->GetReturnType()->name + "' was not found" );
					return CreateVoidExpr();
				}
			}
			else
			{
				EmitError("expected identifier after '.'", tokens[bestSplit + 1].loc);
				return CreateVoidExpr();
			}
		}
		else
		{
			auto* binop = new BinaryOpExpr;
			ast.unassignedNodes.AppendChild(binop);
			binop->loc = tokens[bestSplit].loc;
			binop->SetReturnType(ast.GetVoidType());
			binop->opType = ttSplit;
			binop->AppendChild(lft); // LFT
			curToken = bestSplit + 1;
			binop->AppendChild(ParseExpr(endTokenType, endPos)); // RGT
			assert(binop->childCount == 2);

			ASTType* rt0 = binop->GetLft()->GetReturnType();
			ASTType* rt1 = binop->GetRgt()->GetReturnType();
			ASTType* commonType = nullptr;
			if (ttSplit == STT_OP_Assign)
			{
				commonType = rt0;
				if (CanCast(rt1, rt0, false) == false)
				{
					EmitError("cannot assign '" + rt1->GetName() +
						"' to variable of type '" + rt0->GetName() + "'");
				}
			}
			else
			{
				commonType = FindCommonOpType(rt0, rt1);
				if (ttSplit == STT_OP_LogicalAnd || ttSplit == STT_OP_LogicalOr)
				{
					commonType = ast.CastToBool(commonType);
				}
			}

			if (commonType == nullptr)
			{
				EmitError("cannot apply operator '" + TokenToString(bestSplit) +
					"' to types '" + rt0->GetName() + "' and '" + rt1->GetName() + "'",
					tokens[bestSplit].loc);

				curToken = bkCur;
				return binop;
			}

			binop->SetReturnType(TokenIsOpCompare(ttSplit) ? ast.CastToBool(commonType) : commonType);
			CastExprTo(binop->GetLft(), commonType);
			CastExprTo(binop->GetRgt(), commonType);

			if (ttSplit == STT_OP_Mul || ttSplit == STT_OP_Mod)
			{
				auto* op = new OpExpr;
				ast.unassignedNodes.AppendChild(op);
				op->loc = tokens[bestSplit].loc;
				op->SetReturnType(binop->GetReturnType());
				op->AppendChild(binop->GetLft());
				op->AppendChild(binop->GetLft());
				op->opKind = ttSplit == STT_OP_Mul ? Op_Multiply : Op_Modulus;
				delete binop;
				return op;
			}

			curToken = bkCur;
			return binop;
		}
	}
}

ASTType* Parser::Promote(ASTType* a, ASTType* b)
{
	// bool -> int -> float32
	//        half -> float32
	// scalar -> vector/matrix

	// can't promote types that do not resemble numbers
	if (a->IsNumericBased() == false || b->IsNumericBased() == false)
	{
		return nullptr;
	}

	// when one is a vector/matrix type, the other must be N/VM1
	if (a->kind == ASTType::Matrix || a->kind == ASTType::Vector)
	{
		if (a->IsSameSizeVM(b) == false && a->IsNumericOrVM1() == false && b->IsNumericOrVM1() == false)
			return nullptr;
	}
	if (b->kind == ASTType::Matrix || b->kind == ASTType::Vector)
	{
		if (b->IsSameSizeVM(a) == false && a->IsNumericOrVM1() == false && b->IsNumericOrVM1() == false)
			return nullptr;
	}

	ASTType* na = a->kind == ASTType::Vector || a->kind == ASTType::Matrix ? a->subType : a;
	ASTType* nb = b->kind == ASTType::Vector || b->kind == ASTType::Matrix ? b->subType : b;

	ASTType* no;
	if (na->kind == ASTType::Bool && nb->kind == ASTType::Bool)
		no = ast.GetInt32Type();
	else if (na == nb)
		no = na;
	else if (na->kind == ASTType::Float32 || nb->kind == ASTType::Float32 ||
		na->kind == ASTType::Float16 || nb->kind == ASTType::Float16)
		no = ast.GetFloat32Type();
	else if (na->kind == ASTType::Int32 || nb->kind == ASTType::Int32)
		no = ast.GetInt32Type();
	else if (na->kind == ASTType::UInt32 || nb->kind == ASTType::UInt32)
		no = ast.GetUInt32Type();
	else
		EmitFatalError("UNHANDLED SCALAR PROMOTION!");

	// both are scalars
	if (na == a && nb == b)
		return no;
	if (a->kind == ASTType::Vector || b->kind == ASTType::Vector)
	{
		int size = a->IsNumericOrVM1() == false ? a->sizeX : b->sizeX;
		return ast.GetVectorType(no, size);
	}
	if (a->kind == ASTType::Matrix || b->kind == ASTType::Matrix)
	{
		ASTType* mt = a->IsNumericOrVM1() == false ? a : b;
		return ast.GetMatrixType(no, mt->sizeX, mt->sizeY);
	}
	EmitFatalError("UNHANDLED PROMOTION!");
	return nullptr;
}

ASTType* Parser::FindCommonOpType(ASTType* rt0, ASTType* rt1)
{
	if (rt0 == rt1 && rt0->IsNumberBased())
	{
		// all operators work with equal number types
		return rt0;
	}
//	FILEStream(stderr) << "PROMOTE " << rt0->GetName() << "," << rt1->GetName() << ": ";
//	Promote( rt0, rt1 )->Dump( FILEStream(stderr) );
//	FILEStream(stderr) << "\n";
	if ((rt0->IsNumericOrVM1() && rt1->IsNumericBased()) ||
		(rt0->IsNumericBased() && rt1->IsNumericOrVM1()) ||
		(rt0->kind == ASTType::Vector && rt1->kind == ASTType::Vector) ||
		(rt0->kind == ASTType::Matrix && rt1->kind == ASTType::Matrix))
		return Promote(rt0, rt1);
	return nullptr;
}

static bool HLSLIsTypeClass_Scalar(ASTType* t)
{
	return t->IsNumericOrVM1();
}
static bool HLSLIsTypeClass_Vector(ASTType* t)
{
	return t->kind == ASTType::Vector
		;//|| (t->kind == ASTType::Matrix && (t->sizeX == 1 || t->sizeY == 1))
		;//|| t->IsNumericStructure();
}
static bool HLSLIsTypeClass_Matrix(ASTType* t)
{
	return t->kind == ASTType::Matrix;
}
bool Parser::CanCast(ASTType* from, ASTType* to, bool castExplicitly)
{
	if (from == to) return true; // shortcut for objects

	if (HLSLIsTypeClass_Scalar(from))
	{
		// scalar -> {scalar,vector,matrix}
		if (to->IsNumericBased()) return true;
		// scalar -> num.structure
	//	if (to->IsNumericStructure()) return true;
	}
	if (HLSLIsTypeClass_Vector(from))
	{
		// vector -> scalar
		if (HLSLIsTypeClass_Scalar(to)) return true;
		// vector -> {vector,num.structure}
		if (HLSLIsTypeClass_Vector(to)) return from->GetAccessPointCount() >= to->GetAccessPointCount();
		// vector -> matrix
		if (HLSLIsTypeClass_Matrix(to)) return from->GetAccessPointCount() == to->GetAccessPointCount();
	}
	if (HLSLIsTypeClass_Matrix(from))
	{
		// matrix -> scalar
		if (HLSLIsTypeClass_Scalar(to)) return true;
		// matrix -> {vector,num.structure}
		// > cannot [implicitly] convert from 'float4x4' to 'float4'
		if (HLSLIsTypeClass_Vector(to)) return from->GetAccessPointCount() == to->GetAccessPointCount();
		// matrix -> matrix
		// > cannot implicitly convert from 'float4x1' to 'float4' (which is float1x4)
		// > cannot [implicitly] convert from 'float4' (which is float1x4) to 'float4x1'
		if (HLSLIsTypeClass_Matrix(to)) return from->sizeX >= to->sizeX && from->sizeY >= to->sizeY;
	}
	if (castExplicitly)
	{
		bool fns = from->IsNumericStructure();
		bool tns = to->IsNumericStructure();
		if((fns || tns) &&
			(fns || from->IsNumericBased()) &&
			(tns || to->IsNumericBased()) &&
			(from->GetAccessPointCount() == to->GetAccessPointCount()
				|| from->IsNumericOrVM1() || to->IsNumericOrVM1()))
		{
			return true;
		}
	}

	return false;
}

void Parser::ParseExprList(ASTNode* out, SLTokenType endTokenType, size_t endPos)
{
	while (curToken < endPos && TT() != endTokenType)
	{
		out->AppendChild(ParseExpr(endTokenType, endPos));
		if (TT() != endTokenType)
		{
			EXPECT(STT_Comma);
			FWD();
		}
	}
}

void Parser::ParseInitList(ASTNode* out, int numItems, bool ctor)
{
	if (ctor)
	{
		ParseExprList(out, STT_RParen, SIZE_MAX);
	}
	else
	{
		int level = 1;
		while (level > 0)
		{
			while (TT() == STT_LBrace)
			{
				FWD();
				level++;
			}

			out->AppendChild(ParseExpr(STT_RBrace));

			while (TT() == STT_Comma || TT() == STT_RBrace)
			{
				if (TT() == STT_Comma)
				{
					FWD();
				}
				if (TT() == STT_RBrace)
				{
					level--;
					if (level == 0)
						break;
					FWD();
				}
			}
		}
	}

	int cnt = 0;
	for (ASTNode* ch = out->firstChild; ch; ch = ch->next)
	{
		int elc = ch->ToExpr()->GetReturnType()->GetAccessPointCount();
		if (elc)
		{
			cnt += elc;
		}
		else
		{
			EmitError(ctor ? "invalid type passed to constructor" : "invalid type passed to initializer");
		}
	}
	if (cnt != numItems)
	{
		EmitError(ctor ? "constructor arguments do not match type" : "initializer does not match type");
	}
}

struct VarDeclSaver
{
	VarDeclSaver(Parser* p) : parser(p), vdbackup(parser->funcInfo.scopeVars) {}
	~VarDeclSaver() { parser->funcInfo.scopeVars = vdbackup; }

	Parser* parser;
	VarDecl* vdbackup;
};

Stmt* Parser::ParseStatement()
{
	auto tt = TT();
	if (tt == STT_LBrace)
	{
		VarDeclSaver vds(this);

		auto* blockStmt = new BlockStmt;
		ast.unassignedNodes.AppendChild(blockStmt);
		blockStmt->loc = T().loc;
		FWD();
		while (TT() != STT_RBrace)
		{
			blockStmt->AppendChild(ParseStatement());
			FWD();
		}
		return blockStmt;
	}
	else if (tt == STT_KW_Return)
	{
		auto* ret = new ReturnStmt;
		ast.unassignedNodes.AppendChild(ret);
		ret->loc = T().loc;
		FWD();
		if (TT() != STT_Semicolon)
		{
			ret->SetExpr(ParseExpr());
			TryCastExprTo(ret->GetExpr(), funcInfo.func->GetReturnType(), "return expression");
		}
		ret->AddToFunction(funcInfo.func);
		return ret;
	}
	else if (tt == STT_KW_Discard)
	{
		if (ast.stage != ShaderStage_Pixel)
		{
			EmitError("'discard' is only supported for pixel shaders");
		}
		auto* dsc = new DiscardStmt;
		ast.unassignedNodes.AppendChild(dsc);
		dsc->loc = T().loc;
		FWD();
		EXPECT(STT_Semicolon);
		return dsc;
	}
	else if (tt == STT_KW_Break)
	{
		auto* bst = new BreakStmt;
		ast.unassignedNodes.AppendChild(bst);
		bst->loc = T().loc;
		FWD();
		EXPECT(STT_Semicolon);
		return bst;
	}
	else if (tt == STT_KW_Continue)
	{
		auto* cst = new ContinueStmt;
		ast.unassignedNodes.AppendChild(cst);
		cst->loc = T().loc;
		FWD();
		EXPECT(STT_Semicolon);
		return cst;
	}
	else if (tt == STT_KW_If)
	{
		auto* ifelse = new IfElseStmt;
		ast.unassignedNodes.AppendChild(ifelse);
		ifelse->loc = T().loc;
		FWD();
		EXPECT(STT_LParen);
		FWD();
		ifelse->AppendChild(ParseExpr(STT_RParen)); // COND

		auto* crt = ifelse->GetCond()->GetReturnType();
		if (crt->kind != ASTType::Bool)
		{
			if (crt->IsNumericOrVM1() == false)
			{
				EmitError("if - expected scalar condition value, got " + crt->GetName());
			}
			CastExprTo(ifelse->GetCond(), ast.GetBoolType());
		}
		FWD();

		ifelse->AppendChild(ParseStatement()); // TRUE
		FWD();
		if (TT() == STT_KW_Else)
		{
			FWD();
			ifelse->AppendChild(ParseStatement()); // FALSE
		}
		else curToken--;

		return ifelse;
	}
	else if (tt == STT_KW_While)
	{
		auto* whst = new WhileStmt;
		ast.unassignedNodes.AppendChild(whst);
		whst->loc = T().loc;
		FWD();
		EXPECT(STT_LParen);
		FWD();
		whst->AppendChild(ParseExpr(STT_RParen)); // COND

		auto* crt = whst->GetCond()->GetReturnType();
		if (crt->kind != ASTType::Bool)
		{
			if (crt->IsNumericOrVM1() == false)
			{
				EmitError("while - expected scalar condition value, got " + crt->GetName());
			}
			CastExprTo(whst->GetCond(), ast.GetBoolType());
		}
		FWD();

		whst->AppendChild(ParseStatement()); // BODY

		return whst;
	}
	else if (tt == STT_KW_Do)
	{
		auto* dwst = new DoWhileStmt;
		ast.unassignedNodes.AppendChild(dwst);
		dwst->loc = T().loc;
		FWD();

		dwst->PrependChild(ParseStatement()); // BODY
		FWD();
		EXPECT(STT_KW_While);
		FWD();
		EXPECT(STT_LParen);
		FWD();
		dwst->PrependChild(ParseExpr(STT_RParen)); // COND

		auto* crt = dwst->GetCond()->GetReturnType();
		if (crt->kind != ASTType::Bool)
		{
			if (crt->IsNumericOrVM1() == false)
			{
				EmitError("while - expected scalar condition value, got " + crt->GetName());
			}
			CastExprTo(dwst->GetCond(), ast.GetBoolType());
		}
		FWD();
		EXPECT(STT_Semicolon);

		return dwst;
	}
	else if (tt == STT_KW_For)
	{
		auto* forst = new ForStmt;
		ast.unassignedNodes.AppendChild(forst);
		forst->loc = T().loc;
		FWD();
		EXPECT(STT_LParen);
		FWD();

		forst->AppendChild(ParseExprDeclStatement()); // INIT
		EXPECT(STT_Semicolon);
		FWD();
		if (TT() != STT_Semicolon)
			forst->AppendChild(ParseExpr(STT_Semicolon)); // COND
		else
			forst->AppendChild(CreateVoidExpr());
		FWD();
		if (TT() != STT_RParen)
			forst->AppendChild(ParseExpr(STT_RParen)); // INCR
		else
			forst->AppendChild(CreateVoidExpr());
		FWD();
		forst->AppendChild(ParseStatement()); // BODY

		auto* crt = forst->GetCond()->GetReturnType();
		if (crt->kind != ASTType::Bool && crt->kind != ASTType::Void)
		{
			if (crt->IsNumericOrVM1() == false)
			{
				EmitError("for - expected scalar condition value, got " + crt->GetName());
			}
			CastExprTo(forst->GetCond(), ast.GetBoolType());
		}

		return forst;
	}
	else return ParseExprDeclStatement();
}

Stmt* Parser::ParseExprDeclStatement()
{
	auto tt = TT();
	if (tt == STT_Semicolon)
	{
		auto* bs = new EmptyStmt;
		ast.unassignedNodes.AppendChild(bs);
		bs->loc = T().loc;
		return bs;
	}
	else if (tt == STT_Ident && ast.IsTypeName(TokenStringC()))
	{
		auto* varDecl = new VarDeclStmt;
		ast.unassignedNodes.AppendChild(varDecl);
		varDecl->loc = T().loc;

		ASTType* commonType = ParseType();

		for (;;)
		{
			ASTType* curType = commonType;
			VarDecl* vd = new VarDecl;
			vd->loc = T().loc;
			varDecl->AppendChild(vd);

			EXPECT(STT_Ident);
			vd->name = TokenStringData();
			FWD();

			if (TT() == STT_LBracket)
			{
				FWD();
				EXPECT(STT_Int32Lit);
				int32_t arrSize = TokenInt32Data();
				FWD();
				EXPECT(STT_RBracket);
				FWD();

				if (arrSize < 1)
				{
					EmitError("expected positive (>= 1) array size");
				}
				else
				{
					curType = ast.GetArrayType(curType, arrSize);
				}
			}

			vd->SetType(curType);

			if (TT() == STT_OP_Assign)
			{
				FWD();

				if (TT() == STT_LBrace)
				{
					auto* ilist = new InitListExpr;
					ilist->loc = T().loc;
					vd->SetInitExpr(ilist);

					FWD();

					ilist->SetReturnType(curType);

					ParseInitList(ilist, curType->GetAccessPointCount(), false);

					EXPECT(STT_RBrace);
					FWD();
				}
				else
				{
					vd->SetInitExpr(ParseExpr());
					TryCastExprTo(vd->GetInitExpr(), curType, "initialization expression");
				}
			}

			vd->prevScopeDecl = funcInfo.scopeVars;
			funcInfo.scopeVars = vd;
			funcInfo.func->tmpVars.push_back(vd);

			if (TT() == STT_Comma)
			{
				FWD();
				continue;
			}
			else break;
		}

		EXPECT(STT_Semicolon);

		return varDecl;
	}
	else
	{
		auto* out = new ExprStmt;
		ast.unassignedNodes.AppendChild(out);
		out->loc = T().loc;
		out->SetExpr(ParseExpr());

		EXPECT(STT_Semicolon);

		return out;
	}
}

bool Parser::TryCastExprTo(Expr* expr, ASTType* tty, const char* what)
{
	bool ret = CanCast(expr->GetReturnType(), tty, false);
	CastExprTo(expr, tty);
	if (ret)
	{
		return true;
	}
	else
	{
		EmitError("cannot cast " + std::string(what) + " from '"
			+ expr->GetReturnType()->GetName() + "' to '"
			+ tty->GetName() + "'");
		return false;
	}
}

int32_t Parser::ParseRegister(char ch, bool comp, int32_t limit)
{
	char errbuf[128];

	const char* type;
	switch (ch)
	{
	case 'c': type = comp ? "packoffset" : "constant"; break;
	case 'b': type = "buffer"; break;
	case 's': type = "sampler"; break;
	default: type = "?"; break;
	}

	EXPECT(STT_Ident);
	std::string regname = TokenStringData();
	if (regname.size() < 2 || regname[0] != ch)
	{
		sprintf(errbuf, "bad %s register, expected %c<number>%s",
			type, ch, comp ? "[.x|y|z|w]" : "");
		EmitError(errbuf);
		return -1;
	}

	uint64_t num = 0;
	size_t i = 1;
	for (; i < regname.size(); ++i)
	{
		char c = regname[i];
		if (comp && c == '.')
			break;
		if (c < '0' || c > '9')
		{
			sprintf(errbuf, "bad %s register, expected %c<number>%s",
				type, ch, comp ? "[.x|y|z|w]" : "");
			EmitError(errbuf);
			return -1;
		}
		num *= 10;
		num += c - '0';
		if (num >= uint64_t(limit))
		{
			sprintf(errbuf, "exceeded %s register range (0-%d)", type, limit - 1);
			EmitError(errbuf);
			return -1;
		}
	}
	FWD();
	if (comp)
		num *= 4;
	if (comp && TT() == STT_OP_Member)
	{
		FWD();
		EXPECT(STT_Ident);
		if (TokenStringDataEquals(STRLIT_SIZE("x")));
		else if (TokenStringDataEquals(STRLIT_SIZE("y"))) num += 1;
		else if (TokenStringDataEquals(STRLIT_SIZE("z"))) num += 2;
		else if (TokenStringDataEquals(STRLIT_SIZE("w"))) num += 3;
		else
		{
			EmitError("bad packoffset component");
			return -1;
		}
		FWD();
	}
	return num;
}

void Parser::ParseDecl()
{
	auto tt = TT();
	if (tt == STT_KW_Struct)
	{
		FWD();
		EXPECT(STT_Ident);

		std::string name = TokenStringData();
		if (ast.IsTypeName(name.c_str()))
			EmitError("type name already used: " + name);

		FWD();
		EXPECT(STT_LBrace);
		FWD();

		uint32_t totalAPCount = 0;
		std::vector<AccessPointDecl> members;
		while (TT() != STT_RBrace)
		{
			AccessPointDecl member;

			member.type = ParseType();
			totalAPCount += member.type->GetAccessPointCount();

			EXPECT(STT_Ident);
			member.name = TokenStringData();
			FWD();

			ParseSemantic(member.semanticName, member.semanticIndex);

			EXPECT(STT_Semicolon);
			FWD();

			members.push_back(std::move(member));
		}

		ASTStructType* stc = ast.CreateStructType(name);
		stc->totalAccessPointCount = totalAPCount;
		std::swap(stc->members, members);

		FWD();
		EXPECT(STT_Semicolon);
		curToken++;
	}
	else if (tt == STT_KW_CBuffer)
	{
		FWD();
		EXPECT(STT_Ident);

		auto* cb = new CBufferDecl;
		cb->name = TokenStringData();
		cb->loc = T().loc;
		ast.globalVars.AppendChild(cb);

		FWD();
		if (TT() == STT_Colon)
		{
			FWD();
			EXPECT(STT_KW_Register);
			FWD();
			EXPECT(STT_LParen);
			FWD();
			cb->bufRegID = ParseRegister('b', false, 14);
			EXPECT(STT_RParen);
			FWD();
		}

		EXPECT(STT_LBrace);
		FWD();

		while (TT() != STT_RBrace)
		{
			auto* vd = new VarDecl;
			vd->loc = T().loc;
			cb->AppendChild(vd);

			vd->prevScopeDecl = funcInfo.scopeVars;
			funcInfo.scopeVars = vd;

			vd->loc = T().loc;
			vd->type = ParseType();
			vd->flags |= VarDecl::ATTR_Uniform | VarDecl::ATTR_Global;

			EXPECT(STT_Ident);
			vd->name = TokenStringData();
			FWD();

			if (TT() == STT_Colon)
			{
				FWD();
				EXPECT(STT_KW_PackOffset);
				FWD();
				EXPECT(STT_LParen);
				FWD();
				vd->regID = ParseRegister('c', true, 0x7ffffff);
				EXPECT(STT_RParen);
				FWD();
			}

			EXPECT(STT_Semicolon);
			FWD();
		}
		curToken++;
	}
	else if (tt == STT_KW_Const
		|| tt == STT_KW_Uniform
		|| tt == STT_KW_Static
		|| (tt == STT_Ident && ast.IsTypeName(TokenStringC())))
	{
		uint32_t flags = VarDecl::ATTR_Global;
		if (tt == STT_KW_Const)
		{
			EmitError("use 'static const' to create a compile-time constant");
			flags |= VarDecl::ATTR_Const;
			FWD();
		}
		else if (tt == STT_KW_Uniform)
		{
			flags |= VarDecl::ATTR_Uniform;
			FWD();
		}
		else if (tt == STT_KW_Static)
		{
			flags |= VarDecl::ATTR_Static;
			FWD();
			if (TT() == STT_KW_Const)
			{
				flags |= VarDecl::ATTR_Const;
				FWD();
			}
		}

		auto loc = T().loc;
		ASTType* commonType = ParseType(true);

		EXPECT(STT_Ident);
		std::string name = TokenStringData();
		FWD();

		if (TT() == STT_LParen)
		{
			BlockStmt tmpStmt;
			auto* func = new ASTFunction;
			ast.functionList.AppendChild(func);
			func->AppendChild(&tmpStmt); // placeholder for body
			func->SetReturnType(commonType);
			func->name = name;
			func->loc = loc;
			if (name == entryPointName)
			{
				entryPointCount++;
				ast.entryPoint = func;
			}

			FWD();

			ParseArgList(func);

			EXPECT(STT_RParen);
			FWD();

			VarDeclSaver vds(this);
			std::string mangledName = "F" + std::to_string(name.size()) + name;
			for (ASTNode* arg = func->GetFirstArg(); arg; arg = arg->next)
			{
				auto* vd = arg->ToVarDecl();
				mangledName += "_";
				vd->GetMangling(mangledName);
				vd->prevScopeDecl = funcInfo.scopeVars;
				funcInfo.scopeVars = vd;
			}
			// demangle entry point name since it's unique
			func->mangledName = ast.entryPoint == func ? entryPointName : mangledName;

			ParseSemantic(func->returnSemanticName, func->returnSemanticIndex);

			EXPECT(STT_LBrace);

			funcInfo.func = func;
			func->GetCode()->ReplaceWith(ParseStatement());
			funcInfo.func = nullptr;

			auto& funcsWithName = functions[name];
			for (ASTFunction* otherFN : funcsWithName)
			{
				if (otherFN->mangledName == mangledName)
				{
					EmitError("function '" + name + "' redefined with same parameters");
					break;
				}
			}
			funcsWithName.push_back(func);

			curToken++;
		}
		else
		{
			ASTType* type = commonType;
			if (type->IsVoid())
				EmitError("void type can only be used as function return value");

			while (TT() == STT_LBracket)
			{
				FWD();
				EXPECT(STT_Int32Lit);
				int32_t arrSize = TokenInt32Data();
				FWD();
				EXPECT(STT_RBracket);
				FWD();

				if (arrSize < 1)
				{
					EmitError("expected positive (>= 1) array size");
				}
				else
				{
					type = ast.GetArrayType(type, arrSize);
				}
			}

			auto* gvardef = ast.CreateGlobalVar();
			gvardef->SetType(type);
			gvardef->name = name;

			if (flags & VarDecl::ATTR_Static)
			{
				if (TT() != STT_OP_Assign)
				{
					EmitError("expected initialization expression for 'static const'");
				}
				else
				{
					FWD();
					if (TT() == STT_LBrace)
					{
						auto* ile = new InitListExpr;
						ile->loc = T().loc;
						ile->SetReturnType(type);
						gvardef->AppendChild(ile);

						FWD();
						ParseInitList(ile, type->GetAccessPointCount(), false);

						EXPECT(STT_RBrace);
						FWD();
					}
					else
					{
						gvardef->AppendChild(ParseExpr());
					}
				}
			}
			else
			{
				if (TT() == STT_OP_Assign)
				{
					EmitError("cannot have initialization expression for uniforms");
				}
				flags |= VarDecl::ATTR_Uniform;
			}
			gvardef->flags = flags;

			if (TT() == STT_Colon)
			{
				FWD();
				EXPECT(STT_KW_Register);
				FWD();
				EXPECT(STT_LParen);
				FWD();
				gvardef->regID = gvardef->type->IsSampler() ?
					ParseRegister('s', false, 16) :
					ParseRegister('c', false, 0x7ffffff);
				EXPECT(STT_RParen);
				FWD();
			}

			gvardef->prevScopeDecl = funcInfo.scopeVars;
			funcInfo.scopeVars = gvardef;

			EXPECT(STT_Semicolon);
			curToken++;
		}
	}
	else
	{
		EmitFatalError("unexpected token: " + TokenToString());
	}
}


static bool TokenIsExprPreceding(SLTokenType tt)
{
//	FILEStream(stderr) << "preceding:" << TokenTypeToString(tt) << "\n";
	switch (tt)
	{
	case STT_Ident:
	case STT_RParen:
	case STT_RBracket:
	case STT_StrLit:
	case STT_BoolLit:
	case STT_Int32Lit:
	case STT_Float32Lit:
		return true;
	default:
		return false;
	}
}

int Parser::GetSplitScore(const std::vector<SLToken>& tokenArr,
	size_t pos, size_t start, bool preProcSplit)
{
	// http://en.cppreference.com/w/c/language/operator_precedence

	auto tt = tokenArr[pos].type;

	if (TokenIsOpAssign(tt)) return 14 | SPLITSCORE_RTLASSOC;

	if (tt == STT_OP_Ternary) return 13 | SPLITSCORE_RTLASSOC;

	if (start < pos && TokenIsExprPreceding(tokenArr[pos - 1].type))
	{
		if (tt == STT_OP_LogicalOr) return 12;
		if (tt == STT_OP_LogicalAnd) return 11;
		if (tt == STT_OP_Or) return 10;
		if (tt == STT_OP_Xor) return 9;
		if (tt == STT_OP_And) return 8;

		if (tt == STT_OP_Eq || tt == STT_OP_NEq) return 7;
		if (tt == STT_OP_Less || tt == STT_OP_LEq || tt == STT_OP_Greater || tt == STT_OP_GEq)
			return 6;

		if (tt == STT_OP_Lsh || tt == STT_OP_Rsh) return 5;

		if (tt == STT_OP_Add || tt == STT_OP_Sub) return 4;
		if (tt == STT_OP_Mul || tt == STT_OP_Div || tt == STT_OP_Mod) return 3;
	}

	// unary operators
	if (pos == start)
	{
		if (tt == STT_OP_Inc || tt == STT_OP_Dec ||
			tt == STT_OP_Add || tt == STT_OP_Sub ||
			tt == STT_OP_Inv || tt == STT_OP_Not)
			return 2 | SPLITSCORE_RTLASSOC;
		// explicit cast
		if (!preProcSplit &&
			tt == STT_LParen &&
			pos + 1 < tokenArr.size() &&
			tokenArr[pos + 1].type == STT_Ident &&
			ast.IsTypeName(TokenStringC(tokenArr[pos + 1])))
			return 2 | SPLITSCORE_RTLASSOC;
	}

	if (!preProcSplit && start < pos)
	{
		if (tt == STT_OP_Inc || tt == STT_OP_Dec)
			return 1;
		// function call
		if (tt == STT_LParen) return 1;
		// array index
		if (tt == STT_LBracket) return 1;
	}

	// member operator
	if (tt == STT_OP_Member) return 1;

	return -1;
}

bool Parser::TokenStringDataEquals(const SLToken& t, const char* comp, size_t compsz) const
{
	auto tt = t.type;
	if (tt != STT_Ident && tt != STT_StrLit)
		return false;
	int32_t	len = 0;
	memcpy(&len, &tokenData[t.dataOff], 4);
	return len == compsz && memcmp((const char*) &tokenData[t.dataOff + 4], comp, compsz) == 0;
}

const char* Parser::TokenStringC(const SLToken& t) const
{
	if (t.type != STT_Ident && t.type != STT_StrLit)
		return nullptr;
	return (const char*) &tokenData[t.dataOff + 4];
}

std::string Parser::TokenStringData(const SLToken& t) const
{
	auto tt = t.type;
	if (tt != STT_Ident && tt != STT_StrLit)
		return {};
	int32_t	len = 0;
	memcpy(&len, &tokenData[t.dataOff], 4);
	return std::string((const char*) &tokenData[t.dataOff + 4], len);
}

bool Parser::TokenBoolData(const SLToken& t) const
{
	if (t.type != STT_BoolLit)
		return false;
	return t.dataOff != 0;
}

int32_t Parser::TokenInt32Data(const SLToken& t) const
{
	if (t.type != STT_Int32Lit)
		return 0;
	int32_t data;
	memcpy(&data, &tokenData[t.dataOff], 4);
	return data;
}

double Parser::TokenFloatData(const SLToken& t) const
{
	if (t.type != STT_Float32Lit)
		return 0;
	double data;
	memcpy(&data, &tokenData[t.dataOff], 8);
	return data;
}

std::string Parser::TokenToString(const SLToken& t) const
{
	char bfr[32];
	switch (t.type)
	{
	case STT_Ident: return TokenStringData(t);
	case STT_StrLit: return TokenStringData(t);
	case STT_BoolLit: return TokenBoolData(t) ? "true" : "false";
	case STT_Int32Lit:
		sprintf(bfr, "%d", TokenInt32Data(t));
		return bfr;
	case STT_Float32Lit:
		sprintf(bfr, "%gf", TokenFloatData(t));
		return bfr;
	default:
		return TokenTypeToString(t.type);
	}
}

bool Parser::TokenStringDataEquals(size_t i, const char* comp, size_t compsz) const
{
	return TokenStringDataEquals(tokens[i], comp, compsz);
}
const char* Parser::TokenStringC(size_t i) const { return TokenStringC(tokens[i]); }
std::string Parser::TokenStringData(size_t i) const { return TokenStringData(tokens[i]); }
bool Parser::TokenBoolData(size_t i) const { return TokenBoolData(tokens[i]); }
int32_t Parser::TokenInt32Data(size_t i) const { return TokenInt32Data(tokens[i]); }
double Parser::TokenFloatData(size_t i) const { return TokenFloatData(tokens[i]); }
std::string Parser::TokenToString(size_t i) const { return TokenToString(tokens[i]); }

bool Parser::TokenStringDataEquals(const char* comp, size_t compsz) const
{
	return TokenStringDataEquals(tokens[curToken], comp, compsz);
}
const char* Parser::TokenStringC() const { return TokenStringC(tokens[curToken]); }
std::string Parser::TokenStringData() const { return TokenStringData(tokens[curToken]); }
bool Parser::TokenBoolData() const { return TokenBoolData(tokens[curToken]); }
int32_t Parser::TokenInt32Data() const { return TokenInt32Data(tokens[curToken]); }
double Parser::TokenFloatData() const { return TokenFloatData(tokens[curToken]); }
std::string Parser::TokenToString() const { return TokenToString(tokens[curToken]); }


