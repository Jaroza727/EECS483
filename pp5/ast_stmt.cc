/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    /* You can use your pp3 semantic analysis or leave it out if
     * you want to avoid the clutter.  We won't test pp4 against 
     * semantically-invalid programs.
     */
}

void Program::Build()
{
    int offset = 0;
    for (int i = 0; i < decls->NumElements(); ++i)
    {
        Decl *decl = decls->Nth(i);
        VarDecl *var = dynamic_cast<VarDecl*>(decl);
        ClassDecl *cla = dynamic_cast<ClassDecl*>(decl);
        FnDecl *fn = dynamic_cast<FnDecl*>(decl);
        const char *name = NULL;
        if (cla) Enter(cla->GetName(), cla);
        if (var)
        {
            name = var->GetName();
            var->SetLoc(new Location(Segment::gpRelative, offset, name));
            offset += 4;
            Insert(name, var);
        }
        if (fn)
        {
            name = fn->GetName();
            std::string label(name);
            if (label.compare("main")) label = "_" + label;
            fn->SetLabel(label);
            Add(name, fn);
        }
    }
}

void Program::Emit()
{
    Build();
    for (int i = 0; i < decls->NumElements(); ++i) decls->Nth(i)->Build();
    for (int i = 0; i < decls->NumElements(); ++i) decls->Nth(i)->Emit();
    if (Ask("main")) CG.DoFinalCodeGen();
    else ReportError::NoMainFound();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::Emit()
{
    FnDecl *fn = GetFn();
    for (int i = 0; i < decls->NumElements(); ++i)
    {
        VarDecl *var = decls->Nth(i);
        const char *name = var->GetName();
        var->SetLoc(new Location(Segment::fpRelative, fn->GetOffset(), name));
        fn->UpdateOffset();
        fn->Insert(name, var);
    }
    for (int i = 0; i < stmts->NumElements(); ++i) stmts->Nth(i)->Emit();
    for (int i = 0; i < decls->NumElements(); ++i)
    {
        VarDecl *var = decls->Nth(i);
        fn->Remove(var->GetName(), var);
    }
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

void ForStmt::Emit()
{
    init->Emit();
    const char *label1 = CG.NewLabel();
    CG.GenLabel(label1);
    test->Emit();
    endLabel = CG.NewLabel();
    CG.GenIfZ(test->GetLoc(), endLabel);
    body->Emit();
    step->Emit();
    CG.GenGoto(label1);
    CG.GenLabel(endLabel);
}

void WhileStmt::Emit()
{
    const char *label1 = CG.NewLabel();
    CG.GenLabel(label1);
    test->Emit();
    endLabel = CG.NewLabel();
    CG.GenIfZ(test->GetLoc(), endLabel);
    body->Emit();
    CG.GenGoto(label1);
    CG.GenLabel(endLabel);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::Emit()
{
    test->Emit();
    const char *label1 = CG.NewLabel();
    CG.GenIfZ(test->GetLoc(), label1);
    body->Emit();
    if (elseBody)
    {
        const char *label2 = CG.NewLabel();
        CG.GenGoto(label2);
        CG.GenLabel(label1);
        elseBody->Emit();
        CG.GenLabel(label2);
    }
    else CG.GenLabel(label1);
}

void BreakStmt::Emit()
{
    Node *p = this;
    while (!dynamic_cast<LoopStmt*>(p)) p = p->GetParent();
    CG.GenGoto(dynamic_cast<LoopStmt*>(p)->GetEndLabel());
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}

void ReturnStmt::Emit()
{
    expr->Emit();
    CG.GenReturn(expr->GetLoc());
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::Emit()
{
    for (int i = 0; i < args->NumElements(); ++i)
    {
        Expr *arg = args->Nth(i);
        Type *type = arg->GetType();
        arg->Emit();
        if (type == Type::intType) CG.GenBuiltInCall(BuiltIn::PrintInt, arg->GetLoc());
        if (type == Type::boolType) CG.GenBuiltInCall(BuiltIn::PrintBool, arg->GetLoc());
        if (type == Type::stringType) CG.GenBuiltInCall(BuiltIn::PrintString, arg->GetLoc());
    }
}
