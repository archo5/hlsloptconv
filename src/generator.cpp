

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
	bool IsGE4(){ return shaderFormat == OSF_HLSL_SM4; }
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
		case Op_Add:       fnstr = ""; opstr = "+"; break;
		case Op_Subtract:  fnstr = ""; opstr = "-"; break;
		case Op_Multiply:  fnstr = ""; opstr = "*"; break;
		case Op_Divide:    fnstr = ""; opstr = "/"; break;
		case Op_Modulus:   fnstr = ""; opstr = "%"; break;
		case Op_Abs:       fnstr = "abs";       break;
		case Op_ACos:      fnstr = "acos";      break;
		case Op_All:       fnstr = "all";       break;
		case Op_Any:       fnstr = "any";       break;
		case Op_ASin:      fnstr = "asin";      break;
		case Op_ATan:      fnstr = "atan";      break;
		case Op_Ceil:      fnstr = "ceil";      break;
		case Op_Clamp:     fnstr = "clamp";     break;
		case Op_Cos:       fnstr = "cos";       break;
		case Op_CosH:      fnstr = "cosh";      break;
		case Op_Cross:     fnstr = "cross";     break;
		case Op_Degrees:   fnstr = "degrees";   break;
		case Op_Distance:  fnstr = "distance";  break;
		case Op_Dot:       fnstr = "dot";       break;
		case Op_Exp:       fnstr = "exp";       break;
		case Op_Exp2:      fnstr = "exp2";      break;
		case Op_Floor:     fnstr = "floor";     break;
		case Op_FWidth:    fnstr = "fwidth";    break;
		case Op_Length:    fnstr = "length";    break;
		case Op_Log:       fnstr = "log";       break;
		case Op_Log10:     fnstr = "log10";     break;
		case Op_Log2:      fnstr = "log2";      break;
		case Op_Max:       fnstr = "max";       break;
		case Op_Min:       fnstr = "min";       break;
		case Op_Normalize: fnstr = "normalize"; break;
		case Op_Pow:       fnstr = "pow";       break;
		case Op_Radians:   fnstr = "radians";   break;
		case Op_Reflect:   fnstr = "reflect";   break;
		case Op_Refract:   fnstr = "refract";   break;
		case Op_Round:     fnstr = "round";     break;
		case Op_Sign:      fnstr = "sign";      break;
		case Op_Sin:       fnstr = "sin";       break;
		case Op_SinH:      fnstr = "sinh";      break;
		case Op_SmoothStep:fnstr = "smoothstep";break;
		case Op_Sqrt:      fnstr = "sqrt";      break;
		case Op_Step:      fnstr = "step";      break;
		case Op_Tan:       fnstr = "tan";       break;
		case Op_TanH:      fnstr = "tanh";      break;
		case Op_Trunc:     fnstr = "trunc";     break;
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
	case ASTType::Sampler1D:   out << (IsGE4() ? "SAMPLER_1D" : "sampler1D"); break;
	case ASTType::Sampler2D:   out << (IsGE4() ? "SAMPLER_2D" : "sampler2D"); break;
	case ASTType::Sampler3D:   out << (IsGE4() ? "SAMPLER_3D" : "sampler3D"); break;
	case ASTType::SamplerCube: out << (IsGE4() ? "SAMPLER_CUBE" : "samplerCUBE"); break;
	case ASTType::Sampler1DCmp:   out << "SAMPLER_1D_CMP"; break;
	case ASTType::Sampler2DCmp:   out << "SAMPLER_2D_CMP"; break;
	case ASTType::SamplerCubeCmp: out << "SAMPLER_CUBE_CMP"; break;
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
	else if (auto* dre = dyn_cast<const DeclRefExpr>(node))
	{
		if (IsGE4() &&
			dre->decl &&
			(dre->decl->flags & VarDecl::ATTR_Global) &&
			dre->decl->GetType()->IsSampler())
		{
			out << "GET_" << dre->decl->name << "()";
			return;
		}
	}
	else if (auto* op = dyn_cast<const OpExpr>(node))
	{
		if (IsGE4())
		{
			enum Type
			{
				Normal,
				Bias,
				Grad,
				Level,
				Cmp,
				CmpLevelZero,
			};
			Type type;
			switch (op->opKind)
			{
			case Op_Tex1D:       type = Normal;   goto texSample;
			case Op_Tex1DBias:   type = Bias;     goto texSample;
			case Op_Tex1DGrad:   type = Grad;     goto texSample;
			case Op_Tex1DLOD:    type = Level;    goto texSample;
			case Op_Tex1DProj:   assert( false ); return;
			case Op_Tex2D:       type = Normal;   goto texSample;
			case Op_Tex2DBias:   type = Bias;     goto texSample;
			case Op_Tex2DGrad:   type = Grad;     goto texSample;
			case Op_Tex2DLOD:    type = Level;    goto texSample;
			case Op_Tex2DProj:   assert( false ); return;
			case Op_Tex3D:       type = Normal;   goto texSample;
			case Op_Tex3DBias:   type = Bias;     goto texSample;
			case Op_Tex3DGrad:   type = Grad;     goto texSample;
			case Op_Tex3DLOD:    type = Level;    goto texSample;
			case Op_Tex3DProj:   assert( false ); return;
			case Op_TexCube:     type = Normal;   goto texSample;
			case Op_TexCubeBias: type = Bias;     goto texSample;
			case Op_TexCubeGrad: type = Grad;     goto texSample;
			case Op_TexCubeLOD:  type = Level;    goto texSample;
			case Op_TexCubeProj: assert( false ); return;

			case Op_Tex1DCmp:
			case Op_Tex2DCmp:
			case Op_TexCubeCmp: type = Cmp; goto texSample;
			case Op_Tex1DLOD0Cmp:
			case Op_Tex2DLOD0Cmp:
			case Op_TexCubeLOD0Cmp: type = CmpLevelZero; goto texSample;
			default: break;
			texSample:
				EmitExpr(op->firstChild->ToExpr());
				out << ".tex.";
				switch (type)
				{
				case Normal: out << "Sample";      break;
				case Bias:   out << "SampleBias";  break;
				case Grad:   out << "SampleGrad";  break;
				case Level:  out << "SampleLevel"; break;
				case Cmp:    out << "SampleCmp";   break;
				case CmpLevelZero: out << "SampleCmpLevelZero"; break;
				}
				out << "(";
				EmitExpr(op->firstChild->ToExpr());
				out << ".smp";
				for (ASTNode* ch = op->firstChild->next; ch; ch = ch->next)
				{
					out << ",";
					EmitExpr(ch->ToExpr());
				}
				out << ")";
				return;
			}
		}
		const char* fnstr = nullptr;
		const char* opstr = ",";
		switch (op->opKind)
		{
		case Op_ATan2:     fnstr = "atan2";     break;
		case Op_Clip:      fnstr = "clip";      break;
		case Op_Determinant: fnstr = "determinant"; break;
		case Op_DDX:       fnstr = "ddx";       break;
		case Op_DDY:       fnstr = "ddy";       break;
		case Op_FMod:      fnstr = "fmod";      break;
		case Op_Frac:      fnstr = "frac";      break;
		case Op_LdExp:     fnstr = "ldexp";     break;
		case Op_Lerp:      fnstr = "lerp";      break;
		case Op_MulMM:
		case Op_MulMV:
		case Op_MulVM:     fnstr = "mul";       break;
		case Op_RSqrt:     fnstr = "rsqrt";     break;
		case Op_Saturate:  fnstr = "saturate";  break;
		case Op_Tex1D:     fnstr = "tex1D";     break;
		case Op_Tex1DBias: fnstr = "tex1Dbias"; break;
		case Op_Tex1DGrad: fnstr = "tex1Dgrad"; break;
		case Op_Tex1DLOD:  fnstr = "tex1Dlod";  break;
		case Op_Tex1DProj: fnstr = "tex1Dproj"; break;
		case Op_Tex2D:     fnstr = "tex2D";     break;
		case Op_Tex2DBias: fnstr = "tex2Dbias"; break;
		case Op_Tex2DGrad: fnstr = "tex2Dgrad"; break;
		case Op_Tex2DLOD:  fnstr = "tex2Dlod";  break;
		case Op_Tex2DProj: fnstr = "tex2Dproj"; break;
		case Op_Tex3D:     fnstr = "tex3D";     break;
		case Op_Tex3DBias: fnstr = "tex3Dbias"; break;
		case Op_Tex3DGrad: fnstr = "tex3Dgrad"; break;
		case Op_Tex3DLOD:  fnstr = "tex3Dlod";  break;
		case Op_Tex3DProj: fnstr = "tex3Dproj"; break;
		case Op_TexCube:     fnstr = "texCUBE";     break;
		case Op_TexCubeBias: fnstr = "texCUBEbias"; break;
		case Op_TexCubeGrad: fnstr = "texCUBEgrad"; break;
		case Op_TexCubeLOD:  fnstr = "texCUBElod";  break;
		case Op_TexCubeProj: fnstr = "texCUBEproj"; break;
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
	if (shaderFormat == OSF_HLSL_SM4)
	{
		out <<
			"struct SAMPLER_1D { Texture1D tex; SamplerState smp; };\n"
			"struct SAMPLER_2D { Texture2D tex; SamplerState smp; };\n"
			"struct SAMPLER_3D { Texture3D tex; SamplerState smp; };\n"
			"struct SAMPLER_Cube { TextureCube tex; SamplerState smp; };\n"
			"struct SAMPLER_1D_CMP { Texture1D tex; SamplerComparisonState smp; };\n"
			"struct SAMPLER_2D_CMP { Texture2D tex; SamplerComparisonState smp; };\n"
			"struct SAMPLER_Cube_CMP { TextureCube tex; SamplerComparisonState smp; };\n"
		;
	}

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
			auto* vd = g->ToVarDecl();
			if (vd->flags & VarDecl::ATTR_Hidden)
				continue;
			if (IsGE4() && vd->GetType()->IsSampler())
			{
				const char* sfx = nullptr;
				bool smpComp = false;
				switch (vd->GetType()->kind)
				{
				case ASTType::Sampler1D:   sfx = "1D";   break;
				case ASTType::Sampler2D:   sfx = "2D";   break;
				case ASTType::Sampler3D:   sfx = "3D";   break;
				case ASTType::SamplerCube: sfx = "Cube"; break;
				case ASTType::Sampler1DCmp:   sfx = "1D";   smpComp = true; break;
				case ASTType::Sampler2DCmp:   sfx = "2D";   smpComp = true; break;
				case ASTType::SamplerCubeCmp: sfx = "Cube"; smpComp = true; break;
				}
				const char* smpSfx = smpComp ? "_CMP" : "";
				out << "Texture" << sfx << " TEX_" << vd->name;
				if (vd->regID >= 0)
					out << " : register(t" << vd->regID << ")";
				out << ";\n";

				out << (smpComp ? "SamplerComparisonState" : "SamplerState");
				out << " SMP_" << vd->name;
				if (vd->regID >= 0)
					out << " : register(s" << vd->regID << ")";
				out << ";\n";

				out << "SAMPLER_" << sfx << smpSfx << " GET_" << vd->name
					<< "(){ SAMPLER_" << sfx << smpSfx << " v = { TEX_" << vd->name
					<< ", SMP_" << vd->name << " }; return v; }\n";
			}
			else
			{
				EmitVarDecl(vd);
				out << ";\n";
			}
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
	case ASTType::SamplerCube: out << "samplerCube"; break;
	case ASTType::Sampler1DCmp: out << "sampler1DShadow"; break;
	case ASTType::Sampler2DCmp: out << "sampler2DShadow"; break;
	case ASTType::SamplerCubeCmp: out << "samplerCubeShadow"; break;
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
		case ASTType::Bool: out << "mat"; break;
		case ASTType::Int32: out << "mat"; break;
		case ASTType::UInt32: out << "mat"; break;
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
		const char* fnEnd = ")";
		switch (op->opKind)
		{
		case Op_Multiply:
			if (op->GetLft()->GetReturnType()->kind == ASTType::Matrix &&
				op->GetRgt()->GetReturnType()->kind == ASTType::Matrix)
				fnstr = "matrixCompMult";
			break;
		case Op_Modulus:
			if (op->GetLft()->GetReturnType()->IsFloatBased() ||
				op->GetRgt()->GetReturnType()->IsFloatBased())
				fnstr = "mod";
			break;
		case Op_ATan2:     fnstr = "atan";  break;
		case Op_Clip:      assert( false ); break;
		case Op_DDX:       fnstr = "dFdx";  break;
		case Op_DDY:       fnstr = "dFdy";  break;
		case Op_FMod:      assert( false ); break;
		case Op_Frac:      fnstr = "fract"; break;
		case Op_LdExp:
			out << "(";
			EmitExpr(op->GetFirstArg()->ToExpr());
			out << "*exp2(";
			EmitExpr(op->GetFirstArg()->next->ToExpr());
			out << "))";
			return;
		case Op_Lerp:      fnstr = "mix";   break;
		case Op_ModGLSL:   fnstr = "mod";   break;
		case Op_MulMM:
		case Op_MulMV:
		case Op_MulVM:     fnstr = ""; opstr = "*"; break;
		case Op_Round:     assert( version >= 130 ); break;
		case Op_RSqrt:     fnstr = "inversesqrt"; break;
		case Op_Saturate:
			out << "clamp(";
			EmitExpr(op->GetFirstArg()->ToExpr());
			out << ",0.0,1.0)";
			return;
		case Op_Tex1D:     fnstr = version < 140 ? "UNAVAILABLE"      : "texture";     break;
		case Op_Tex1DBias: fnstr = version < 140 ? "UNAVAILABLE"      : "texture";     break;
		case Op_Tex1DGrad: fnstr = version < 140 ? "UNAVAILABLE"      : "textureGrad"; break;
		case Op_Tex1DLOD:  fnstr = version < 140 ? "UNAVAILABLE"      : "textureLod";  break;
		case Op_Tex1DProj: fnstr = version < 140 ? "UNAVAILABLE"      : "textureProj"; break;
		case Op_Tex2D:     fnstr = version < 140 ? "texture2D"        : "texture";     break;
		case Op_Tex2DBias: fnstr = version < 140 ? "texture2D"        : "texture";     break;
		case Op_Tex2DGrad: fnstr = version < 140 ? "texture2DGradEXT" : "textureGrad"; break;
		case Op_Tex2DLOD:  fnstr = version < 140 ?
			(ast.stage == ShaderStage_Pixel ? "texture2DLodEXT" : "texture2DLod") :
			"textureLod";  break;
		case Op_Tex2DProj: fnstr = version < 140 ? "texture2DProj"    : "textureProj"; break;
		case Op_Tex3D:     fnstr = version < 140 ? "UNAVAILABLE"      : "texture";     break;
		case Op_Tex3DBias: fnstr = version < 140 ? "UNAVAILABLE"      : "texture";     break;
		case Op_Tex3DGrad: fnstr = version < 140 ? "UNAVAILABLE"      : "textureGrad"; break;
		case Op_Tex3DLOD:  fnstr = version < 140 ? "UNAVAILABLE"      : "textureLod";  break;
		case Op_Tex3DProj: fnstr = version < 140 ? "UNAVAILABLE"      : "textureProj"; break;
		case Op_TexCube:     fnstr = version < 140 ? "textureCube"        : "texture";     break;
		case Op_TexCubeBias: fnstr = version < 140 ? "textureCube"        : "texture";     break;
		case Op_TexCubeGrad: fnstr = version < 140 ? "textureCubeGradEXT" : "textureGrad"; break;
		case Op_TexCubeLOD:  fnstr = version < 140 ?
			(ast.stage == ShaderStage_Pixel ? "textureCubeLodEXT" : "textureCubeLod") :
			"textureLod";  break;
		case Op_TexCubeProj: fnstr = version < 140 ? "textureCubeProj"    : "textureProj"; break;
		case Op_Tex1DLOD0Cmp:
		case Op_Tex2DLOD0Cmp:
		case Op_TexCubeLOD0Cmp: fnEnd = ",-32)"; // <- bias LOD away from lower mip levels
			// passthrough
		case Op_Tex1DCmp:
		case Op_Tex2DCmp:
		case Op_TexCubeCmp: fnstr = version < 140 ? "UNAVAILABLE" : "texture"; break;
		case Op_Trunc:     assert(version >= 130); break;
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
			out << fnEnd; // normally )
			return;
		}
	}
	else if (auto* binop = dyn_cast<const BinaryOpExpr>(node))
	{
		if (binop->GetLft()->GetReturnType()->kind == ASTType::Vector ||
			binop->GetLft()->GetReturnType()->kind == ASTType::Vector)
		{
			const char* opstr = nullptr;
			switch (binop->opType)
			{
			case STT_OP_Eq: opstr = "equal"; goto vcmp;
			case STT_OP_NEq: opstr = "notEqual"; goto vcmp;
			case STT_OP_Less: opstr = "lessThan"; goto vcmp;
			case STT_OP_LEq: opstr = "lessThanEqual"; goto vcmp;
			case STT_OP_Greater: opstr = "greaterThan"; goto vcmp;
			case STT_OP_GEq: opstr = "greaterThanEqual"; goto vcmp;
			default: break;
			vcmp:
				out << opstr << "(";
				EmitExpr(binop->GetLft());
				out << ",";
				EmitExpr(binop->GetRgt());
				out << ")";
				return;
			}
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
		if ((ast.usingLODTextureSampling && ast.stage == ShaderStage_Pixel) ||
			ast.usingGradTextureSampling)
		{
			out << "#extension GL_EXT_shader_texture_lod : enable\n";
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

