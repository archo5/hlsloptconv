

#include "compiler.hpp"
#include "hlslparser.hpp"

#include <algorithm>


static void LVL(OutStream& out, int level)
{
	while (level-- > 0)
		out << "  ";
}


unsigned ASTType::GetElementCount() const
{
	switch (kind)
	{
	case Bool:
	case Int32:
	case UInt32:
	case Float16:
	case Float32:
		return 1;
	case Vector: return sizeX;
	case Matrix: return sizeX * sizeY;
	default:
		return 0;
	}
}

unsigned ASTType::GetAccessPointCount() const
{
	switch (kind)
	{
	case Bool:
	case Int32:
	case UInt32:
	case Float16:
	case Float32: return 1;
	case Vector: return sizeX;
	case Matrix: return sizeX * sizeY;
	case Array: return subType->GetAccessPointCount() * elementCount;
	case Structure: return ToStructType()->totalAccessPointCount;
	default: return 0;
	}
}

ASTType::SubTypeCount ASTType::CountSubTypes() const
{
	switch (kind)
	{
	case Bool:
	case Int32:
	case UInt32:
	case Float16:
	case Float32:
	case Vector:
	case Matrix: return { 1, 0 };
	case Array: return subType->CountSubTypes();
	case Structure:
	{
		SubTypeCount cnt = { 0, 0 };
		for (const auto& m : static_cast<const ASTStructType*>(this)->members)
			cnt += m.type->CountSubTypes();
		return cnt;
	}
	default: return { 0, 1 };
	}
}

ASTType::Kind ASTType::GetNVMKind() const
{
	// only guaranteed to return correct values for numeric/vector/matrix types
	switch (kind)
	{
	case Vector:
	case Matrix: return subType->kind;
	default: return kind;
	}
}

void ASTType::GetMangling(String& out) const
{
	char bfr[32];
	switch (kind)
	{
	case Void:    out += "v"; break;
	case Bool:    out += "b"; break;
	case Int32:   out += "i"; break;
	case UInt32:  out += "u"; break;
	case Float16: out += "p"; break;
	case Float32: out += "f"; break;
	case Vector:
		out += "V";
		out += '0' + sizeX;
		subType->GetMangling(out);
		break;
	case Matrix:
		out += "M";
		out += '0' + sizeX;
		out += '0' + sizeY;
		subType->GetMangling(out);
		break;
	case Array:
		sprintf(bfr, "A%dE", int(elementCount));
		out += bfr;
		subType->GetMangling(out);
		break;
	case Structure:
		out += "S";
		sprintf(bfr, "%d", int(static_cast<const ASTStructType*>(this)->name.size()));
		out += bfr;
		out += static_cast<const ASTStructType*>(this)->name;
		break;
	case Function:    out += "<FN?>"; break;
	case Sampler1D:   out += "s1"; break;
	case Sampler2D:   out += "s2"; break;
	case Sampler3D:   out += "s3"; break;
	case SamplerCube: out += "sc"; break;
	default: out += "<ERROR>"; break;
	}
}

static const char* gVecNumbers[4] = { "1", "2", "3", "4" };
static const char* gMtxNumbers[16] =
{
	"1x1", "2x1", "3x1", "4x1",
	"1x2", "2x2", "3x2", "4x2",
	"1x3", "2x3", "3x3", "4x3",
	"1x4", "2x4", "3x4", "4x4",
};
void ASTType::Dump(OutStream& out) const
{
	if (this == nullptr)
	{
		out << "<NULL>";
		return;
	}
	switch (kind)
	{
	case Void:        out << "void"; break;
	case Bool:        out << "bool"; break;
	case Int32:       out << "int"; break;
	case UInt32:      out << "uint"; break;
	case Float16:     out << "half"; break;
	case Float32:     out << "float"; break;
	case Vector:      subType->Dump(out); out << gVecNumbers[sizeX - 1]; break;
	case Matrix:      subType->Dump(out); out << gMtxNumbers[sizeX - 1 + (sizeY - 1) * 4]; break;
	case Array:       subType->Dump(out); out << "[" << elementCount << "]"; break;
	case Structure:   out << "struct(" << static_cast<const ASTStructType*>(this)->name << ")"; break;
	case Function:    out << "function"; break;
	case Sampler1D:   out << "sampler1D"; break;
	case Sampler2D:   out << "sampler2D"; break;
	case Sampler3D:   out << "sampler3D"; break;
	case SamplerCube: out << "samplerCUBE"; break;
	}
}

String ASTType::GetName() const
{
	switch (kind)
	{
	case Void:        return "void";
	case Bool:        return "bool";
	case Int32:       return "int";
	case UInt32:      return "uint";
	case Float16:     return "half";
	case Float32:     return "float";
	case Vector:      return subType->GetName() + gVecNumbers[sizeX - 1];
	case Matrix:      return subType->GetName() + gMtxNumbers[sizeX - 1 + (sizeY - 1) * 4];
	case Array:       return subType->GetName() + "[" + StdToString(elementCount) + "]";
	case Structure:   return "struct(" + static_cast<const ASTStructType*>(this)->name + ")";
	case Function:    return "function";
	case Sampler1D:   return "sampler1D";
	case Sampler2D:   return "sampler2D";
	case Sampler3D:   return "sampler3D";
	case SamplerCube: return "samplerCUBE";
	default: return "<ERROR>";
	}
}

ASTStructType* ASTType::ToStructType()
{
	return kind == Structure ? static_cast<ASTStructType*>(this) : nullptr;
}



void AccessPointDecl::Dump(OutStream& out) const
{
	type->Dump(out);
	out << " " << name;
	if (semanticName.empty() == false)
	{
		out << " : " << semanticName;
		if (semanticIndex >= 0)
			out << "[" << semanticIndex << "]";
	}
}

ASTStructType::ASTStructType() : ASTType(ASTType::Structure)
{
}

void ASTStructType::Dump(OutStream& out) const
{
	out << "struct " << name << "\n{\n";
	for (const auto& m : members)
	{
		out << "  ";
		m.Dump(out);
		out << ";\n";
	}
	out << "}\n";
}



bool TokenIsOpAssign(SLTokenType tt)
{
	switch (tt)
	{
	case STT_OP_AddEq:
	case STT_OP_SubEq:
	case STT_OP_MulEq:
	case STT_OP_DivEq:
	case STT_OP_ModEq:
	case STT_OP_AndEq:
	case STT_OP_OrEq:
	case STT_OP_XorEq:
	case STT_OP_LshEq:
	case STT_OP_RshEq:
	case STT_OP_Assign:
		return true;
	}
	return false;
}

bool TokenIsOpCompare(SLTokenType tt)
{
	switch (tt)
	{
	case STT_OP_Eq:
	case STT_OP_NEq:
	case STT_OP_Less:
	case STT_OP_LEq:
	case STT_OP_Greater:
	case STT_OP_GEq:
		return true;
	}
	return false;
}

String TokenTypeToString(SLTokenType tt)
{
	const char* str;
	switch (tt)
	{
	case STT_NULL:            str = "<END>"; break;
	case STT_LParen:          str = "("; break;
	case STT_RParen:          str = ")"; break;
	case STT_LBrace:          str = "{"; break;
	case STT_RBrace:          str = "}"; break;
	case STT_LBracket:        str = "["; break;
	case STT_RBracket:        str = "]"; break;
	case STT_Comma:           str = ","; break;
	case STT_Semicolon:       str = ";"; break;
	case STT_Colon:           str = ":"; break;
	case STT_Hash:            str = "#"; break;
	case STT_Ident:           str = "identifier"; break;
	case STT_StrLit:          str = "string literal"; break;
	case STT_BoolLit:         str = "boolean literal"; break;
	case STT_Int32Lit:        str = "int literal"; break;
	case STT_Float32Lit:      str = "float literal"; break;
	case STT_KW_Struct:       str = "struct"; break;
	case STT_KW_Return:       str = "return"; break;
	case STT_KW_Discard:      str = "discard"; break;
	case STT_KW_Break:        str = "break"; break;
	case STT_KW_Continue:     str = "continue"; break;
	case STT_KW_If:           str = "if"; break;
	case STT_KW_Else:         str = "else"; break;
	case STT_KW_While:        str = "while"; break;
	case STT_KW_Do:           str = "do"; break;
	case STT_KW_For:          str = "for"; break;
	case STT_KW_In:           str = "in"; break;
	case STT_KW_Out:          str = "out"; break;
	case STT_KW_InOut:        str = "inout"; break;
	case STT_KW_Const:        str = "const"; break;
	case STT_KW_Static:       str = "static"; break;
	case STT_KW_Uniform:      str = "uniform"; break;
	case STT_KW_CBuffer:      str = "cbuffer"; break;
	case STT_KW_Register:     str = "register"; break;
	case STT_KW_PackOffset:   str = "packoffset"; break;
	case STT_OP_Eq:           str = "=="; break;
	case STT_OP_NEq:          str = "!="; break;
	case STT_OP_LEq:          str = "<="; break;
	case STT_OP_GEq:          str = ">="; break;
	case STT_OP_Less:         str = "<"; break;
	case STT_OP_Greater:      str = ">"; break;
	case STT_OP_AddEq:        str = "+="; break;
	case STT_OP_SubEq:        str = "-="; break;
	case STT_OP_MulEq:        str = "*="; break;
	case STT_OP_DivEq:        str = "/="; break;
	case STT_OP_ModEq:        str = "%="; break;
	case STT_OP_AndEq:        str = "&="; break;
	case STT_OP_OrEq:         str = "|="; break;
	case STT_OP_XorEq:        str = "^="; break;
	case STT_OP_LshEq:        str = "<<="; break;
	case STT_OP_RshEq:        str = ">>="; break;
	case STT_OP_Assign:       str = "="; break;
	case STT_OP_LogicalAnd:   str = "&&"; break;
	case STT_OP_LogicalOr:    str = "||"; break;
	case STT_OP_Add:          str = "+"; break;
	case STT_OP_Sub:          str = "-"; break;
	case STT_OP_Mul:          str = "*"; break;
	case STT_OP_Div:          str = "/"; break;
	case STT_OP_Mod:          str = "%"; break;
	case STT_OP_And:          str = "&"; break;
	case STT_OP_Or:           str = "|"; break;
	case STT_OP_Xor:          str = "^"; break;
	case STT_OP_Lsh:          str = "<<"; break;
	case STT_OP_Rsh:          str = ">>"; break;
	case STT_OP_Member:       str = "."; break;
	case STT_OP_Not:          str = "!"; break;
	case STT_OP_Inv:          str = "~"; break;
	case STT_OP_Inc:          str = "++"; break;
	case STT_OP_Dec:          str = "--"; break;
	default:                  str = "<UNKNOWN>"; break;
	}
	return str;
}



ASTNode::~ASTNode()
{
	Unlink();
	while (firstChild)
		delete firstChild;
}

ASTNode* ASTNode::DeepClone() const
{
	ASTNode* nn = Clone();
	for (ASTNode* ch = firstChild; ch; ch = ch->next)
	{
		nn->AppendChild(ch->DeepClone());
	}
	return nn;
}


static const char* g_NodeTypeNames[] =
{
	"None (?)",
	"VarDecl",
	"CBufferDecl",
	"VoidExpr",
	"DeclRefExpr",
	"BoolExpr",
	"Int32Expr",
	"Float32Expr",
	"CastExpr",
	"InitListExpr",
	"IncDecOpExpr",
	"OpExpr",
	"UnaryOpExpr",
	"BinaryOpExpr",
	"TernaryOpExpr",
	"MemberExpr",
	"IndexExpr",
	"EmptyStmt",
	"ExprStmt",
	"BlockStmt",
	"ReturnStmt",
	"DiscardStmt",
	"BreakStmt",
	"ContinueStmt",
	"IfElseStmt",
	"WhileStmt",
	"DoWhileStmt",
	"ForStmt",
	"VarDeclStmt",
	"ASTFunction",
};
static_assert(
	sizeof(g_NodeTypeNames) / sizeof(g_NodeTypeNames[0]) == ASTNode::Kind__COUNT,
	"node type name count != node type count"
);
const char* ASTNode::GetNodeTypeName() const
{
	if (kind > Kind_None && kind < Kind__COUNT)
		return g_NodeTypeNames[kind];
	return "ASTNode(?)";
}

void ASTNode::Unlink()
{
	if (!parent)
		return;

	if (prev)
		prev->next = next;
	else
		parent->firstChild = next;

	if (next)
		next->prev = prev;
	else
		parent->lastChild = prev;

	prev = nullptr;
	next = nullptr;
	parent->childCount--;
	parent = nullptr;
}

void ASTNode::InsertBefore(ASTNode* ch, ASTNode* before)
{
	if (before == nullptr)
	{
		AppendChild(ch);
		return;
	}

	assert(before->parent == this);
	ch->Unlink();

	ch->parent = this;
	ch->next = before;
	ch->prev = before->prev;
	before->prev = ch;
	if (ch->prev)
		ch->prev->next = ch;
	else // firstChild == before
		firstChild = ch;
	childCount++;
}

void ASTNode::AppendChild(ASTNode* ch)
{
	ch->Unlink();

	ch->parent = this;
	ch->prev = lastChild;
	if (lastChild)
		lastChild->next = ch;
	else
		firstChild = ch;
	lastChild = ch;
	childCount++;
}

ASTNode* ASTNode::ReplaceWith(ASTNode* ch)
{
	assert(parent);
	if (ch)
		parent->InsertBefore(ch, this);
	Unlink();
	return this;
}

void ASTNode::SetFirst(ASTNode* ch)
{
	if (firstChild)
		firstChild->ReplaceWith(ch);
	else
		AppendChild(ch);
}

Expr* ASTNode::ToExpr()
{
	return dyn_cast<Expr>(this);
}

Stmt* ASTNode::ToStmt()
{
	return dyn_cast<Stmt>(this);
}

VarDecl* ASTNode::ToVarDecl()
{
	return dyn_cast<VarDecl>(this);
}

ASTFunction* ASTNode::ToFunction()
{
	return dyn_cast<ASTFunction>(this);
}

void ASTNode::ChangeAssocType(ASTType* t)
{
	if (auto* vd = ToVarDecl())
		vd->SetType(t);
	else if (auto* expr = ToExpr())
		expr->SetReturnType(t);
	else if (auto* func = ToFunction())
		func->SetReturnType(t);
}

void ASTNode::_RegisterTypeUse(ASTType* type)
{
	prevTypeUse = type->lastUse;
	if (type->lastUse)
		type->lastUse->nextTypeUse = this;
	else
		type->firstUse = this;
	type->lastUse = this;
}

void ASTNode::_UnregisterTypeUse(ASTType* type)
{
	if (prevTypeUse)
		prevTypeUse->nextTypeUse = nextTypeUse;
	else
		type->firstUse = nextTypeUse;
	if (nextTypeUse)
		nextTypeUse->prevTypeUse = prevTypeUse;
	else
		type->lastUse = prevTypeUse;
	prevTypeUse = nullptr;
	nextTypeUse = nullptr;
}

void ASTNode::_ChangeUsedType(ASTType*& mytype, ASTType* t)
{
	if (mytype == t)
		return;
	if (mytype)
		_UnregisterTypeUse(mytype);
	mytype = t;
	if (t)
		_RegisterTypeUse(t);
}


Expr::Expr(const Expr& o) : ASTNode(o)
{
	SetReturnType(o.GetReturnType());
}

void Expr::SetReturnType(ASTType* t)
{
	_ChangeUsedType(returnType, t);
}

Expr::~Expr()
{
	SetReturnType(nullptr);
}


void EmptyStmt::Dump(OutStream& out, int) const
{
	out << "<empty statement>\n";
}

void VoidExpr::Dump(OutStream& out, int) const
{
	out << "VOID(parse error)\n";
}


VarDecl::VarDecl(const VarDecl& o) : ASTNode(o)
{
	kind = Kind_VarDecl;
	SetType(o.GetType());
}

VarDecl::~VarDecl()
{
	SetType(nullptr);
}

