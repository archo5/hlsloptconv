

#include "compiler.hpp"


struct SLGenerator
{
	SLGenerator(const AST& a, OutStream& o, OutputShaderFormat osf) : ast(a), out(o), shaderFormat(osf) {}
	virtual void EmitTypeRef(const ASTType* type) = 0;
	virtual void EmitAccessPointDecl(const AccessPointDecl& apd);
	virtual void EmitVarDecl(const VarDecl* vd);
	virtual void EmitExpr(const Expr* node);
	virtual void EmitStmt(const Stmt* node, int level);
	void Generate();

	void LVL(int level)
	{
		while (level-- > 0)
			out << "  ";
	}

	const AST& ast;
	OutStream& out;
	OutputShaderFormat shaderFormat;
	bool supportsSemantics = true;
	bool supportsStatic = true;
	bool supportsPacking = true;
};


struct HLSLGenerator : SLGenerator
{
	HLSLGenerator(const AST& a, OutStream& o) : SLGenerator(a, o, OSF_HLSL_SM3)
	{
		supportsSemantics = true;
		supportsStatic = true;
		supportsPacking = true;
	}
	void EmitTypeRef(const ASTType* type);
	void EmitExpr(const Expr* node);
	void Generate();
};


struct GLSLGenerator : SLGenerator
{
	GLSLGenerator(const AST& a, OutStream& o) : SLGenerator(a, o, OSF_GLSL_140)
	{
		supportsSemantics = false;
		supportsStatic = false;
		supportsPacking = false;
	}
	void EmitTypeRef(const ASTType* type);
	void EmitExpr(const Expr* node);
	void Generate();
};


void SLGenerator::EmitAccessPointDecl(const AccessPointDecl& apd)
{
	if (apd.type->kind == ASTType::Array)
	{
		EmitTypeRef(apd.type->subType);
		out << " " << apd.name << "[" << apd.type->elementCount << "]";
	}
	else
	{
		EmitTypeRef(apd.type);
		out << " " << apd.name;
	}

	if (supportsSemantics && apd.semantic.empty() == false)
		out << " : " << apd.semantic;
}

void SLGenerator::EmitVarDecl(const VarDecl* vd)
{
	if ((vd->flags & (VarDecl::ATTR_In | VarDecl::ATTR_Out)) == (VarDecl::ATTR_In | VarDecl::ATTR_Out))
		out << "inout ";
	else if (vd->flags & VarDecl::ATTR_In)
		out << "in ";
	else if (vd->flags & VarDecl::ATTR_Out)
		out << "out ";
	if (supportsStatic)
	{
		if (vd->flags & VarDecl::ATTR_Static)
			out << "static ";
		if (vd->flags & VarDecl::ATTR_Const)
			out << "const ";
	}
	else
	{
		if ((vd->flags & VarDecl::ATTR_Static) && (vd->flags & VarDecl::ATTR_Const))
			out << "const ";
	}
	if (vd->flags & VarDecl::ATTR_Uniform)
		out << "uniform ";

	EmitAccessPointDecl(*vd);

	if (supportsPacking && vd->regID >= 0)
	{
		out << " : ";
		if (dynamic_cast<const CBufferDecl*>(vd->parent))
		{
			out << "packoffset(c" << (vd->regID / 4);
			if (vd->regID % 4)
				out << "." << "xyzw"[vd->regID % 4];
		}
		else
		{
			out << "register(" << "cs"[vd->GetType()->IsSampler()] << vd->regID;
		}
		out << ")";
	}

	if (vd->GetInitExpr())
	{
		out << " = ";
		EmitExpr(vd->GetInitExpr());
	}
}

