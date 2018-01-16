

#include "compiler.hpp"


void ConstantPropagation::PostVisit(ASTNode* node)
{
	if (auto* unop = dynamic_cast<const UnaryOpExpr*>(node))
	{
		if (auto* src = dynamic_cast<const ConstExpr*>(unop->GetSource()))
		{
			switch (src->GetReturnType()->kind)
			{
			case ASTType::Bool:
			{
				bool sv = dynamic_cast<const BoolExpr*>(src)->value;
				bool out = false;
				switch (unop->opType)
				{
				case STT_OP_Not: out = sv ? 0 : 1; goto unop_replbool;
				default: break;
				unop_replbool:
					delete node->ReplaceWith(new BoolExpr(out, unop->GetReturnType()));
					return;
				}
				break;
			}
			case ASTType::Int32:
			{
				int32_t sv = dynamic_cast<const Int32Expr*>(src)->value;
				int32_t out = 0;
				switch (unop->opType)
				{
				case STT_OP_Sub: out = -sv; goto unop_replint;
				case STT_OP_Inv: out = ~sv; goto unop_replint;
				default: break;
				unop_replint:
					delete node->ReplaceWith(new Int32Expr(out, unop->GetReturnType()));
					return;
				}
				break;
			}
			case ASTType::Float16:
			case ASTType::Float32:
			{
				double sv = dynamic_cast<const Float32Expr*>(src)->value;
				double out = 0;
				switch (unop->opType)
				{
				case STT_OP_Sub: out = -sv; goto unop_replfloat;
				default: break;
				unop_replfloat:
					delete node->ReplaceWith(new Float32Expr(out, unop->GetReturnType()));
					return;
				}
				break;
			}
			}
		}
	}
	else if (auto* binop = dynamic_cast<const BinaryOpExpr*>(node))
	{
		auto* lft = dynamic_cast<const ConstExpr*>(binop->GetLft());
		auto* rgt = dynamic_cast<const ConstExpr*>(binop->GetRgt());
		if (lft && rgt)
		{
			switch (binop->GetReturnType()->kind)
			{
			case ASTType::Int32:
			{
				int32_t lv = dynamic_cast<const Int32Expr*>(lft)->value;
				int32_t rv = dynamic_cast<const Int32Expr*>(rgt)->value;
				int32_t out = 0;
				switch (binop->opType)
				{
				case STT_OP_Add: out = lv + rv; goto binop_replint;
				case STT_OP_Sub: out = lv - rv; goto binop_replint;
				case STT_OP_Mul: out = lv * rv; goto binop_replint;
				case STT_OP_Div: out = lv / rv; goto binop_replint;
				case STT_OP_Mod: out = fmodf(lv, rv); goto binop_replint;
				default: break;
				binop_replint:
					delete node->ReplaceWith(new Int32Expr(out, binop->GetReturnType()));
					return;
				}
				break;
			}
			case ASTType::Float16:
			case ASTType::Float32:
			{
				double lv = dynamic_cast<const Float32Expr*>(lft)->value;
				double rv = dynamic_cast<const Float32Expr*>(rgt)->value;
				double out = 0;
				switch (binop->opType)
				{
				case STT_OP_Add: out = lv + rv; goto binop_replfloat;
				case STT_OP_Sub: out = lv - rv; goto binop_replfloat;
				case STT_OP_Mul: out = lv * rv; goto binop_replfloat;
				case STT_OP_Div: out = lv / rv; goto binop_replfloat;
				case STT_OP_Mod: out = fmodf(lv, rv); goto binop_replfloat;
				default: break;
				binop_replfloat:
					delete node->ReplaceWith(new Float32Expr(out, binop->GetReturnType()));
					return;
				}
				break;
			}
			}
		}
	}
	else if (auto* cast = dynamic_cast<const CastExpr*>(node))
	{
		if (auto* src = dynamic_cast<const ConstExpr*>(cast->GetSource()))
		{
			if (cast->GetReturnType()->kind == ASTType::Bool)
			{
				bool out = false;
				switch (src->GetReturnType()->kind)
				{
				case ASTType::Bool:
					out = static_cast<const BoolExpr*>(src)->value;
					goto cast_replbool;
				case ASTType::Int32:
					out = static_cast<const Int32Expr*>(src)->value != 0;
					goto cast_replbool;
				case ASTType::Float16:
				case ASTType::Float32:
					out = static_cast<const Float32Expr*>(src)->value != 0;
					goto cast_replbool;
				default: break;
				cast_replbool:
					delete node->ReplaceWith(new BoolExpr(out, cast->GetReturnType()));
					return;
				}
			}
			else if (cast->GetReturnType()->kind == ASTType::Int32)
			{
				int32_t out = 0;
				switch (src->GetReturnType()->kind)
				{
				case ASTType::Bool:
					out = static_cast<const BoolExpr*>(src)->value ? 1 : 0;
					goto cast_replint;
				case ASTType::Int32:
					out = static_cast<const Int32Expr*>(src)->value;
					goto cast_replint;
				case ASTType::Float16:
				case ASTType::Float32:
					out = static_cast<const Float32Expr*>(src)->value;
					goto cast_replint;
				default: break;
				cast_replint:
					delete node->ReplaceWith(new Int32Expr(out, cast->GetReturnType()));
					return;
				}
			}
			else if (cast->GetReturnType()->IsFloat())
			{
				double out = 0;
				switch (src->GetReturnType()->kind)
				{
				case ASTType::Bool:
					out = static_cast<const BoolExpr*>(src)->value ? 1 : 0;
					goto cast_replfloat;
				case ASTType::Int32:
					out = static_cast<const Int32Expr*>(src)->value;
					goto cast_replfloat;
				case ASTType::Float16:
				case ASTType::Float32:
					out = static_cast<const Float32Expr*>(src)->value;
					goto cast_replfloat;
				default: break;
				cast_replfloat:
					delete node->ReplaceWith(new Float32Expr(out, cast->GetReturnType()));
					return;
				}
			}
		}
	}

	else if (auto* ifelse = dynamic_cast<IfElseStmt*>(node))
	{
		if (auto* cond = dynamic_cast<const BoolExpr*>(ifelse->GetCond()))
		{
			if (cond->value)
			{
				node->ReplaceWith(ifelse->GetTrueBr());
			}
			else
			{
				// TODO clean up
				node->ReplaceWith(ifelse->GetFalseBr() ? ifelse->GetFalseBr() : new BlockStmt);
			}
			return;
		}
	}
}