void VarDecl::SetType(ASTType* t)
{
	_ChangeUsedType(type, t);
}

void VarDecl::GetMangling(String& out) const
{
	if ((flags & (ATTR_In | ATTR_Out)) == (ATTR_In | ATTR_Out))
		out += "Q";
	else if (flags & ATTR_In)
		out += "I";
	else if (flags & ATTR_Out)
		out += "O";
	else
		out += "_";
	type->GetMangling(out);
}

void VarDecl::Dump(OutStream& out, int level) const
{
	if ((flags & (ATTR_In | ATTR_Out)) == (ATTR_In | ATTR_Out))
		out << "inout ";
	else if (flags & ATTR_In)
		out << "in ";
	else if (flags & ATTR_Out)
		out << "out ";
	if (flags & ATTR_Uniform)
		out << "uniform ";
	if (flags & ATTR_Const)
		out << "const ";

	AccessPointDecl::Dump(out);

	if (regID >= 0)
	{
		out << " : ";
		if (dyn_cast<const CBufferDecl>(parent))
		{
			out << "packoffset(c" << (regID / 4);
			if (regID % 4)
				out << "." << "xyzw"[regID % 4];
		}
		else
		{
			out << "register(" << "cs"[GetType()->IsSampler()] << regID;
		}
		out << ")";
	}

	if (Expr* v = GetInitExpr())
	{
		out << " = ";
		v->Dump(out, level + 1);
	}
	else out << "\n";
}

void CBufferDecl::Dump(OutStream& out, int level) const
{
	LVL(out, level); out << "cbuffer " << name;
	if (bufRegID >= 0)
		out << " : register(b" << bufRegID << ")";
	out << "\n";
	LVL(out, level); out << "{\n"; level++;
	for (ASTNode* arg = firstChild; arg; arg = arg->next)
	{
		LVL(out, level);
		arg->Dump(out, level);
	}
	level--; LVL(out, level); out << "}\n";
}

void DeclRefExpr::Dump(OutStream& out, int level) const
{
	out << "declref(" << (decl ? decl->name.c_str() : "NULL") << ") [";
	GetReturnType()->Dump(out);
	out << "]\n";
}

void BoolExpr::Dump(OutStream& out, int level) const
{
	out << "bool(" << value << ")\n";
}

void Int32Expr::Dump(OutStream& out, int level) const
{
	out << "int32(" << value << ")\n";
}

void Float32Expr::Dump(OutStream& out, int level) const
{
	out << "float32(" << value << ")\n";
}

void CastExpr::Dump(OutStream& out, int level) const
{
	out << "cast[";
	GetReturnType()->Dump(out);
	out << "] ";
	GetSource()->Dump(out, level);
}

void InitListExpr::Dump(OutStream& out, int level) const
{
	out << "initlist [";
	GetReturnType()->Dump(out);
	out << "]\n";
	LVL(out, level); out << "{\n"; level++;

	int i = 0;
	for (ASTNode* ch = firstChild; ch; ch = ch->next)
	{
		LVL(out, level);
		out << "arg[" << i++ << "] = ";
		ch->Dump(out, level);
	}

	level--; LVL(out, level); out << "}\n";
}

void IncDecOpExpr::Dump(OutStream& out, int level) const
{
	out << "unop(" << (dec ? "--" : "++") << " " << (post ? "post" : "pre") << ") [";
	GetReturnType()->Dump(out);
	out << "]\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); GetSource()->Dump(out, level);
	level--; LVL(out, level); out << "}\n";
}

static const char* g_OpKindNames[] =
{
	"FCall",

	"Add",
	"Subtract",
	"Multiply",
	"Divide",
	"Modulus",

	"Abs",
	"ACos",
	"All",
	"Any",
	"ASin",
	"ATan",
	"ATan2",
	"Ceil",
	"Clamp",
	"Clip",
	"Cos",
	"CosH",
	"Cross",
	"DDX",
	"DDY",
	"Degrees",
	"Determinant",
	"Distance",
	"Dot",
	"Exp",
	"Exp2",
	"FaceForward",
	"Floor",
	"FMod",
	"Frac",
	"FWidth",
	"IsFinite",
	"IsInf",
	"IsNaN",
	"LdExp",
	"Length",
	"Lerp",
	"Log",
	"Log10",
	"Log2",
	"Max",
	"Min",
	"Mod(GLSL)",
	"MulMM",
	"MulMV",
	"MulVM",
	"Normalize",
	"Pow",
	"Radians",
	"Reflect",
	"Refract",
	"Round",
	"RSqrt",
	"Saturate",
	"Sign",
	"Sin",
	"SinH",
	"SmoothStep",
	"Sqrt",
	"Step",
	"Tan",
	"TanH",
	"Transpose",
	"Trunc",

	"Tex1D",
	"Tex1DBias",
	"Tex1DGrad",
	"Tex1DLOD",
	"Tex1DProj",
	"Tex2D",
	"Tex2DBias",
	"Tex2DGrad",
	"Tex2DLOD",
	"Tex2DProj",
	"Tex3D",
	"Tex3DBias",
	"Tex3DGrad",
	"Tex3DLOD",
	"Tex3DProj",
	"TexCube",
	"TexCubeBias",
	"TexCubeGrad",
	"TexCubeLOD",
	"TexCubeProj",

	"Tex1DCmp",
	"Tex1DLOD0Cmp",
	"Tex2DCmp",
	"Tex2DLOD0Cmp",
	"TexCubeCmp",
	"TexCubeLOD0Cmp",
};
static_assert(
	sizeof(g_OpKindNames) / sizeof(g_OpKindNames[0]) == Op_COUNT,
	"op kind name count != op kind count"
);
const char* OpKindToString(OpKind kind)
{
	if (kind >= 0 && kind < Op_COUNT)
		return g_OpKindNames[kind];
	return "op(UNKNOWN)";
}

void OpExpr::Dump(OutStream& out, int level) const
{
	out << OpKindToString(opKind);
	if (resolvedFunc)
		out << " (" << resolvedFunc->mangledName << ")";
	out << " [";
	GetReturnType()->Dump(out);
	out << "]\n";
	LVL(out, level); out << "{\n"; level++;
	for (ASTNode* ch = firstChild; ch; ch = ch->next)
	{
		LVL(out, level);
		ch->Dump(out, level);
	}
	level--; LVL(out, level); out << "}\n";
}

void UnaryOpExpr::Dump(OutStream& out, int level) const
{
	out << "unop(" << TokenTypeToString(opType) << ") [";
	GetReturnType()->Dump(out);
	out << "]\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); GetSource()->Dump(out, level);
	level--; LVL(out, level); out << "}\n";
}

void BinaryOpExpr::Dump(OutStream& out, int level) const
{
	out << "binop(" << TokenTypeToString(opType) << ") [";
	GetReturnType()->Dump(out);
	out << "]\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); GetLft()->Dump(out, level);
	LVL(out, level); GetRgt()->Dump(out, level);
	level--; LVL(out, level); out << "}\n";
}

void TernaryOpExpr::Dump(OutStream& out, int level) const
{
	out << "?: op [";
	GetReturnType()->Dump(out);
	out << "]\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level-1); out << "cond:\n";
	LVL(out, level); GetCond()->Dump(out, level);
	LVL(out, level-1); out << "true:\n";
	LVL(out, level); GetTrueExpr()->Dump(out, level);
	LVL(out, level-1); out << "false:\n";
	LVL(out, level); GetFalseExpr()->Dump(out, level);
	level--; LVL(out, level); out << "}\n";
}

void MemberExpr::Dump(OutStream& out, int level) const
{
	GetSource()->Dump(out, level);
	LVL(out, level + 1);
	out << ".";
	WriteName(out);
	out << " [";
	GetReturnType()->Dump(out);
	out << "]" << "\n";
}

static const char* g_MtxSwizzle[] =
{
	"_11", "_21", "_31", "_41",
	"_12", "_22", "_32", "_42",
	"_13", "_23", "_33", "_43",
	"_14", "_24", "_34", "_44",
};
void MemberExpr::WriteName(OutStream& out) const
{
	auto* srcTy = GetSource()->GetReturnType();
	switch (srcTy->kind)
	{
	case ASTType::Structure:
		out << srcTy->ToStructType()->members[memberID].name;
		break;
	case ASTType::Array:
		assert(false);
		break;
	case ASTType::Matrix:
		for (int i = 0; i < swizzleComp; ++i)
		{
			out << g_MtxSwizzle[(memberID >> (i * 4)) & 0xf];
		}
		break;
	case ASTType::Vector:
		for (int i = 0; i < swizzleComp; ++i)
		{
			out << "xyzw"[(memberID >> (i * 2)) & 0x3];
		}
		break;
	case ASTType::Void:
	case ASTType::Function:
		break;
	default:
		// scalar swizzle
		out << &"xxxx"[4 - swizzleComp];
		break;
	}
}

void IndexExpr::Dump(OutStream& out, int level) const
{
	GetSource()->Dump(out, level);
	LVL(out, level); out << "[\n";
	LVL(out, level + 1);
	GetIndex()->Dump(out, level + 1);
	LVL(out, level); out << "] [";
	GetReturnType()->Dump(out);
	out << "]" << "\n";
}

void ExprStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level);
	out << "ExprStmt:\n";
	LVL(out, level);
	GetExpr()->Dump(out, level);
}

void BlockStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level); out << "{\n"; level++;
	for (ASTNode* ch = firstChild; ch; ch = ch->next)
		ch->Dump(out, level);
	level--; LVL(out, level); out << "}\n";
}

void ReturnStmt::AddToFunction(ASTFunction* fn)
{
	RemoveFromFunction();

	func = fn;
	prevRetStmt = fn->lastRetStmt;
	if (fn->lastRetStmt)
		fn->lastRetStmt->nextRetStmt = this;
	else
		fn->firstRetStmt = this;
	fn->lastRetStmt = this;
}

void ReturnStmt::RemoveFromFunction()
{
	if (!func)
		return;

	if (prevRetStmt)
		prevRetStmt->nextRetStmt = nextRetStmt;
	else
		func->firstRetStmt = nextRetStmt;

	if (nextRetStmt)
		nextRetStmt->prevRetStmt = prevRetStmt;
	else
		func->lastRetStmt = prevRetStmt;

	prevRetStmt = nullptr;
	nextRetStmt = nullptr;
	func = nullptr;
}

void ReturnStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level);
	out << "return ";
	if (GetExpr())
		GetExpr()->Dump(out, level);
	else
		out << "[-]\n";
}

void DiscardStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level);
	out << "discard\n";
}

void BreakStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level);
	out << "break\n";
}

void ContinueStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level);
	out << "continue\n";
}

void IfElseStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level); out << "if\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); out << "cond = ";
	GetCond()->Dump(out, level + 1);
	LVL(out, level); out << "true:\n";
	GetTrueBr()->Dump(out, level + 1);
	LVL(out, level); out << "false:\n";
	if (GetFalseBr())
		GetFalseBr()->Dump(out, level + 1);
	else
	{
		LVL(out, level + 1);
		out << "<none>\n";
	}
	level--; LVL(out, level); out << "}\n";
}

void WhileStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level); out << "while\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); out << "cond = ";
	GetCond()->Dump(out, level + 1);
	LVL(out, level); out << "body:\n";
	GetBody()->Dump(out, level + 1);
	level--; LVL(out, level); out << "}\n";
}

void DoWhileStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level); out << "do-while\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); out << "cond = ";
	GetCond()->Dump(out, level + 1);
	LVL(out, level); out << "body:\n";
	GetBody()->Dump(out, level + 1);
	level--; LVL(out, level); out << "}\n";
}

void ForStmt::Dump(OutStream& out, int level) const
{
	LVL(out, level); out << "for\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); out << "init:\n";
	GetInit()->Dump(out, level + 1);
	LVL(out, level); out << "cond = ";
	GetCond()->Dump(out, level + 1);
	LVL(out, level); out << "incr:\n";
	LVL(out, level + 1); GetIncr()->Dump(out, level + 1);
	LVL(out, level); out << "body:\n";
	GetBody()->Dump(out, level + 1);
	level--; LVL(out, level); out << "}\n";
}

void VarDeclStmt::Dump(OutStream& out, int level) const
{
	for (ASTNode* ch = firstChild; ch; ch = ch->next)
	{
		LVL(out, level);
		ch->Dump(out, level);
	}
}



ASTFunction::~ASTFunction()
{
	SetReturnType(nullptr);
}

void ASTFunction::SetReturnType(ASTType* t)
{
	_ChangeUsedType(returnType, t);
}

void ASTFunction::Dump(OutStream& out, int level) const
{
	out << "function ";
	GetReturnType()->Dump(out);
	out << " " << name << "(\n";
	for (ASTNode* arg = GetFirstArg(); arg; arg = arg->next)
	{
		LVL(out, level + 1);
		arg->Dump(out, level + 1);
	}
	out << ")";
	if (returnSemanticName.empty() == false)
	{
		out << ":" << returnSemanticName;
		if (returnSemanticIndex >= 0)
			out << "[" << returnSemanticIndex << "]";
	}
	out << " // mangledName=" << mangledName << "\n";
	GetCode()->Dump(out, level);
	out << "\n";
}


TypeSystem::TypeSystem() :
	typeVoidDef        (ASTType::Void       ),
	typeFunctionDef    (ASTType::Function   ),
	typeSampler1DDef   (ASTType::Sampler1D  ),
	typeSampler2DDef   (ASTType::Sampler2D  ),
	typeSampler3DDef   (ASTType::Sampler3D  ),
	typeSamplerCubeDef (ASTType::SamplerCube),
	typeSampler1DCmpDef   (ASTType::Sampler1DCmp  ),
	typeSampler2DCmpDef   (ASTType::Sampler2DCmp  ),
	typeSamplerCubeCmpDef (ASTType::SamplerCubeCmp),
	typeBoolDef        (ASTType::Bool       ),
	typeInt32Def       (ASTType::Int32      ),
	typeUInt32Def      (ASTType::UInt32     ),
	typeFloat16Def     (ASTType::Float16    ),
	typeFloat32Def     (ASTType::Float32    )
{
	// initialize vector/matrix types later
}

TypeSystem::~TypeSystem()
{
	while (firstAllocType)
	{
		ASTType* at = firstAllocType;
		firstAllocType = at->nextAllocType;
		delete at;
	}
}

void TypeSystem::InitBasicTypes()
{
	ASTType* baseTypes[5]     =
	{
		GetBoolType(),
		GetInt32Type(),
		GetUInt32Type(),
		GetFloat16Type(),
		GetFloat32Type()
	};
	ASTType* vecTypeArrays[5] =
	{
		typeBoolVecDefs,
		typeInt32VecDefs,
		typeUInt32VecDefs,
		typeFloat16VecDefs,
		typeFloat32VecDefs
	};
	ASTType* mtxTypeArrays[5] =
	{
		typeBoolMtxDefs,
		typeInt32MtxDefs,
		typeUInt32MtxDefs,
		typeFloat16MtxDefs,
		typeFloat32MtxDefs
	};
	for (int t = 0; t < 5; ++t)
	{
		ASTType* baseType = baseTypes[t];
		for (int x = 1; x <= 4; ++x)
		{
			ASTType* vt = &vecTypeArrays[t][x - 1];
			vt->kind = ASTType::Vector;
			vt->subType = baseType;
			vt->sizeX = x;
			for (int y = 1; y <= 4; ++y)
			{
				ASTType* mt = &mtxTypeArrays[t][(x - 1) + (y - 1) * 4];
				mt->kind = ASTType::Matrix;
				mt->subType = baseType;
				mt->sizeX = x;
				mt->sizeY = y;
			}
		}
	}
}

ASTType* TypeSystem::CastToBool(ASTType* t)
{
	switch (t->kind)
	{
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::UInt32:
	case ASTType::Float16:
	case ASTType::Float32: return GetBoolType();
	case ASTType::Vector:  return &typeBoolVecDefs[t->sizeX - 1];
	case ASTType::Matrix:  return &typeBoolMtxDefs[(t->sizeX - 1) + (t->sizeY - 1) * 4];
	default: return nullptr;
	}
}

