

#include "compiler.hpp"


struct SLGenerator
{
	SLGenerator(const AST& a, OutStream& o, OutputShaderFormat osf) : ast(a), out(o), shaderFormat(osf) {}
	virtual void EmitTypeRef(const ASTType* type) = 0;
	virtual void EmitAccessPointTypeAndName(ASTType* type, const std::string& name);
	virtual void EmitAccessPointDecl(const AccessPointDecl& apd);
	virtual void EmitVarDecl(const VarDecl* vd);
	virtual void EmitExpr(const Expr* node);
	virtual void EmitStmt(const Stmt* node, int level);
	void GenerateStructs();
	void GenerateFunctions();

	void LVL(int level)
	{
		while (level-- > 0)
			out << "  ";
	}

	const AST& ast;
	OutStream& out;
	OutputShaderFormat shaderFormat;
	bool supportsStageInOut = true;
	bool supportsSemantics = true;
	bool supportsStatic = true;
	bool supportsPacking = true;
	bool supportsDoubles = true;
	bool supportsScalarSwizzle = true;
	int varDeclNameID = 0;
};


struct HLSLGenerator : SLGenerator
{
	HLSLGenerator(const AST& a, OutStream& o) : SLGenerator(a, o, OSF_HLSL_SM3)
	{
		supportsSemantics = true;
		supportsStatic = true;
		supportsPacking = true;
		supportsDoubles = true;
		supportsScalarSwizzle = true;
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
		supportsDoubles = false;
		supportsScalarSwizzle = false;
	}
	void EmitTypeRef(const ASTType* type);
	void EmitExpr(const Expr* node);
	void Generate();

	int version = 140;
};


void SLGenerator::EmitAccessPointTypeAndName(ASTType* type, const std::string& name)
{
	if (type->kind == ASTType::Array)
	{
		EmitAccessPointTypeAndName(type->subType, name);
		out << "[" << type->elementCount << "]";
	}
	else
	{
		EmitTypeRef(type);
		out << " " << name;
	}
}

void SLGenerator::EmitAccessPointDecl(const AccessPointDecl& apd)
{
	EmitAccessPointTypeAndName(apd.type, apd.name);

	if (supportsSemantics && apd.semanticName.empty() == false)
	{
		out << " : " << apd.semanticName;
		if (apd.semanticIndex >= 0)
			out << apd.semanticIndex;
	}
}

