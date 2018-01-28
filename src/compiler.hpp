

#pragma once
#include "common.hpp"

#include <unordered_map>


struct ASTStructType;
struct ASTNode;
struct ASTFunction;


enum SLTokenType
{
	STT_NULL = 0,
	
	STT_LParen, // (
	STT_RParen, // )
	STT_LBrace, // {
	STT_RBrace, // }
	STT_LBracket, // [
	STT_RBracket, // ]
	STT_Comma, // ,
	STT_Semicolon, // ;
	STT_Colon, // :
	STT_Hash, // #
	
	STT_Ident,
	STT_IdentPPNoReplace, // only used by preprocessor temporarily
	STT_StrLit,
	STT_BoolLit,
	STT_Int32Lit,
	STT_Float32Lit,
	
	STT_KW_Struct,
	STT_KW_Return,
	STT_KW_Discard,
	STT_KW_Break,
	STT_KW_Continue,
	STT_KW_If,
	STT_KW_Else,
	STT_KW_While,
	STT_KW_Do,
	STT_KW_For,
	STT_KW_In,
	STT_KW_Out,
	STT_KW_InOut,
	STT_KW_Const,
	STT_KW_Static,
	STT_KW_Uniform,
	STT_KW_CBuffer,
	STT_KW_Register,
	STT_KW_PackOffset,
	
	STT_OP_Eq,     /* ==   */
	STT_OP_NEq,    /* !=   */
	STT_OP_LEq,    /* <=   */
	STT_OP_GEq,    /* >=   */
	STT_OP_Less,   /* <    */
	STT_OP_Greater, /* >   */
	STT_OP_AddEq,  /* +=   */
	STT_OP_SubEq,  /* -=   */
	STT_OP_MulEq,  /* *=   */
	STT_OP_DivEq,  /* /=   */
	STT_OP_ModEq,  /* %=   */
	STT_OP_AndEq,  /* &=   */
	STT_OP_OrEq,   /* |=   */
	STT_OP_XorEq,  /* ^=   */
	STT_OP_LshEq,  /* <<=  */
	STT_OP_RshEq,  /* >>=  */
	STT_OP_Assign, /* =    */
	STT_OP_LogicalAnd, /* && */
	STT_OP_LogicalOr,  /* || */
	STT_OP_Add,    /* +    */
	STT_OP_Sub,    /* -    */
	STT_OP_Mul,    /* *    */
	STT_OP_Div,    /* /    */
	STT_OP_Mod,    /* %    */
	STT_OP_And,    /* &    */
	STT_OP_Or,     /* |    */
	STT_OP_Xor,    /* ^    */
	STT_OP_Lsh,    /* <<   */
	STT_OP_Rsh,    /* >>   */
	STT_OP_Member, /* .    */
	STT_OP_Not,    /* !    */
	STT_OP_Inv,    /* ~    */
	STT_OP_Inc,    /* ++   */
	STT_OP_Dec,    /* --   */
	STT_OP_Ternary, /* ?   */
};

bool TokenIsOpAssign(SLTokenType tt);
bool TokenIsOpCompare(SLTokenType tt);
std::string TokenTypeToString(SLTokenType tt);

struct SLToken
{
	SLTokenType type;
	Location loc;
	uint32_t logicalLine;
	uint32_t dataOff;
};


struct ASTType
{
	enum Kind
	{
		Void,
		Bool,
		Int32,
		UInt32,
		Float16,
		Float32,
		Vector,
		Matrix,
		Array,
		Structure,
		Function,
		Sampler1D,
		Sampler2D,
		Sampler3D,
		SamplerCUBE,
	};

	struct SubTypeCount
	{
		bool IsNumeric() const { return numeric && !other; }
		SubTypeCount& operator += (const SubTypeCount& o)
		{
			numeric += o.numeric;
			other += o.other;
			return *this;
		}

		int numeric = 0;
		int other = 0;
	};