ASTType* TypeSystem::CastToInt(ASTType* t)
{
	switch (t->kind)
	{
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::UInt32:
	case ASTType::Float16:
	case ASTType::Float32: return GetInt32Type();
	case ASTType::Vector:  return &typeInt32VecDefs[t->sizeX - 1];
	case ASTType::Matrix:  return &typeInt32MtxDefs[(t->sizeX - 1) + (t->sizeY - 1) * 4];
	default: return nullptr;
	}
}

ASTType* TypeSystem::CastToFloat(ASTType* t)
{
	switch (t->kind)
	{
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::UInt32:  return GetFloat32Type();
	case ASTType::Float16:
	case ASTType::Float32: return t;
	case ASTType::Vector:
		return &(t->subType->kind == ASTType::Float16 ? typeFloat16VecDefs : typeFloat32VecDefs)
			[t->sizeX - 1];
	case ASTType::Matrix:
		return &(t->subType->kind == ASTType::Float16 ? typeFloat16MtxDefs : typeFloat32MtxDefs)
			[(t->sizeX - 1) + (t->sizeY - 1) * 4];
	default: return nullptr;
	}
}

ASTType* TypeSystem::CastToScalar(ASTType* t)
{
	switch (t->kind)
	{
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::UInt32:
	case ASTType::Float16:
	case ASTType::Float32: return t;
	case ASTType::Vector:
	case ASTType::Matrix:  return t->subType;
	default: return nullptr;
	}
}

ASTType* TypeSystem::CastToVector(ASTType* t, int size, bool force)
{
	if (size < 1 || size > 4)
		return nullptr;
	switch (t->kind)
	{
	case ASTType::Bool:    return &typeBoolVecDefs   [size - 1];
	case ASTType::Int32:   return &typeInt32VecDefs  [size - 1];
	case ASTType::UInt32:  return &typeUInt32VecDefs [size - 1];
	case ASTType::Float32: return &typeFloat32VecDefs[size - 1];
	case ASTType::Float16: return &typeFloat16VecDefs[size - 1];
	case ASTType::Vector:  return force ? CastToVector(t->subType, size) : t;
	default: return nullptr;
	}
}

ASTType* TypeSystem::GetVectorType(ASTType* t, int size)
{
	if (size < 1 || size > 4)
		return nullptr;
	switch (t->kind)
	{
	case ASTType::Bool:    return &typeBoolVecDefs   [size - 1];
	case ASTType::Int32:   return &typeInt32VecDefs  [size - 1];
	case ASTType::UInt32:  return &typeUInt32VecDefs [size - 1];
	case ASTType::Float32: return &typeFloat32VecDefs[size - 1];
	case ASTType::Float16: return &typeFloat16VecDefs[size - 1];
	default: return nullptr;
	}
}

ASTType* TypeSystem::GetMatrixType(ASTType* t, int sizeX, int sizeY)
{
	if (sizeX < 1 || sizeX > 4 || sizeY < 1 || sizeY > 4)
		return nullptr;
	switch (t->kind)
	{
	case ASTType::Bool:    return &typeBoolMtxDefs   [(sizeX - 1) + (sizeY - 1) * 4];
	case ASTType::Int32:   return &typeInt32MtxDefs  [(sizeX - 1) + (sizeY - 1) * 4];
	case ASTType::UInt32:  return &typeUInt32MtxDefs [(sizeX - 1) + (sizeY - 1) * 4];
	case ASTType::Float32: return &typeFloat32MtxDefs[(sizeX - 1) + (sizeY - 1) * 4];
	case ASTType::Float16: return &typeFloat16MtxDefs[(sizeX - 1) + (sizeY - 1) * 4];
	default: return nullptr;
	}
}

ASTType* TypeSystem::GetArrayType(ASTType* t, uint32_t size)
{
	for (ASTType* at = firstArrayType; at; at = at->nextArrayType)
	{
		if (at->subType == t && at->elementCount == size)
			return at;
	}

	auto* nat = new ASTType(ASTType::Array);
	nat->subType = t;
	nat->elementCount = size;

	nat->nextArrayType = firstArrayType;
	firstArrayType = nat;

	nat->nextAllocType = firstAllocType;
	firstAllocType = nat;

	return nat;
}

ASTStructType* TypeSystem::CreateStructType(const String& name)
{
	auto* stc = new ASTStructType;
	stc->name = name;

	stc->nextAllocType = firstAllocType;
	firstAllocType = stc;

	stc->prevStructType = lastStructType;
	if (lastStructType)
		lastStructType->nextStructType = stc;
	lastStructType = stc;
	if (!firstStructType)
		firstStructType = stc;

	return stc;
}

ASTType* TypeSystem::_GetSVMTypeByName(ASTType* t, const char* sub)
{
	if (sub[0] == '\0')
		return t;
	if (sub[0] >= '1' && sub[0] <= '4')
	{
		if (sub[1] == '\0')
			return GetVectorType(t, sub[0] - '0');
		if (sub[1] == 'x' && sub[2] >= '1' && sub[2] <= '4' && sub[3] == '\0')
			return GetMatrixType(t, sub[0] - '0', sub[2] - '0');
	}
	return nullptr;
}

ASTType* TypeSystem::GetBaseTypeByName(const char* name)
{
	if (strncmp(name, STRLIT_SIZE("bool")) == 0)
		return _GetSVMTypeByName(&typeBoolDef, name + sizeof("bool") - 1);
	if (strncmp(name, STRLIT_SIZE("int")) == 0)
		return _GetSVMTypeByName(&typeInt32Def, name + sizeof("int") - 1);
	if (strncmp(name, STRLIT_SIZE("uint")) == 0)
		return _GetSVMTypeByName(&typeUInt32Def, name + sizeof("uint") - 1);
	if (strncmp(name, STRLIT_SIZE("half")) == 0)
		return _GetSVMTypeByName(&typeFloat16Def, name + sizeof("half") - 1);
	if (strncmp(name, STRLIT_SIZE("float")) == 0)
		return _GetSVMTypeByName(&typeFloat32Def, name + sizeof("float") - 1);
	if (strncmp(name, STRLIT_SIZE("sampler")) == 0)
	{
		if (strcmp(name + sizeof("sampler") - 1, "1D") == 0)
			return &typeSampler1DDef;
		if (strcmp(name + sizeof("sampler") - 1, "2D") == 0)
			return &typeSampler2DDef;
		if (strcmp(name + sizeof("sampler") - 1, "3D") == 0)
			return &typeSampler3DDef;
		if (strcmp(name + sizeof("sampler") - 1, "CUBE") == 0)
			return &typeSamplerCubeDef;
		if (strcmp(name + sizeof("sampler") - 1, "1Dcmp") == 0)
			return &typeSampler1DCmpDef;
		if (strcmp(name + sizeof("sampler") - 1, "2Dcmp") == 0)
			return &typeSampler2DCmpDef;
		if (strcmp(name + sizeof("sampler") - 1, "CUBEcmp") == 0)
			return &typeSamplerCubeCmpDef;
	}
	if (strcmp(name, "void") == 0)
		return &typeVoidDef;
	return nullptr;
}

ASTStructType* TypeSystem::GetStructTypeByName(const char* name)
{
	for (ASTStructType* strTy = firstStructType; strTy; strTy = strTy->nextStructType)
	{
		if (strTy->name == name)
			return strTy;
	}
	return nullptr;
}

ASTType* TypeSystem::GetTypeByName(const char* name)
{
	if (auto* t = GetBaseTypeByName(name))
		return t;
	return GetStructTypeByName(name);
}

bool TypeSystem::IsTypeName(const char* name)
{
	return GetTypeByName(name) != nullptr;
}



VarDecl* AST::CreateGlobalVar()
{
	auto* vd = new VarDecl;
	globalVars.AppendChild(vd);
	return vd;
}

struct UsedFuncMarker : ASTVisitor<UsedFuncMarker>
{
	UsedFuncMarker(ASTFunction* fn) : func(fn)
	{
		fn->used = true;
		functionsToProcess.push_back(fn);
	}
	void PreVisit(ASTNode* node)
	{
		if (auto* fcall = dyn_cast<OpExpr>(node))
		{
			if (fcall->resolvedFunc && fcall->resolvedFunc->used == false)
			{
				fcall->resolvedFunc->used = true;
				functionsToProcess.push_back(fcall->resolvedFunc);
			}
		}
	}
	void Process()
	{
		while (functionsToProcess.empty() == false)
		{
			ASTFunction* fn = functionsToProcess.back();
			functionsToProcess.pop_back();
			VisitFunction(fn);
		}
	}

	ASTFunction* func;
	std::vector<ASTFunction*> functionsToProcess;
};

void AST::MarkUsed(Diagnostic& diag)
{
	// clear flag
	for (ASTNode* ch = functionList.firstChild; ch; ch = ch->next)
	{
		dyn_cast<ASTFunction>(ch)->used = false;
	}

	// mark all non-uniform arguments as stage i/o
	for (ASTNode* arg = entryPoint->GetFirstArg(); arg; arg = arg->next)
	{
		auto* vd = arg->ToVarDecl();
		if (vd && !(vd->flags & VarDecl::ATTR_Uniform))
		{
			vd->flags |= VarDecl::ATTR_StageIO;

			if (stage == ShaderStage_Vertex &&
				vd->semanticName == "POSITION" &&
				(vd->flags & VarDecl::ATTR_Out) &&
				(vd->type->kind != ASTType::Vector || vd->type->sizeX != 4))
			{
				diag.EmitError("vertex shader POSITION output must be a 4-component vector",
					vd->loc);
				diag.hasFatalErrors = true;
				return;
			}
		}
	}
	if (stage == ShaderStage_Vertex &&
		entryPoint->returnSemanticName == "POSITION" &&
		(entryPoint->returnType->kind != ASTType::Vector || entryPoint->returnType->sizeX != 4))
	{
		diag.EmitError("vertex shader POSITION output must be a 4-component vector",
			entryPoint->loc);
		diag.hasFatalErrors = true;
		return;
	}

	UsedFuncMarker ufm(entryPoint);
	ufm.Process();
}

void AST::Dump(OutStream& out) const
{
	for (ASTStructType* s = firstStructType; s; s = s->nextStructType)
	{
		s->Dump(out);
	}
	for (ASTNode* g = globalVars.firstChild; g; g = g->next)
	{
		g->Dump(out);
	}
	for (ASTNode* f = functionList.firstChild; f; f = f->next)
	{
		f->Dump(out);
	}
}



void VariableAccessValidator::RunOnAST(const AST& ast)
{
	// functions
	for (ASTNode* f = ast.functionList.firstChild; f; f = f->next)
	{
		auto* fn = dyn_cast<ASTFunction>(f);

		ValidateSetupFunc(fn);

		curASTFunction = fn;

		if (ProcessStmt(fn->GetCode()) == false)
		{
			if (fn->GetReturnType()->IsVoid())
			{
				ValidateCheckOutputElementsWritten(fn->loc);
			}
			else
			{
				diag.EmitError("'" + fn->name +
					"' - not all control paths return a value",
					fn->loc);
			}
		}
	}
}

void VariableAccessValidator::ProcessReadExpr(const Expr* node)
{
	if (auto* bexpr = dyn_cast<const BoolExpr>(node))
	{
		return;
	}
	else if (auto* i32expr = dyn_cast<const Int32Expr>(node))
	{
		return;
	}
	else if (auto* f32expr = dyn_cast<const Float32Expr>(node))
	{
		return;
	}
	else if (auto* idop = dyn_cast<const IncDecOpExpr>(node))
	{
		ProcessReadExpr(idop->GetSource());
		if (diag.hasFatalErrors)
			return;
		ProcessWriteExpr(idop->GetSource());
		return;
	}
	else if (auto* op = dyn_cast<const OpExpr>(node))
	{
		if (auto* rf = op->resolvedFunc)
		{
			for (ASTNode *arg = op->GetFirstArg(), *argdecl = rf->GetFirstArg();
				arg && argdecl;
				arg = arg->next, argdecl = argdecl->next)
			{
				if (argdecl->ToVarDecl()->flags & VarDecl::ATTR_In)
					ProcessReadExpr(arg->ToExpr());
				if (diag.hasFatalErrors)
					return;
			}

			for (ASTNode *arg = op->GetFirstArg(), *argdecl = rf->GetFirstArg();
				arg && argdecl;
				arg = arg->next, argdecl = argdecl->next)
			{
				if (argdecl->ToVarDecl()->flags & VarDecl::ATTR_Out)
					ProcessWriteExpr(arg->ToExpr());
				if (diag.hasFatalErrors)
					return;
			}
			return;
		}
		else
		{
			// TODO write-out intrinsics
			for (ASTNode* arg = op->firstChild; arg; arg = arg->next)
			{
				ProcessReadExpr(arg->ToExpr());
				if (diag.hasFatalErrors)
					return;
			}
			return;
		}
	}
	else if (auto* unop = dyn_cast<const UnaryOpExpr>(node))
	{
		ProcessReadExpr(unop->GetSource());
		return;
	}
	else if (auto* binop = dyn_cast<const BinaryOpExpr>(node))
	{
		if (binop->opType == STT_OP_Assign)
		{
			ProcessReadExpr(binop->GetRgt());
			if (diag.hasFatalErrors)
				return;
			ProcessWriteExpr(binop->GetLft());
			if (diag.hasFatalErrors)
				return;
			ProcessReadExpr(binop->GetLft());
		}
		else
		{
			ProcessReadExpr(binop->GetLft());
			if (diag.hasFatalErrors)
				return;
			ProcessReadExpr(binop->GetRgt());
		}
		return;
	}
	else if (auto* tnop = dyn_cast<const TernaryOpExpr>(node))
	{
		ProcessReadExpr(tnop->GetCond());
		if (diag.hasFatalErrors)
			return;
		ProcessReadExpr(tnop->GetTrueExpr());
		if (diag.hasFatalErrors)
			return;
		ProcessReadExpr(tnop->GetFalseExpr());
		return;
	}
	else if (auto* castexpr = dyn_cast<const CastExpr>(node))
	{
		ProcessReadExpr(castexpr->GetSource());
		return;
	}
	else if (auto* ile = dyn_cast<const InitListExpr>(node))
	{
		for (ASTNode* ch = ile->firstChild; ch; ch = ch->next)
		{
			ProcessReadExpr(ch->ToExpr());
			if (diag.hasFatalErrors)
				return;
		}
		return;
	}
	else if (auto* sve = dyn_cast<const SubValExpr>(node))
	{
		std::vector<const SubValExpr*> revTrail { sve };
		Expr* exprIt = sve->GetSource();
		while (auto* ssve = dyn_cast<const SubValExpr>(exprIt))
		{
			revTrail.push_back(ssve);
			exprIt = ssve->GetSource();
		}

		for (auto* sve : revTrail)
		{
			if (auto* idxe = dyn_cast<const IndexExpr>(sve))
			{
				ProcessReadExpr(idxe->GetIndex());
				if (diag.hasFatalErrors)
					return;
			}
		}

		if (auto* dre = dyn_cast<const DeclRefExpr>(exprIt))
		{
			// check if variable part is written
			int rf = dre->decl->APRangeFrom;
			int rt = dre->decl->APRangeTo;
			if (rf < rt)
			{
				uint32_t swizzle = 0;
				int swzSize = 0;
				bool isMtxSwizzle = false;

				if (revTrail.empty() == false)
				{
					const Expr* sizeExpr = dre;
					for (size_t i = revTrail.size(); i > 0; )
					{
						--i;
						auto* sve = revTrail[i];
						int32_t index = -1;
						bool isMmbSwizzle = false;
						if (auto* mmb = dyn_cast<const MemberExpr>(sve))
						{
							if (mmb->swizzleComp == 0)
								index = mmb->memberID;
							else
							{
								isMmbSwizzle = true;
								swizzle = mmb->memberID;
								swzSize = mmb->GetReturnType()->GetElementCount();
								isMtxSwizzle = mmb->GetSource()->GetReturnType()->kind == ASTType::Matrix;
							}
						}
						else if (auto* idx = dyn_cast<const IndexExpr>(sve))
						{
							if (auto* ci = dyn_cast<const Int32Expr>(idx->GetIndex()))
							{
								index = ci->value;
							}
						}
						if (index >= 0)
						{
							sizeExpr = sve;
							rf += sve->GetReturnType()->GetAccessPointCount() * index;
						}
						else if (!isMmbSwizzle)
							break;
					}
					rt = rf + sizeExpr->GetReturnType()->GetAccessPointCount();
				}

				if (swzSize)
				{
					int mult = isMtxSwizzle ? 4 : 2;
					uint32_t mask = isMtxSwizzle ? 0xf : 0x3;
					for (int i = 0; i < swzSize; ++i)
					{
						if (elementsWritten[rf + ((swizzle >> (i * mult)) & mask)] == 0)
						{
							ValidateCheckVariableError(dre);
						}
					}
				}
				else
				{
					for (int i = rf; i < rt; ++i)
					{
						if (elementsWritten[i] == 0)
						{
							ValidateCheckVariableError(dre);
						}
					}
				}
			}
		}
		else
		{
			ProcessReadExpr(exprIt);
		}
		return;
	}
	else if (auto* dre = dyn_cast<const DeclRefExpr>(node))
	{
		ValidateCheckVariableInitialized(dre);
		return;
	}
	else if (dyn_cast<const VoidExpr>(node))
	{
		return;
	}

	assert(!"unhandled read expr type");
}

