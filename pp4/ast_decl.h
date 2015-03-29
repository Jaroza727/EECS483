/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp3: You will need to extend the Decl classes to implement 
 * semantic processing including detection of declaration conflicts 
 * and managing scoping issues.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "ast_type.h"
#include "list.h"
#include <map>

class Identifier;
class Stmt;

class Decl : public Node 
{
  protected:
    Identifier *id;
  
  public:
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    Identifier *GetId() { return id; }
    virtual void Check() {}
    virtual tNode GetNode() { return tNode::NodeT; }
};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    Location *location;
    
  public:
    VarDecl(Identifier *name, Type *type);
    void Check();
    tNode GetNode() { return tNode::VarDeclT; }
    Type *GetType() { return type; }
    Location *GenCode() override;
    Location *GenGlobalCode();
    Location *GenArgCode();
    Location *GetLocation() { return location; }
};

class ClassDecl; // Forward declaration
class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;
    
  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    tNode GetNode() { return tNode::FnDeclT; }
    Type *GetType() { return returnType; }
    List<VarDecl*> *GetFormals() { return formals; }
    void Check();
    Location *GenCode() override;
    void GenClassCode(ClassDecl *base);
};

class InterfaceDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
    tNode GetNode() { return tNode::InterfaceDeclT; }
    List<Decl*> *GetMembers() { return members; }
    void Check();
};

class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;
    NamedType type;
    int memberVariableSize;
    void CheckOverride(FnDecl *decl, FnDecl *prev, bool override = true);
    void CheckImplement(NamedType *impl);

    struct Comparator
    {
      bool operator() (const char* lhs, const char* rhs)
      { return strcmp(lhs, rhs) < 0; }
    };
    std::map<const char*, const char*, Comparator> methodLabels;

  public:
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    void Check();
    tNode GetNode() { return tNode::ClassDeclT; }
    NamedType *GetExtends() { return extends; }
    List<NamedType*> *GetImpls() { return implements; }
    NamedType *GetType() { return &type; }
    Location *GenCode() override;
    int GetMemberVariableSize() { return memberVariableSize; }
};

#endif