	ASTType() {} // for array init
	ASTType(Kind k) : kind(k) {}
	ASTType(ASTType* sub, uint8_t x) : kind(Vector), subType(sub), sizeX(x) {}
	ASTType(ASTType* sub, uint8_t x, uint8_t y) : kind(Matrix), subType(sub), sizeX(x), sizeY(y) {}

	unsigned GetElementCount() const;
	unsigned GetAccessPointCount() const;
	SubTypeCount CountSubTypes() const;
	Kind GetNVMKind() const;
	void GetMangling(std::string& out) const;
	virtual void Dump(OutStream& out) const;
	std::string GetName() const;
	ASTStructType* ToStructType();
	const ASTStructType* ToStructType() const { return const_cast<ASTType*>(this)->ToStructType(); }
	
	bool IsVoid() const { return kind == Void; }
	bool IsSampler() const { return kind == Sampler1D || kind == Sampler2D || kind == Sampler3D || kind == SamplerCUBE; }
	bool IsFloat() const { return kind == Float16 || kind == Float32; }
	bool IsNumber() const { return kind == Int32 || kind == UInt32 || kind == Float16 || kind == Float32; }
	bool IsBoolBased() const { return kind == Bool || ((kind == Vector || kind == Matrix) && subType->kind == Bool); }
	bool IsIntBased() const { return kind == Int32 || kind == UInt32 || ((kind == Vector || kind == Matrix) && (subType->kind == Int32 || subType->kind == UInt32)); }
	bool IsFloatBased() const { return kind == Float16 || kind == Float32
		|| ((kind == Vector || kind == Matrix) && (subType->kind == Float16 || subType->kind == Float32)); }
	bool IsNumeric() const { return kind == Bool || IsNumber(); }
	bool IsNumericBased() const { return kind == Bool || IsNumber() || kind == Vector || kind == Matrix; }
	bool IsVM1() const { return (kind == Vector && sizeX == 1) || (kind == Matrix && sizeX * sizeY == 1); }
	int GetM1Dim() const { return kind == Matrix ? ( sizeX == 1 ? sizeY : ( sizeY == 1 ? sizeX : 0 ) ) : 0; }
	bool IsNumericOrVM1() const { return kind == Bool || IsNumber() || IsVM1(); }
	bool IsNumVector() const { return kind == Vector && subType->IsNumber(); }
	bool IsNumMatrix() const { return kind == Matrix && subType->IsNumber(); }
	bool IsNumberBased() const { return IsNumber() || IsNumVector() || IsNumMatrix(); }
	bool IsNumberOrVM1() const { return IsNumber() ||
		(IsNumVector() && sizeX == 1) ||
		(IsNumMatrix() && sizeX * sizeY == 1); }
	bool IsSameSizeVM(const ASTType* o) const
	{
		return (kind == Vector || kind == Matrix)
			&& kind == o->kind
			&& sizeX == o->sizeX
			&& (kind == Vector || sizeY == o->sizeY);
	}
	bool IsNumericStructure() const { return kind == Structure && CountSubTypes().IsNumeric(); }
	bool IsIndexable() const { return kind == Vector || kind == Matrix || kind == Array; }

	ASTNode* firstUse = nullptr;
	ASTNode* lastUse = nullptr;
	ASTType* nextAllocType = nullptr;
	ASTType* nextArrayType = nullptr;

	Kind kind = Void;
	// vector/matrix/array types
	ASTType* subType = nullptr;
	uint8_t sizeX = 1; // vector width / matrix rows
	uint8_t sizeY = 1; // matrix columns
	uint32_t elementCount = 1; // array size
};

struct AccessPointDecl
{
	void Dump(OutStream& out) const;
	int GetSemanticIndex() const { return semanticIndex >= 0 ? semanticIndex : 0; }

	std::string name;
	ASTType* type = nullptr;
	std::string semanticName;
	int semanticIndex = -1;
};

struct ASTStructType : ASTType
{
	ASTStructType();

	void Dump(OutStream& out) const;

	std::string name;
	std::vector<AccessPointDecl> members;
	uint32_t totalAccessPointCount = 0;
	ASTStructType* prevStructType = nullptr;
	ASTStructType* nextStructType = nullptr;
};