void VariableAccessValidator::ProcessWriteExpr(const Expr* node)
{
	if (auto* sve = dyn_cast<const SubValExpr>(node))
	{
		std::vector<const SubValExpr*> revTrail { sve };
		Expr* exprIt = sve->GetSource();
		while (auto* ssve = dyn_cast<const SubValExpr>(exprIt))
		{
			revTrail.push_back(ssve);
			exprIt = ssve->GetSource();
		}

		if (auto* dre = dyn_cast<const DeclRefExpr>(exprIt))
		{
			if (dre->decl->flags & VarDecl::ATTR_Const)
			{
				diag.EmitError("cannot write to constant value", dre->loc);
				diag.hasFatalErrors = true;
				return;
			}

			// mark variable part as written
			int rf = dre->decl->APRangeFrom;
			int rt = dre->decl->APRangeTo;
			if (rf < rt)
			{
				uint32_t swizzle = 0;
				int swzSize = 0;
				bool isMtxSwizzle = false;

				if (revTrail.empty() == false)
				{
					const Expr* sizeExpr = dre;
					for (size_t i = revTrail.size(); i > 0; )
					{
						--i;
						auto* sve = revTrail[i];
						if (auto* mmb = dyn_cast<const MemberExpr>(sve))
						{
							sizeExpr = sve;
							if (mmb->swizzleComp == 0)
							{
								// structure member
								auto* strTy = mmb->GetSource()->GetReturnType()->ToStructType();
								for (uint32_t i = 0; i < mmb->memberID; ++i)
									rf += strTy->members[i].type->GetAccessPointCount();
							}
							else // scalar/vector/matrix swizzle
							{
								swizzle = mmb->memberID;
								swzSize = mmb->GetReturnType()->GetElementCount();
								isMtxSwizzle = mmb->GetSource()->GetReturnType()->kind == ASTType::Matrix;
							}
						}
						else if (auto* idx = dyn_cast<const IndexExpr>(sve))
						{
							// array
							if (auto* ci = dyn_cast<const Int32Expr>(idx->GetIndex()))
							{
								sizeExpr = sve;
								rf += sve->GetReturnType()->GetAccessPointCount() * ci->value;
							}
							else
							{
								diag.EmitError("cannot write to local array from a computed index",
									idx->loc);
							}
						}
					}
					rt = rf + sizeExpr->GetReturnType()->GetAccessPointCount();
				}

				if (swzSize)
				{
					int mult = isMtxSwizzle ? 4 : 2;
					uint32_t mask = isMtxSwizzle ? 0xf : 0x3;
					for (int i = 0; i < swzSize; ++i)
						elementsWritten[rf + ((swizzle >> (i * mult)) & mask)] = true;
				}
				else
				{
					for (int i = rf; i < rt; ++i)
						elementsWritten[i] = true;
				}
			}

			return;
		}
		else
		{
			diag.EmitError("cannot write to temporary value", exprIt->loc);
			diag.hasFatalErrors = true;
			return;
		}
	}
	else if (auto* dre = dyn_cast<const DeclRefExpr>(node))
	{
		if (dre->decl->flags & VarDecl::ATTR_Const)
		{
			diag.EmitError("cannot write to constant value", dre->loc);
			diag.hasFatalErrors = true;
			return;
		}

		// mark variable as written
		for (int i = dre->decl->APRangeFrom; i < dre->decl->APRangeTo; ++i)
			elementsWritten[i] = true;
		return;
	}

	diag.EmitError("cannot write to read-only expression", node->loc);
	diag.hasFatalErrors = true;
}

bool VariableAccessValidator::ProcessStmt(const Stmt* node)
{
	if (auto* blkstmt = dyn_cast<const BlockStmt>(node))
	{
		for (ASTNode* ch = blkstmt->firstChild; ch; ch = ch->next)
		{
			// optimization - do not process code after first fully returning statement
			if (ProcessStmt(ch->ToStmt()))
				return true;
		}
		return false;
	}
	else if (auto* ifelsestmt = dyn_cast<const IfElseStmt>(node))
	{
		ProcessReadExpr(ifelsestmt->GetCond());
		bool rif = ProcessStmt(ifelsestmt->GetTrueBr());
		bool relse = ifelsestmt->GetFalseBr() && ProcessStmt(ifelsestmt->GetFalseBr());
		return rif && relse;
	}
	else if (auto* whilestmt = dyn_cast<const WhileStmt>(node))
	{
		ProcessReadExpr(whilestmt->GetCond());
		return ProcessStmt(whilestmt->GetBody());
	}
	else if (auto* dowhilestmt = dyn_cast<const DoWhileStmt>(node))
	{
		bool rb = ProcessStmt(dowhilestmt->GetBody());
		ProcessReadExpr(dowhilestmt->GetCond());
		return rb;
	}
	else if (auto* forstmt = dyn_cast<const ForStmt>(node))
	{
		ProcessStmt(forstmt->GetInit()); // cannot return
		ProcessReadExpr(forstmt->GetCond());
		bool rb = ProcessStmt(forstmt->GetBody());
		ProcessReadExpr(forstmt->GetIncr());
		return rb;
	}
	else if (auto* exprstmt = dyn_cast<const ExprStmt>(node))
	{
		// easy special cases for root that don't need the value
		if (auto* binop = dyn_cast<const BinaryOpExpr>(exprstmt->GetExpr()))
		{
			if (binop->opType == STT_OP_Assign)
			{
				ProcessReadExpr(binop->GetRgt());
				ProcessWriteExpr(binop->GetLft());
				return false;
			}
		}

		ProcessReadExpr(exprstmt->GetExpr());
		return false;
	}
	else if (auto* retstmt = dyn_cast<const ReturnStmt>(node))
	{
		if (curASTFunction->GetReturnType()->IsVoid() == false)
		{
			ProcessReadExpr(retstmt->GetExpr());
		}

		ValidateCheckOutputElementsWritten(retstmt->loc);
		return true;
	}
	else if (auto* dscstmt = dyn_cast<const DiscardStmt>(node))
	{
		// still need to return something somewhere, otherwise shader is somewhat pointless
		return false;
	}
	else if (auto* brkstmt = dyn_cast<const BreakStmt>(node))
	{
		return false;
	}
	else if (auto* cntstmt = dyn_cast<const ContinueStmt>(node))
	{
		return false;
	}
	else if (auto* vdstmt = dyn_cast<const VarDeclStmt>(node))
	{
		for (ASTNode* ch = vdstmt->firstChild; ch; ch = ch->next)
		{
			VarDecl* vd = ch->ToVarDecl();
			if (Expr* expr = vd->GetInitExpr())
			{
				// mark variable as written
				for (int i = vd->APRangeFrom; i < vd->APRangeTo; ++i)
					elementsWritten[i] = true;

				ProcessReadExpr(expr);
			}
		}
		return false;
	}
	else if (dyn_cast<const EmptyStmt>(node))
	{
		return false;
	}

	assert(!"unhandled statement type");
	return false;
}

void VariableAccessValidator::ValidateSetupFunc(const ASTFunction* fn)
{
	// allocate output ranges
	int numElements = 0;
	// - return values
	for (ASTNode* arg = fn->GetFirstArg(); arg; arg = arg->next)
	{
		auto* argvd = arg->ToVarDecl();
		if (argvd->flags & VarDecl::ATTR_Out)
		{
			int ap = argvd->GetType()->GetAccessPointCount();
			argvd->APRangeFrom = numElements;
			argvd->APRangeTo = numElements + ap;
			numElements += ap;
		}
	}
	endOfOutputElements = numElements;
	// - temporary variables
	for (auto* vd : fn->tmpVars)
	{
		int ap = vd->GetType()->GetAccessPointCount();
		vd->APRangeFrom = numElements;
		vd->APRangeTo = numElements + ap;
		numElements += ap;
	}

	elementsWritten.resize(numElements);
	memset(elementsWritten.data(), 0, elementsWritten.size());
}

void VariableAccessValidator::ValidateCheckOutputElementsWritten(Location loc)
{
	for (int i = 0; i < endOfOutputElements; ++i)
	{
		if (elementsWritten[i] == 0)
		{
			String err = "not all outputs have been assigned before 'return':";
			for (ASTNode* arg = curASTFunction->GetFirstArg(); arg; arg = arg->next)
			{
				auto* argvd = arg->ToVarDecl();
				if (argvd->APRangeFrom == argvd->APRangeTo)
					continue;
				AddMissingOutputAccessPoints(err, argvd->GetType(), argvd->APRangeFrom, argvd->name);
			}
			diag.EmitError(err, loc);
			break;
		}
	}
}

static const char* g_VecSuffixStr[4] = { ".x", ".y", ".z", ".w" };
static const char* g_Arr4SuffixStr[4] = { "[0]", "[1]", "[2]", "[3]" };
void VariableAccessValidator::AddMissingOutputAccessPoints(
	String& outerr, ASTType* type, int from, String pfx /* TODO twine */)
{
	switch (type->kind)
	{
	case ASTType::Void:
		break;
	case ASTType::Function:
	case ASTType::Sampler1D:
	case ASTType::Sampler2D:
	case ASTType::Sampler3D:
	case ASTType::SamplerCube:
		// ...
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::UInt32:
	case ASTType::Float16:
	case ASTType::Float32:
		if (elementsWritten[from] == 0)
		{
			outerr += "\n - ";
			outerr += pfx;
		}
		break;
	case ASTType::Vector:
		for (int i = 0; i < type->sizeX; ++i)
		{
			AddMissingOutputAccessPoints(outerr, type->subType, from + i, pfx + g_VecSuffixStr[i]);
		}
		break;
	case ASTType::Matrix:
		for (int x = 0; x < type->sizeX; ++x)
		{
			for (int y = 0; y < type->sizeY; ++y)
			{
				AddMissingOutputAccessPoints(outerr, type->subType, from + x * type->sizeY + y,
					pfx + g_Arr4SuffixStr[x] + g_Arr4SuffixStr[y]);
			}
		}
		break;
	case ASTType::Structure:
		{
			auto* strTy = type->ToStructType();
			for (auto& mmb : strTy->members)
			{
				AddMissingOutputAccessPoints(outerr, mmb.type, from, pfx + "." + mmb.name);
				from += mmb.type->GetAccessPointCount();
			}
		}
		break;
	case ASTType::Array:
		for (uint32_t i = 0; i < type->elementCount; ++i)
		{
			AddMissingOutputAccessPoints(outerr, type->subType, from, pfx + "[" + StdToString(i) + "]");
			from += type->subType->GetAccessPointCount();
		}
		break;
	}
}

void VariableAccessValidator::ValidateCheckVariableInitialized(const DeclRefExpr* dre)
{
	for (int i = dre->decl->APRangeFrom; i < dre->decl->APRangeTo; ++i)
	{
		if (elementsWritten[i] == 0)
		{
			ValidateCheckVariableError(dre);
		}
	}
}

void VariableAccessValidator::ValidateCheckVariableError(const DeclRefExpr* dre)
{
	diag.EmitError("variable '" + dre->decl->name + "' used before sufficient initialization", dre->loc);
}


struct ContentValidator : ASTWalker<ContentValidator>
{
	ContentValidator(Diagnostic& d, OutputShaderFormat fmt) : diag(d), outputFmt(fmt) {}
	void RunOnAST(AST& ast)
	{
		VisitAST(ast);
	}
	void PreVisit(ASTNode* node)
	{
		if (auto* op = dyn_cast<OpExpr>(node))
		{
			if (outputFmt == OSF_HLSL_SM3 || outputFmt == OSF_GLSL_ES_100)
			{
				switch (op->opKind)
				{
				case Op_Tex1DCmp:
				case Op_Tex1DLOD0Cmp:
				case Op_Tex2DCmp:
				case Op_Tex2DLOD0Cmp:
				case Op_TexCubeCmp:
				case Op_TexCubeLOD0Cmp:
					diag.EmitError("shadow/comparison sampling intrinsics are not supported for this output",
						op->loc);
					break;
				}
			}
		}
	}
	Diagnostic& diag;
	OutputShaderFormat outputFmt;
};


static void InOutFixSemanticsAndNames(VarDecl* vd, ShaderStage stage, OutputShaderFormat shaderFormat)
{
	bool out = !!(vd->flags & VarDecl::ATTR_Out);
	switch (shaderFormat)
	{
	case OSF_HLSL_SM3:
		return;
	case OSF_HLSL_SM4:
		if (out && stage == ShaderStage_Pixel && vd->semanticName == "COLOR")
		{
			vd->semanticName = "SV_TARGET";
			return;
		}
		if (out && stage == ShaderStage_Pixel && vd->semanticName == "DEPTH")
		{
			vd->semanticName = "SV_DEPTH";
			return;
		}
		if (out && stage == ShaderStage_Vertex && vd->semanticName == "POSITION")
		{
			vd->semanticName = "SV_POSITION";
			return;
		}
		return;
	case OSF_GLSL_ES_100:
	case OSF_GLSL_140:
		if (out)
		{
			if (stage == ShaderStage_Vertex)
			{
				if (vd->semanticName == "POSITION")
				{
					vd->name = "gl_Position";
					vd->flags |= VarDecl::ATTR_Hidden;
					return;
				}
			}
			if (stage == ShaderStage_Pixel)
			{
				if (vd->semanticName == "COLOR")
				{
					if (shaderFormat == OSF_GLSL_ES_100)
					{
						vd->name = "gl_FragColor";
						vd->flags |= VarDecl::ATTR_Hidden;
						return;
					}
					else
					{
						char bfr[32];
						sprintf(bfr, "_PSCOLOR%d", vd->GetSemanticIndex());
						vd->name = bfr;
					}
					return;
				}
			}
		}

		// TODO geometry shaders?
	//	if (shaderFormat == OSF_GLSL_ES_100)
		{
			// force rename varyings to semantics to automate linkage
			if (((vd->flags & VarDecl::ATTR_Out) && stage == ShaderStage_Vertex) ||
				((vd->flags & VarDecl::ATTR_In) && stage == ShaderStage_Pixel))
			{
				vd->name = "attr" + vd->semanticName + StdToString(vd->GetSemanticIndex());
			}
		}

		if (vd->name.empty())
		{
			vd->name = vd->semanticName;
		}
		return;
	}
}