void SLGenerator::EmitExpr(const Expr* node)
{
	if (auto* binop = dynamic_cast<const BinaryOpExpr*>(node))
	{
		out << "(";
		EmitExpr(binop->GetLft());
		const char* opstr = "[TODO 2op]";
		switch (binop->opType)
		{
		case STT_OP_Eq: opstr = "=="; break;
		case STT_OP_NEq: opstr = "!="; break;
		case STT_OP_Less: opstr = "<"; break;
		case STT_OP_LEq: opstr = "<="; break;
		case STT_OP_Greater: opstr = ">"; break;
		case STT_OP_GEq: opstr = ">="; break;

		case STT_OP_AddEq: opstr = "+="; break;
		case STT_OP_SubEq: opstr = "-="; break;
		case STT_OP_MulEq: opstr = "*="; break;
		case STT_OP_DivEq: opstr = "/="; break;
		case STT_OP_ModEq: opstr = "%="; break;

		case STT_OP_AndEq: opstr = "&="; break;
		case STT_OP_OrEq: opstr = "|="; break;
		case STT_OP_XorEq: opstr = "^="; break;
		case STT_OP_LshEq: opstr = "<<="; break;
		case STT_OP_RshEq: opstr = ">>="; break;

		case STT_OP_Assign: opstr = "="; break;

		case STT_OP_Add: opstr = "+"; break;
		case STT_OP_Sub: opstr = "-"; break;
		case STT_OP_Mul: opstr = "*"; break;
		case STT_OP_Div: opstr = "/"; break;
		case STT_OP_Mod: opstr = "%"; break;

		case STT_OP_And: opstr = "&"; break;
		case STT_OP_Or: opstr = "|"; break;
		case STT_OP_Xor: opstr = "^"; break;
		case STT_OP_Lsh: opstr = "<<"; break;
		case STT_OP_Rsh: opstr = ">>"; break;

		case STT_OP_LogicalAnd: opstr = "&&"; break;
		case STT_OP_LogicalOr: opstr = "||"; break;
		}
		out << " " << opstr << " ";
		EmitExpr(binop->GetRgt());
		out << ")";
		return;
	}
	else if (auto* unop = dynamic_cast<const UnaryOpExpr*>(node))
	{
		out << "(";
		const char* opstr = "[TODO 1op]";
		switch (unop->opType)
		{
		case STT_OP_Sub: opstr = "-"; break;
		case STT_OP_Not: opstr = "!"; break;
		case STT_OP_Inv: opstr = "~"; break;
		}
		out << opstr;
		EmitExpr(unop->GetSource());
		out << ")";
		return;
	}
	else if (auto* bexpr = dynamic_cast<const BoolExpr*>(node))
	{
		out << (bexpr->value ? "true" : "false");
		return;
	}
	else if (auto* i32expr = dynamic_cast<const Int32Expr*>(node))
	{
		out << i32expr->value;
		return;
	}
	else if (auto* f32expr = dynamic_cast<const Float32Expr*>(node))
	{
		char bfr[32];
		sprintf(bfr, "%.6g", f32expr->value);
		out << bfr;
		if (strstr(bfr, ".") == nullptr)
			out << '.';
		out << 'f';
		return;
	}
	else if (auto* ide = dynamic_cast<const IndexExpr*>(node))
	{
		EmitExpr(ide->GetSource());
		out << "[";
		EmitExpr(ide->GetIndex());
		out << "]";
		return;
	}

	out << "[TODO " << typeid(*node).name() << "]";
}