struct ASTNode
{
	FINLINE ASTNode() {}
	FINLINE ASTNode(const ASTNode&) {}
	virtual ~ASTNode();
	virtual void Dump(OutStream& out, int level = 0) const = 0;
	ASTNode* DeepClone() const;
	virtual ASTNode* Clone() const = 0;
#define IMPLEMENT_CLONE( cls ) \
	ASTNode* Clone() const override { return new cls(*this); }

	void Unlink();
	void InsertBefore(ASTNode* ch, ASTNode* before);
	void AppendChild(ASTNode* ch);
	ASTNode* ReplaceWith(ASTNode* ch); // returns this
	void SetFirst(ASTNode* ch);
	struct Expr* ToExpr();
	struct Stmt* ToStmt();
	struct VarDecl* ToVarDecl();
	ASTFunction* ToFunction();
	FINLINE const ASTFunction* ToFunction() const { return const_cast<ASTNode*>(this)->ToFunction(); }
	void ChangeAssocType(ASTType* t);

	FINLINE void InsertBeforeMe(ASTNode* ch) { parent->InsertBefore(ch, this); }
	FINLINE void InsertAfterMe(ASTNode* ch) { parent->InsertBefore(ch, next); }
	FINLINE void PrependChild(ASTNode* ch) { InsertBefore(ch, firstChild); }
	template< class T > FINLINE T* AppendChildT(T* ch) { AppendChild(ch); return ch; }

	void _RegisterTypeUse(ASTType* type);
	void _UnregisterTypeUse(ASTType* type);
	void _ChangeUsedType(ASTType*& mytype, ASTType* t);

	ASTNode* parent = nullptr;
	ASTNode* prev = nullptr; // siblings
	ASTNode* next = nullptr;
	ASTNode* firstChild = nullptr;
	ASTNode* lastChild = nullptr;
	ASTNode* prevTypeUse = nullptr;
	ASTNode* nextTypeUse = nullptr;
	int childCount = 0;
	Location loc = Location::BAD();
};

struct Stmt : ASTNode
{
};

struct Expr : ASTNode
{
	FINLINE Expr() {}
	Expr(const Expr& o);
	~Expr();
	ASTType* GetReturnType() const { return returnType; }
	void SetReturnType(ASTType* t);

	ASTType* returnType = nullptr;
};

struct EmptyStmt : Stmt
{
	IMPLEMENT_CLONE(EmptyStmt);
	void Dump(OutStream& out, int) const;
};

struct VoidExpr : Expr
{
	IMPLEMENT_CLONE(VoidExpr);
	void Dump(OutStream& out, int) const;
};

struct VarDecl : ASTNode, AccessPointDecl
{
	enum Flags
	{
		ATTR_In      = 0x0001,
		ATTR_Out     = 0x0002,
		ATTR_Uniform = 0x0004,
		ATTR_Const   = 0x0008,
		ATTR_Static  = 0x0010,
		ATTR_Hidden  = 0x0020, // does not get printed (for built-in in/out variables)
		ATTR_StageIO = 0x0040, // whether the vardecl is stage i/o, not (just) function i/o
	};

	FINLINE VarDecl() {}
	VarDecl(const VarDecl& o);
	~VarDecl();
	IMPLEMENT_CLONE(VarDecl);
	Expr* GetInitExpr() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	void SetInitExpr(Expr* e) { SetFirst(e); }
	ASTType* GetType() const { return type; }
	void SetType(ASTType* t);

	void GetMangling(std::string& out) const;
	void Dump(OutStream& out, int level = 0) const;

	uint32_t flags = 0;
	int32_t regID = -1;

	VarDecl* prevScopeDecl = nullptr; // previous declaration in scope

	// for use with later stages, to avoid hash tables:
	// - access point range
	mutable int APRangeFrom = 0;
	mutable int APRangeTo = 0;
	// - usage
	mutable bool used = false;
};

struct CBufferDecl : ASTNode
{
	// all children must be VarDecl