struct StructLevel
{
	ASTStructType* strTy;
	uint32_t mmbID;
	ASTNode* levILE;
};
static void GLSLAppendShaderIOVar(AST& ast, ASTFunction* F,
	ShaderStage stage, OutputShaderFormat shaderFormat,
	ASTNode* outILE, ASTNode* inSRC, ASTStructType* topStc)
{
	std::vector<StructLevel> mmbIndices;
	mmbIndices.push_back({ topStc, 0, outILE });
	while (mmbIndices.empty() == false)
	{
		auto& mmb = mmbIndices.back().strTy->members[mmbIndices.back().mmbID];
		if (mmb.type->kind == ASTType::Structure)
		{
			InitListExpr* ile = nullptr;
			if (outILE)
			{
				ile = new InitListExpr;
				ile->SetReturnType(mmb.type);
				mmbIndices.back().levILE->AppendChild(ile);
			}

			mmbIndices.push_back({ mmb.type->ToStructType(), 0, ile });
			continue;
		}


		VarDecl* vd = new VarDecl;
		F->AppendChild(vd);
		vd->name = inSRC->ToVarDecl()->name;
		for (auto& mmbidx : mmbIndices)
		{
			vd->name += "_";
			vd->name += mmbidx.strTy->members[mmbidx.mmbID].name;
		}
		vd->SetType(mmb.type);
		vd->flags = (outILE ? VarDecl::ATTR_In : VarDecl::ATTR_Out) | VarDecl::ATTR_StageIO;
		vd->semanticName = mmb.semanticName;
		vd->semanticIndex = mmb.semanticIndex;
		InOutFixSemanticsAndNames(vd, stage, shaderFormat);

		if (outILE)
		{
			auto* dre = new DeclRefExpr;
			dre->decl = vd;
			dre->SetReturnType(vd->GetType());
			mmbIndices.back().levILE->AppendChild(dre);
		}
		else
		{
			for (ReturnStmt* ret = F->firstRetStmt; ret; ret = ret->nextRetStmt)
			{
				if (dyn_cast<BlockStmt>(ret->parent) == nullptr)
				{
					BlockStmt* blk = new BlockStmt;
					ret->ReplaceWith(blk);
					blk->AppendChild(ret);
				}

				auto* exprst = new ExprStmt;
				auto* assign = new BinaryOpExpr;
				auto* dreout = new DeclRefExpr;
				auto* drestruct = new DeclRefExpr;
				dreout->decl = vd;
				dreout->SetReturnType(vd->GetType());
				drestruct->decl = inSRC->ToVarDecl();
				drestruct->SetReturnType(drestruct->decl->GetType());
				ret->InsertBeforeMe(exprst);
				exprst->AppendChild(assign);
				assign->opType = STT_OP_Assign;
				assign->AppendChild(dreout);
				assign->SetReturnType(dreout->GetReturnType());

				ASTNode* tgtexpr = assign;
				for (size_t i = mmbIndices.size(); i > 0; )
				{
					--i;
					const auto& mmbinfo = mmbIndices[i].strTy->members[mmbIndices[i].mmbID];

					auto* mmbexpr = new MemberExpr;
					mmbexpr->memberID = mmbIndices[i].mmbID;
					mmbexpr->SetReturnType(mmbinfo.type);
					tgtexpr->AppendChild(mmbexpr);
					tgtexpr = mmbexpr;
				}
				tgtexpr->AppendChild(drestruct);
			}
		}


		mmbIndices.back().mmbID++;
		while (mmbIndices.back().mmbID >= mmbIndices.back().strTy->members.size())
		{
			mmbIndices.pop_back();
			if (mmbIndices.empty())
				break;
			else
				mmbIndices.back().mmbID++;
		}
	}
}

void SwapNodesSameParent(ASTNode* a, ASTNode* b)
{
	// in case where a <-> b, a->next = b, b->next must be changed to a
	assert(a->parent == b->parent);
	ASTNode* aprev = a->prev;
	ASTNode* anext = a->next;
	a->prev = b->prev == a ? b : b->prev;
	a->next = b->next == a ? b : b->next;
	b->prev = aprev == b ? a : aprev;
	b->next = anext == b ? a : anext;
	if (a->prev)
		a->prev->next = a;
	if (a->next)
		a->next->prev = a;
	if (b->prev)
		b->prev->next = b;
	if (b->next)
		b->next->prev = b;
	if (a == a->parent->firstChild)
		a->parent->firstChild = b;
	else if (b == a->parent->firstChild)
		a->parent->firstChild = a;
	if (a == a->parent->lastChild)
		a->parent->lastChild = b;
	else if (b == a->parent->lastChild)
		a->parent->lastChild = a;
}

template <class F> void SortNodes(ASTNode* first, ASTNode* last, const F& comp)
{
	while (first != last)
	{
		// forward pass
		for (ASTNode* n = first; n != last; )
		{
		//	n->Dump(FILEStream(stderr),0);
		//	n->next->Dump(FILEStream(stderr),0);
			ASTNode* a = n;
			ASTNode* b = n->next;
			if (comp(a, b) > 0)
			{
				SwapNodesSameParent(a, b);
				// preserve first/last
				if (first == a)
					first = b;
				if (last == b)
					last = a;
			}
			else n = n->next;
		}
		last = last->prev;

		if (first == last)
			break;

		// backwards pass
		for (ASTNode* n = last; n != first; )
		{
		//	n->prev->Dump(FILEStream(stderr),0);
		//	n->Dump(FILEStream(stderr),0);
			ASTNode* a = n->prev;
			ASTNode* b = n;
			if (comp(a, b) > 0)
			{
				SwapNodesSameParent(a, b);
				// preserve first/last
				if (first == a)
					first = b;
				if (last == b)
					last = a;
			}
			else n = n->prev;
		}
		first = first->next;
	}
}

static void UnpackEntryPoint(AST& ast, ShaderStage stage, OutputShaderFormat shaderFormat)
{
	if (shaderFormat == OSF_HLSL_SM3)
		return; // same as input, no need to edit

	auto* F = ast.entryPoint;
	// extract return value
	if (F->GetReturnType()->IsVoid() == false)
	{
		auto* vd = new VarDecl;
		F->AppendChild(vd);
		vd->semanticName = F->returnSemanticName;
		vd->semanticIndex = F->returnSemanticIndex;
		F->returnSemanticName = "";
		F->returnSemanticIndex = -1;
		vd->SetType(F->GetReturnType());
		vd->flags |= VarDecl::ATTR_Out | VarDecl::ATTR_StageIO;
		InOutFixSemanticsAndNames(vd, stage, shaderFormat);

		F->SetReturnType(ast.GetVoidType());

		for (ReturnStmt* ret = F->firstRetStmt; ret; ret = ret->nextRetStmt)
		{
			if (dyn_cast<BlockStmt>(ret->parent) == nullptr)
			{
				BlockStmt* blk = new BlockStmt;
				ret->ReplaceWith(blk);
				blk->AppendChild(ret);
			}

			auto* exprst = new ExprStmt;
			auto* assign = new BinaryOpExpr;
			auto* dre = new DeclRefExpr;
			dre->decl = vd;
			dre->SetReturnType(vd->type);
			ret->InsertBeforeMe(exprst);
			exprst->AppendChild(assign);
			assign->AppendChild(dre);
			assign->AppendChild(ret->GetExpr());
			assign->opType = STT_OP_Assign;
			assign->SetReturnType(dre->GetReturnType());
		}
	}
	else // append a return statement at the end for AST normalization
	{
		ReturnStmt* ret = new ReturnStmt;
		ret->AddToFunction(F);
		F->GetCode()->AppendChild(ret);
	}

	// fold arguments into body, initialize all inputs, extract all outputs
	for (ASTNode* argdecl = F->GetFirstArg(); argdecl; )
	{
		VarDecl* argvd = argdecl->ToVarDecl();
		argdecl = argdecl->next;

		if (argvd->GetType()->kind != ASTType::Structure)
		{
			InOutFixSemanticsAndNames(argvd, stage, shaderFormat);
		}
		else
		{
			uint32_t flags = argvd->flags;
			argvd->flags &= ~(VarDecl::ATTR_In | VarDecl::ATTR_Out);

			auto* vds = dyn_cast<VarDeclStmt>(F->GetCode()->firstChild);
			if (vds == nullptr)
			{
				vds = new VarDeclStmt;
				F->GetCode()->PrependChild(vds);
			}

			if (flags & VarDecl::ATTR_In)
			{
				auto* ile = new InitListExpr;
				ile->SetReturnType(argvd->GetType());
				argvd->SetInitExpr(ile);

				GLSLAppendShaderIOVar(ast, F, stage, shaderFormat,
					ile, argvd, argvd->GetType()->ToStructType());
			}
			else
				GLSLAppendShaderIOVar(ast, F, stage, shaderFormat,
					nullptr, argvd, argvd->GetType()->ToStructType());

			vds->PrependChild(argvd);
		}
	}

	if (shaderFormat == OSF_HLSL_SM4)
	{
		// sort the arguments by type (in,out), then by class (normal,special), then by semantic
		assert(dyn_cast<const VarDecl>(F->GetFirstArg()));
		assert(dyn_cast<const VarDecl>(F->GetLastArg()));
		SortNodes(F->GetFirstArg(), F->GetLastArg(), [](const ASTNode* a, const ASTNode* b) -> int
		{
			auto* vda = dyn_cast<const VarDecl>(a);
			auto* vdb = dyn_cast<const VarDecl>(b);
			bool a_out = !!(vda->flags & VarDecl::ATTR_Out);
			bool b_out = !!(vdb->flags & VarDecl::ATTR_Out);
			if (a_out != b_out)
				return a_out - b_out;
			bool a_spec = vda->semanticName == "SV_POSITION"
				|| vda->semanticName == "SV_TARGET"
				|| vda->semanticName == "SV_DEPTH";
			bool b_spec = vdb->semanticName == "SV_POSITION"
				|| vdb->semanticName == "SV_TARGET"
				|| vdb->semanticName == "SV_DEPTH";
			if (a_spec != b_spec)
				return a_spec - b_spec;
			int name_diff = vda->semanticName.compare(vdb->semanticName);
			if (name_diff)
				return name_diff;
			int a_idx = vda->semanticIndex >= 0 ? vda->semanticIndex : 0;
			int b_idx = vdb->semanticIndex >= 0 ? vdb->semanticIndex : 0;
			return a_idx - b_idx;
		});
	//	F->Dump(FILEStream(stderr),0);
	}
	if (shaderFormat == OSF_GLSL_140 || shaderFormat == OSF_GLSL_ES_100)
	{
		while (F->GetFirstArg())
			ast.globalVars.AppendChild(F->GetFirstArg());
	}
}


// loop expressions are tough to fold out so it is avoided
// pre-condition loop gets transformed into an infinite loop with 'if(!cond)break;' at the beginning
static bool IsExprInPreCondLoop(Expr* expr)
{
	ASTNode* n = expr;
	while (n->parent->ToStmt() == nullptr)
		n = n->parent;
	if (auto* whilestmt = dyn_cast<WhileStmt>(n->parent))
	{
		if (whilestmt->GetCond() == n)
			return true;
	}
	else if (auto* forstmt = dyn_cast<ForStmt>(n->parent))
	{
		if (forstmt->GetCond() == n)
			return true;
	}
	return false;
}

static Stmt* GetStmtOfForcedExpr(Expr* expr)
{
	ASTNode* n = expr;
	while (n->parent->ToStmt() == nullptr)
		n = n->parent;
	if (auto* ifelsestmt = dyn_cast<IfElseStmt>(n->parent))
	{
		if (ifelsestmt->GetCond() == n)
			return ifelsestmt;
	}
	else if (auto* whilestmt = dyn_cast<WhileStmt>(n->parent))
	{
		if (whilestmt->GetCond() == n)
			return whilestmt;
	}
	else if (auto* dowhilestmt = dyn_cast<DoWhileStmt>(n->parent))
	{
		if (dowhilestmt->GetCond() == n)
			return dowhilestmt;
	}
	else if (auto* forstmt = dyn_cast<ForStmt>(n->parent))
	{
		if (forstmt->GetCond() == n)
			return forstmt;
	}
	return nullptr;
}

static int DistanceToSingleDeclRefExpr(Expr* expr)
{
	int dist = 0;
	while (auto* mmbexpr = dyn_cast<MemberExpr>(expr))
	{
		expr = mmbexpr->GetSource();
		dist++;
	}
	if (dyn_cast<DeclRefExpr>(expr))
		return dist;
	else
		return -1;
}

static void ExpandBlock(Stmt* stmt)
{
	if (dyn_cast<BlockStmt>(stmt->parent) == nullptr)
	{
		auto* bs = new BlockStmt;
		stmt->ReplaceWith(bs);
		bs->AppendChild(stmt);
	}
}

static Stmt* FindParentStatement(Expr* expr)
{
	ASTNode* n = expr->parent;
	while (n)
	{
		if (auto* s = n->ToStmt())
			return s;
		n = n->parent;
	}
	return nullptr;
}

static void FoldInLoopCond(Expr* expr)
{
	Stmt* feStmt = GetStmtOfForcedExpr(expr);
	if (!feStmt)
		return;
	if (auto* whilestmt = dyn_cast<WhileStmt>(feStmt))
	{
		// while( <cond> )
		//  ->
		// while(true)
		//   if( !<cond> ) break
		ExpandBlock(whilestmt->GetBody());

		auto* cond = whilestmt->GetCond();
		assert(cond->GetReturnType()->kind == ASTType::Bool);
		auto* truexpr = new BoolExpr(true, cond->GetReturnType());
		cond->ReplaceWith(truexpr);

		auto* ifstmt = new IfElseStmt;
		auto* notexpr = new UnaryOpExpr;
		auto* breakstmt = new BreakStmt;

		ifstmt->AppendChild(notexpr);
		ifstmt->AppendChild(breakstmt);
		notexpr->SetSource(cond);
		notexpr->opType = STT_OP_Not;
		whilestmt->GetBody()->PrependChild(ifstmt);
	}
	else if (auto* dowhilestmt = dyn_cast<DoWhileStmt>(feStmt))
	{
		// do ... while( <cond> )
		//  ->
		// do
		//   bool wasBreak = false;
		//   do ...
		//   while(0)
		//   if( wasBreak ) break
		//   if( !<cond> ) break
		// while(true)
		ExpandBlock(dowhilestmt->GetBody());
		assert(!"TODO");
	}
}

static DeclRefExpr* FoldOut(Expr* expr)
{
	FoldInLoopCond(expr);

	Stmt* marker = FindParentStatement(expr);
	ExpandBlock(marker);

	auto* vds = new VarDeclStmt;
	auto* vd = new VarDecl;
	auto* dre = new DeclRefExpr;

	vd->name = "";
	vd->SetType(expr->GetReturnType());
	dre->decl = vd;
	dre->SetReturnType(expr->GetReturnType());

	expr->ReplaceWith(dre);
	vd->SetInitExpr(expr);
	vds->AppendChild(vd);
	marker->InsertBeforeMe(vds);

	return dre;
}

static Expr* FoldOutIfBest(Expr* expr)
{
	bool inLoop = IsExprInPreCondLoop(expr);
	int dist2dre = DistanceToSingleDeclRefExpr(expr);

	if (dist2dre == 0)
		return expr; // DeclRefExpr is the source
	if (dist2dre != -1 && inLoop)
		return expr; // not a complex expression and is part of loop expression, avoid breaking the loop

	return FoldOut(expr);
}

static ASTNode* GetWriteContext(Expr* expr)
{
	while (expr)
	{
		if (auto* binop = dyn_cast<BinaryOpExpr>(expr->parent))
		{
			if (TokenIsOpAssign(binop->opType) && binop->GetLft() == expr)
				return binop;
		}
#if 0
		else if (auto* fcall = dyn_cast<FCallExpr>(expr->parent))
		{
			// TODO
		}
#endif
		expr = expr->parent->ToExpr();
	}
	return nullptr;
}