void RemoveUnusedFunctions::RunOnAST(AST& ast)
{
	std::unordered_map<std::string, ASTFuncMap> funcmap;
	for (auto& fgdef : ast.functions)
	{
		ASTFuncMap mfuncmap;
		for (auto& fdef : fgdef.second)
		{
			if (fdef.second->used)
				mfuncmap[fdef.first] = std::move(fdef.second);
			else
				delete fdef.second;
		}
		if (mfuncmap.empty() == false)
			funcmap[fgdef.first] = std::move(mfuncmap);
	}
	std::swap(ast.functions, funcmap);
}


void MarkUnusedVariables::PreVisit(ASTNode* node)
{
	if (auto* dre = dynamic_cast<DeclRefExpr*>(node))
	{
		if (dre->decl)
			dre->decl->used = true;
	}
	else if (auto* vds = dynamic_cast<VarDeclStmt*>(node))
	{
		for (ASTNode* ch = vds->firstChild; ch; ch = ch->next)
			ch->ToVarDecl()->used = true;
	}
}

void MarkUnusedVariables::VisitFunction(ASTFunction* fn)
{
	for (ASTNode* arg = fn->GetFirstArg(); arg; arg = arg->next)
		arg->ToVarDecl()->used = false;
	ASTWalker::VisitFunction(fn);
}

void MarkUnusedVariables::RunOnAST(AST& ast)
{
	for (ASTNode* g = ast.globalVars.firstChild; g; g = g->next)
	{
		if (auto* cbuf = dynamic_cast<CBufferDecl*>(g))
		{
			for (ASTNode* cbv = cbuf->firstChild; cbv; cbv = cbv->next)
				cbv->ToVarDecl()->used = false;
		}
		else
			g->ToVarDecl()->used = false;
	}
	VisitAST(ast);
}


void RemoveUnusedVariables::RunOnAST(AST& ast)
{
	for (ASTNode* g = ast.globalVars.firstChild; g; )
	{
		if (auto* cbuf = dynamic_cast<CBufferDecl*>(g))
		{
			g = g->next;
			for (ASTNode* cbv = cbuf->firstChild; cbv; )
			{
				auto* cbvd = cbv->ToVarDecl();
				cbv = cbv->next;
				if (cbvd->used == false)
					delete cbvd;
			}
			if (cbuf->childCount == 0)
				delete cbuf;
		}
		else if (VarDecl* cg = g->ToVarDecl())
		{
			g = g->next;
			if (cg->used == false)
				delete cg;
		}
	}

	VisitAST(ast);
}

