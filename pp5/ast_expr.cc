/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

void CompoundExpr::Emit()
{
    if (left) left->Emit();
    right->Emit();
}

void ArithmeticExpr::Emit()
{
    CompoundExpr::Emit();
    if (left) loc = CG.GenBinaryOp(op->GetName(), left->GetLoc(), right->GetLoc());
    else
    {
        IntConstant zero = IntConstant(yyltype(), 0);
        zero.Emit();
        loc = CG.GenBinaryOp("-", zero.GetLoc(), right->GetLoc());
    }
}

void RelationalExpr::Emit()
{
    CompoundExpr::Emit();
    std::string opName(op->GetName());
    bool l = !opName.compare("<"), g = !opName.compare(">"),
        le = !opName.compare("<="), ge = !opName.compare(">=");
    Location *diff, *equal;
    if (l || le) diff = CG.GenBinaryOp("<", left->GetLoc(), right->GetLoc());
    if (g || ge) diff = CG.GenBinaryOp("<", right->GetLoc(), left->GetLoc());
    if (l || g) loc = diff;
    else
    {
        if (le) equal = CG.GenBinaryOp("==", left->GetLoc(), right->GetLoc());
        else equal = CG.GenBinaryOp("==", right->GetLoc(), left->GetLoc());
        loc = CG.GenBinaryOp("||", diff, equal);
    }
}

void EqualityExpr::Emit()
{
    CompoundExpr::Emit();
    std::string opName(op->GetName());
    Location *equal = NULL;
    if (left->GetType() == Type::stringType && right->GetType() == Type::stringType)
        equal = CG.GenBuiltInCall(BuiltIn::StringEqual, left->GetLoc(), right->GetLoc());
    else equal = CG.GenBinaryOp("==", left->GetLoc(), right->GetLoc());
    if (!opName.compare("==")) loc = equal;
    else
    {
        BoolConstant fal = BoolConstant(yyltype(), false);
        fal.Emit();
        loc = CG.GenBinaryOp("==", equal, fal.GetLoc());
    }
}

void LogicalExpr::Emit()
{
    CompoundExpr::Emit();
    if (left) loc = CG.GenBinaryOp(op->GetName(), left->GetLoc(), right->GetLoc());
    else
    {
        BoolConstant fal = BoolConstant(yyltype(), false);
        fal.Emit();
        loc = CG.GenBinaryOp("==", right->GetLoc(), fal.GetLoc());
    }
}

void AssignExpr::Emit()
{
    CompoundExpr::Emit();
    ArrayAccess *l1 = dynamic_cast<ArrayAccess*>(left);
    FieldAccess *l2 = dynamic_cast<FieldAccess*>(left);
    if (l1) CG.GenStore(left->GetLoc(), right->GetLoc(), 0);
    else if (l2 && l2->GetOffset()) CG.GenStore(left->GetLoc(), right->GetLoc(), l2->GetOffset());
    else CG.GenAssign(left->GetLoc(), right->GetLoc());
    loc = left->GetLoc();
}
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void This::Emit()
{
    loc = GetFn()->Lookup("this")->GetLoc();
}

Type *This::GetType()
{
    return GetFn()->Lookup("this")->GetType();
}