struct MatrixSwizzleUnpacker : ASTWalker<MatrixSwizzleUnpacker>
{
	MatrixSwizzleUnpacker(AST& a) : ast(a) {}
	void PostVisit(ASTNode* node)
	{
		if (auto* mmbexpr = dyn_cast<MemberExpr>(node))
		{
			if (mmbexpr->swizzleComp && mmbexpr->GetSource()->GetReturnType()->kind == ASTType::Matrix)
			{
				ASTType* mtxType = mmbexpr->GetSource()->GetReturnType();
				ASTType* scalarType = ast.CastToScalar(mtxType);

				if (mmbexpr->swizzleComp == 1)
				{
					// just change to double array index lookup
					int idx = mmbexpr->memberID & 0xf;
					int row = idx / 4;
					int col = idx % 4;

					IndexExpr* cellExpr = new IndexExpr;
					IndexExpr* colExpr = new IndexExpr;

					cellExpr->AppendChild(colExpr);
					cellExpr->AppendChild(new Int32Expr(row, ast.GetInt32Type()));
					cellExpr->SetReturnType(ast.CastToVector(scalarType, 4));
					colExpr->AppendChild(mmbexpr->GetSource());
					colExpr->AppendChild(new Int32Expr(col, ast.GetInt32Type()));
					colExpr->SetReturnType(scalarType);
					delete mmbexpr->ReplaceWith(cellExpr);
					return;
				}
				else
				{
					if (ASTNode* writeCtx = GetWriteContext(mmbexpr))
					{
						// multiple cases:
						//---------------
						// out.MTXSWZ = src
						//  ->
						// <type> TMP = src
						// out[X][Y] = TMP[Z] x N
						//---------------
						// if( resolve( out.MTXSWZ = src ) ) ...
						//  ->
						// <type> TMP = src
						// out[X][Y] = TMP[Z] x N
						// if( resolve( TMP ) ) ...
						//---------------
						// while( resolve( out.MTXSWZ = src ) ) ...
						//  ->
						// while(true)
						//   <type> TMP = src
						//   out[X][Y] = TMP[Z] x N
						//   if( !resolve( TMP ) ) break
						//   ...
						//---------------
						// do ... while( resolve( out.MTXSWZ = src ) )
						//  ->
						// bool doWhileCond;
						// do ...
						//   <type> TMP = src
						//   out[X][Y] = TMP[Z] x N
						//   doWhileCond = resolve( TMP );
						// while( doWhileCond )
						//---------------
						// func( out.MTXSWZ )
						//  ->
						// <type> TMP
						// func( TMP )
						// out.MTXSWZ = TMP
						//---------------
						if (auto* binop = dyn_cast<BinaryOpExpr>(writeCtx)) // assignment
						{
							ASTNode* insertPos;
							assert(binop->GetLft() == mmbexpr);
							ASTNode* delNode = nullptr;
							if (binop->parent->ToStmt() == nullptr || GetStmtOfForcedExpr(binop))
							{
								// if( resolve( binop ) ) -> <type> TMP = binop; if( resolve( TMP ) )
								FoldOut(binop);
								insertPos = binop->parent->parent;
							}
							else
							{
								insertPos = binop->parent;
								delNode = binop->parent;
							}
							// validate statement insert position
							assert(dyn_cast<ExprStmt>(insertPos)
								|| dyn_cast<VarDeclStmt>(insertPos));
							ExpandBlock(insertPos->ToStmt());

							Expr* dst = mmbexpr->GetSource();
							Expr* src = FoldOutIfBest(binop->GetRgt());

							for (int i = 0; i < mmbexpr->swizzleComp; ++i)
							{
								Expr* myDst = i == 0 ? dst : dst->DeepClone()->ToExpr();
								Expr* mySrc = i == 0 ? src : src->DeepClone()->ToExpr();

								int idx = (mmbexpr->memberID >> (4 * i)) & 0xf;
								int row = idx / 4;
								int col = idx % 4;

								auto* exprStmt = new ExprStmt;
								auto* elBinOp = new BinaryOpExpr;
								auto* cellExpr = new IndexExpr;
								auto* colExpr = new IndexExpr;
								auto* elExpr = new IndexExpr;

								cellExpr->AppendChild(colExpr);
								cellExpr->AppendChild(new Int32Expr(row, ast.GetInt32Type()));
								cellExpr->SetReturnType(ast.CastToVector(scalarType, mtxType->sizeX));
								colExpr->AppendChild(myDst);
								colExpr->AppendChild(new Int32Expr(col, ast.GetInt32Type()));
								colExpr->SetReturnType(scalarType);
								elExpr->AppendChild(mySrc);
								elExpr->AppendChild(new Int32Expr(i, ast.GetInt32Type()));
								elExpr->SetReturnType(scalarType);
								elBinOp->AppendChild(cellExpr);
								elBinOp->AppendChild(elExpr);
								elBinOp->SetReturnType(mmbexpr->GetReturnType());
								elBinOp->opType = STT_OP_Assign;

								exprStmt->AppendChild(elBinOp);
							//	exprStmt->Dump(std::cout,0);
								insertPos->InsertAfterMe(exprStmt);
								insertPos = exprStmt;
							}

							if (delNode)
								delete delNode;
							else
								delete binop->ReplaceWith(binop->GetRgt());
						//	insertPos->parent->Dump(std::cout);
							return;
						}
#if 0
						else if (auto* fcall = dyn_cast<FCallExpr>(writeCtx)) // 'out' argument
						{
							// TODO
						}
#endif
					}
					else
					{
						// read-only context, always the same
						Expr* src = FoldOutIfBest(mmbexpr->GetSource());

						auto* ile = new InitListExpr;
						ile->SetReturnType(mmbexpr->GetReturnType());

						for (int i = 0; i < mmbexpr->swizzleComp; ++i)
						{
							Expr* mySrc = i == 0 ? src : src->DeepClone()->ToExpr();

							int idx = (mmbexpr->memberID >> (4 * i)) & 0xf;
							int row = idx / 4;
							int col = idx % 4;

							IndexExpr* cellExpr = new IndexExpr;
							IndexExpr* colExpr = new IndexExpr;

							cellExpr->AppendChild(colExpr);
							cellExpr->AppendChild(new Int32Expr(row, ast.GetInt32Type()));
							cellExpr->SetReturnType(ast.CastToVector(scalarType, mtxType->sizeX));
							colExpr->AppendChild(mySrc);
							colExpr->AppendChild(new Int32Expr(col, ast.GetInt32Type()));
							colExpr->SetReturnType(scalarType);

							ile->AppendChild(cellExpr);
						}

						delete mmbexpr->ReplaceWith(ile);
					}
				}
			}
		}
	}
	AST& ast;
};

static void UnpackMatrixSwizzle(AST& ast)
{
	MatrixSwizzleUnpacker(ast).VisitAST(ast);
}


// TODO merge with hlslparser's version
static Expr* CastExprTo(Expr* val, ASTType* to)
{
	assert(to);
	if (to != val->GetReturnType())
	{
		CastExpr* cast = new CastExpr;
		cast->SetReturnType(to);
		val->ReplaceWith(cast);
		cast->SetSource(val);
		return cast;
	}
	return val;
}

static Expr* GetReferenceToElement(AST& ast, Expr* src, int accessPointNum)
{
	ASTType* t = src->GetReturnType();
	assert(accessPointNum >= 0 && accessPointNum < t->GetAccessPointCount());
	switch (t->kind)
	{
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::UInt32:
	case ASTType::Float16:
	case ASTType::Float32:
	case ASTType::Sampler1D:
	case ASTType::Sampler2D:
	case ASTType::Sampler3D:
	case ASTType::SamplerCube:
		return src;
	case ASTType::Vector:
		{
			auto* idxexpr = new IndexExpr;
			idxexpr->SetSource(src);
			idxexpr->AppendChild(new Int32Expr(accessPointNum, ast.GetInt32Type()));
			idxexpr->SetReturnType(t->subType);
			return idxexpr;
		}
	case ASTType::Matrix:
		{
			auto* idxexprA = new IndexExpr;
			auto* idxexprB = new IndexExpr;
			idxexprA->SetSource(src);
			idxexprA->AppendChild(new Int32Expr(accessPointNum / 4, ast.GetInt32Type()));
			idxexprA->SetReturnType(ast.GetVectorType(t->subType, t->sizeX));
			idxexprB->SetSource(idxexprA);
			idxexprB->AppendChild(new Int32Expr(accessPointNum % 4, ast.GetInt32Type()));
			idxexprB->SetReturnType(t->subType);
			return idxexprB;
		}
	case ASTType::Structure:
		{
			auto* strTy = t->ToStructType();
			int offset = 0;
			for (size_t i = 0; i < strTy->members.size(); ++i)
			{
				int apc = strTy->members[i].type->GetAccessPointCount();
				if (accessPointNum - offset < apc)
				{
					auto* mmbexpr = new MemberExpr;
					mmbexpr->SetSource(src);
					mmbexpr->SetReturnType(strTy->members[i].type);
					mmbexpr->memberID = i;
					return GetReferenceToElement(ast, mmbexpr, accessPointNum - offset);
				}
				offset += apc;
			}
		}
	case ASTType::Array:
		{
			int subapc = t->subType->GetAccessPointCount();
			int idx = accessPointNum / subapc;
			auto* idxexpr = new IndexExpr;
			idxexpr->SetSource(src);
			idxexpr->AppendChild(new Int32Expr(idx, ast.GetInt32Type()));
			idxexpr->SetReturnType(t->subType);
			return GetReferenceToElement(ast, idxexpr, accessPointNum % subapc);
		}
	default:
		return nullptr;
	}
}

static void GenerateComponentAssignments(AST& ast, Expr* ile_or_cast)
{
	int numAPs = ile_or_cast->GetReturnType()->GetAccessPointCount();
	int origInputs = ile_or_cast->childCount;

	VarDecl* vd;
	if (VarDecl* pvd = dyn_cast<VarDecl>(ile_or_cast->parent))
	{
		vd = pvd;
	}
	else
	{
		DeclRefExpr* src = FoldOut(ile_or_cast);
		vd = src->decl;
	}
	ASTNode* insertPos = vd->parent;
	assert(dyn_cast<VarDeclStmt>(insertPos));

	Expr* sourceNode = ile_or_cast->firstChild->ToExpr();
	bool isSource1 = ile_or_cast->childCount == 1
		&& sourceNode->GetReturnType()->GetAccessPointCount() == 1;
	int sourceOffset = 0;
	for (int i = 0; i < numAPs; ++i)
	{
		if (isSource1 == false)
		{
			for (;;)
			{
				int srcapc = sourceNode->GetReturnType()->GetAccessPointCount();
				if (i - sourceOffset < srcapc)
					break;
				sourceOffset += srcapc;
				sourceNode = sourceNode->next->ToExpr();
			}
		}
		auto* exprStmt = new ExprStmt;
		auto* binop = new BinaryOpExpr;
		auto* dstdre = new DeclRefExpr;
		dstdre->decl = vd;
		dstdre->SetReturnType(vd->GetType());
		binop->opType = STT_OP_Assign;
		binop->AppendChild(GetReferenceToElement(ast, dstdre, i));
		binop->AppendChild(GetReferenceToElement(ast, sourceNode->DeepClone()->ToExpr(),
			isSource1 ? 0 : i - sourceOffset));
		CastExprTo(binop->GetRgt(), binop->GetLft()->GetReturnType());
		binop->SetReturnType(binop->GetLft()->GetReturnType());
		exprStmt->AppendChild(binop);
		insertPos->InsertAfterMe(exprStmt);
		insertPos = exprStmt;
	}

	delete ile_or_cast; // now replaced with explicit component assignments
}


enum ModRoundMode
{
	MRM_Trunc,
	MRM_IntCast,
	MRM_Floor,
};

Expr* ImplementMod(AST& ast, OpExpr* op, ModRoundMode roundMode)
{
	// x - y * trunc(x/y)
	// -(x, *(y, trunc(/(x,y))))
	auto* x = FoldOutIfBest(op->GetFirstArg()->ToExpr());
	auto* y = FoldOutIfBest(op->GetFirstArg()->next->ToExpr());
	auto* sub = new OpExpr;
	auto* mul = new OpExpr;
	auto* roundOp = roundMode != MRM_IntCast ? new OpExpr : nullptr;
	auto* div = new OpExpr;

	auto* rt = x->GetReturnType();
	sub->SetReturnType(rt);
	sub->opKind = Op_Subtract;
	sub->AppendChild(x);
	sub->AppendChild(mul);
	mul->SetReturnType(rt);
	mul->opKind = Op_Multiply;
	mul->AppendChild(y);
	mul->AppendChild(roundOp ? roundOp : div);
	if (roundOp)
	{
		roundOp->SetReturnType(rt);
		roundOp->opKind = roundMode == MRM_Floor ? Op_Floor : Op_Trunc;
		roundOp->AppendChild(div);
	}
	div->SetReturnType(rt);
	div->opKind = Op_Divide;
	div->AppendChild(x->DeepClone());
	div->AppendChild(y->DeepClone());
	if (!roundOp)
	{
		CastExprTo(mul->GetRgt(), ast.CastToInt(mul->GetRgt()->GetReturnType()));
		CastExprTo(mul->GetRgt(), ast.CastToFloat(mul->GetRgt()->GetReturnType()));
	}

	delete op->ReplaceWith(sub);
	return sub;
}

struct APIPadding : ASTWalker<APIPadding>
{
	APIPadding(AST& a, Diagnostic& d, OutputShaderFormat of) : ast(a), diag(d), outputFmt(of){}
	void PreVisit(ASTNode* node)
	{
		if (auto* op = dyn_cast<OpExpr>(node))
		{
			if (outputFmt == OSF_HLSL_SM3 || outputFmt == OSF_HLSL_SM4)
			{
				switch (op->opKind)
				{
				case Op_ModGLSL:
					curPos = ImplementMod(ast, op, MRM_Floor);
					break;
				}
			}
		}
	}
	AST& ast;
	Diagnostic& diag;
	OutputShaderFormat outputFmt;
};

static void PadAPI(AST& ast, Diagnostic& diag, OutputShaderFormat outputFmt)
{
	APIPadding(ast, diag, outputFmt).VisitAST(ast);
}


