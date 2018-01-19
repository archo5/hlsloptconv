

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
	case Float16:
	case Float32: return 1;
	case Vector: return sizeX;
	case Matrix: return sizeX * sizeY;
	case Array: return subType->GetAccessPointCount() * elementCount;
	case Structure:
	{
		int out = 0;
		for (const auto& m : static_cast<const ASTStructType*>(this)->members)
			out += m.type->GetAccessPointCount();
		return out;
	}
	default: return 0;
	}
}

ASTType::SubTypeCount ASTType::CountSubTypes() const
{
	switch (kind)
	{
	case Bool:
	case Int32:
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

ASTType::Kind ASTType::GetNVM1Kind() const
{
	// only guaranteed to return correct values for Numeric/VM1 types
	switch (kind)
	{
	case Vector:
	case Matrix: return subType->kind;
	default: return kind;
	}
}

void ASTType::GetMangling(std::string& out) const
{
	char bfr[32];
	switch (kind)
	{
	case Void:    out += "v"; break;
	case Bool:    out += "b"; break;
	case Int32:   out += "i"; break;
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
	case SamplerCUBE: out += "sc"; break;
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
	case SamplerCUBE: out << "samplerCUBE"; break;
	}
}

std::string ASTType::GetName() const
{
	switch (kind)
	{
	case Void:        return "void";
	case Bool:        return "bool";
	case Int32:       return "int";
	case Float16:     return "half";
	case Float32:     return "float";
	case Vector:      return subType->GetName() + gVecNumbers[sizeX - 1];
	case Matrix:      return subType->GetName() + gMtxNumbers[sizeX - 1 + (sizeY - 1) * 4];
	case Array:       return subType->GetName() + "[" + std::to_string(elementCount) + "]";
	case Structure:   return "struct(" + static_cast<const ASTStructType*>(this)->name + ")";
	case Function:    return "function";
	case Sampler1D:   return "sampler1D";
	case Sampler2D:   return "sampler2D";
	case Sampler3D:   return "sampler3D";
	case SamplerCUBE: return "samplerCUBE";
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

std::string TokenTypeToString(SLTokenType tt)
{
	const char* str;
	switch (tt)
	{
	case STT_NULL:            str = "<NULL>"; break;
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
	return dynamic_cast<Expr*>(this);
}

Stmt* ASTNode::ToStmt()
{
	return dynamic_cast<Stmt*>(this);
}

VarDecl* ASTNode::ToVarDecl()
{
	return dynamic_cast<VarDecl*>(this);
}

ASTFunction* ASTNode::ToFunction()
{
	return dynamic_cast<ASTFunction*>(this);
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


void VoidExpr::Dump(OutStream& out, int) const
{
	out << "VOID(parse error)\n";
}


VarDecl::VarDecl(const VarDecl& o) : ASTNode(o)
{
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

void VarDecl::GetMangling(std::string& out) const
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
		if (dynamic_cast<const CBufferDecl*>(parent))
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
	out << "declref(" << name << ") [";
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

void FCallExpr::Dump(OutStream& out, int level) const
{
	out << "fcall [";
	GetReturnType()->Dump(out);
	out << "]";
	if (isBuiltinFunc)
		out << " builtin";
	out << "\n";
	LVL(out, level); out << "{\n"; level++;
	LVL(out, level); out << "func = ";
	GetFunc()->Dump(out, level + 1);
	int i = 0;
	for (ASTNode* arg = GetFirstArg(); arg; arg = arg->next)
	{
		LVL(out, level);
		out << "arg[" << i++ << "] = ";
		arg->Dump(out, level);
	}
	level--; LVL(out, level); out << "}\n";
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

void MemberExpr::Dump(OutStream& out, int level) const
{
	GetSource()->Dump(out, level);
	LVL(out, level + 1);
	out << "." << memberName << " [";
	GetReturnType()->Dump(out);
	out << "]" << "\n";
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
	typeSamplerCUBEDef (ASTType::SamplerCUBE),
	typeBoolDef        (ASTType::Bool       ),
	typeInt32Def       (ASTType::Int32      ),
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
	typeNameMap["void"]        = GetVoidType();
	typeNameMap["function"]    = GetFunctionType();
	typeNameMap["sampler1D"]   = GetSampler1DType();
	typeNameMap["sampler2D"]   = GetSampler2DType();
	typeNameMap["sampler3D"]   = GetSampler3DType();
	typeNameMap["samplerCUBE"] = GetSamplerCUBEType();

	const char* typeNames[4]  = { "bool", "int", "half", "float" };
	ASTType* baseTypes[4]     = { GetBoolType(), GetInt32Type(), GetFloat16Type(), GetFloat32Type() };
	ASTType* vecTypeArrays[4] = { typeBoolVecDefs, typeInt32VecDefs, typeFloat16VecDefs, typeFloat32VecDefs };
	ASTType* mtxTypeArrays[4] = { typeBoolMtxDefs, typeInt32MtxDefs, typeFloat16MtxDefs, typeFloat32MtxDefs };
	char bfr[32];
	for (int t = 0; t < 4; ++t)
	{
		ASTType* baseType = baseTypes[t];
		typeNameMap[typeNames[t]] = baseType;
		for (int x = 1; x <= 4; ++x)
		{
			sprintf(bfr, "%s%d", typeNames[t], x);
			ASTType* vt = &vecTypeArrays[t][x - 1];
			vt->kind = ASTType::Vector;
			vt->subType = baseType;
			vt->sizeX = x;
			typeNameMap[bfr] = vt;
			for (int y = 1; y <= 4; ++y)
			{
				sprintf(bfr, "%s%dx%d", typeNames[t], x, y);
				ASTType* mt = &mtxTypeArrays[t][(x - 1) + (y - 1) * 4];
				mt->kind = ASTType::Matrix;
				mt->subType = baseType;
				mt->sizeX = x;
				mt->sizeY = y;
				typeNameMap[bfr] = mt;
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
	case ASTType::Int32:   return GetFloat32Type();
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
	case ASTType::Float16:
	case ASTType::Float32: return t;
	case ASTType::Vector:
	case ASTType::Matrix:  return t->subType;
	default: return nullptr;
	}
}

ASTType* TypeSystem::CastToVector(ASTType* t, int size)
{
	if (size < 1 || size > 4)
		return nullptr;
	switch (t->kind)
	{
	case ASTType::Bool:    return &typeBoolVecDefs   [size - 1];
	case ASTType::Int32:   return &typeInt32VecDefs  [size - 1];
	case ASTType::Float32: return &typeFloat32VecDefs[size - 1];
	case ASTType::Float16: return &typeFloat16VecDefs[size - 1];
	case ASTType::Vector:  return t;
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
	case ASTType::Float32: return &typeFloat32VecDefs[size - 1];
	case ASTType::Float16: return &typeFloat16VecDefs[size - 1];
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

ASTStructType* TypeSystem::CreateStructType(const std::string& name)
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

	typeNameMap[name] = stc;

	return stc;
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
		if (auto* fcall = dynamic_cast<FCallExpr*>(node))
		{
			if (fcall->isBuiltinFunc == false && fcall->resolvedFunc->used == false)
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

void AST::MarkUsed(Diagnostic& diag, const std::string& entryPoint)
{
	// clear flag
	for (const auto& fns : functions)
	{
		for (const auto& fn : fns.second)
		{
			fn.second->used = false;
		}
	}

	auto fns = functions.find(entryPoint);
	if (fns == functions.end())
	{
		diag.EmitFatalError("entry point '" + entryPoint + "' was not found",
			Location::BAD());
	}
	if (fns->second.size() != 1)
	{
		diag.EmitFatalError("too many functions named '" + entryPoint + "', expected one entry point",
			Location::BAD());
	}

	// demangle entry point name since it's unique
	fns->second.begin()->second->mangledName = entryPoint;
	fns->second.begin()->second->isEntryPoint = true;

	UsedFuncMarker ufm(fns->second.begin()->second);
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
	for (const auto& fgdef : functions)
	{
		for (const auto& fdef : fgdef.second)
		{
			fdef.second->Dump(out);
		}
	}
}



void VariableAccessValidator::RunOnAST(const AST& ast)
{
	// functions
	for (auto& fnsp : ast.functions)
	{
		for (auto& fnp : fnsp.second)
		{
			auto* fn = fnp.second;

			ValidateSetupFunc(fn);

			curASTFunction = fn;

			if (ProcessStmt(fn->GetCode()) == false)
			{
				if (fn->GetReturnType()->IsVoid())
				{
					ValidateCheckOutputElementsWritten();
				}
				else
				{
					// TODO location
					diag.EmitError("'" + fn->name +
						"' - not all control paths return a value",
						Location::BAD());
				}
			}
		}
	}

	diag.CheckNonFatalErrors();
}

void VariableAccessValidator::ProcessReadExpr(const Expr* node)
{
	if (auto* binop = dynamic_cast<const BinaryOpExpr*>(node))
	{
		if (binop->opType == STT_OP_Assign)
		{
			ProcessReadExpr(binop->GetRgt());
			ProcessWriteExpr(binop->GetLft());
			ProcessReadExpr(binop->GetLft());
		}
		else
		{
			ProcessReadExpr(binop->GetLft());
			ProcessReadExpr(binop->GetRgt());
		}
		return;
	}
	else if (auto* unop = dynamic_cast<const UnaryOpExpr*>(node))
	{
		ProcessReadExpr(unop->GetSource());
		return;
	}
	else if (auto* bexpr = dynamic_cast<const BoolExpr*>(node))
	{
		return;
	}
	else if (auto* i32expr = dynamic_cast<const Int32Expr*>(node))
	{
		return;
	}
	else if (auto* f32expr = dynamic_cast<const Float32Expr*>(node))
	{
		return;
	}
	else if (auto* castexpr = dynamic_cast<const CastExpr*>(node))
	{
		ProcessReadExpr(castexpr->GetSource());
		return;
	}
	else if (auto* fce = dynamic_cast<const FCallExpr*>(node))
	{
		if (auto* name = dynamic_cast<const DeclRefExpr*>(fce->GetFunc()))
		{
			if (fce->isBuiltinFunc)
			{
#if 0
				IntrinsicType ity = FindIntrinsicByName(name->name.c_str());
				// TODO location
				if (ity == IT__COUNT)
					diag.EmitFatalError("(internal) intrinsic not found: " + name->name, Location::BAD());
				if (GetIntrinsicArgumentCount(ity) != fce->GetArgCount())
					diag.EmitFatalError("(internal) incorrect number of arguments for intrinsic "
						+ name->name, Location::BAD());
#endif
				// TODO write-out intrinsics
				for (ASTNode* arg = fce->GetFirstArg(); arg; arg = arg->next)
					ProcessReadExpr(arg->ToExpr());
				return;
			}
			else
			{
				auto* rf = fce->resolvedFunc;

				for (ASTNode *arg = fce->GetFirstArg(), *argdecl = rf->GetFirstArg();
					arg && argdecl;
					arg = arg->next, argdecl = argdecl->next)
				{
					if (argdecl->ToVarDecl()->flags & VarDecl::ATTR_In)
						ProcessReadExpr(arg->ToExpr());
				}

				for (ASTNode *arg = fce->GetFirstArg(), *argdecl = rf->GetFirstArg();
					arg && argdecl;
					arg = arg->next, argdecl = argdecl->next)
				{
					if (argdecl->ToVarDecl()->flags & VarDecl::ATTR_Out)
						ProcessWriteExpr(arg->ToExpr());
				}
				return;
			}
		}
	}
	else if (auto* ile = dynamic_cast<const InitListExpr*>(node))
	{
		for (ASTNode* ch = ile->firstChild; ch; ch = ch->next)
		{
			ProcessReadExpr(ch->ToExpr());
		}
		return;
	}
	else if (auto* sve = dynamic_cast<const SubValExpr*>(node))
	{
		std::vector<const SubValExpr*> revTrail { sve };
		Expr* exprIt = sve->GetSource();
		while (auto* ssve = dynamic_cast<const SubValExpr*>(exprIt))
		{
			revTrail.push_back(ssve);
			exprIt = ssve->GetSource();
		}

		for (auto* sve : revTrail)
			if (auto* idxe = dynamic_cast<const IndexExpr*>(sve))
				ProcessReadExpr(idxe->GetIndex());

		if (auto* dre = dynamic_cast<const DeclRefExpr*>(exprIt))
		{
			// check if variable part is written
			int rf = dre->decl->APRangeFrom;
			int rt = dre->decl->APRangeTo;
			if (rf < rt)
			{
				uint32_t swizzle = 0;
				int swzSize = 0;
				bool isMatrixSwizzle = false;

				if (revTrail.empty() == false)
				{
					const Expr* sizeExpr = dre;
					for (size_t i = revTrail.size(); i > 0; )
					{
						--i;
						auto* sve = revTrail[i];
						int32_t index = -1;
						bool isMmbSwizzle = false;
						if (auto* mmb = dynamic_cast<const MemberExpr*>(sve))
						{
							if (mmb->swizzleComp == 0)
								index = mmb->memberID;
							else
							{
								isMmbSwizzle = true;
								swizzle = mmb->memberID;
								swzSize = mmb->GetReturnType()->GetElementCount();
								isMatrixSwizzle = mmb->GetSource()->GetReturnType()->kind == ASTType::Matrix;
							}
						}
						else if (auto* idx = dynamic_cast<const IndexExpr*>(sve))
						{
							if (auto* ci = dynamic_cast<const Int32Expr*>(idx->GetIndex()))
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
					int mult = isMatrixSwizzle ? 4 : 2;
					uint32_t mask = isMatrixSwizzle ? 0xf : 0x3;
					for (int i = 0; i < swzSize; ++i)
					{
						if (elementsWritten[rf + ((swizzle >> (i * mult)) & mask)] == 0)
						{
							ValidateCheckVariableError(dre->decl->name);
						}
					}
				}
				else
				{
					for (int i = rf; i < rt; ++i)
					{
						if (elementsWritten[i] == 0)
						{
							ValidateCheckVariableError(dre->decl->name);
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
	else if (auto* dre = dynamic_cast<const DeclRefExpr*>(node))
	{
		ValidateCheckVariableInitialized(dre->decl->APRangeFrom, dre->decl->APRangeTo, dre->decl->name);
		return;
	}

	diag.EmitFatalError(std::string("UNHANDLED READ EXPR: ") + typeid(*node).name(), Location::BAD());
}

void VariableAccessValidator::ProcessWriteExpr(const Expr* node)
{
	if (auto* sve = dynamic_cast<const SubValExpr*>(node))
	{
		std::vector<const SubValExpr*> revTrail { sve };
		Expr* exprIt = sve->GetSource();
		while (auto* ssve = dynamic_cast<const SubValExpr*>(exprIt))
		{
			revTrail.push_back(ssve);
			exprIt = ssve->GetSource();
		}

		if (auto* dre = dynamic_cast<const DeclRefExpr*>(exprIt))
		{
			// mark variable part as written
			int rf = dre->decl->APRangeFrom;
			int rt = dre->decl->APRangeTo;
			if (rf < rt)
			{
				uint32_t swizzle = 0;
				int swzSize = 0;
				bool isMatrixSwizzle = false;

				if (revTrail.empty() == false)
				{
					const Expr* sizeExpr = dre;
					for (size_t i = revTrail.size(); i > 0; )
					{
						--i;
						auto* sve = revTrail[i];
						int32_t index = -1;
						bool isMmbSwizzle = false;
						if (auto* mmb = dynamic_cast<const MemberExpr*>(sve))
						{
							if (mmb->swizzleComp == 0)
								index = mmb->memberID;
							else
							{
								isMmbSwizzle = true;
								swizzle = mmb->memberID;
								swzSize = mmb->GetReturnType()->GetElementCount();
								isMatrixSwizzle = mmb->GetSource()->GetReturnType()->kind == ASTType::Matrix;
							}
						}
						else if (auto* idx = dynamic_cast<const IndexExpr*>(sve))
						{
							if (auto* ci = dynamic_cast<const Int32Expr*>(idx->GetIndex()))
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
						{
							// TODO location
							diag.EmitError("cannot write to local array from a computed index", Location::BAD());
						}
					}
					rt = rf + sizeExpr->GetReturnType()->GetAccessPointCount();
				}

				if (swzSize)
				{
					int mult = isMatrixSwizzle ? 4 : 2;
					uint32_t mask = isMatrixSwizzle ? 0xf : 0x3;
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
			// TODO location
			diag.EmitFatalError("cannot write to temporary value", Location::BAD());
		}
	}
	else if (auto* dre = dynamic_cast<const DeclRefExpr*>(node))
	{
		// mark variable as written
		for (int i = dre->decl->APRangeFrom; i < dre->decl->APRangeTo; ++i)
			elementsWritten[i] = true;
		return;
	}

	// TODO node location
	diag.EmitFatalError("cannot write to read-only expression", Location::BAD());
}

bool VariableAccessValidator::ProcessStmt(const Stmt* node)
{
	if (auto* blkstmt = dynamic_cast<const BlockStmt*>(node))
	{
		for (ASTNode* ch = blkstmt->firstChild; ch; ch = ch->next)
		{
			// optimization - do not process code after first fully returning statement
			if (ProcessStmt(ch->ToStmt()))
				return true;
		}
		return false;
	}
	else if (auto* ifelsestmt = dynamic_cast<const IfElseStmt*>(node))
	{
		ProcessReadExpr(ifelsestmt->GetCond());
		bool rif = ProcessStmt(ifelsestmt->GetTrueBr());
		bool relse = ifelsestmt->GetFalseBr() && ProcessStmt(ifelsestmt->GetFalseBr());
		return rif && relse;
	}
	else if (auto* whilestmt = dynamic_cast<const WhileStmt*>(node))
	{
		ProcessReadExpr(whilestmt->GetCond());
		return ProcessStmt(whilestmt->GetBody());
	}
	else if (auto* dowhilestmt = dynamic_cast<const DoWhileStmt*>(node))
	{
		bool rb = ProcessStmt(dowhilestmt->GetBody());
		ProcessReadExpr(dowhilestmt->GetCond());
		return rb;
	}
	else if (auto* forstmt = dynamic_cast<const ForStmt*>(node))
	{
		ProcessStmt(forstmt->GetInit()); // cannot return
		ProcessReadExpr(forstmt->GetCond());
		bool rb = ProcessStmt(forstmt->GetBody());
		ProcessReadExpr(forstmt->GetIncr());
		return rb;
	}
	else if (auto* exprstmt = dynamic_cast<const ExprStmt*>(node))
	{
		// easy special cases for root that don't need the value
		if (auto* binop = dynamic_cast<const BinaryOpExpr*>(exprstmt->GetExpr()))
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
	else if (auto* retstmt = dynamic_cast<const ReturnStmt*>(node))
	{
		if (curASTFunction->GetReturnType()->IsVoid() == false)
		{
			ProcessReadExpr(retstmt->GetExpr());
		}

		ValidateCheckOutputElementsWritten();
		return true;
	}
	else if (auto* dscstmt = dynamic_cast<const DiscardStmt*>(node))
	{
		// still need to return something somewhere, otherwise shader is somewhat pointless
		return false;
	}
	else if (auto* brkstmt = dynamic_cast<const BreakStmt*>(node))
	{
		return false;
	}
	else if (auto* cntstmt = dynamic_cast<const ContinueStmt*>(node))
	{
		return false;
	}
	else if (auto* vdstmt = dynamic_cast<const VarDeclStmt*>(node))
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

	// TODO node location
	diag.EmitFatalError("(internal) statement "
		+ std::string(typeid(*node).name()) + " not processed", Location::BAD());
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

void VariableAccessValidator::ValidateCheckOutputElementsWritten()
{
	for (int i = 0; i < endOfOutputElements; ++i)
	{
		if (elementsWritten[i] == 0)
		{
			// TODO location
			// TODO detailed info
			diag.EmitError("not all outputs have been assigned before 'return'", Location::BAD());
			break;
		}
	}
}

void VariableAccessValidator::ValidateCheckVariableInitialized(int from, int to, const std::string& varname)
{
	for (int i = from; i < to; ++i)
	{
		if (elementsWritten[i] == 0)
		{
			ValidateCheckVariableError(varname);
		}
	}
}

void VariableAccessValidator::ValidateCheckVariableError(const std::string& varname)
{
	// TODO location
	diag.EmitError("variable '" + varname + "' used before sufficient initialization", Location::BAD());
}



static void GLSLRenameInOut(VarDecl* vd, ShaderStage stage)
{
	if (vd->flags & VarDecl::ATTR_Out)
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
				char bfr[32];
				sprintf(bfr, "_PSCOLOR%d", vd->semanticIndex >= 0 ? vd->semanticIndex : 0);
				vd->name = bfr;
				return;
			}
		}
	}

	if (vd->name.empty())
	{
		vd->name = vd->semanticName;
	}
}

struct StructLevel
{
	ASTStructType* strTy;
	uint32_t mmbID;
	ASTNode* levILE;
};
static void GLSLAppendShaderIOVar(AST& ast, ASTFunction* F, ShaderStage stage,
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


		VarDecl* vd = ast.CreateGlobalVar();
		vd->name = inSRC->ToVarDecl()->name;
		for (auto& mmbidx : mmbIndices)
		{
			vd->name += "_";
			vd->name += mmbidx.strTy->members[mmbidx.mmbID].name;
		}
		vd->SetType(mmb.type);
		vd->flags = outILE ? VarDecl::ATTR_In : VarDecl::ATTR_Out;
		vd->semanticName = mmb.semanticName;
		vd->semanticIndex = mmb.semanticIndex;
		GLSLRenameInOut(vd, stage);

		if (outILE)
		{
			auto* dre = new DeclRefExpr;
			dre->decl = vd;
			dre->name = vd->name;
			dre->SetReturnType(vd->GetType());
			mmbIndices.back().levILE->AppendChild(dre);
		}
		else
		{
			for (ReturnStmt* ret = F->firstRetStmt; ret; ret = ret->nextRetStmt)
			{
				if (dynamic_cast<BlockStmt*>(ret->parent) == nullptr)
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
				dreout->name = vd->name;
				dreout->SetReturnType(vd->GetType());
				drestruct->decl = inSRC->ToVarDecl();
				drestruct->name = drestruct->decl->name;
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
					mmbexpr->memberName = mmbinfo.name;
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

static void GLSLUnpackEntryPoint(AST& ast, ShaderStage stage)
{
	for (const auto& fgdef : ast.functions)
	{
		for (const auto& fdef : fgdef.second)
		{
			ASTFunction* F = fdef.second;
			if (F->isEntryPoint)
			{
				// extract return value
				if (F->GetReturnType()->IsVoid() == false)
				{
					VarDecl* vd = ast.CreateGlobalVar();
					vd->semanticName = F->returnSemanticName;
					vd->semanticIndex = F->returnSemanticIndex;
					vd->SetType(F->GetReturnType());
					vd->flags |= VarDecl::ATTR_Out;
					GLSLRenameInOut(vd, stage);

					F->SetReturnType(ast.GetVoidType());

					for (ReturnStmt* ret = F->firstRetStmt; ret; ret = ret->nextRetStmt)
					{
						if (dynamic_cast<BlockStmt*>(ret->parent) == nullptr)
						{
							BlockStmt* blk = new BlockStmt;
							ret->ReplaceWith(blk);
							blk->AppendChild(ret);
						}

						auto* exprst = new ExprStmt;
						auto* assign = new BinaryOpExpr;
						auto* dre = new DeclRefExpr;
						dre->name = vd->name;
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
						argvd->Unlink();
						GLSLRenameInOut(argvd, stage);
						ast.globalVars.AppendChild(argvd);
					}
					else
					{
						uint32_t flags = argvd->flags;
						argvd->flags &= ~(VarDecl::ATTR_In | VarDecl::ATTR_Out);

						auto* vds = dynamic_cast<VarDeclStmt*>(F->GetCode()->firstChild);
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

							GLSLAppendShaderIOVar(ast, F, stage, ile, argvd, argvd->GetType()->ToStructType());
						}
						else
							GLSLAppendShaderIOVar(ast, F, stage, nullptr, argvd, argvd->GetType()->ToStructType());

						vds->PrependChild(argvd);
					}
				}

				break;
			}
		}
	}
}


// loop expressions are tough to fold out so it is avoided
// pre-condition loop gets transformed into an infinite loop with 'if(!cond)break;' at the beginning
static bool IsExprInPreCondLoop(Expr* expr)
{
	ASTNode* n = expr;
	while (n->parent->ToStmt() == nullptr)
		n = n->parent;
	if (auto* whilestmt = dynamic_cast<WhileStmt*>(n->parent))
	{
		if (whilestmt->GetCond() == n)
			return true;
	}
	else if (auto* forstmt = dynamic_cast<ForStmt*>(n->parent))
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
	if (auto* ifelsestmt = dynamic_cast<IfElseStmt*>(n->parent))
	{
		if (ifelsestmt->GetCond() == n)
			return ifelsestmt;
	}
	else if (auto* whilestmt = dynamic_cast<WhileStmt*>(n->parent))
	{
		if (whilestmt->GetCond() == n)
			return whilestmt;
	}
	else if (auto* dowhilestmt = dynamic_cast<DoWhileStmt*>(n->parent))
	{
		if (dowhilestmt->GetCond() == n)
			return dowhilestmt;
	}
	else if (auto* forstmt = dynamic_cast<ForStmt*>(n->parent))
	{
		if (forstmt->GetCond() == n)
			return forstmt;
	}
	return nullptr;
}

static int DistanceToSingleDeclRefExpr(Expr* expr)
{
	int dist = 0;
	while (auto* mmbexpr = dynamic_cast<MemberExpr*>(expr))
	{
		expr = mmbexpr->GetSource();
		dist++;
	}
	if (dynamic_cast<DeclRefExpr*>(expr))
		return dist;
	else
		return -1;
}

static void ExpandBlock(Stmt* stmt)
{
	if (dynamic_cast<BlockStmt*>(stmt->parent) == nullptr)
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
	if (auto* whilestmt = dynamic_cast<WhileStmt*>(feStmt))
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
	else if (auto* dowhilestmt = dynamic_cast<DoWhileStmt*>(feStmt))
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
		if (auto* binop = dynamic_cast<BinaryOpExpr*>(expr->parent))
		{
			if (TokenIsOpAssign(binop->opType) && binop->GetLft() == expr)
				return binop;
		}
		else if (auto* fcall = dynamic_cast<FCallExpr*>(expr->parent))
		{
			// TODO
		}
		expr = expr->parent->ToExpr();
	}
	return nullptr;
}

struct MatrixSwizzleUnpacker : ASTWalker<MatrixSwizzleUnpacker>
{
	MatrixSwizzleUnpacker(AST& a) : ast(a) {}
	void PostVisit(ASTNode* node)
	{
		if (auto* mmbexpr = dynamic_cast<MemberExpr*>(node))
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
						if (auto* binop = dynamic_cast<BinaryOpExpr*>(writeCtx)) // assignment
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
							assert(dynamic_cast<ExprStmt*>(insertPos)
								|| dynamic_cast<VarDeclStmt*>(insertPos));
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
						else if (auto* fcall = dynamic_cast<FCallExpr*>(writeCtx)) // 'out' argument
						{
							// TODO
						}
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


static Expr* GetReferenceToElement(AST& ast, Expr* src, int accessPointNum)
{
	ASTType* t = src->GetReturnType();
	assert(accessPointNum >= 0 && accessPointNum < t->GetAccessPointCount());
	switch (t->kind)
	{
	case ASTType::Bool:
	case ASTType::Int32:
	case ASTType::Float16:
	case ASTType::Float32:
	case ASTType::Sampler1D:
	case ASTType::Sampler2D:
	case ASTType::Sampler3D:
	case ASTType::SamplerCUBE:
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
					mmbexpr->memberName = strTy->members[i].name;
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
	}
}


struct GLSLConversionPass : ASTWalker<GLSLConversionPass>
{
	GLSLConversionPass(AST& a) : ast(a){}
	// TODO merge with hlslparser's version
	static void CastExprTo(Expr* val, ASTType* to)
	{
		assert(to);
		if (to != val->GetReturnType())
		{
			CastExpr* cast = new CastExpr;
			cast->SetReturnType(to);
			val->ReplaceWith(cast);
			cast->SetSource(val);
		}
	}
	void MatrixUnpack1(FCallExpr* fcintrin)
	{
		auto* mtxTy = fcintrin->GetReturnType();
		if (mtxTy->kind != ASTType::Matrix)
			return;

		FoldOutIfBest(fcintrin->GetFirstArg()->ToExpr());
		int numCols = mtxTy->sizeX;
		auto* ile = new InitListExpr;
		auto* idxe = new IndexExpr;
		idxe->SetReturnType(ast.GetVectorType(mtxTy->subType, mtxTy->sizeY));
		idxe->SetSource(fcintrin->GetFirstArg()->ToExpr());
		idxe->AppendChild(new Int32Expr(0, ast.GetInt32Type()));
		ile->SetReturnType(mtxTy);
		fcintrin->AppendChild(idxe);
		fcintrin->ReplaceWith(ile);
		ile->AppendChild(fcintrin);
		for(int i = 1; i < numCols; ++i)
		{
			auto* fcx = dynamic_cast<FCallExpr*>(fcintrin->DeepClone());
			dynamic_cast<Int32Expr*>(dynamic_cast<IndexExpr*>(fcx->GetFirstArg())->GetIndex())->value = i;
			ile->AppendChild(fcx);
		}
	}
	void PostVisit(ASTNode* node)
	{
		if (auto* fcall = dynamic_cast<FCallExpr*>(node))
		{
			if (fcall->isBuiltinFunc)
			{
				if (auto* dre = dynamic_cast<DeclRefExpr*>(fcall->GetFunc()))
				{
					if (dre->name == "abs")
					{
						MatrixUnpack1(fcall);
						return;
					}
					if (dre->name == "tex2D")
					{
						dre->name = "texture";
						return;
					}
					if (dre->name == "lerp")
					{
						dre->name = "mix";
						return;
					}
					if (dre->name == "clip")
					{
						// clip return type is 'void' so parent should be ExprStmt
						assert(fcall->parent->ToStmt());

						// convert to `if (<x> < 0) discard;` for each access point
						Expr* arg = fcall->GetFirstArg()->ToExpr();

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
							case ASTType::Int32: cnst = new Int32Expr(0, srcTy); break;
							case ASTType::Float16:
							case ASTType::Float32: cnst = new Float32Expr(0, srcTy); break;
							}
							binop->AppendChild(cnst);
						}

						delete fcall; // leaves unused ExprStmt
						return;
					}
				}
			}
		}
		if (auto* exprStmt = dynamic_cast<ExprStmt*>(node))
		{
			if (!exprStmt->GetExpr())
			{
				delete exprStmt; // cleanup
				return;
			}
		}
	}
	AST& ast;
};

static void GLSLConvert(AST& ast)
{
	GLSLConversionPass(ast).VisitAST(ast);
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
		if (auto* idxe = dynamic_cast<IndexExpr*>(node))
		{
			if (auto* srcidxe = dynamic_cast<IndexExpr*>(idxe->GetSource()))
			{
				if (idxe->GetSource()->GetReturnType()->kind == ASTType::Array &&
					srcidxe->GetSource()->GetReturnType()->kind == ASTType::Array)
				{
					// idxe { srcidxe { <source>, <idxin> }, <idxout> }
					//  ->
					// idxe { <source>, <idxin> * srcidxe.rettype.elementCount + <idxout> }
					auto* mul = new BinaryOpExpr;
					mul->AppendChild(srcidxe->GetIndex());
					mul->AppendChild(new Int32Expr(srcidxe->GetReturnType()->elementCount, ast.GetInt32Type()));
					mul->opType = STT_OP_Mul;
					mul->SetReturnType(ast.GetInt32Type());
					auto* add = new BinaryOpExpr;
					add->AppendChild(mul);
					add->AppendChild(idxe->GetIndex());
					add->opType = STT_OP_Add;
					add->SetReturnType(ast.GetInt32Type());
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


struct InitListExprRepacker : ASTWalker<InitListExprRepacker>
{
	InitListExprRepacker(AST& a) : ast(a){}
	void PostVisit(ASTNode* node)
	{
		if (auto* ile = dynamic_cast<InitListExpr*>(node))
		{
			if (ile->GetReturnType()->kind == ASTType::Structure)
			{
				int numAPs = ile->GetReturnType()->GetAccessPointCount();
				auto* strTy = ile->GetReturnType()->ToStructType();
				int origInputs = ile->childCount;

				DeclRefExpr* src = FoldOut(ile);
				VarDecl* vd = src->decl;
				ASTNode* insertPos = src->decl->parent;
				assert(dynamic_cast<VarDeclStmt*>(insertPos));
				insertPos = insertPos->next;

				Expr* sourceNode = ile->firstChild->ToExpr();
				int sourceOffset = 0;
				for (int i = 0; i < numAPs; ++i)
				{
					for (;;)
					{
						int srcapc = sourceNode->GetReturnType()->GetAccessPointCount();
						if (i - sourceOffset < srcapc)
							break;
						sourceOffset += srcapc;
						sourceNode = sourceNode->next->ToExpr();
					}
					auto* exprStmt = new ExprStmt;
					auto* binop = new BinaryOpExpr;
					auto* dstdre = new DeclRefExpr;
					dstdre->decl = vd;
					dstdre->SetReturnType(vd->GetType());
					binop->opType = STT_OP_Assign;
					binop->AppendChild(GetReferenceToElement(ast, dstdre, i));
					binop->AppendChild(GetReferenceToElement(ast, sourceNode->DeepClone()->ToExpr(), i - sourceOffset));
					binop->SetReturnType(binop->GetLft()->GetReturnType());
					exprStmt->AppendChild(binop);
					insertPos->InsertBeforeMe(exprStmt);
				}

				delete ile; // now replaced with explicit component assignments
			}
		}
	}
	AST& ast;
};

static void RepackInitListExprs(AST& ast)
{
	InitListExprRepacker(ast).VisitAST(ast);
}


struct AssignVarDeclNames : ASTWalker<AssignVarDeclNames>
{
	void PreVisit(ASTNode* node)
	{
		if (auto* dre = dynamic_cast<const DeclRefExpr*>(node))
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


bool Compiler::CompileFile(const char* name, const char* code)
{
	Diagnostic diag(errorOutputStream, name);
	try
	{
		Parser p(diag, stage, loadIncludeFilePFN, loadIncludeFileUD);
		p.ParseCode(code);
		p.ast.MarkUsed(diag, entryPoint);

		if (ASTDumpStream)
		{
			*ASTDumpStream << "AST before optimization:\n";
			p.ast.Dump(*ASTDumpStream);
			ASTDumpStream->Flush();
		}

		// ignore unused functions entirely
		RemoveUnusedFunctions ruf;
		ruf.RunOnAST(p.ast);

		// validate all
		VariableAccessValidator vav(diag);
		vav.RunOnAST(p.ast);

		// output-specific transformations (emulation/feature mapping)
		if (outputFmt == OSF_GLSL_140)
		{
			UnpackMatrixSwizzle(p.ast);
			RemoveVM1AndM1DTypes(p.ast);
			RepackInitListExprs(p.ast);
			RemoveArraysOfArrays(p.ast);
			GLSLUnpackEntryPoint(p.ast, stage);
			GLSLConvert(p.ast);
		}

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
			case OSF_GLSL_140:
				GenerateGLSL_140(p.ast, *codeOutputStream);
				break;
			}
		}
		return true;
	}
	catch (FatalError err)
	{
		diag.PrintError(err.msg, err.loc);
	}
	catch (NonFatalErrors)
	{
		// do nothing, errors have already been printed
	}
	return false;
}