void SLGenerator::EmitVarDecl(const VarDecl* vd)
{
	if (supportsStageInOut || !(vd->flags & VarDecl::ATTR_StageIO))
	{
		if ((vd->flags & (VarDecl::ATTR_In | VarDecl::ATTR_Out)) == (VarDecl::ATTR_In | VarDecl::ATTR_Out))
			out << "inout ";
		else if (vd->flags & VarDecl::ATTR_In)
			out << "in ";
		else if (vd->flags & VarDecl::ATTR_Out)
			out << "out ";
	}
	else if (vd->flags & (VarDecl::ATTR_In | VarDecl::ATTR_Out))
	{
		if (vd->flags & VarDecl::ATTR_In)
		{
			out << (ast.stage == ShaderStage_Vertex ? "attribute " : "varying ");
		}
		else if (vd->flags & VarDecl::ATTR_Out)
		{
			out << (ast.stage == ShaderStage_Vertex ? "varying " : "<UNRESOLVED-PS-OUTPUT> ");
		}
	}
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
		if (dyn_cast<const CBufferDecl>(vd->parent))
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
	if (auto* bexpr = dyn_cast<const BoolExpr>(node))
	{
		out << (bexpr->value ? "true" : "false");
		return;
	}
	else if (auto* i32expr = dyn_cast<const Int32Expr>(node))
	{
		out << i32expr->value;
		return;
	}
	else if (auto* f32expr = dyn_cast<const Float32Expr>(node))
	{
		char bfr[32];
		sprintf(bfr, "%.18g", f32expr->value);
		out << bfr;
		if (strstr(bfr, ".") == nullptr &&
			strstr(bfr, "e") == nullptr &&
			strstr(bfr, "E") == nullptr)
			out << ".0";
		if (supportsDoubles)
			out << "f";
		return;
	}
	else if (auto* idop = dyn_cast<const IncDecOpExpr>(node))
	{
		out << "(";
		const char* opstr = idop->dec ? "--" : "++";
		if (!idop->post) out << opstr;
		EmitExpr(idop->GetSource());
		if (idop->post) out << opstr;
		out << ")";
		return;
	}
	else if (auto* op = dyn_cast<const OpExpr>(node))
	{
		const char* fnstr = "[UNIMPL op]";
		const char* opstr = ",";
		switch (op->opKind)
		{
		case Op_FCall:     fnstr = op->resolvedFunc->mangledName.c_str(); break;
		case Op_Abs:       fnstr = "abs";       break;
		case Op_ACos:      fnstr = "acos";      break;
		case Op_All:       fnstr = "all";       break;
		case Op_Any:       fnstr = "any";       break;
		case Op_ASin:      fnstr = "asin";      break;
		case Op_ATan:      fnstr = "atan";      break;
		case Op_Clamp:     fnstr = "clamp";     break;
		case Op_Cos:       fnstr = "cos";       break;
		case Op_CosH:      fnstr = "cosh";      break;
		case Op_Cross:     fnstr = "cross";     break;
		case Op_Distance:  fnstr = "distance";  break;
		case Op_Dot:       fnstr = "dot";       break;
		case Op_FWidth:    fnstr = "fwidth";    break;
		case Op_Length:    fnstr = "length";    break;
		case Op_Normalize: fnstr = "normalize"; break;
		case Op_Reflect:   fnstr = "reflect";   break;
		case Op_Refract:   fnstr = "refract";   break;
		case Op_Sin:       fnstr = "sin";       break;
		case Op_SinH:      fnstr = "sinh";      break;
		case Op_Tan:       fnstr = "tan";       break;
		case Op_TanH:      fnstr = "tanh";      break;
		}
		out << fnstr << "(";
		for (ASTNode* ch = op->firstChild; ch; ch = ch->next)
		{
			EmitExpr(ch->ToExpr());
			if (ch->next)
				out << opstr;
		}
		out << ")";
		return;
	}
	else if (auto* unop = dyn_cast<const UnaryOpExpr>(node))
	{
		out << "(";
		const char* opstr = "[UNIMPL 1op]";
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
	else if (auto* binop = dyn_cast<const BinaryOpExpr>(node))
	{
		out << "(";
		EmitExpr(binop->GetLft());
		const char* opstr = "[UNIMPL 2op]";
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
	else if (auto* tnop = dyn_cast<const TernaryOpExpr>(node))
	{
		out << "(";
		EmitExpr(tnop->GetCond());
		out << " ? ";
		EmitExpr(tnop->GetTrueExpr());
		out << " : ";
		EmitExpr(tnop->GetFalseExpr());
		out << ")";
		return;
	}
#if 0+TODO
	else if (auto* fce = dyn_cast<const FCallExpr>(node))
	{
		if (fce->resolvedFunc)
		{
			out << fce->resolvedFunc->mangledName;
		}
		else
		{
			EmitExpr(fce->GetFunc());
		}
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
#endif
	else if (auto* mbe = dyn_cast<const MemberExpr>(node))
	{
		if (!supportsScalarSwizzle && mbe->GetSource()->GetReturnType()->IsNumeric())
		{
			if (mbe->swizzleComp > 1) // 1-component swizzles could otherwise generate vec1-like types
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
	else if (auto* ide = dyn_cast<const IndexExpr>(node))
	{
		EmitExpr(ide->GetSource());
		out << "[";
		EmitExpr(ide->GetIndex());
		out << "]";
		return;
	}
	else if (auto* dre = dyn_cast<const DeclRefExpr>(node))
	{
		if (dre->decl)
			out << dre->decl->name;
		else
			out << dre->name;
		return;
	}
	else if (dyn_cast<const VoidExpr>(node))
	{
		out << "/*--*/";
		return;
	}

	out << "[UNIMPL " << node->GetNodeTypeName() << "]";
}

void SLGenerator::EmitStmt(const Stmt* node, int level)
{
	if (auto* blkstmt = dyn_cast<const BlockStmt>(node))
	{
		out << "\n"; LVL(level); out << "{"; level++;
		for (ASTNode* ch = blkstmt->firstChild; ch; ch = ch->next)
		{
			EmitStmt(ch->ToStmt(), level);
			// optimization - do not generate code after first return statement
			if (dyn_cast<const ReturnStmt>(ch->ToStmt()))
				break;
		}
		out << "\n"; level--; LVL(level); out << "}";
		return;
	}
	else if (auto* ifelsestmt = dyn_cast<const IfElseStmt>(node))
	{
		out << "\n"; LVL(level); out << "if(";
		EmitExpr(ifelsestmt->GetCond());
		out << ")";
		EmitStmt(ifelsestmt->GetTrueBr(), level
			+ !dyn_cast<const BlockStmt>(ifelsestmt->GetTrueBr()));
		if (ifelsestmt->GetFalseBr())
		{
			out << "\n"; LVL(level); out << "else";
			EmitStmt(ifelsestmt->GetFalseBr(), level
				+ !dyn_cast<const BlockStmt>(ifelsestmt->GetFalseBr()));
		}
		return;
	}
	else if (auto* whilestmt = dyn_cast<const WhileStmt>(node))
	{
		out << "\n"; LVL(level); out << "while(";
		EmitExpr(whilestmt->GetCond());
		out << ")";
		EmitStmt(whilestmt->GetBody(), level
			+ !dyn_cast<const BlockStmt>(whilestmt->GetBody()));
		return;
	}
	else if (auto* dowhilestmt = dyn_cast<const DoWhileStmt>(node))
	{
		out << "\n"; LVL(level); out << "do";
		EmitStmt(dowhilestmt->GetBody(), level
			+ !dyn_cast<const BlockStmt>(dowhilestmt->GetBody()));
		out << "\n"; LVL(level); out << "while(";
		EmitExpr(dowhilestmt->GetCond());
		out << ");";
		return;
	}
	else if (auto* forstmt = dyn_cast<const ForStmt>(node))
	{
		out << "\n"; LVL(level); out << "for(";
		EmitStmt(forstmt->GetInit(), level + 1);
		EmitExpr(forstmt->GetCond());
		out << "; ";
		EmitExpr(forstmt->GetIncr());
		out << ")";
		EmitStmt(forstmt->GetBody(), level
			+ !dyn_cast<const BlockStmt>(forstmt->GetBody()));
		return;
	}
	else if (auto* exprstmt = dyn_cast<const ExprStmt>(node))
	{
		out << "\n"; LVL(level);
		EmitExpr(exprstmt->GetExpr());
		out << ";";
		return;
	}
	else if (auto* retstmt = dyn_cast<const ReturnStmt>(node))
	{
		out << "\n"; LVL(level);
		out << "return ";
		if (retstmt->GetExpr())
			EmitExpr(retstmt->GetExpr());
		out << ";";
		return;
	}
	else if (dyn_cast<const DiscardStmt>(node))
	{
		out << "\n"; LVL(level);
		out << "discard;";
		return;
	}
	else if (dyn_cast<const BreakStmt>(node))
	{
		out << "\n"; LVL(level);
		out << "break;";
		return;
	}
	else if (dyn_cast<const ContinueStmt>(node))
	{
		out << "\n"; LVL(level);
		out << "continue;";
		return;
	}
	else if (auto* vdstmt = dyn_cast<const VarDeclStmt>(node))
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
	else if (dyn_cast<const EmptyStmt>(node))
	{
		out << ";";
		return;
	}

	out << "\n"; LVL(level);
	out << "[UNIMPL " << node->GetNodeTypeName() << "]";
}

void SLGenerator::GenerateStructs()
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
}

void SLGenerator::GenerateFunctions()
{
	for (const ASTNode* fnn = ast.functionList.firstChild; fnn; fnn = fnn->next)
	{
		const ASTFunction* F = fnn->ToFunction();

		EmitTypeRef(F->GetReturnType());
		out << " " << F->mangledName << "(";
		for (ASTNode* arg = F->GetFirstArg(); arg; arg = arg->next)
		{
			EmitVarDecl(arg->ToVarDecl());
			if (arg->next)
				out << ",";
		}
		out << ")";

		if (supportsSemantics && F->returnSemanticName.empty() == false)
		{
			out << ":" << F->returnSemanticName;
			if (F->returnSemanticIndex >= 0)
				out << F->returnSemanticIndex;
		}
		EmitStmt(F->GetCode(), 0);
		out << "\n";
	}
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
	case ASTType::UInt32:      out << "uint"; break;
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
	if (auto* castexpr = dyn_cast<const CastExpr>(node))
	{
		out << "((";
		EmitTypeRef(castexpr->GetReturnType());
		out << ")";
		EmitExpr(castexpr->GetSource());
		out << ")";
		return;
	}
	else if (auto* ile = dyn_cast<const InitListExpr>(node))
	{
		if (ile->GetReturnType()->kind == ASTType::Array ||
			ile->GetReturnType()->kind == ASTType::Structure)
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
	else if (auto* op = dyn_cast<const OpExpr>(node))
	{
		const char* fnstr = nullptr;
		const char* opstr = ",";
		switch (op->opKind)
		{
		case Op_ATan2:     fnstr = "atan2";     break;
		case Op_DDX:       fnstr = "ddx";       break;
		case Op_DDY:       fnstr = "ddy";       break;
		case Op_Frac:      fnstr = "frac";      break;
		case Op_Lerp:      fnstr = "lerp";      break;
		case Op_RSqrt:     fnstr = "rsqrt";     break;
		case Op_Saturate:  fnstr = "saturate";  break;
		case Op_Tex2D:     fnstr = "tex2D";     break;
		case Op_Tex2DBias: fnstr = "tex2Dbias"; break;
		case Op_Tex2DGrad: fnstr = "tex2Dgrad"; break;
		case Op_Tex2DLOD:  fnstr = "tex2Dlod";  break;
		case Op_Tex2DProj: fnstr = "tex2Dproj"; break;
		}
		if (fnstr)
		{
			out << fnstr << "(";
			for (ASTNode* ch = op->firstChild; ch; ch = ch->next)
			{
				EmitExpr(ch->ToExpr());
				if (ch->next)
					out << opstr;
			}
			out << ")";
			return;
		}
	}

	SLGenerator::EmitExpr(node);
}

void HLSLGenerator::Generate()
{
	SLGenerator::GenerateStructs();

	for (ASTNode* g = ast.globalVars.firstChild; g; g = g->next)
	{
		if (auto* cbuf = dyn_cast<CBufferDecl>(g))
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
			if (g->ToVarDecl()->flags & VarDecl::ATTR_Hidden)
				continue;
			EmitVarDecl(g->ToVarDecl());
			out << ";\n";
		}
	}

	SLGenerator::GenerateFunctions();
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
	case ASTType::UInt32: out << "uint"; break;
	case ASTType::Float16: out << "mediump float"; break;
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
		case ASTType::UInt32: out << "uvec"; break;
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
		case ASTType::UInt32: out << "umat"; break;
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
	if (auto* castexpr = dyn_cast<const CastExpr>(node))
	{
		EmitTypeRef(castexpr->GetReturnType());
		out << "(";
		EmitExpr(castexpr->GetSource());
		out << ")";
		return;
	}
	else if (auto* ile = dyn_cast<const InitListExpr>(node))
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
	else if (auto* op = dyn_cast<const OpExpr>(node))
	{
		const char* fnstr = nullptr;
		const char* opstr = ",";
		switch (op->opKind)
		{
		case Op_ATan2:     fnstr = "atan";  break;
		case Op_DDX:       fnstr = "dFdx";  break;
		case Op_DDY:       fnstr = "dFdy";  break;
		case Op_Frac:      fnstr = "fract"; break;
		case Op_Lerp:      fnstr = "mix";   break;
		case Op_RSqrt:     fnstr = "inversesqrt"; break;
		case Op_Tex2D:     fnstr = version < 140 ? "texture2D"     : "texture";     break;
		case Op_Tex2DBias: fnstr = version < 140 ? "texture2DBias" : "textureBias"; break;
		case Op_Tex2DGrad: fnstr = version < 140 ? "texture2DGrad" : "textureGrad"; break;
		case Op_Tex2DLOD:  fnstr = version < 140 ? "texture2DLod"  : "textureLod";  break;
		case Op_Tex2DProj: fnstr = version < 140 ? "texture2DProj" : "textureProj"; break;
		}
		if (fnstr)
		{
			out << fnstr << "(";
			for (ASTNode* ch = op->firstChild; ch; ch = ch->next)
			{
				EmitExpr(ch->ToExpr());
				if (ch->next)
					out << opstr;
			}
			out << ")";
			return;
		}
	}

	SLGenerator::EmitExpr(node);
}

void GLSLGenerator::Generate()
{
	out << "#version " << version << "\n";
	if (version == 100)
	{
		out << "precision highp float;\n";
		if (ast.usingDerivatives)
		{
			out << "#extension GL_OES_standard_derivatives : enable\n";
		}
	}

	SLGenerator::GenerateStructs();

	for (ASTNode* g = ast.globalVars.firstChild; g; g = g->next)
	{
		if (auto* cbuf = dyn_cast<CBufferDecl>(g))
		{
			if (version >= 140)
			{
				out << "layout(std140) uniform " << cbuf->name;
				out << "\n{\n";
			}
			for (ASTNode* cbv = cbuf->firstChild; cbv; cbv = cbv->next)
			{
				LVL(1);
				EmitVarDecl(cbv->ToVarDecl());
				out << ";\n";
			}
			if (version >= 140)
			{
				out << "};\n";
			}
		}
		else
		{
			auto* gv = g->ToVarDecl();
			if (gv->flags & VarDecl::ATTR_Hidden)
				continue;
			EmitVarDecl(gv);
			out << ";\n";
		}
	}

	SLGenerator::GenerateFunctions();
}


void GenerateHLSL_SM3(const AST& ast, OutStream& out)
{
	HLSLGenerator gen(ast, out);
	gen.Generate();
}

void GenerateHLSL_SM4(const AST& ast, OutStream& out)
{
	HLSLGenerator gen(ast, out);
	gen.shaderFormat = OSF_HLSL_SM4;
	gen.Generate();
}

void GenerateGLSL_140(const AST& ast, OutStream& out)
{
	GLSLGenerator gen(ast, out);
	gen.Generate();
}

void GenerateGLSL_ES_100(const AST& ast, OutStream& out)
{
	GLSLGenerator gen(ast, out);
	gen.shaderFormat = OSF_GLSL_ES_100;
	gen.version = 100;
	gen.supportsStageInOut = false;
	gen.Generate();
}