	IMPLEMENT_CLONE(CBufferDecl);
	void Dump(OutStream& out, int) const override;

	std::string name;
	int32_t bufRegID = -1;
};

struct DeclRefExpr : Expr
{
	IMPLEMENT_CLONE(DeclRefExpr);
	void Dump(OutStream& out, int) const override;

	std::string name;
	VarDecl* decl = nullptr;
};

struct ConstExpr : Expr
{
};

struct BoolExpr : ConstExpr
{
	BoolExpr() {}
	BoolExpr(bool v, ASTType* rt) : value(v) { SetReturnType(rt); }
	IMPLEMENT_CLONE(BoolExpr);
	void Dump(OutStream& out, int) const override;

	bool value = false;
};

struct Int32Expr : ConstExpr
{
	Int32Expr() {}
	Int32Expr(int32_t v, ASTType* rt) : value(v) { SetReturnType(rt); }
	IMPLEMENT_CLONE(Int32Expr);
	void Dump(OutStream& out, int) const override;

	int32_t value = 0;
};

struct Float32Expr : ConstExpr
{
	Float32Expr() {}
	Float32Expr(double v, ASTType* rt) : value(v) { SetReturnType(rt); }
	IMPLEMENT_CLONE(Float32Expr);
	void Dump(OutStream& out, int) const override;

	double value = 0;
};

struct CastExpr : Expr
{
	IMPLEMENT_CLONE(CastExpr);
	Expr* GetSource() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	void SetSource(Expr* e) { SetFirst(e); }
	void Dump(OutStream& out, int level) const override;
};

struct FCallExpr : Expr
{
	IMPLEMENT_CLONE(FCallExpr);
	Expr* GetFunc() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	ASTNode* GetFirstArg() const { return childCount >= 1 ? firstChild->next : nullptr; }
	int GetArgCount() const { return childCount >= 1 ? childCount - 1 : 0; }
	void Dump(OutStream& out, int level) const override;

	ASTFunction* resolvedFunc = nullptr;
	bool isBuiltinFunc = false;
};

struct InitListExpr : Expr
{
	IMPLEMENT_CLONE(InitListExpr);
	void Dump(OutStream& out, int level) const override;
};

struct IncDecOpExpr : Expr
{
	IMPLEMENT_CLONE(IncDecOpExpr);
	Expr* GetSource() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	void SetSource(Expr* e) { SetFirst(e); }

	void Dump(OutStream& out, int level) const override;

	bool dec = false;
	bool post = false;
};

struct UnaryOpExpr : Expr
{
	IMPLEMENT_CLONE(UnaryOpExpr);
	Expr* GetSource() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	void SetSource(Expr* e) { SetFirst(e); }

	void Dump(OutStream& out, int level) const override;

	SLTokenType opType = STT_NULL;
};

struct BinaryOpExpr : Expr
{
	IMPLEMENT_CLONE(BinaryOpExpr);
	Expr* GetLft() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	Expr* GetRgt() const { return firstChild != lastChild ? lastChild->ToExpr() : nullptr; }

	void Dump(OutStream& out, int level) const override;

	SLTokenType opType = STT_NULL;
};

struct TernaryOpExpr : Expr
{
	IMPLEMENT_CLONE(TernaryOpExpr);
	Expr* GetCond() const { return childCount >= 1 ? firstChild->ToExpr() : nullptr; }
	Expr* GetTrueExpr() const { return childCount >= 2 ? firstChild->next->ToExpr() : nullptr; }
	Expr* GetFalseExpr() const { return childCount >= 3 ? firstChild->next->next->ToExpr() : nullptr; }

	void Dump(OutStream& out, int level) const override;
};

struct SubValExpr : Expr
{
	Expr* GetSource() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	void SetSource(Expr* e) { SetFirst(e); }
};

struct MemberExpr : SubValExpr
{
	IMPLEMENT_CLONE(MemberExpr);
	void Dump(OutStream& out, int level) const override;

	std::string memberName;
	uint32_t memberID = 0;
	int swizzleComp = 0; // 0 - not a swizzle
};