void ArrayAccess::Emit()
{
    IntConstant zero = IntConstant(yyltype(), 0), four = IntConstant(yyltype(), 4);
    base->Emit();
    subscript->Emit();
    zero.Emit();
    Location *check1 = CG.GenBinaryOp("<", subscript->GetLoc(), zero.GetLoc()),
        *length = CG.GenLoad(base->GetLoc(), -4),
        *check2 = CG.GenBinaryOp("<", subscript->GetLoc(), length),
        *check3 = CG.GenBinaryOp("==", check2, zero.GetLoc()),
        *check = CG.GenBinaryOp("||", check1, check3);
    const char *label = CG.NewLabel();
    CG.GenIfZ(check, label);
    StringConstant error = StringConstant(yyltype(), err_arr_out_of_bounds);
    error.Emit();
    CG.GenBuiltInCall(BuiltIn::PrintString, error.GetLoc());
    CG.GenBuiltInCall(BuiltIn::Halt);
    CG.GenLabel(label);
    four.Emit();
    Location *pos = CG.GenBinaryOp("*", four.GetLoc(), subscript->GetLoc()),
        *addr = CG.GenBinaryOp("+", base->GetLoc(), pos);
    AssignExpr *assign = dynamic_cast<AssignExpr*>(parent);
    if (!assign || assign->GetLeft() != this) loc = CG.GenLoad(addr, 0);
    else loc = addr;
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

void FieldAccess::Emit()
{
    VarDecl *var = FindField();
    if (base) base->Emit();
    if (offset)
    {
        loc = base ? base->GetLoc() : GetFn()->Lookup("this")->GetLoc();
        AssignExpr *assign = dynamic_cast<AssignExpr*>(parent);
        if (!assign || assign->GetLeft() != this) loc = CG.GenLoad(loc, offset);
    }
    else loc = var->GetLoc();
}

Type *FieldAccess::GetType()
{
    return FindField()->GetType();
}

VarDecl *FieldAccess::FindField()
{
    const char *name = field->GetName();
    VarDecl *var = NULL;
    ClassDecl *cla = NULL;
    if (base)
    {
        cla = GetProgram()->Query(((NamedType*)base->GetType())->GetName());
        var = cla->Lookup(name);
        offset = var->GetLoc()->GetOffset();
        return var;
    }
    else
    {
        FnDecl *fn = GetFn();
        if (fn && (var = fn->Lookup(name))) return var;
        cla = GetClass();
        if (cla && (var = cla->Lookup(name)))
        {
            offset = var->GetLoc()->GetOffset();
            return var;
        }
        return GetProgram()->Lookup(name);
    }
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::Emit()
{
    if (base && dynamic_cast<ArrayType*>(base->GetType()))
    {
        base->Emit();
        loc = CG.GenLoad(base->GetLoc(), -4);
        return;
    }
    FnDecl *fn = FindField();
    bool ifLCall = fn->ifLCall();
    const char *label = fn->GetLabel();
    for (int i = 0; i < actuals->NumElements(); ++i) actuals->Nth(i)->Emit();
    if (base) base->Emit();
    if (ifLCall)
    {
        for (int i = actuals->NumElements() - 1; i >= 0; --i) CG.GenPushParam(actuals->Nth(i)->GetLoc());
        loc = CG.GenLCall(label, fn->GetType() != Type::voidType);
    }
    else
    {
        ClassDecl *cla = base ? GetProgram()->Query(((NamedType*)base->GetType())->GetName()) : GetClass();
        Location *baseLoc = base ? base->GetLoc() : GetFn()->Lookup("this")->GetLoc(),
            *vtable = CG.GenLoad(baseLoc, 0), *code = CG.GenLoad(vtable, cla->GetOffset(fn->GetLabel()));
        for (int i = actuals->NumElements() - 1; i >= 0; --i) CG.GenPushParam(actuals->Nth(i)->GetLoc());
        CG.GenPushParam(baseLoc);
        loc = CG.GenACall(code, fn->GetType() != Type::voidType);
    }
    CG.GenPopParams(actuals->NumElements() * 4 + (ifLCall ? 0 : 4));
}

Type *Call::GetType()
{
    if (base && dynamic_cast<ArrayType*>(base->GetType())) return Type::intType;
    return FindField()->GetType();
}

FnDecl *Call::FindField()
{
    const char *name = field->GetName();
    FnDecl *fn = NULL;
    ClassDecl *cla = NULL;
    if (base)
    {
        cla = GetProgram()->Query(((NamedType*)base->GetType())->GetName());
        return cla->Ask(name);
    }
    else
    {
        cla = GetClass();
        if (cla && (fn = cla->Ask(name))) return fn;
        return GetProgram()->Ask(name);
    }
}

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

void NewExpr::Emit()
{
    const char *name = cType->GetName();
    ClassDecl *cla = GetProgram()->Query(name);
    IntConstant size = IntConstant(yyltype(), cla->GetSize());
    size.Emit();
    Location *left = CG.GenBuiltInCall(BuiltIn::Alloc, size.GetLoc()), *right = CG.GenLoadLabel(name);
    CG.GenStore(left, right, 0);
    loc = left;
}

void NewArrayExpr::Emit()
{
    IntConstant one = IntConstant(yyltype(), 1), four = IntConstant(yyltype(), 4);
    size->Emit();
    one.Emit();
    Location *check = CG.GenBinaryOp("<", size->GetLoc(), one.GetLoc());
    const char *label = CG.NewLabel();
    CG.GenIfZ(check, label);
    StringConstant error = StringConstant(yyltype(), err_arr_bad_size);
    error.Emit();
    CG.GenBuiltInCall(BuiltIn::PrintString, error.GetLoc());
    CG.GenBuiltInCall(BuiltIn::Halt);
    CG.GenLabel(label);
    one.Emit();
    Location *total = CG.GenBinaryOp("+", one.GetLoc(), size->GetLoc());
    four.Emit();
    Location *bytes = CG.GenBinaryOp("*", total, four.GetLoc()),
        *array = CG.GenBuiltInCall(BuiltIn::Alloc, bytes);
    CG.GenStore(array, size->GetLoc(), 0);
    loc = CG.GenBinaryOp("+", array, four.GetLoc());
}

Type *NewArrayExpr::GetType()
{
    return new ArrayType(yyltype(), elemType);
}

void ReadIntegerExpr::Emit()
{
    loc = CG.GenBuiltInCall(BuiltIn::ReadInteger);
}

void ReadLineExpr::Emit()
{
    loc = CG.GenBuiltInCall(BuiltIn::ReadLine);
}