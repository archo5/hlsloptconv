

#pragma once
#include "compiler.hpp"


struct PreprocMacro
{
	std::vector<std::string> args;
	std::vector<SLToken> tokens;
	bool isFunc = false;
};
typedef std::unordered_map<std::string, PreprocMacro> PreprocMacroMap;

struct CurFunctionInfo
{
	ASTFunction* func = nullptr;
	VarDecl* scopeVars = nullptr;
};

struct Parser
{
	Parser(Diagnostic& d, ShaderStage s, LoadIncludeFilePFN lifpfn, void* lifud) :
		diag(d),
		stage(s),
		loadIncludeFilePFN(lifpfn),
		loadIncludeFileUD(lifud)
	{
		// for int bool token
		int32_t i01[2] = { 0, 1 };
		tokenData.insert(tokenData.end(),
			reinterpret_cast<char*>(i01),
			reinterpret_cast<char*>(i01 + 2));
	}
	void ParseCode(const char* text);
	void ParseTokens(const char* text, uint32_t source);
	void PreprocessTokens(PreprocMacroMap macros, uint32_t source);

	SLToken RequestIntBoolToken(bool v);
	int EvaluateConstantIntExpr(const std::vector<SLToken>& tokenArr, size_t startPos, size_t endPos);

	ASTType* GetType(const std::string& name);
	ASTType* ParseType(bool isFuncRet = false);
	ASTType* FindMemberType(ASTType* t, const std::string& name, uint32_t& memberID, int& swizzleComp);
	VoidExpr* CreateVoidExpr(); // for errors
	void ParseArgList(ASTNode* out);
	int32_t CalcOverloadMatchFactor(ASTFunction* func, FCallExpr* fcall, ASTType** equalArgs, bool err);
	void FindFunction(FCallExpr* fcall, const Location& loc);
	void FindBestSplit(const std::vector<SLToken>& tokenArr, bool allowFunctions,
		size_t& curPos, size_t endPos, SLTokenType endTokenType, size_t& bestSplit, int& bestScore);
	Expr* ParseExpr(SLTokenType endTokenType = STT_Semicolon, size_t endPos = SIZE_MAX);
	ASTType* Promote(ASTType* a, ASTType* b);
	ASTType* FindCommonOpType(ASTType* rt0, ASTType* rt1, SLTokenType token);
	bool CanCast(ASTType* from, ASTType* to, bool castExplicitly);
	void ParseExprList(ASTNode* out, SLTokenType endTokenType, size_t endPos);
	void ParseInitList(ASTNode* out, int numItems, bool ctor);
	Stmt* ParseStatement();
	Stmt* ParseExprDeclStatement();
	bool TryCastExprTo(Expr* expr, ASTType* tty, const char* what);
	int32_t ParseRegister(char ch, bool comp, int32_t limit);
	void ParseDecl();

	const SLToken& T() const { return tokens[curToken]; }
	SLTokenType TT() const { return tokens[curToken].type; }

	void FWD(std::vector<SLToken>& arr, size_t& i)
	{
		if (++i >= arr.size())
			EmitFatalError("unexpected end of file", false);
	}
	void FWD() { FWD(tokens, curToken); }

	void PPFWD(std::vector<SLToken>& arr, size_t& i)
	{
		FWD(arr, i);
		if (arr[i - 1].logicalLine != arr[i].logicalLine)
			EmitFatalError("unexpected end of preprocessor directive", arr[i - 1].loc);
	}
	void PPFWD() { PPFWD(tokens, curToken); }

	void EXPECT(const SLToken& t, SLTokenType tt)
	{
		if (t.type != tt)
			EmitFatalError("expected '" + TokenTypeToString(tt) + "', got '" + TokenToString(t) + "'");
	}
	void EXPECT(SLTokenType tt) { EXPECT(T(), tt); }

	void EXPECTERR(const SLToken& t, const std::string& exp)
	{
		EmitFatalError("expected " + exp + ", got '" + TokenToString(t) + "'");
	}
	void EXPECTERR(const std::string& exp) { EXPECTERR(T(), exp); }

	void EmitError(const std::string& msg, const Location& loc)
	{
		diag.EmitError(msg, loc);
	}
	void EmitError(const std::string& msg, bool withLoc = true)
	{
		diag.EmitError(msg, withLoc && curToken < tokens.size() ? T().loc : Location::BAD());
	}
	void EmitFatalError(const std::string& msg, const Location& loc)
	{
		diag.EmitFatalError(msg, loc);
	}
	void EmitFatalError(const std::string& msg, bool withLoc = true)
	{
		diag.EmitFatalError(msg, withLoc && curToken < tokens.size() ? T().loc : Location::BAD());
	}

#define SPLITSCORE_RTLASSOC 0x80
	int GetSplitScore(SLTokenType tt, size_t pos, size_t start, bool allowFunctions) const;

	bool TokenStringDataEquals(const SLToken& t, const char* comp, size_t compsz) const;
	std::string TokenStringData(const SLToken& t) const;
	bool TokenBoolData(const SLToken& t) const;
	int32_t TokenInt32Data(const SLToken& t) const;
	double TokenFloatData(const SLToken& t) const;
	std::string TokenToString(const SLToken& t) const;

	bool TokenStringDataEquals(size_t i, const char* comp, size_t compsz) const;
	std::string TokenStringData(size_t i) const;
	bool TokenBoolData(size_t i) const;
	int32_t TokenInt32Data(size_t i) const;
	double TokenFloatData(size_t i) const;
	std::string TokenToString(size_t i) const;

	bool TokenStringDataEquals(const char* comp, size_t compsz) const;
	std::string TokenStringData() const;
	bool TokenBoolData() const;
	int32_t TokenInt32Data() const;
	double TokenFloatData() const;
	std::string TokenToString() const;


	bool IsAttrib() const
	{
		auto tt = TT();
		return tt == STT_KW_In || tt == STT_KW_Out || tt == STT_KW_InOut;
	}


	Diagnostic& diag;
	ShaderStage stage;
	LoadIncludeFilePFN loadIncludeFilePFN;
	void* loadIncludeFileUD;


	std::vector<std::string> filenames;
	std::vector<char> tokenData;
	std::vector<SLToken> tokens;
	size_t curToken = 0;
	bool isWriteCtx = false; // if current expression is part of a write

	CurFunctionInfo funcInfo;

	AST ast;
};