struct IndexExpr : SubValExpr
{
	IMPLEMENT_CLONE(IndexExpr);
	Expr* GetIndex() const { return childCount >= 2 ? firstChild->next->ToExpr() : nullptr; }

	void Dump(OutStream& out, int level) const override;
};

struct ExprStmt : Stmt
{
	IMPLEMENT_CLONE(ExprStmt);
	Expr* GetExpr() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	void SetExpr(Expr* e) { SetFirst(e); }

	void Dump(OutStream& out, int level) const override;
};

struct BlockStmt : Stmt
{
	// all children must be Stmt

	IMPLEMENT_CLONE(BlockStmt);
	void Dump(OutStream& out, int level) const override;
};

struct ReturnStmt : Stmt
{
	~ReturnStmt() { RemoveFromFunction(); }
	IMPLEMENT_CLONE(ReturnStmt);

	void AddToFunction(ASTFunction* fn);
	void RemoveFromFunction();

	Expr* GetExpr() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	void SetExpr(Expr* e) { SetFirst(e); }

	void Dump(OutStream& out, int level) const override;

	ASTFunction* func = nullptr;
	ReturnStmt* prevRetStmt = nullptr;
	ReturnStmt* nextRetStmt = nullptr;
};

struct DiscardStmt : Stmt
{
	IMPLEMENT_CLONE(DiscardStmt);
	void Dump(OutStream& out, int level) const override;
};

struct BreakStmt : Stmt
{
	IMPLEMENT_CLONE(BreakStmt);
	void Dump(OutStream& out, int level) const override;
};

struct ContinueStmt : Stmt
{
	IMPLEMENT_CLONE(ContinueStmt);
	void Dump(OutStream& out, int level) const override;
};

struct IfElseStmt : Stmt
{
	IMPLEMENT_CLONE(IfElseStmt);
	Expr* GetCond() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	Stmt* GetTrueBr() const { return childCount >= 2 ? firstChild->next->ToStmt() : nullptr; }
	Stmt* GetFalseBr() const { return childCount >= 3 ? firstChild->next->next->ToStmt() : nullptr; }

	void Dump(OutStream& out, int level) const override;
};

struct WhileStmt : Stmt
{
	IMPLEMENT_CLONE(WhileStmt);
	Expr* GetCond() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	Stmt* GetBody() const { return childCount >= 2 ? firstChild->next->ToStmt() : nullptr; }

	void Dump(OutStream& out, int level) const override;
};

struct DoWhileStmt : Stmt
{
	IMPLEMENT_CLONE(DoWhileStmt);
	Expr* GetCond() const { return firstChild ? firstChild->ToExpr() : nullptr; }
	Stmt* GetBody() const { return childCount >= 2 ? firstChild->next->ToStmt() : nullptr; }

	void Dump(OutStream& out, int level) const override;
};

struct ForStmt : Stmt
{
	IMPLEMENT_CLONE(ForStmt);
	Stmt* GetInit() const { return childCount >= 1 ? firstChild->ToStmt() : nullptr; }
	Expr* GetCond() const { return childCount >= 2 ? firstChild->next->ToExpr() : nullptr; }
	Expr* GetIncr() const { return childCount >= 3 ? firstChild->next->next->ToExpr() : nullptr; }
	Stmt* GetBody() const { return childCount >= 4 ? firstChild->next->next->next->ToStmt() : nullptr; }

	void Dump(OutStream& out, int level) const override;
};

struct VarDeclStmt : Stmt
{
	// all children must be VarDecl

	IMPLEMENT_CLONE(VarDeclStmt);
	void Dump(OutStream& out, int level) const override;
};

struct ASTFunction : ASTNode
{
	~ASTFunction();
	IMPLEMENT_CLONE(ASTFunction);
	Stmt* GetCode() const { return firstChild ? firstChild->ToStmt() : nullptr; }
	// VarDecl:
	ASTNode* GetFirstArg() const { return childCount >= 1 ? firstChild->next : nullptr; }
	int GetArgCount() const { return childCount >= 1 ? childCount - 1 : 0; }