void SLGenerator::EmitStmt(const Stmt* node, int level)
{
	if (auto* blkstmt = dynamic_cast<const BlockStmt*>(node))
	{
		out << "\n"; LVL(level); out << "{"; level++;
		for (ASTNode* ch = blkstmt->firstChild; ch; ch = ch->next)
		{
			EmitStmt(ch->ToStmt(), level);
			// optimization - do not generate code after first return statement
			if (dynamic_cast<const ReturnStmt*>(ch->ToStmt()))
				break;
		}
		out << "\n"; level--; LVL(level); out << "}";
		return;
	}
	else if (auto* ifelsestmt = dynamic_cast<const IfElseStmt*>(node))
	{
		out << "\n"; LVL(level); out << "if(";
		EmitExpr(ifelsestmt->GetCond());
		out << ")";
		EmitStmt(ifelsestmt->GetTrueBr(), level
			+ !dynamic_cast<const BlockStmt*>(ifelsestmt->GetTrueBr()));
		if (ifelsestmt->GetFalseBr())
		{
			out << "\n"; LVL(level); out << "else";
			EmitStmt(ifelsestmt->GetFalseBr(), level
				+ !dynamic_cast<const BlockStmt*>(ifelsestmt->GetFalseBr()));
		}
		return;
	}
	else if (auto* whilestmt = dynamic_cast<const WhileStmt*>(node))
	{
		out << "\n"; LVL(level); out << "while(";
		EmitExpr(whilestmt->GetCond());
		out << ")";
		EmitStmt(whilestmt->GetBody(), level
			+ !dynamic_cast<const BlockStmt*>(whilestmt->GetBody()));
		return;
	}
	else if (auto* dowhilestmt = dynamic_cast<const DoWhileStmt*>(node))
	{
		out << "\n"; LVL(level); out << "do";
		EmitStmt(dowhilestmt->GetBody(), level
			+ !dynamic_cast<const BlockStmt*>(dowhilestmt->GetBody()));
		out << "\n"; LVL(level); out << "while(";
		EmitExpr(dowhilestmt->GetCond());
		out << ");";
		return;
	}
	else if (auto* forstmt = dynamic_cast<const ForStmt*>(node))
	{
		out << "\n"; LVL(level); out << "for(";
		EmitStmt(forstmt->GetInit(), level + 1);
		EmitExpr(forstmt->GetCond());
		out << "; ";
		EmitExpr(forstmt->GetIncr());
		out << ")";
		EmitStmt(forstmt->GetBody(), level
			+ !dynamic_cast<const BlockStmt*>(forstmt->GetBody()));
		return;
	}
	else if (auto* exprstmt = dynamic_cast<const ExprStmt*>(node))
	{
		out << "\n"; LVL(level);
		EmitExpr(exprstmt->GetExpr());
		out << ";";
		return;
	}
	else if (auto* retstmt = dynamic_cast<const ReturnStmt*>(node))
	{
		out << "\n"; LVL(level);
		out << "return ";
		if (retstmt->GetExpr())
			EmitExpr(retstmt->GetExpr());
		out << ";";
		return;
	}
	else if (dynamic_cast<const DiscardStmt*>(node))
	{
		out << "\n"; LVL(level);
		out << "discard;";
		return;
	}
	else if (dynamic_cast<const BreakStmt*>(node))
	{
		out << "\n"; LVL(level);
		out << "break;";
		return;
	}
	else if (dynamic_cast<const ContinueStmt*>(node))
	{
		out << "\n"; LVL(level);
		out << "continue;";
		return;
	}
	else if (auto* vdstmt = dynamic_cast<const VarDeclStmt*>(node))
	{
		for (ASTNode* ch = vdstmt->firstChild; ch; ch = ch->next)
		{
			VarDecl* vd = ch->ToVarDecl();
			out << "\n"; LVL(level);
			EmitVarDecl(vd);
			out << ";";
		}
		return;
	}

	out << "\n"; LVL(level);
	out << "[TODO " << typeid(*node).name() << "]";
}


//////////////
//// HLSL ////
//////////////

void HLSLGenerator::EmitTypeRef(const ASTType* type)
{
	switch (type->kind)
	{
	case ASTType::Void:        out << "void"; break;
	case ASTType::Bool:        out << "bool"; break;
	case ASTType::Int32:       out << "int"; break;
	case ASTType::Float16:     out << "half"; break;
	case ASTType::Float32:     out << "float"; break;
	case ASTType::Sampler1D:   out << "sampler1D"; break;
	case ASTType::Sampler2D:   out << "sampler2D"; break;
	case ASTType::Sampler3D:   out << "sampler3D"; break;
	case ASTType::SamplerCUBE: out << "samplerCUBE"; break;
	case ASTType::Structure:
		out << type->ToStructType()->name;
		break;
	case ASTType::Vector:
		EmitTypeRef(type->subType);
		out << int(type->sizeX);
		break;
	case ASTType::Matrix:
		EmitTypeRef(type->subType);
		out << int(type->sizeX) << "x" << int(type->sizeY);
		break;
	case ASTType::Array:
		EmitTypeRef(type->subType);
		out << "[" << type->elementCount << "]";
		break;
	}
}

void HLSLGenerator::EmitExpr(const Expr* node)
{
	if (auto* castexpr = dynamic_cast<const CastExpr*>(node))
	{
		out << "((";
		EmitTypeRef(castexpr->GetReturnType());
		out << ")";
		EmitExpr(castexpr->GetSource());
		out << ")";
		return;
	}
	else if (auto* fce = dynamic_cast<const FCallExpr*>(node))
	{
		EmitExpr(fce->GetFunc());
		out << "(";
		for (ASTNode* arg = fce->GetFirstArg(); arg; arg = arg->next)
		{
			EmitExpr(arg->ToExpr());
			if (arg->next)
				out << ", ";
		}
		out << ")";
		return;
	}
	else if (auto* ile = dynamic_cast<const InitListExpr*>(node))
	{
		if (ile->GetReturnType()->kind == ASTType::Array)
		{
			out << "{";
			for (ASTNode* ch = ile->firstChild; ch; ch = ch->next)
			{
				if (ch->prev)
					out << ", ";
				EmitExpr(ch->ToExpr());
			}
			out << "}";
		}
		else
		{
			EmitTypeRef(ile->GetReturnType());
			out << "(";
			for (ASTNode* ch = ile->firstChild; ch; ch = ch->next)
			{
				if (ch->prev)
					out << ", ";
				EmitExpr(ch->ToExpr());
			}
			out << ")";
		}
		return;
	}
	else if (auto* mbe = dynamic_cast<const MemberExpr*>(node))
	{
		EmitExpr(mbe->GetSource());
		out << "." << mbe->memberName;
		return;
	}
	else if (auto* dre = dynamic_cast<const DeclRefExpr*>(node))
	{
		out << dre->name;
		return;
	}

	SLGenerator::EmitExpr(node);
}

