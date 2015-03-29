/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
        
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n), location(nullptr) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}
  

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n), type(NamedType(n)) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}


InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

    
FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void VarDecl::Check()
{
    type->Check();
}

Location *VarDecl::GenCode() {
    location = g_code_generator_ptr->GenLocalVariable(id->GetName());
    return location;
}

Location *VarDecl::GenGlobalCode() {
    location = g_code_generator_ptr->GenGlobalVariable(id->GetName());
    return location;
}

Location *VarDecl::GenArgCode() {
    location = g_code_generator_ptr->GenArgVariable(id->GetName());
    return location;
}

void ClassDecl::CheckOverride(FnDecl *decl, FnDecl *prev, bool override)
{
    bool flag = decl->GetType()->IsEquivalentTo(prev->GetType());
    List<VarDecl*> *declFormals = decl->GetFormals(), *prevFormals = prev->GetFormals();
    flag &= declFormals->NumElements() == prevFormals->NumElements();
    for (int i = 0; i < declFormals->NumElements() && i < prevFormals->NumElements(); ++i)
        flag &= declFormals->Nth(i)->GetType()->IsEquivalentTo(prevFormals->Nth(i)->GetType());
    if (flag) Insert(decl, override);
    else ReportError::OverrideMismatch(decl);
}

void ClassDecl::CheckImplement(NamedType *impl)
{
    Decl *decl = GetRoot()->Lookup(impl->GetId());
    if (!decl || decl->GetNode() != tNode::InterfaceDeclT)
    {
        ReportError::IdentifierNotDeclared(impl->GetId(), reasonT::LookingForInterface);
        return;
    }
    List<Decl*> *members = ((InterfaceDecl*)decl)->GetMembers();
    for (int i = 0; i < members->NumElements(); ++i)
    {
        FnDecl *expect = (FnDecl*)members->Nth(i);
        Decl *given = Lookup(expect->GetId());
        if (!given || given->GetNode() != FnDeclT)
        {
            ReportError::InterfaceNotImplemented(this, impl);
            return;
        }
        CheckOverride((FnDecl*)given, expect, false);
    }
}

void ClassDecl::Check()
{
    Node *root = GetRoot();
    NamedType *super = extends;

    // Prepare table
    while (super)
    {
        Identifier *id = super->GetId();
        Decl *decl = root->Lookup(id);
        if (!decl || decl->GetNode() != tNode::ClassDeclT) break;
        ClassDecl *classDecl = (ClassDecl*)decl;
        List<Decl*> *superMembers = classDecl->members;
        super = classDecl->extends;
        for (int i = 0; i < superMembers->NumElements(); ++i) Insert(superMembers->Nth(i), true);
    }
    for (int i = 0; i < members->NumElements(); ++i)
    {
        Decl *decl = members->Nth(i), *prev = Insert(decl);
        if (decl != prev)
        {
            if (decl->GetNode() == tNode::FnDeclT && prev->GetNode() == tNode::FnDeclT)
                CheckOverride((FnDecl*)decl, (FnDecl*)prev);
            else ReportError::DeclConflict(decl, prev);
        }
    }

    //Check
    if (extends) extends->CheckClass();
    for (int i = 0; i < implements->NumElements(); ++i) CheckImplement(implements->Nth(i));
    for (int i = 0; i < members->NumElements(); ++i) members->Nth(i)->Check();

    memberVariableSize = 1;
    if (extends)
    {
        auto parentClass = dynamic_cast<ClassDecl*>(GetRoot()->Lookup(extends->GetId()));
        Assert(parentClass);
        auto& parentMethodLabels = parentClass->methodLabels;
        methodLabels.insert(parentMethodLabels.begin(), parentMethodLabels.end());
        memberVariableSize = parentClass->memberVariableSize;
    }
}

Location *ClassDecl::GenCode()
{
    // Generate class vtable
    for (int i = 0; i < members->NumElements(); ++i)
    {
        if (auto fnDecl = dynamic_cast<FnDecl*>(members->Nth(i)))
        {
            fnDecl->GenClassCode(this);
            methodLabels[fnDecl->GetId()->GetName()] = id->GetName();
        }
        else
        {
            memberVariableSize++;
        }
    }
    auto methodLabelList = new List<const char*>;
    for (auto& pair : methodLabels)
    {
        char* name = new char[66];
        sprintf(name, "%s_%s", pair.second, pair.first);
        methodLabelList->Append(name);
    }
    g_code_generator_ptr->GenVTable(id->GetName(), methodLabelList);

    return nullptr;
}

void InterfaceDecl::Check()
{
    for (int i = 0; i < members->NumElements(); ++i)
    {
        Decl *decl = members->Nth(i), *prev = Insert(decl);
        if (decl != prev) ReportError::DeclConflict(decl, prev);
        decl->Check();
    }
}

void FnDecl::Check()
{
    returnType->Check();
    for (int i = 0; i < formals->NumElements(); ++i)
    {
        Decl *decl = formals->Nth(i), *prev = Insert(formals->Nth(i));
        if (decl != prev) ReportError::DeclConflict(decl, prev);
        formals->Nth(i)->Check();
    }
    if (body) body->Check();
}

Location *FnDecl::GenCode() {
    char label[33];
    if (!strcmp(id->GetName(), "main"))
        sprintf(label, "main");
    else
        sprintf(label, "_%s", id->GetName());
    g_code_generator_ptr->GenLabel(label);

    for (int i = 0; i < formals->NumElements(); ++i)
        formals->Nth(i)->GenArgCode();
    BeginFunc *beginFunc = g_code_generator_ptr->GenBeginFunc();
    if (body) {
        body->GenCode();
    }
    beginFunc->SetFrameSize(g_code_generator_ptr->GetFrameSize());
    g_code_generator_ptr->GenEndFunc();
    return nullptr;
}

void FnDecl::GenClassCode(ClassDecl *base)
{
    auto name = new char[66];
    sprintf(name, "%s_%s", base->GetId()->GetName(), id->GetName());
    g_code_generator_ptr->GenLabel(name);
    g_code_generator_ptr->GenArgVariable("this"); // "this"
    for (int i = 0; i < formals->NumElements(); ++i)
        formals->Nth(i)->GenArgCode();
    BeginFunc *beginFunc = g_code_generator_ptr->GenBeginFunc();
    if (body) {
        body->GenCode();
    }
    beginFunc->SetFrameSize(g_code_generator_ptr->GetFrameSize());
    g_code_generator_ptr->GenEndFunc();
}