	ASTType* GetReturnType() const { return returnType; }
	void SetReturnType(ASTType* t);

	void Dump(OutStream& out, int level = 0) const;
	int GetReturnSemanticIndex() const { return returnSemanticIndex >= 0 ? returnSemanticIndex : 0; }

	ASTType* returnType = nullptr;
	std::string returnSemanticName;
	int returnSemanticIndex = -1;
	std::string name;
	std::string mangledName;
	ReturnStmt* firstRetStmt = nullptr;
	ReturnStmt* lastRetStmt = nullptr;
	std::vector<VarDecl*> tmpVars;
	bool used = false;
	bool isEntryPoint = false;
};

struct TypeSystem
{
	TypeSystem();
	~TypeSystem();
	void InitBasicTypes();
	ASTType* CastToBool(ASTType* t);
	ASTType* CastToInt(ASTType* t);
	ASTType* CastToFloat(ASTType* t);
	ASTType* CastToScalar(ASTType* t);
	ASTType* CastToVector(ASTType* t, int size = 1);
	ASTType* GetVectorType(ASTType* t, int size);
	const ASTType* GetVectorType(ASTType* t, int size) const {
		return const_cast<TypeSystem*>(this)->GetVectorType(t, size); }
	ASTType* GetMatrixType(ASTType* t, int sizeX, int sizeY);
	const ASTType* GetMatrixType(ASTType* t, int sizeX, int sizeY) const {
		return const_cast<TypeSystem*>(this)->GetMatrixType(t, sizeX, sizeY); }
	ASTType* GetArrayType(ASTType* t, uint32_t size);

	ASTStructType* CreateStructType(const std::string& name);

	ASTType* _GetSVMTypeByName(ASTType* t, const char* sub);
	ASTType* GetBaseTypeByName(const char* name);
	ASTStructType* GetStructTypeByName(const char* name);
	ASTType* GetTypeByName(const char* name);
	bool IsTypeName(const std::string& id);

	ASTType* firstAllocType = nullptr;
	ASTType* firstArrayType = nullptr;
	ASTStructType* firstStructType = nullptr;
	ASTStructType* lastStructType = nullptr;
	
	ASTType* GetVoidType()        { return &typeVoidDef; }
	ASTType* GetFunctionType()    { return &typeFunctionDef; }
	ASTType* GetSampler1DType()   { return &typeSampler1DDef; }
	ASTType* GetSampler2DType()   { return &typeSampler2DDef; }
	ASTType* GetSampler3DType()   { return &typeSampler3DDef; }
	ASTType* GetSamplerCUBEType() { return &typeSamplerCUBEDef; }
	ASTType* GetBoolType()        { return &typeBoolDef; }
	ASTType* GetInt32Type()       { return &typeInt32Def; }
	ASTType* GetUInt32Type()      { return &typeUInt32Def; }
	ASTType* GetFloat16Type()     { return &typeFloat16Def; }
	ASTType* GetFloat32Type()     { return &typeFloat32Def; }
	ASTType* GetBoolVecType   (int size) { return &typeBoolVecDefs   [size - 1]; }
	ASTType* GetInt32VecType  (int size) { return &typeInt32VecDefs  [size - 1]; }
	ASTType* GetUInt32VecType (int size) { return &typeUInt32VecDefs [size - 1]; }
	ASTType* GetFloat16VecType(int size) { return &typeFloat16VecDefs[size - 1]; }
	ASTType* GetFloat32VecType(int size) { return &typeFloat32VecDefs[size - 1]; }
	ASTType* GetBoolMtxType   (int sizeX, int sizeY) { return &typeBoolMtxDefs   [(sizeX - 1) + (sizeY - 1) * 4]; }
	ASTType* GetInt32MtxType  (int sizeX, int sizeY) { return &typeInt32MtxDefs  [(sizeX - 1) + (sizeY - 1) * 4]; }
	ASTType* GetUInt32MtxType (int sizeX, int sizeY) { return &typeUInt32MtxDefs [(sizeX - 1) + (sizeY - 1) * 4]; }
	ASTType* GetFloat16MtxType(int sizeX, int sizeY) { return &typeFloat16MtxDefs[(sizeX - 1) + (sizeY - 1) * 4]; }
	ASTType* GetFloat32MtxType(int sizeX, int sizeY) { return &typeFloat32MtxDefs[(sizeX - 1) + (sizeY - 1) * 4]; }
	