void HLSLGenerator::Generate()
{
	// structs
	for (const ASTStructType* stc = ast.firstStructType; stc; stc = stc->nextStructType)
	{
		out << "struct " << stc->name << "\n{";
		for (const auto& m : stc->members)
		{
			out << "\n";
			LVL(1);
			EmitAccessPointDecl(m);
			out << ";";
		}
		out << "\n};\n";
	}
	for (ASTNode* g = ast.globalVars.firstChild; g; g = g->next)
	{
		if (auto* cbuf = dynamic_cast<CBufferDecl*>(g))
		{
			out << "cbuffer " << cbuf->name;
			if (cbuf->bufRegID >= 0)
			{
				out << " : register(b" << cbuf->bufRegID << ")";
			}
			out << "\n{\n";
			for (ASTNode* cbv = cbuf->firstChild; cbv; cbv = cbv->next)
			{
				LVL(1);
				EmitVarDecl(cbv->ToVarDecl());
				out << ";\n";
			}
			out << "}\n";
		}
		else
		{
			EmitVarDecl(g->ToVarDecl());
			out << ";\n";
		}
	}
	for (const auto& fgdef : ast.functions)
	{
		for (const auto& fdef : fgdef.second)
		{
			ASTFunction* F = fdef.second;
			EmitTypeRef(F->GetReturnType());
			out << " " << F->name << "(";
			for (ASTNode* arg = F->GetFirstArg(); arg; arg = arg->next)
			{
				EmitVarDecl(arg->ToVarDecl());
				if (arg->next)
					out << ",";
			}
			out << ")";
			if (F->returnSemantic.empty() == false)
				out << ":" << F->returnSemantic;
			EmitStmt(F->GetCode(), 0);
			out << "\n";
		}
	}
}


//////////////
//// GLSL ////
//////////////

void GLSLGenerator::EmitTypeRef(const ASTType* type)
{
	switch (type->kind)
	{
	case ASTType::Void: out << "void"; break;
	case ASTType::Bool: out << "bool"; break;
	case ASTType::Int32: out << "int"; break;
	case ASTType::Float16: out << "half"; break;
	case ASTType::Float32: out << "float"; break;
	case ASTType::Sampler1D: out << "sampler1D"; break;
	case ASTType::Sampler2D: out << "sampler2D"; break;
	case ASTType::Sampler3D: out << "sampler3D"; break;
	case ASTType::SamplerCUBE: out << "samplerCUBE"; break;
	case ASTType::Structure:
		out << type->ToStructType()->name;
		break;
	case ASTType::Vector:
		switch (type->subType->kind)
		{
		case ASTType::Bool: out << "bvec"; break;
		case ASTType::Int32: out << "ivec"; break;
		case ASTType::Float16: out << "mediump vec"; break;
		case ASTType::Float32: out << "vec"; break;
		}
		out << int(type->sizeX);
		break;
	case ASTType::Matrix:
		switch (type->subType->kind)
		{
		case ASTType::Bool: out << "bmat"; break;
		case ASTType::Int32: out << "imat"; break;
		case ASTType::Float16: out << "mediump mat"; break;
		case ASTType::Float32: out << "mat"; break;
		}
		out << int(type->sizeX);
		if (type->sizeX != type->sizeY)
			out << "x" << int(type->sizeY);
		break;
	case ASTType::Array:
		EmitTypeRef(type->subType);
		out << "[" << type->elementCount << "]";
		break;
	}
}