struct GLSLConversionPass : ASTWalker<GLSLConversionPass>
{
	GLSLConversionPass(AST& a, Diagnostic& d, OutputShaderFormat of) : ast(a), diag(d), outputFmt(of){}
	enum MtxUnpackRecombineMode
	{
		MURM_Matrix,  // recombine parts back into a matrix
		MURM_Cascade, // recombine parts into a vector and pass that through the same op
	};
	void MatrixUnpack(OpExpr* fcintrin, MtxUnpackRecombineMode recombineMode = MURM_Matrix)
	{
		auto* retTy = fcintrin->GetReturnType();
		ASTType* mtxTy = nullptr;
		switch (recombineMode)
		{
		case MURM_Matrix:
			mtxTy = retTy;
			break;
		case MURM_Cascade:
			mtxTy = fcintrin->GetFirstArg()->ToExpr()->GetReturnType();
			break;
		}
		if (mtxTy->kind != ASTType::Matrix)
			return;

		int numCols = mtxTy->sizeX;
		auto* ile = new InitListExpr;
		ile->SetReturnType(recombineMode == MURM_Matrix ? mtxTy : ast.GetVectorType(retTy, numCols));
		fcintrin->ReplaceWith(ile);
		ile->AppendChild(fcintrin);

		for (auto* arg = fcintrin->GetFirstArg(); arg; )
		{
			auto* argExpr = arg->ToExpr();
			arg = arg->next;
			assert(argExpr->GetReturnType()->IsSameSizeVM(mtxTy));

			argExpr = FoldOutIfBest(argExpr);
			auto* idxe = new IndexExpr;
			argExpr->ReplaceWith(idxe);
			idxe->SetSource(argExpr);
			idxe->SetReturnType(ast.GetVectorType(mtxTy->subType, mtxTy->sizeY));
			idxe->AppendChild(new Int32Expr(0, ast.GetInt32Type()));
		}
		for(int i = 1; i < numCols; ++i)
		{
			auto* fcx = dyn_cast<OpExpr>(fcintrin->DeepClone());
			for (auto* arg = fcx->GetFirstArg(); arg; arg = arg->next)
			{
				dyn_cast<Int32Expr>(dyn_cast<IndexExpr>(arg)->GetIndex())->value = i;
			}
			ile->AppendChild(fcx);
		}
		if (recombineMode == MURM_Cascade)
		{
			auto* fcrecomb = fcintrin->Clone();
			ile->ReplaceWith(fcrecomb);
			fcrecomb->AppendChild(ile);
		}
	//	ile->Dump(FILEStream(stderr),0);
	}
	void CastArgsToFloat(OpExpr* fcintrin, bool preserveInts)
	{
		int numInts = 0;
		for (ASTNode* arg = fcintrin->GetFirstArg(); arg; )
		{
			Expr* curArg = arg->ToExpr();
			arg = arg->next;

			auto* rt = curArg->GetReturnType();
			if (rt->IsIntBased())
				numInts++;
			if (rt->IsFloatBased() == false)
				CastExprTo(curArg, ast.CastToFloat(rt));
		}
		if (preserveInts && fcintrin->childCount == numInts)
		{
			CastExprTo(fcintrin, ast.CastToInt(fcintrin->GetReturnType()));
		}
	}
	void CastArgsES100(OpExpr* fcintrin, bool preserveInts)
	{
		if (outputFmt == OSF_GLSL_ES_100)
			CastArgsToFloat(fcintrin, preserveInts);
	}
	bool IsNewSamplingAPI(){ return outputFmt == OSF_GLSL_140; }
	void PreVisit(ASTNode* node)
	{
		if (auto* op = dyn_cast<OpExpr>(node))
		{
			switch (op->opKind)
			{
			case Op_FMod:
				curPos = ImplementMod(ast, op, outputFmt == OSF_GLSL_ES_100 ? MRM_IntCast : MRM_Trunc);
				break;
			case Op_Log10:
				// log10(x) -> log(x) / log(10)
				{
					op->opKind = Op_Log;
					auto* div = new OpExpr;
					div->opKind = Op_Divide;
					div->SetReturnType(op->GetReturnType());
					op->ReplaceWith(div);
					div->AppendChild(op);
					div->AppendChild(new Float32Expr(log(10), ast.GetFloat32Type()));
					CastExprTo(div->GetRgt(), op->GetReturnType());
					CastArgsES100(div, false);
					MatrixUnpack(div);
				}
				// PostVisit will do matrix expansion/argument casting as needed on Op_Log
				break;
			case Op_Round:
				if (outputFmt == OSF_GLSL_ES_100)
				{
					// floor(x + 0.5)
					auto* x = FoldOutIfBest(op->GetFirstArg()->ToExpr());
					auto* half = new Float32Expr(0.5, ast.GetFloat32Type());
					auto* add = new OpExpr;
					op->opKind = Op_Floor;
					x->ReplaceWith(add);
					add->opKind = Op_Add;
					add->SetReturnType(x->GetReturnType());
					add->AppendChild(x);
					add->AppendChild(half);
					CastExprTo(half, x->GetReturnType());
				}
				break;
			case Op_Trunc:
				if (outputFmt == OSF_GLSL_ES_100)
				{
					CastExprTo(op->GetFirstArg()->ToExpr(),
						ast.CastToInt(op->GetFirstArg()->ToExpr()->GetReturnType()));
					CastExprTo(op->GetFirstArg()->ToExpr(),
						ast.CastToFloat(op->GetFirstArg()->ToExpr()->GetReturnType()));
					curPos = op->GetFirstArg();
					delete op->ReplaceWith(op->GetFirstArg());
				}
				break;
			}
		}
	}
	void PostVisit(ASTNode* node)
	{
		if (auto* op = dyn_cast<OpExpr>(node))
		{
			switch (op->opKind)
			{
			case Op_Modulus:
				MatrixUnpack(op);
				break;
			case Op_Abs:
			case Op_ACos:
			case Op_ASin:
			case Op_ATan:
			case Op_ATan2:
			case Op_Ceil:
			case Op_Cos:
			case Op_CosH:
			case Op_Degrees:
			case Op_Exp:
			case Op_Exp2:
			case Op_Floor:
			case Op_LdExp:
			case Op_Lerp:
			case Op_Log:
			case Op_Log2:
			case Op_Radians:
			case Op_Sin:
			case Op_SinH:
			case Op_SmoothStep:
			case Op_Tan:
			case Op_TanH:
				CastArgsES100(op, false);
				MatrixUnpack(op);
				break;
			case Op_Clip:
				{
					// clip return type is 'void' so parent should be ExprStmt
					assert(op->parent->ToStmt());

					// convert to `if (<x> < 0) discard;` for each access point
					Expr* arg = op->GetFirstArg()->ToExpr();

					int numAPs = arg->GetReturnType()->GetAccessPointCount();
					for (int i = 0; i < numAPs; ++i)
					{
						auto* ifstmt = new IfElseStmt;
						auto* binop = new BinaryOpExpr;
						auto* discard = new DiscardStmt;

						ifstmt->AppendChild(binop);
						ifstmt->AppendChild(discard);
						binop->opType = STT_OP_Less;
						binop->SetReturnType(ast.GetBoolType());
						binop->AppendChild(GetReferenceToElement(ast, arg->DeepClone()->ToExpr(), i));
						Expr* cnst = nullptr;
						auto* srcTy = binop->GetLft()->GetReturnType();
						switch (srcTy->kind)
						{
						case ASTType::Bool: CastExprTo(binop->GetLft(), ast.GetInt32Type()); // ...
						case ASTType::Int32:
						case ASTType::UInt32: cnst = new Int32Expr(0, srcTy); break;
						case ASTType::Float16:
						case ASTType::Float32: cnst = new Float32Expr(0, srcTy); break;
						}
						binop->AppendChild(cnst);
						op->parent->InsertBeforeMe(ifstmt);
					}

					delete op; // leaves unused ExprStmt
					break;
				}
			case Op_Distance:
				CastArgsToFloat(op, false);
				break;
			case Op_Dot:
				CastArgsToFloat(op, true);
				break;
			case Op_Clamp:
			case Op_Max:
			case Op_Min:
				CastArgsES100(op, true);
				MatrixUnpack(op);
				break;
			case Op_Sign:
				CastArgsES100(op, false);
				op->SetReturnType(ast.CastToFloat(op->GetReturnType()));
				CastExprTo(op, ast.CastToInt(op->GetReturnType()));
				MatrixUnpack(op);
				break;
			case Op_Saturate:
				CastArgsES100(op, false);
				MatrixUnpack(op);
				break;
			case Op_All:
			case Op_Any:
				if (op->GetFirstArg()->ToExpr()->GetReturnType()->IsNumeric())
				{
					// scalar passed, replace with bool cast
					delete op->ReplaceWith(curPos = CastExprTo(
						op->GetFirstArg()->ToExpr(), ast.GetBoolType()));
				}
				else
				{
					MatrixUnpack(op, MURM_Cascade);
					for (ASTNode* expr = op; expr; expr = expr->next)
					{
						auto* opin = dyn_cast<OpExpr>(expr);
						auto* arg = opin->GetFirstArg()->ToExpr();
						CastExprTo(arg, ast.CastToBool(arg->GetReturnType()));
					}
				}
				break;
			case Op_ModGLSL:
			case Op_Pow:
			case Op_Round:
			case Op_Trunc:
				MatrixUnpack(op);
				break;
			}
			return;
		}
		if (auto* idxe = dyn_cast<IndexExpr>(node))
		{
			if (!idxe->GetIndex()->GetReturnType()->IsIntBased())
			{
				CastExprTo(idxe->GetIndex(), ast.CastToInt(idxe->GetIndex()->GetReturnType()));
			}
			return;
		}
		if (auto* ile = dyn_cast<InitListExpr>(node))
		{
			if (ile->GetReturnType()->kind == ASTType::Structure ||
				ile->GetReturnType()->kind == ASTType::Array)
			{
				GenerateComponentAssignments(ast, ile);
			}
			return;
		}
		if (auto* cast = dyn_cast<CastExpr>(node))
		{
			if ((cast->GetReturnType()->kind == ASTType::Structure &&
				cast->GetSource()->GetReturnType()->IsNumericBased()) ||
				(cast->GetSource()->GetReturnType()->kind == ASTType::Structure &&
				cast->GetReturnType()->IsNumericBased()))
			{
				GenerateComponentAssignments(ast, cast);
			}
			return;
		}
		if (auto* exprStmt = dyn_cast<ExprStmt>(node))
		{
			if (!exprStmt->GetExpr())
			{
				delete exprStmt; // cleanup
			}
			return;
		}
	}
	AST& ast;
	Diagnostic& diag;
	OutputShaderFormat outputFmt;
};

static void GLSLConvert(AST& ast, Diagnostic& diag, OutputShaderFormat outputFmt)
{
	GLSLConversionPass(ast, diag, outputFmt).VisitAST(ast);
}


struct SplitTexSampleArgsPass : ASTWalker<SplitTexSampleArgsPass>
{
	SplitTexSampleArgsPass(AST& a, Diagnostic& d, OutputShaderFormat of)
		: ast(a), diag(d), outputFmt(of){}
	void ExpandCoord(ASTNode* arg, int dims, bool projDivide = false)
	{
		auto* argcoord = FoldOutIfBest(arg->ToExpr());
		auto* argW = argcoord->Clone()->ToExpr();
		argcoord->InsertAfterMe(argW);
		auto* coordSwizzle = new MemberExpr;
		coordSwizzle->swizzleComp = dims;
		coordSwizzle->memberID = 0 | (1<<2) | (2<<4);
		auto* WSwizzle = new MemberExpr;
		WSwizzle->memberID = 3;
		WSwizzle->swizzleComp = 1;
		argcoord->ReplaceWith(coordSwizzle);
		argW->ReplaceWith(WSwizzle);
		coordSwizzle->SetReturnType(ast.CastToVector(argcoord->GetReturnType(), dims, true));
		coordSwizzle->AppendChild(argcoord);
		WSwizzle->SetReturnType(ast.CastToScalar(argcoord->GetReturnType()));
		WSwizzle->AppendChild(argW);

		if (projDivide)
		{
			auto* div = new OpExpr;
			div->opKind = Op_Divide;
			div->SetReturnType(coordSwizzle->GetReturnType());
			coordSwizzle->ReplaceWith(div);
			div->AppendChild(coordSwizzle);
			div->AppendChild(WSwizzle);
			CastExprTo(WSwizzle, coordSwizzle->GetReturnType());
		}
	}
	void CombineCoords(ASTNode* arg, int dims, bool ins0)
	{
		auto* argcoord = arg->ToExpr();
		auto* argz = arg->next->ToExpr();
		auto* ile = new InitListExpr;
		ile->SetReturnType(ast.CastToVector(arg->ToExpr()->GetReturnType(), dims, true));
		argcoord->ReplaceWith(ile);
		ile->AppendChild(argcoord);
		if (ins0)
		{
			ile->AppendChild(new Float32Expr(0, ast.GetFloat32Type()));
		}
		ile->AppendChild(argz);
	}
	void ConvertV1ToV2(ASTNode* arg, bool all = false)
	{
		for (;;)
		{
			auto* argexpr = arg->ToExpr();
			arg = arg->next;

			auto* ile = new InitListExpr;
			ile->SetReturnType(ast.CastToVector(argexpr->GetReturnType(), 2, true));
			argexpr->ReplaceWith(ile);
			ile->AppendChild(argexpr);
			ile->AppendChild(new Float32Expr(0, ast.GetFloat32Type()));

			if (!all || !arg)
				break;
		}
	}
	void PostVisit(ASTNode* node)
	{
		if (auto* op = dyn_cast<OpExpr>(node))
		{
			switch (op->opKind)
			{
			case Op_Tex1DBias:
			case Op_Tex1DLOD:
				ExpandCoord(op->GetFirstArg()->next, 1);
				break;
			case Op_Tex2DBias:
			case Op_Tex2DLOD:
				ExpandCoord(op->GetFirstArg()->next, 2);
				break;
			case Op_Tex3DBias:
			case Op_Tex3DLOD:
			case Op_TexCubeBias:
			case Op_TexCubeLOD:
				ExpandCoord(op->GetFirstArg()->next, 3);
				break;
			}

			if (outputFmt == OSF_GLSL_ES_100)
			{
				switch (op->opKind)
				{
				case Op_Tex1D:
					op->opKind = Op_Tex2D;
					ConvertV1ToV2(op->GetFirstArg()->next);
					break;
				case Op_Tex1DBias:
					op->opKind = Op_Tex2DBias;
					ConvertV1ToV2(op->GetFirstArg()->next);
					break;
				case Op_Tex1DGrad:
					op->opKind = Op_Tex2DGrad;
					ConvertV1ToV2(op->GetFirstArg()->next, true);
					break;
				case Op_Tex1DLOD:
					op->opKind = Op_Tex2DLOD;
					ConvertV1ToV2(op->GetFirstArg()->next);
					break;
				case Op_Tex1DProj:
					op->opKind = Op_Tex2DProj;
					break;
				case Op_Tex3D:
				case Op_Tex3DBias:
				case Op_Tex3DGrad:
				case Op_Tex3DLOD:
				case Op_Tex3DProj:
					diag.EmitError("GLSL ES 1.0 does not support 3D textures", op->loc);
					break;
				case Op_TexCubeProj:
					op->opKind = Op_TexCube;
					ExpandCoord(op->GetFirstArg()->next, 3, true);
					break;
				}
			}
			else if (outputFmt == OSF_GLSL_140)
			{
				switch (op->opKind)
				{
				case Op_TexCubeProj:
					op->opKind = Op_TexCube;
					ExpandCoord(op->GetFirstArg()->next, 3, true);
					break;
				case Op_Tex1DCmp:
				case Op_Tex1DLOD0Cmp:
					CombineCoords(op->GetFirstArg()->next, 3, true);
					break;
				case Op_Tex2DCmp:
				case Op_Tex2DLOD0Cmp:
					CombineCoords(op->GetFirstArg()->next, 3, false);
					break;
				case Op_TexCubeCmp:
				case Op_TexCubeLOD0Cmp:
					CombineCoords(op->GetFirstArg()->next, 4, false);
					break;
				}
			}
			else if (outputFmt == OSF_HLSL_SM4)
			{
				switch (op->opKind)
				{
				case Op_Tex1DProj:
					ExpandCoord(op->GetFirstArg()->next, 1, true);
					op->opKind = Op_Tex1D;
					break;
				case Op_Tex2DProj:
					ExpandCoord(op->GetFirstArg()->next, 2, true);
					op->opKind = Op_Tex2D;
					break;
				case Op_Tex3DProj:
					ExpandCoord(op->GetFirstArg()->next, 3, true);
					op->opKind = Op_Tex3D;
					break;
				case Op_TexCubeProj:
					ExpandCoord(op->GetFirstArg()->next, 3, true);
					op->opKind = Op_TexCube;
					break;
				}
			}
		}
	}
	AST& ast;
	Diagnostic& diag;
	OutputShaderFormat outputFmt;
};

static void SplitTexSampleArgs(AST& ast, Diagnostic& diag, OutputShaderFormat outputFmt)
{
	if (outputFmt == OSF_GLSL_ES_100)
	{
		// replace sampler1D with sampler2D for GLSL ES 1.0
		auto* typeS1D = ast.GetSampler1DType();
		auto* typeS2D = ast.GetSampler2DType();
		while (typeS1D->firstUse)
			typeS1D->firstUse->ChangeAssocType(typeS2D);
	}
	SplitTexSampleArgsPass(ast, diag, outputFmt).VisitAST(ast);
}


static void ReplaceVM1Type(AST& ast, ASTType* type)
{
	// cast all return/declaration types in code
	ASTType* scalarType = ast.CastToScalar(type);
	while (type->firstUse)
		type->firstUse->ChangeAssocType(scalarType);
}