	ASTType typeVoidDef;
	ASTType typeFunctionDef;
	ASTType typeSampler1DDef;
	ASTType typeSampler2DDef;
	ASTType typeSampler3DDef;
	ASTType typeSamplerCUBEDef;
	ASTType typeBoolDef;
	ASTType typeInt32Def;
	ASTType typeUInt32Def;
	ASTType typeFloat16Def;
	ASTType typeFloat32Def;
	ASTType typeBoolVecDefs   [4];
	ASTType typeInt32VecDefs  [4];
	ASTType typeUInt32VecDefs [4];
	ASTType typeFloat16VecDefs[4];
	ASTType typeFloat32VecDefs[4];
	ASTType typeBoolMtxDefs   [16];
	ASTType typeInt32MtxDefs  [16];
	ASTType typeUInt32MtxDefs [16];
	ASTType typeFloat16MtxDefs[16];
	ASTType typeFloat32MtxDefs[16];
};

typedef std::unordered_map<std::string, ASTFunction*> ASTFuncMap;
struct AST : TypeSystem
{
	VarDecl* CreateGlobalVar();
	void MarkUsed(Diagnostic& diag, const std::string& entryPoint);
	void Dump(OutStream& out) const;

	ShaderStage stage;
	BlockStmt functionList;
	BlockStmt globalVars; // contains VarDecl/CBufferDecl nodes only
	std::unordered_map<std::string, ASTFuncMap> functions;

	BlockStmt unassignedNodes;

	bool usingDerivatives = false;
};

template< class V > struct ASTVisitor
{
	void PreVisit(ASTNode* node) {}
	void PostVisit(ASTNode* node) {}
	void VisitNode(ASTNode* node)
	{
		static_cast<V*>(this)->PreVisit(node);

		for (ASTNode* ch = node->firstChild; ch; )
		{
			ASTNode* cch = ch;
			ch = ch->next;
			VisitNode(cch);
		}

		static_cast<V*>(this)->PostVisit(node);
	}
	void VisitFunction(ASTFunction* fn)
	{
		VisitNode(fn->GetCode());
	}
	void VisitAST(AST& ast)
	{
		for (const auto& fns : ast.functions)
		{
			for (const auto& fn : fns.second)
			{
				static_cast<V*>(this)->VisitFunction(fn.second);
			}
		}
	}
};