void GLSLGenerator::EmitExpr(const Expr* node)
{
	if (auto* binop = dynamic_cast<const BinaryOpExpr*>(node))
	{
		const char* func = "[TODO glsl op func]";
		switch (binop->opType)
		{
		case STT_OP_Mul:
			if (binop->GetReturnType()->kind == ASTType::Matrix)
			{
				func = "matrixCompMult";
				goto emitBinOp;
			}
			break;
		case STT_OP_Mod:
			func = "mod";
			goto emitBinOp;
		default:
			break;
		emitBinOp:
			out << "(";
			EmitExpr(binop->GetLft());
			out << ", ";
			EmitExpr(binop->GetRgt());
			out << ")";
			return;
		}
		// if not one of exceptions, use default output
		// TODO implement hlsl2glsl general pass
	}
	else if (auto* castexpr = dynamic_cast<const CastExpr*>(node))
	{
		EmitTypeRef(castexpr->GetReturnType());
		out << "(";
		EmitExpr(castexpr->GetSource());
		out << ")";
		return;
	}
	else if (auto* fce = dynamic_cast<const FCallExpr*>(node))
	{
		if (fce->isBuiltinFunc)
		{
			if (auto* name = dynamic_cast<const DeclRefExpr*>(fce->GetFunc()))
			{
				if (name->name == "mul")
				{
					out << "(";
					EmitExpr(fce->GetFirstArg()->ToExpr());
					out << " * ";
					EmitExpr(fce->GetFirstArg()->next->ToExpr());
					out << ")";
					return;
				}
			}
		}

		EmitExpr(fce->GetFunc());
		out << "(";
		for (ASTNode* arg = fce->GetFirstArg(); arg; arg = arg->next)
		{
			EmitExpr(arg->ToExpr());
			if (arg->next)
				out << ", ";
		}
		out << ")";
		return;
	}
	else if (auto* ile = dynamic_cast<const InitListExpr*>(node))
	{
		EmitTypeRef(ile->GetReturnType());
		out << "(";
		for (ASTNode* ch = ile->firstChild; ch; ch = ch->next)
		{
			if (ch->prev)
				out << ", ";
			EmitExpr(ch->ToExpr());
		}
		out << ")";
		return;
	}
	else if (auto* mbe = dynamic_cast<const MemberExpr*>(node))
	{
		if (mbe->GetSource()->GetReturnType()->IsNumeric())
		{
			EmitTypeRef(ast.GetVectorType(mbe->GetSource()->GetReturnType(), mbe->memberName.size()));
			out << "(";
			EmitExpr(mbe->GetSource());
			out << ")";
		}
		else
		{
			EmitExpr(mbe->GetSource());
			out << "." << mbe->memberName;
		}
		return;
	}
	else if (auto* dre = dynamic_cast<const DeclRefExpr*>(node))
	{
		if (dre->decl)
		{
			if ((dre->decl->flags & VarDecl::ATTR_Out) &&
				dre->decl->semantic.compare(0, 8, "POSITION") == 0)
				out << "gl_Position";
			else
				out << dre->decl->name;//semantic;
		}
		else
			out << dre->name;
		return;
	}

	SLGenerator::EmitExpr(node);
}

void GLSLGenerator::Generate()
{
	out << "#version 140\n";

	// structs
	for (const ASTStructType* stc = ast.firstStructType; stc; stc = stc->nextStructType)
	{
		out << "struct " << stc->name << "\n{";
		for (const auto& m : stc->members)
		{
			out << "\n";
			LVL(1);
			EmitAccessPointDecl(m);
			out << ";";
		}
		out << "\n};\n";
	}
	for (ASTNode* g = ast.globalVars.firstChild; g; g = g->next)
	{
		if (auto* cbuf = dynamic_cast<CBufferDecl*>(g))
		{
			out << "layout(std140) uniform " << cbuf->name;
			out << "\n{\n";
			for (ASTNode* cbv = cbuf->firstChild; cbv; cbv = cbv->next)
			{
				LVL(1);
				EmitVarDecl(cbv->ToVarDecl());
				out << ";\n";
			}
			out << "};\n";
		}
		else
		{
			auto* gv = g->ToVarDecl();
			if ((gv->flags & VarDecl::ATTR_Out) &&
				gv->semantic.compare(0, 8, "POSITION") == 0)
				continue;
			EmitVarDecl(gv);
			out << ";\n";
		}
	}
	for (const auto& fgdef : ast.functions)
	{
		for (const auto& fdef : fgdef.second)
		{
			ASTFunction* F = fdef.second;

			EmitTypeRef(F->GetReturnType());
			out << " " << F->name << "(";
			for (ASTNode* arg = F->GetFirstArg(); arg; arg = arg->next)
			{
				EmitVarDecl(arg->ToVarDecl());
				if (arg->next)
					out << ",";
			}
			out << ")";

			EmitStmt(F->GetCode(), 0);
			out << "\n";
		}
	}
}


void GenerateHLSL_SM3(const AST& ast, OutStream& out)
{
	HLSLGenerator gen(ast, out);
	gen.Generate();
}

void GenerateGLSL_140(const AST& ast, OutStream& out)
{
	GLSLGenerator gen(ast, out);
	gen.Generate();
}

