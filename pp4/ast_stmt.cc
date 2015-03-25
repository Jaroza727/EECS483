/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast.h"
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include <cassert>


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    for (int i = 0; i < decls->NumElements(); ++i)
    {
        Decl *decl = decls->Nth(i), *prev = Insert(decl);
        if (decl != prev) ReportError::DeclConflict(decl, prev);
    }
    for (int i = 0; i < decls->NumElements(); ++i) decls->Nth(i)->Check();
}

void Program::Emit() {
    g_code_generator_ptr = new CodeGenerator;
    for (int i = 0; i < decls->NumElements(); ++i) {
        if (auto decl = dynamic_cast<VarDecl*>(decls->Nth(i))) {
            decl->GenGlobalCode();
            decl->GetLocation()->Print();
        } else {
            decls->Nth(i)->GenCode();
        }
    }
    g_code_generator_ptr->DoFinalCodeGen();
    delete g_code_generator_ptr;
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void StmtBlock::Check()
{
    for (int i = 0; i < decls->NumElements(); ++i)
    {
        Decl *decl = decls->Nth(i), *prev = Insert(decl);
        decl->Check();
        if (decl != prev) ReportError::DeclConflict(decl, prev);
    }
    for (int i = 0; i < stmts->NumElements(); ++i) stmts->Nth(i)->Check();
}

Location *StmtBlock::GenCode() {
    for (int i = 0; i < decls->NumElements(); ++i) {
        decls->Nth(i)->GenCode();
        decls->Nth(i)->GetLocation()->Print();
    }
    for (int i = 0; i < stmts->NumElements(); ++i) stmts->Nth(i)->GenCode();
    return nullptr;
}

void ConditionalStmt::Check()
{
    test->Check();
    body->Check();
    if (!test->GetType()->IsEquivalentTo(Type::boolType)) ReportError::TestNotBoolean(test);
}

Location *WhileStmt::GenCode()
{
    auto startLabel = g_code_generator_ptr->NewLabel();
    endLabel = g_code_generator_ptr->NewLabel();

    g_code_generator_ptr->GenLabel(startLabel);
    auto testResult = test->GenCode();
    g_code_generator_ptr->GenIfZ(testResult, endLabel);
    body->GenCode();
    g_code_generator_ptr->GenGoto(startLabel);
    g_code_generator_ptr->GenLabel(endLabel);
    return nullptr;
}

void ForStmt::Check()
{
    ConditionalStmt::Check();
    init->Check();
    step->Check();
}

Location *ForStmt::GenCode()
{
    auto startLabel = g_code_generator_ptr->NewLabel();
    endLabel = g_code_generator_ptr->NewLabel();

    init->GenCode();
    g_code_generator_ptr->GenLabel(startLabel);
    auto testResult = test->GenCode();
    g_code_generator_ptr->GenIfZ(testResult, endLabel);
    body->GenCode();
    step->GenCode();
    g_code_generator_ptr->GenGoto(startLabel);
    g_code_generator_ptr->GenLabel(endLabel);
    return nullptr;
}

void IfStmt::Check()
{
    ConditionalStmt::Check();
    if (elseBody) elseBody->Check();
}

Location *IfStmt::GenCode()
{
    auto elseLabel = g_code_generator_ptr->NewLabel();
    auto testResult = test->GenCode();
    g_code_generator_ptr->GenIfZ(testResult, elseLabel);
    body->GenCode();
    g_code_generator_ptr->GenLabel(elseLabel);
    if (elseBody) elseBody->GenCode();
    return nullptr;
}

void BreakStmt::Check()
{
    Node *p = this;
    while (p && p->GetNode() != tNode::LoopStmtT) p = p->GetParent();
    if (!p) ReportError::BreakOutsideLoop(this);
}

Location *BreakStmt::GenCode()
{
    Node *p = this;
    while (p && p->GetNode() != tNode::LoopStmtT) p = p->GetParent();
    assert(p);
    auto endLabel = dynamic_cast<LoopStmt*>(p)->getEndLabel();
    assert(endLabel);
    g_code_generator_ptr->GenGoto(endLabel);
    return nullptr;
}

void ReturnStmt::Check()
{
    expr->Check();
    Node *p = this;
    while (p->GetNode() != tNode::FnDeclT) p = p->GetParent();
    Type *expect = ((FnDecl*)p)->GetType(), *given = expr->GetType();
    if (!given->IsCompatibleWith(expect)) ReportError::ReturnMismatch(this, given, expect);
}

void PrintStmt::Check()
{
    for (int i = 0; i < args->NumElements(); ++i) args->Nth(i)->Check();
    for (int i = 0; i < args->NumElements(); ++i)
    {
        Type *type = args->Nth(i)->GetType();
        if (!type->IsEquivalentTo(Type::intType) && !type->IsEquivalentTo(Type::boolType) && !type->IsEquivalentTo(Type::stringType))
            ReportError::PrintArgMismatch(args->Nth(i), i + 1, type);
    }
}