template< class V > struct ASTWalker
{
	// minimal-state tree walk
	// a   - has child
	//  b  - has child
	//   c - no child but has next node
	//   d - no child, no next node, backtrack to [b], then go to [e] (next)
	//  e  - has child
	//   f - no child, no next, backtrack to [a] (end)
	void PreVisit(ASTNode* node) {}
	void PostVisit(ASTNode* node) {}
	void VisitFunction(ASTFunction* fn)
	{
		curPos = endPos = fn->GetCode();
		//std::cout << "PREVISIT:" << typeid(*curPos).name() << "\n";
		static_cast<V*>(this)->PreVisit(curPos);
		do
		{
			if (curPos->firstChild)
			{
				curPos = curPos->firstChild;
				//std::cout << "PREVISIT:" << typeid(*curPos).name() << "\n";
				static_cast<V*>(this)->PreVisit(curPos);
				continue;
			}
			if (curPos->next)
			{
				//std::cout << "POSTVISIT:" << typeid(*curPos).name() << "\n";
				ASTNode* n = curPos->next;
				static_cast<V*>(this)->PostVisit(curPos);
				curPos = n;
				//std::cout << "PREVISIT:" << typeid(*curPos).name() << "\n";
				static_cast<V*>(this)->PreVisit(curPos);
				continue;
			}
			while (curPos && curPos != endPos && curPos->parent->lastChild == curPos)
			{
				//std::cout << "POSTVISIT:" << typeid(*curPos).name() << "\n";
				ASTNode* n = curPos->parent;
				static_cast<V*>(this)->PostVisit(curPos);
				curPos = n;
			}
			if (curPos != endPos)
			{
				//std::cout << "POSTVISIT:" << typeid(*curPos).name() << "\n";
				ASTNode* n = curPos->next;
				static_cast<V*>(this)->PostVisit(curPos);
				curPos = n;
				//std::cout << "PREVISIT:" << typeid(*curPos).name() << "\n";
				static_cast<V*>(this)->PreVisit(curPos);
			}
		}
		while (curPos && curPos != endPos);
		//std::cout << "POSTVISIT:" << typeid(*curPos).name() << "\n";
		static_cast<V*>(this)->PostVisit(curPos);
	}
	void VisitAST(AST& ast)
	{
		for (const auto& fns : ast.functions)
		{
			for (const auto& fn : fns.second)
			{
				static_cast<V*>(this)->VisitFunction(fn.second);
			}
		}
	}

	ASTNode* curPos = nullptr;
	ASTNode* endPos = nullptr;
};


struct VariableAccessValidator
{
	VariableAccessValidator(Diagnostic& d) : diag(d) {}
	void RunOnAST(const AST& ast);

	void ProcessReadExpr(const Expr* node);
	void ProcessWriteExpr(const Expr* node);
	// returns if statement returns a value at any point
	bool ProcessStmt(const Stmt* node);

	void ValidateSetupFunc(const ASTFunction* fn);
	void ValidateCheckOutputElementsWritten(Location loc);
	void AddMissingOutputAccessPoints(std::string& outerr, ASTType* type, int from, std::string pfx);
	void ValidateCheckVariableInitialized(const DeclRefExpr* dre);
	void ValidateCheckVariableError(const DeclRefExpr* dre);

	Diagnostic& diag;
	const ASTFunction* curASTFunction = nullptr;
	std::vector<uint8_t> elementsWritten;
	int endOfOutputElements = 0;
};

enum OutputShaderFormat
{
	OSF_HLSL_SM3,
	OSF_HLSL_SM4,
	OSF_GLSL_140,
	OSF_GLSL_ES_100,
};

struct Compiler
{
	bool CompileFile(const char* name, const char* code);

	const char* entryPoint = "main";
	ShaderMacro* defines = nullptr;
	ShaderStage stage = ShaderStage_Vertex;
	OutputShaderFormat outputFmt = OSF_HLSL_SM3;
	LoadIncludeFilePFN loadIncludeFilePFN = nullptr;
	void* loadIncludeFileUD = nullptr;
	OutStream* errorOutputStream = nullptr; // set to &FILEStream(stderr) to write to output
	OutStream* codeOutputStream = nullptr; // set to &FILEStream(stdout) to write to output
	OutStream* ASTDumpStream = nullptr;
};


// optimizer.cpp
struct ConstantPropagation : ASTWalker<ConstantPropagation>
{
	void PostVisit(ASTNode* node);
	void RunOnAST(AST& ast) { VisitAST(ast); }
};

struct RemoveUnusedFunctions
{
	void RunOnAST(AST& ast);
};

struct MarkUnusedVariables : ASTWalker<MarkUnusedVariables>
{
	void PreVisit(ASTNode* node);
	void VisitFunction(ASTFunction* fn);
	void RunOnAST(AST& ast);
};

struct RemoveUnusedVariables : ASTWalker<RemoveUnusedVariables>
{
	void RunOnAST(AST& ast);
};


// generator.cpp
void GenerateHLSL_SM3(const AST& ast, OutStream& out);
void GenerateHLSL_SM4(const AST& ast, OutStream& out);
void GenerateGLSL_140(const AST& ast, OutStream& out);
void GenerateGLSL_ES_100(const AST& ast, OutStream& out);

