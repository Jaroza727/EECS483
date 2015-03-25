/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>
 
/* Class constants
 * ---------------
 * These are public constants for the built-in base types (int, double, etc.)
 * They can be accessed with the syntax Type::intType. This allows you to
 * directly access them and share the built-in types where needed rather that
 * creates lots of copies.
 */

Type *Type::intType    = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType   = new Type("void");
Type *Type::boolType   = new Type("bool");
Type *Type::nullType   = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType  = new Type("error"); 

Type::Type(const char *n) {
    Assert(n);
    typeName = strdup(n);
}
    
NamedType::NamedType(Identifier *i) : Type(*i->GetLocation()) {
    Assert(i != NULL);
    (id=i)->SetParent(this);
} 


ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}

void NamedType::Check()
{
    Decl *decl = GetRoot()->Lookup(id);
    tNode tnode = decl ? decl->GetNode() : tNode::NodeT;
    if (tnode != tNode::ClassDeclT && tnode != tNode::InterfaceDeclT)
        ReportError::IdentifierNotDeclared(id, reasonT::LookingForType);
}

void NamedType::CheckClass()
{
    Decl *decl = GetRoot()->Lookup(id);
    tNode tnode = decl ? decl->GetNode() : tNode::NodeT;
    if (tnode != tNode::ClassDeclT) ReportError::IdentifierNotDeclared(id, reasonT::LookingForClass);
}

void ArrayType::Check()
{
    elemType->Check();
}

bool Type::IsEquivalentTo(Type *other)
{
    return this == other || this == Type::errorType || other == Type::errorType;
}

bool Type::IsCompatibleWith(Type *other)
{
    return IsEquivalentTo(other) || (this == Type::nullType && other->GetNode() == tNode::NamedTypeT);
}

bool NamedType::IsEquivalentTo(Type *other)
{
    return other->GetNode() == tNode::NamedTypeT && !strcmp(((NamedType*)other)->id->GetName(), id->GetName());
}

bool NamedType::IsCompatibleWith(Type *other)
{
    if (IsEquivalentTo(other)) return true;
    Decl *decl = GetRoot()->Lookup(id);
    if (!decl || decl->GetNode() != tNode::ClassDeclT) return false;
    ClassDecl *classDecl = (ClassDecl*)decl;
    NamedType *extends = classDecl->GetExtends();
    List<NamedType*> *impls = classDecl->GetImpls();
    for (int i = 0; i < impls->NumElements(); ++i) if (impls->Nth(i)->IsEquivalentTo(other)) return true;
    return extends && extends->IsCompatibleWith(other);
}

bool ArrayType::IsEquivalentTo(Type *other)
{
    return other->GetNode() == tNode::ArrayTypeT && elemType->IsEquivalentTo(((ArrayType*)other)->elemType);
}

bool ArrayType::IsCompatibleWith(Type *other)
{
    return IsEquivalentTo(other);
}