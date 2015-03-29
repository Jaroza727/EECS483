/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "errors.h"
#include <string.h>


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

Location *IntConstant::GenCode() {
    return g_code_generator_ptr->GenLoadConstant(value);
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

Location *BoolConstant::GenCode() {
    return g_code_generator_ptr->GenLoadConstant(value ? 1 : 0);
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Location *StringConstant::GenCode() {
    return g_code_generator_ptr->GenLoadConstant(value);
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

void CompoundExpr::Check()
{
    if (left) left->Check();
    right->Check();
}

Location *CompoundExpr::GenCode()
{
    if (left && left->GetType() == Type::stringType
        && right->GetType() == Type::stringType
        && (!strcmp(op->GetOp(), "==") || !strcmp(op->GetOp(), "!=")) )
    {
        return g_code_generator_ptr->GenStrOperation(op->GetOp(), left->GenCode(),
                                                     right->GenCode());
    }
    if (left)
        return g_code_generator_ptr->GenOperation(op->GetOp(), left->GenCode(),
                                                  right->GenCode());
    else
        return g_code_generator_ptr->GenOperation(op->GetOp(), nullptr,
                                                  right->GenCode());

    Assert(false);
    return nullptr;
}

void ArithmeticExpr::Check()
{
    CompoundExpr::Check();
    Type *rightType = right->GetType();
    bool rightInt = rightType->IsEquivalentTo(Type::intType), rightDouble = rightType->IsEquivalentTo(Type::doubleType);
    if (left)
    {
        Type *leftType = left->GetType();
        bool leftInt = leftType->IsEquivalentTo(Type::intType), leftDouble = leftType->IsEquivalentTo(Type::doubleType);
        if ((!leftInt || !rightInt) && (!leftDouble || !rightDouble)) ReportError::IncompatibleOperands(op, leftType, rightType);
    }
    else if (!rightInt && !rightDouble) ReportError::IncompatibleOperand(op, rightType);
}

Type *ArithmeticExpr::GetType()
{
    Type *rightType = right->GetType();
    bool rightInt = rightType->IsEquivalentTo(Type::intType), rightDouble = rightType->IsEquivalentTo(Type::doubleType);
    if (left)
    {
        Type *leftType = left->GetType();
        bool leftInt = leftType->IsEquivalentTo(Type::intType), leftDouble = leftType->IsEquivalentTo(Type::doubleType);
        if (leftType == Type::errorType) return Type::errorType;
        return ((leftInt && rightInt) || (leftDouble && rightDouble)) ? rightType : Type::errorType;   
    }
    else return (rightInt || rightDouble) ? rightType : Type::errorType;
}

void RelationalExpr::Check()
{
    CompoundExpr::Check();
    Type *rightType = right->GetType(), *leftType = left->GetType();
    bool rightInt = rightType->IsEquivalentTo(Type::intType), rightDouble = rightType->IsEquivalentTo(Type::doubleType),
        leftInt = leftType->IsEquivalentTo(Type::intType), leftDouble = leftType->IsEquivalentTo(Type::doubleType);
    if ((!leftInt || !rightInt) && (!leftDouble || !rightDouble)) ReportError::IncompatibleOperands(op, leftType, rightType);
}

void EqualityExpr::Check()
{
    CompoundExpr::Check();
    Type *rightType = right->GetType(), *leftType = left->GetType();
    if (!leftType->IsCompatibleWith(rightType) && !rightType->IsCompatibleWith(leftType))
        ReportError::IncompatibleOperands(op, leftType, rightType);
}

void LogicalExpr::Check()
{
    CompoundExpr::Check();
    Type *rightType = right->GetType();
    bool rightBool = rightType->IsEquivalentTo(Type::boolType);
    if (!left)
    {
        if (!rightBool) ReportError::IncompatibleOperand(op, rightType);
    }
    else
    {
        Type *leftType = left->GetType();
        bool leftBool = leftType->IsEquivalentTo(Type::boolType);
        if (!leftBool || !rightBool) ReportError::IncompatibleOperands(op, leftType, rightType);
    }
}

void AssignExpr::Check()
{
    CompoundExpr::Check();
    Type *rightType = right->GetType(), *leftType = left->GetType();
    if (!rightType->IsCompatibleWith(leftType)) ReportError::IncompatibleOperands(op, leftType, rightType);
}

Location *AssignExpr::GenCode() {
    if (auto arrayAccess = dynamic_cast<ArrayAccess*>(left))
    {
        auto lloc = arrayAccess->GenCellAddrCode();
        auto rloc = right->GenCode();
        g_code_generator_ptr->GenStore(lloc, rloc);
        return rloc;
    }
    else
    {
        auto lloc = left->GenCode();
        auto rloc = right->GenCode();
        g_code_generator_ptr->GenAssign(lloc, rloc);
        return lloc;
    }
}

void This::Check()
{
    Node *p = this;
    while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
    if (!p) ReportError::ThisOutsideClassScope(this);
}

Type *This::GetType()
{
    Node *p = this;
    while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
    if (!p) return Type::errorType;
    return ((ClassDecl*)p)->GetType();
}
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void ArrayAccess::Check()
{
    base->Check();
    subscript->Check();
    if (base->GetType()->GetNode() != tNode::ArrayTypeT) ReportError::BracketsOnNonArray(base);
    if (!subscript->GetType()->IsEquivalentTo(Type::intType)) ReportError::SubscriptNotInteger(subscript);
}

Location *ArrayAccess::GenCode()
{
    return g_code_generator_ptr->GenLoad(GenCellAddrCode());
}

Location *ArrayAccess::GenCellAddrCode()
{
    // Check subscript in bound
    auto baseLoc = base->GenCode();
    auto subscriptLoc = subscript->GenCode();
    auto smallerThanZero = g_code_generator_ptr->GenOperation("<", subscriptLoc, g_code_generator_ptr->GenLoadConstant(0));
    auto sizeLoc = g_code_generator_ptr->GenLoad(baseLoc, -CodeGenerator::VarSize);
    auto greaterEqualToSize = g_code_generator_ptr->GenOperation(">=", subscriptLoc, sizeLoc);
    auto endLabel = g_code_generator_ptr->NewLabel();
    auto testResult = g_code_generator_ptr->GenOperation("||", smallerThanZero, greaterEqualToSize);
    g_code_generator_ptr->GenIfZ(testResult, endLabel);
    g_code_generator_ptr->GenBuiltInCall(BuiltIn::PrintString, g_code_generator_ptr->GenLoadConstant(err_arr_out_of_bounds));
    g_code_generator_ptr->GenBuiltInCall(BuiltIn::Halt);
    g_code_generator_ptr->GenLabel(endLabel);

    // Load cell value
    auto cellOffset = g_code_generator_ptr->GenOperation("*", subscriptLoc,
                                                         g_code_generator_ptr->GenLoadConstant(CodeGenerator::VarSize));
    auto cellLoc = g_code_generator_ptr->GenOperation("+", baseLoc, cellOffset);
    return cellLoc;
}

Type *ArrayAccess::GetType()
{
    if (base->GetType()->GetNode() != tNode::ArrayTypeT || !subscript->GetType()->IsEquivalentTo(Type::intType))
        return Type::errorType;
    return ((ArrayType*)base->GetType())->GetElemType();
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}


Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}
 

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

void NewExpr::Check()
{
    cType->CheckClass();
}

Location *NewExpr::GenCode()
{
    auto classDecl = dynamic_cast<ClassDecl*>(GetRoot()->Lookup(cType->GetId()));
    Assert(classDecl);
    auto size = classDecl->GetMemberVariableSize();
    auto loc = g_code_generator_ptr->GenBuiltInCall(BuiltIn::Alloc,
                                                    g_code_generator_ptr->GenLoadConstant(size));
    auto vtableLoc = g_code_generator_ptr->GenLoadLabel(classDecl->GetId()->GetName());
    g_code_generator_ptr->GenStore(loc, vtableLoc);
    return loc;
}

Type *NewExpr::GetType()
{
    Decl *decl = GetRoot()->Lookup(cType->GetId());
    tNode tnode = decl ? decl->GetNode() : tNode::NodeT;
    return (tnode == tNode::ClassDeclT) ? cType : Type::errorType;
}

NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc), type(ArrayType(loc, et)) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

void NewArrayExpr::Check()
{
    size->Check();
    elemType->Check();
    if (!size->GetType()->IsEquivalentTo(Type::intType)) ReportError::NewArraySizeNotInteger(size);
}

Location *NewArrayExpr::GenCode()
{
    // Check array size
    auto sizeLoc = size->GenCode();
    auto smallerThanOne = g_code_generator_ptr->GenOperation("<", sizeLoc, g_code_generator_ptr->GenLoadConstant(1));
    auto endLabel = g_code_generator_ptr->NewLabel();
    g_code_generator_ptr->GenIfZ(smallerThanOne, endLabel);
    g_code_generator_ptr->GenBuiltInCall(BuiltIn::PrintString, g_code_generator_ptr->GenLoadConstant(err_arr_bad_size));
    g_code_generator_ptr->GenBuiltInCall(BuiltIn::Halt);
    g_code_generator_ptr->GenLabel(endLabel);

    // Allocate space
    auto cellNeeded = g_code_generator_ptr->GenOperation("+", sizeLoc, g_code_generator_ptr->GenLoadConstant(1));
    auto varSize = g_code_generator_ptr->GenLoadConstant(CodeGenerator::VarSize);
    auto spaceNeeded = g_code_generator_ptr->GenOperation("*", cellNeeded, varSize);
    auto allocatedSpace = g_code_generator_ptr->GenBuiltInCall(BuiltIn::Alloc, spaceNeeded);
    g_code_generator_ptr->GenStore(allocatedSpace, sizeLoc);
    auto realArrayStartLoc = g_code_generator_ptr->GenOperation("+", varSize, allocatedSpace);    
    return realArrayStartLoc;
}

Type *NewArrayExpr::GetType()
{   
    Type *eType = elemType;
    while (eType->GetNode() == tNode::ArrayTypeT) eType = ((ArrayType*)eType)->GetElemType();
    if (eType->GetNode() == tNode::NodeT) return &type;
    NamedType *cType = (NamedType*)eType;
    Decl *decl = GetRoot()->Lookup(cType->GetId());
    tNode tnode = decl ? decl->GetNode() : tNode::NodeT;
    return (tnode == tNode::ClassDeclT) ? &type : Type::errorType;
}

void FieldAccess::Check()
{
    if (base) base->Check();
    if (base)
    {
        Node *p = this;
        while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
        ClassDecl *classDecl = (ClassDecl*)p;
        Type *type = base->GetType();
        if (type->GetNode() != tNode::NamedTypeT)
        {
            ReportError::FieldNotFoundInBase(field, type);
            return;
        }
        Decl *decl = GetRoot()->Lookup(((NamedType*)type)->GetId());
        if (!decl) return;
        if (base->GetNode() == tNode::ThisT)
        {
            if (!classDecl) return;
            decl = classDecl;
        }
        Decl *var = decl->Lookup(field);
        if (!var || var->GetNode() != tNode::VarDeclT)
        {
            ReportError::FieldNotFoundInBase(field, type);
            return;
        }
        if (!p || !((ClassDecl*)p)->GetType()->IsCompatibleWith(type))
            ReportError::InaccessibleField(field, type);
    }
    else
    {
        Node *p = this;
        while (p)
        {
            Decl *decl = p->Lookup(field);
            if (decl)
            {
                if (decl->GetNode() != tNode::VarDeclT)
                    ReportError::IdentifierNotDeclared(field, reasonT::LookingForVariable);
                return;
            }
            p = p->GetParent();
        }
        ReportError::IdentifierNotDeclared(field, reasonT::LookingForVariable);
    }
}

Location *FieldAccess::FindDeclLocation()
{
    if (base)
    {
        Node *p = this;
        while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
        ClassDecl *classDecl = (ClassDecl*)p;
        Type *type = base->GetType();
        Decl *decl = GetRoot()->Lookup(((NamedType*)type)->GetId());
        Assert(decl);
        if (base->GetNode() == tNode::ThisT)
        {
            Assert(classDecl);
            decl = classDecl;
        }
        VarDecl *varDecl = dynamic_cast<VarDecl*>(decl->Lookup(field));
        Assert(varDecl);
        return varDecl->GetLocation();
    }
    else
    {
        Node *p = this;
        while (p)
        {
            VarDecl *decl = dynamic_cast<VarDecl*>(p->Lookup(field));
            if (decl) return decl->GetLocation();
            p = p->GetParent();
        }
        Assert(false);
        return nullptr;
    }
}

Location *FieldAccess::GenCode()
{
    return FindDeclLocation();
}

Type *FieldAccess::GetType()
{
    if (base)
    {
        Node *p = this;
        while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
        ClassDecl *classDecl = (ClassDecl*)p;
        Type *type = base->GetType();
        if (type->GetNode() != tNode::NamedTypeT) return Type::errorType;
        Decl *decl = GetRoot()->Lookup(((NamedType*)type)->GetId());
        if (!decl) return Type::errorType;
        if (base->GetNode() == tNode::ThisT)
        {
            if (!classDecl) return Type::errorType;
            decl = classDecl;
        }
        Decl *var = decl->Lookup(field);
        if (!var || var->GetNode() != tNode::VarDeclT || !classDecl || !classDecl->GetType()->IsCompatibleWith(type))
            return Type::errorType;
        return ((VarDecl*)var)->GetType();
    }
    else
    {
        Node *p = this;
        while (p)
        {
            Decl *decl = p->Lookup(field);
            if (decl)
            {
                if (decl->GetNode() != tNode::VarDeclT) return Type::errorType;
                else return ((VarDecl*)decl)->GetType();
            }
            p = p->GetParent();
        }
        return Type::errorType;
    }
}

void Call::Check()
{
    FnDecl *fnDecl = NULL;
    if (base) base->Check();
    for (int i = 0; i < actuals->NumElements(); ++i) actuals->Nth(i)->Check();
    if (base)
    {
        Node *p = this;
        while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
        ClassDecl *classDecl = (ClassDecl*)p;
        Type *type = base->GetType();
        if (type->GetNode() == tNode::ArrayTypeT && !strcmp(field->GetName(), "length"))
        {
            int given = actuals->NumElements();
            if (given) ReportError::NumArgsMismatch(field, 0, given);
            return;
        }
        if (type->GetNode() != tNode::NamedTypeT)
        {
            ReportError::FieldNotFoundInBase(field, type);
            return;
        }
        Decl *decl = GetRoot()->Lookup(((NamedType*)type)->GetId());
        if (!decl) return;
        if (base->GetNode() == tNode::ThisT)
        {
            if (!classDecl) return;
            decl = classDecl;
        }
        Decl *var = decl->Lookup(field);
        if (!var || var->GetNode() != tNode::FnDeclT)
        {
            ReportError::FieldNotFoundInBase(field, type);
            return;
        }
        fnDecl = (FnDecl*)var;
        return;
    }
    else
    {
        Node *p = this;
        while (p)
        {
            Decl *decl = p->Lookup(field);
            if (decl)
            {
                if (decl->GetNode() != tNode::FnDeclT)
                {
                    ReportError::IdentifierNotDeclared(field, reasonT::LookingForFunction);
                    return;
                }
                else
                {
                    fnDecl = (FnDecl*)decl;
                    break;
                }
            }
            p = p->GetParent();
        }
        if (!fnDecl)
        {
            ReportError::IdentifierNotDeclared(field, reasonT::LookingForFunction);
            return;
        }
    }
    List<VarDecl*> *formals = fnDecl->GetFormals();
    int numExpect = formals->NumElements(), numGiven = actuals->NumElements();
    if (numExpect != numGiven)
    {
        ReportError::NumArgsMismatch(field, numExpect, numGiven);
        return;
    }
    for (int i = 0; i < numGiven && i < numExpect; ++i)
    {
        Expr *actual = actuals->Nth(i);
        Type *given = actual->GetType(), *expect = formals->Nth(i)->GetType();
        if (!given->IsCompatibleWith(expect)) ReportError::ArgMismatch(actual, i + 1, given, expect);
    }
}

FnDecl *Call::FindDecl()
{
    if (base)
    {
        FnDecl* fnDecl;
        Node *p = this;
        while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
        ClassDecl *classDecl = (ClassDecl*)p;
        Type *type = base->GetType();
        if (type->GetNode() == tNode::ArrayTypeT && !strcmp(field->GetName(), "length"))
        {
            return nullptr;
        }
        Assert(false); // TODO
        if (type->GetNode() != tNode::NamedTypeT)
        {
            ReportError::FieldNotFoundInBase(field, type);
            return nullptr;
        }
        Decl *decl = GetRoot()->Lookup(((NamedType*)type)->GetId());
        if (!decl) return nullptr;
        if (base->GetNode() == tNode::ThisT)
        {
            if (!classDecl) return nullptr;
            decl = classDecl;
        }
        Decl *var = decl->Lookup(field);
        if (!var || var->GetNode() != tNode::FnDeclT)
        {
            ReportError::FieldNotFoundInBase(field, type);
            return nullptr;
        }
        fnDecl = (FnDecl*)var;
        return nullptr;
    }
    else
    {
        Node *p = this;
        while (p)
        {
            Decl *decl = p->Lookup(field);
            if (decl)
                return (FnDecl*)decl;
            p = p->GetParent();
        }
    }
    Assert(false);
    return nullptr;
}

Location *Call::GenCode()
{
    FnDecl *fnDecl = FindDecl();
    if (fnDecl)
    {
        List<Location*> argLocations;
        for (int i = 0; i < actuals->NumElements(); ++i)
            argLocations.Append(actuals->Nth(i)->GenCode());
        for (int i = argLocations.NumElements() - 1; i >= 0; --i)
            g_code_generator_ptr->GenPushParam(argLocations.Nth(i));
        char label[33];
        sprintf(label, "_%s", fnDecl->GetId()->GetName());
        Location *returnLoc = g_code_generator_ptr->GenLCall(label, fnDecl->GetType());
        g_code_generator_ptr->GenPopParams(CodeGenerator::VarSize * argLocations.NumElements());
        return returnLoc;
    }
    else
    {
        // array.length()
        return g_code_generator_ptr->GenLoad(base->GenCode(), -CodeGenerator::VarSize);
    }
}

Type *Call::GetType()
{
    FnDecl *fnDecl = NULL;
    if (base)
    {
        Node *p = this;
        while (p && p->GetNode() != tNode::ClassDeclT) p = p->GetParent();
        ClassDecl *classDecl = (ClassDecl*)p;
        Type *type = base->GetType();
        if (type->GetNode() == tNode::ArrayTypeT && !strcmp(field->GetName(), "length"))
        {
            if (actuals->NumElements()) return Type::errorType;
            else return Type::intType;
        }
        if (type->GetNode() != tNode::NamedTypeT) return Type::errorType;
        Decl *decl = GetRoot()->Lookup(((NamedType*)type)->GetId());
        if (!decl) return Type::errorType;
        if (base->GetNode() == tNode::ThisT)
        {
            if (!classDecl) return Type::errorType;
            decl = classDecl;
        }
        Decl *var = decl->Lookup(field);
        if (!var || var->GetNode() != tNode::FnDeclT) return Type::errorType;
        fnDecl = (FnDecl*)var;
    }
    else
    {
        Node *p = this;
        while (p)
        {
            Decl *decl = p->Lookup(field);
            if (decl)
            {
                if (decl->GetNode() != tNode::FnDeclT) return Type::errorType;
                else
                {
                    fnDecl = (FnDecl*)decl;
                    break;
                }
            }
            p = p->GetParent();
        }
        if (!fnDecl) return Type::errorType;
    }
    List<VarDecl*> *formals = fnDecl->GetFormals();
    int numExpect = formals->NumElements(), numGiven = formals->NumElements();
    if (numExpect != numGiven) return Type::errorType;
    return fnDecl->GetType();
}
