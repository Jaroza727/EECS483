/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp4: You will need to extend the Decl classes to implement 
 * code generation for declarations.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "ast_type.h"
#include "list.h"

class Identifier;
class Stmt;

class Decl : public Node 
{
  protected:
    Identifier *id;
  
  public:
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    const char *GetName() { return id->GetName(); }
    virtual void Emit() {}
    virtual void Build() {}
    virtual Type *GetType() { return NULL; }
};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    Location *loc = NULL;
    
  public:
    VarDecl(Identifier *name, Type *type);
    void SetLoc(Location *l) { loc = l; }
    Location *GetLoc() { return loc; }
    Type *GetType() { return type; }
};

class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;
    NamedType selfType = NamedType(id);
    List<const char*> vtable;
    int size;

  public:
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    const Hashtable<VarDecl*> &GetVars() { return vars; }
    const Hashtable<FnDecl*> &GetFns() { return fns; }
    NamedType *GetType() { return &selfType; }
    void Build();
    void Emit();
    int GetOffset(const char *label);
    int GetSize() { return size; }
};

class InterfaceDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<Decl*> *members);
};

class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;
    std::string label;
    int offset = -8;
    
  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    int GetOffset() { return offset; }
    void UpdateOffset() { offset -= 4; }
    void SetLabel(const std::string &l) { label = l; }
    const char *GetLabel() { return label.c_str(); }
    bool ifLCall();
    void Emit();
    Type *GetType() { return returnType; }
};

#endif