static void ReplaceM1DType(AST& ast, ASTType* type, bool useY)
{
	// cast all return/declaration types in code
	ASTType* scalarType = ast.CastToScalar(type);
	ASTType* vectorType = ast.GetVectorType(scalarType, useY ? type->sizeY : type->sizeX);
	while (type->firstUse)
		type->firstUse->ChangeAssocType(vectorType);
}

static void RemoveVM1AndM1DTypes(AST& ast)
{
	ReplaceVM1Type(ast, ast.GetBoolVecType(1));
	ReplaceVM1Type(ast, ast.GetInt32VecType(1));
	ReplaceVM1Type(ast, ast.GetFloat16VecType(1));
	ReplaceVM1Type(ast, ast.GetFloat32VecType(1));
	ReplaceVM1Type(ast, ast.GetBoolMtxType(1, 1));
	ReplaceVM1Type(ast, ast.GetInt32MtxType(1, 1));
	ReplaceVM1Type(ast, ast.GetFloat16MtxType(1, 1));
	ReplaceVM1Type(ast, ast.GetFloat32MtxType(1, 1));

	for (int i = 2; i <= 4; ++i)
	{
		ReplaceM1DType(ast, ast.GetBoolMtxType(i, 1), false);
		ReplaceM1DType(ast, ast.GetInt32MtxType(i, 1), false);
		ReplaceM1DType(ast, ast.GetFloat16MtxType(i, 1), false);
		ReplaceM1DType(ast, ast.GetFloat32MtxType(i, 1), false);

		ReplaceM1DType(ast, ast.GetBoolMtxType(1, i), true);
		ReplaceM1DType(ast, ast.GetInt32MtxType(1, i), true);
		ReplaceM1DType(ast, ast.GetFloat16MtxType(1, i), true);
		ReplaceM1DType(ast, ast.GetFloat32MtxType(1, i), true);
	}

	for (ASTStructType* stc = ast.firstStructType; stc; stc = stc->nextStructType)
	{
		for (auto& mmb : stc->members)
		{
			if (mmb.type->IsVM1())
				mmb.type = ast.CastToScalar(mmb.type);
			else if (int dim = mmb.type->GetM1Dim())
				mmb.type = ast.CastToVector(ast.CastToScalar(mmb.type), dim);
		}
	}
}


struct ArrayOfArrayRemover : ASTWalker<ArrayOfArrayRemover>
{
	ArrayOfArrayRemover(AST& a) : ast(a){}
	void PostVisit(ASTNode* node)
	{
		// tree: .. IndexExpr[2] { IndexExpr[1] { DeclRefExpr?, <index> }, <index> }
		if (auto* idxe = dyn_cast<IndexExpr>(node))
		{
			if (auto* srcidxe = dyn_cast<IndexExpr>(idxe->GetSource()))
			{
				if (idxe->GetSource()->GetReturnType()->kind == ASTType::Array &&
					srcidxe->GetSource()->GetReturnType()->kind == ASTType::Array)
				{
					// idxe { srcidxe { <source>, <idxin> }, <idxout> }
					//  ->
					// idxe { <source>, <idxin> * srcidxe.rettype.elementCount + <idxout> }
					auto* mul = new BinaryOpExpr;
					mul->AppendChild(srcidxe->GetIndex());
					mul->AppendChild(new Int32Expr(
						srcidxe->GetReturnType()->elementCount,
						ast.GetInt32Type()));
					mul->opType = STT_OP_Mul;
					mul->SetReturnType(mul->GetLft()->GetReturnType()->IsFloatBased()
						? ast.GetFloat32Type() : ast.GetInt32Type());
					auto* add = new BinaryOpExpr;
					add->AppendChild(mul);
					add->AppendChild(idxe->GetIndex());
					add->opType = STT_OP_Add;
					add->SetReturnType(mul->GetReturnType()->IsFloatBased()
						|| add->GetRgt()->GetReturnType()->IsFloatBased()
						? ast.GetFloat32Type() : ast.GetInt32Type());
					idxe->SetSource(srcidxe->GetSource());
					assert(idxe->childCount == 1);
					idxe->AppendChild(add);
					delete srcidxe;
				}
			}
		}
	}
	AST& ast;
};

static void RemoveArraysOfArrays(AST& ast)
{
	ArrayOfArrayRemover(ast).VisitAST(ast);

	// replace all uses with flattened version
	for (ASTType* arrTy = ast.firstArrayType; arrTy; arrTy = arrTy->nextArrayType)
	{
		if (arrTy->subType->kind == ASTType::Array)
		{
			int finalElemCount = arrTy->elementCount;
			ASTType* finalSubType = arrTy->subType;
			while (finalSubType->kind == ASTType::Array)
			{
				finalElemCount *= finalSubType->elementCount;
				finalSubType = finalSubType->subType;
			}
			while (arrTy->firstUse)
				arrTy->firstUse->ChangeAssocType(ast.GetArrayType(finalSubType, finalElemCount));
		}
	}
}


struct AssignVarDeclNames : ASTWalker<AssignVarDeclNames>
{
	void PreVisit(ASTNode* node)
	{
		if (auto* dre = dyn_cast<const DeclRefExpr>(node))
		{
			if (dre->decl && dre->decl->name.empty())
			{
				char bfr[16];
				sprintf(bfr, "_tmp%d", id++);
				dre->decl->name = bfr;
			}
		}
	}
	int id = 0;
};


Compiler::~Compiler()
{
	if (outVarGenerate && outVarOverflowAlloc)
	{
		if (outVarDidOverflowVar && outVarBuf && outVarBufSize)
			delete [] outVarBuf;
		if (outVarDidOverflowStr && outVarStrBuf && outVarStrBufSize)
			delete [] outVarStrBuf;
	}
}

bool Compiler::CompileFile(const char* name, const char* code)
{
	Diagnostic diag(errorOutputStream, name);
	Parser p(diag, stage, entryPoint, loadIncludeFilePFN, loadIncludeFileUD);

	String codeWithDefines;
	if (defines)
	{
		ShaderMacro* d = defines;
		codeWithDefines += "#line 1 \"<arguments>\"\n";
		while (d->name)
		{
			codeWithDefines += "#define ";
			size_t pos = codeWithDefines.size();
			codeWithDefines += d->name;
			if (const char* eqsp = strchr(d->name, '='))
			{
				codeWithDefines[pos + (eqsp - d->name)] = ' ';
			}
			if (d->value)
			{
				codeWithDefines += " ";
				codeWithDefines += d->value;
			}
			codeWithDefines += "\n";
			d++;
		}
		codeWithDefines += "#line 1 \"";
		codeWithDefines += name;
		codeWithDefines += "\"\n";
		codeWithDefines += code;

		code = codeWithDefines.c_str();
	}
//	FILEStream(stderr) << code;

	if (!p.ParseCode(code))
		return false;
	p.ast.MarkUsed(diag);
	if (diag.hasErrors)
		return false;

	if (ASTDumpStream)
	{
		*ASTDumpStream << "AST before optimization:\n";
		p.ast.Dump(*ASTDumpStream);
		ASTDumpStream->Flush();
	}

	// ignore unused functions entirely
	RemoveUnusedFunctions().RunOnAST(p.ast);

	// validate all
	VariableAccessValidator(diag).RunOnAST(p.ast);
	if (diag.hasErrors)
		return false;
	ContentValidator(diag, outputFmt).RunOnAST(p.ast);
	if (diag.hasErrors)
		return false;

	// output-specific transformations (emulation/feature mapping)
	PadAPI(p.ast, diag, outputFmt);
	UnpackEntryPoint(p.ast, stage, outputFmt);
	if (outputFmt != OSF_HLSL_SM3)
	{
		SplitTexSampleArgs(p.ast, diag, outputFmt);
	}
	switch (outputFmt)
	{
	case OSF_GLSL_140:
	case OSF_GLSL_ES_100:
		UnpackMatrixSwizzle(p.ast);
		RemoveVM1AndM1DTypes(p.ast);
		RemoveArraysOfArrays(p.ast);
		GLSLConvert(p.ast, diag, outputFmt);
		break;
	}

	if (diag.hasErrors)
		return false;

	// optimizations
	ConstantPropagation().RunOnAST(p.ast);
	MarkUnusedVariables().RunOnAST(p.ast);
	RemoveUnusedVariables().RunOnAST(p.ast);

	AssignVarDeclNames().VisitAST(p.ast);

	if (ASTDumpStream)
	{
		*ASTDumpStream << "AST after optimization:\n";
		p.ast.Dump(*ASTDumpStream);
		ASTDumpStream->Flush();
	}

	if (codeOutputStream)
	{
		switch (outputFmt)
		{
		case OSF_HLSL_SM3:
			GenerateHLSL_SM3(p.ast, *codeOutputStream);
			break;
		case OSF_HLSL_SM4:
			GenerateHLSL_SM4(p.ast, *codeOutputStream);
			break;
		case OSF_GLSL_140:
			GenerateGLSL_140(p.ast, *codeOutputStream);
			break;
		case OSF_GLSL_ES_100:
			GenerateGLSL_ES_100(p.ast, *codeOutputStream);
			break;
		}
	}

	if (outVarGenerate)
	{
		size_t bufSizes[2] = { 0, 0 };

		_IterateVariables(p.ast, nullptr, nullptr, bufSizes);

		if (bufSizes[0] > outVarBufSize)
			outVarDidOverflowVar = true;
		if (bufSizes[1] > outVarStrBufSize)
			outVarDidOverflowStr = true;

		if (outVarOverflowAlloc)
		{
			outVarBufSize = bufSizes[0];
			if (outVarDidOverflowVar)
				outVarBuf = new ShaderVariable[bufSizes[0]];
			outVarStrBufSize = bufSizes[1];
			if (outVarDidOverflowStr)
				outVarStrBuf = new char[bufSizes[1]];
		}
		else
		{
			if (outVarBufSize > bufSizes[0])
				outVarBufSize = bufSizes[0];
			if (outVarStrBufSize > bufSizes[1])
				outVarStrBufSize = bufSizes[1];
		}

		_IterateVariables(p.ast, outVarBuf, outVarStrBuf, nullptr);
	}

	return true;
}

static ShaderDataType ASTTypeKindToShaderDataType(ASTType::Kind k)
{
	switch (k)
	{
	case ASTType::Bool:    return SDT_Bool;
	case ASTType::Int32:   return SDT_Int32;
	case ASTType::UInt32:  return SDT_UInt32;
	case ASTType::Float16: return SDT_Float16;
	case ASTType::Float32: return SDT_Float32;
	case ASTType::Sampler1D:      return SDT_Sampler1D;
	case ASTType::Sampler2D:      return SDT_Sampler2D;
	case ASTType::Sampler3D:      return SDT_Sampler3D;
	case ASTType::SamplerCube:    return SDT_SamplerCube;
	case ASTType::Sampler1DCmp:   return SDT_Sampler1DComp;
	case ASTType::Sampler2DCmp:   return SDT_Sampler2DComp;
	case ASTType::SamplerCubeCmp: return SDT_SamplerCubeComp;
	case ASTType::Void:
	case ASTType::Function:
	case ASTType::Vector:
	case ASTType::Matrix:
	case ASTType::Array:
	case ASTType::Structure:
		return SDT_None;
	}
	return SDT_None;
}

void Compiler::_IterateVariables(
	const AST& ast,
	ShaderVariable* outVars,
	char* outStrBuf,
	size_t measureBufSizes[2])
{
	char* outsbp = outStrBuf;

	auto AppendVarDecl = [&](const VarDecl* vd)
	{
		ShaderVarType svt;
		int32_t regSemIdx = -1;
		bool needSemantic = false;
		if ((vd->flags & VarDecl::ATTR_In) && stage == ShaderStage_Vertex)
		{
			svt = SVT_VSInput;
			regSemIdx = vd->semanticIndex >= 0 ? vd->semanticIndex : 0;
			needSemantic = true;
		}
		else if ((vd->flags & VarDecl::ATTR_Out) && stage == ShaderStage_Pixel)
		{
			if (vd->semanticName == "COLOR" || vd->semanticName == "SV_TARGET")
			{
				svt = SVT_PSOutputColor;
				regSemIdx = vd->semanticIndex >= 0 ? vd->semanticIndex : 0;
			}
			else if (vd->semanticName == "DEPTH" || vd->semanticName == "SV_DEPTH")
				svt = SVT_PSOutputDepth;
			else
				return;
		}
		else if (vd->flags & VarDecl::ATTR_Uniform)
		{
			svt = vd->type->IsSampler() ? SVT_Sampler : SVT_Uniform;
			regSemIdx = vd->regID;
		}
		else
			return;

		if (measureBufSizes)
		{
			measureBufSizes[0]++;
			measureBufSizes[1] += vd->name.size() + 1;
			if (needSemantic)
				measureBufSizes[1] += vd->semanticName.size() + 1;
		}
		else
		{
			auto* vmTy = vd->type;
			if (vmTy->kind == ASTType::Array)
				vmTy = vmTy->subType;
			auto* valTy = vmTy;
			if (valTy->kind == ASTType::Vector || valTy->kind == ASTType::Matrix)
				valTy = valTy->subType;

			outVars->name      = outsbp - outStrBuf;
			memcpy(outsbp, vd->name.c_str(), vd->name.size() + 1);
			outsbp += vd->name.size() + 1;
			outVars->semantic  = 0;
			if (needSemantic)
			{
				outVars->semantic = outsbp - outStrBuf;
				memcpy(outsbp, vd->semanticName.c_str(), vd->semanticName.size() + 1);
				outsbp += vd->semanticName.size() + 1;
			}
			outVars->regSemIdx = regSemIdx;
			outVars->arraySize = vd->type->kind == ASTType::Array ? vd->type->elementCount : 0;
			outVars->svType    = svt;
			outVars->dataType  = ASTTypeKindToShaderDataType(valTy->kind);
			outVars->sizeX     = vmTy != valTy ? vmTy->sizeX : 0;
			outVars->sizeY     = vmTy != valTy && vmTy->kind == ASTType::Matrix ? vmTy->sizeY : 0;
			outVars++;
		}
	};

	for (const ASTNode* gv = ast.globalVars.firstChild; gv; gv = gv->next)
	{
		if (auto* cbuf = dyn_cast<const CBufferDecl>(gv))
		{
			// beginning of uniform block
			uint32_t bufNameOff = outsbp - outStrBuf;
			if (measureBufSizes)
			{
				measureBufSizes[0]++;
				measureBufSizes[1] += cbuf->name.size() + 1;
			}
			else
			{
				outVars->name      = bufNameOff;
				memcpy(outsbp, cbuf->name.c_str(), cbuf->name.size() + 1);
				outsbp += cbuf->name.size() + 1;
				outVars->semantic  = 0;
				outVars->regSemIdx = cbuf->bufRegID;
				outVars->arraySize = 0;
				outVars->svType    = SVT_UniformBlockBegin;
				outVars->dataType  = SDT_None;
				outVars->sizeX     = 0;
				outVars->sizeY     = 0;
				outVars++;
			}

			for (const ASTNode* cbv = cbuf->firstChild; cbv; cbv = cbv->next)
			{
				AppendVarDecl(dyn_cast<const VarDecl>(cbv));
			}

			// end of uniform block
			if (measureBufSizes)
			{
				measureBufSizes[0]++;
				// name reused from beginning
			}
			else
			{
				outVars->name      = bufNameOff;
				outVars->semantic  = 0;
				outVars->regSemIdx = cbuf->bufRegID;
				outVars->arraySize = 0;
				outVars->svType    = SVT_UniformBlockEnd;
				outVars->dataType  = SDT_None;
				outVars->sizeX     = 0;
				outVars->sizeY     = 0;
				outVars++;
			}
		}
		else if (auto* vd = dyn_cast<const VarDecl>(gv))
		{
			AppendVarDecl(vd);
		}
	}

	for (const ASTNode* arg = ast.entryPoint->GetFirstArg(); arg; arg = arg->next)
	{
		AppendVarDecl(dyn_cast<const VarDecl>(arg));
	}
}

