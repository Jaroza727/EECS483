/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
  
#include <set>
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}
  

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
}

void ClassDecl::Build()
{
    if (vars.NumEntries()) return;
    if (extends)
    {
        ClassDecl *cla = GetProgram()->Query(extends->GetName());
        cla->Build();
        vars = cla->GetVars();
        fns = cla->GetFns();
        vtable = cla->vtable;
    }
    int offset = vars.NumEntries() * 4 + 4;
    std::string className(GetName());
    className = "_" + className + ".";
    for (int i = 0; i < members->NumElements(); ++i)
    {
        VarDecl *var = dynamic_cast<VarDecl*>(members->Nth(i));
        if (var)
        {
            const char *name = var->GetName();
            var->SetLoc(new Location(Segment::fpRelative, offset, name));
            offset += 4;
            Insert(name, var);
        }
    }
    for (int i = 0; i < members->NumElements(); ++i)
    {
        FnDecl *fn = dynamic_cast<FnDecl*>(members->Nth(i));
        if (fn)
        {
            const char *name = fn->GetName();
            fn->SetLabel(className + name);
            char *n = new char[100];
            strcpy(n, fn->GetLabel());
            if (Ask(name))
            {
                for (int i = 0; i < vtable.NumElements(); ++i)
                {
                    std::string old(vtable.Nth(i));
                    if (!old.substr(old.find(".") + 1).compare(name))
                    {
                        vtable.RemoveAt(i);
                        vtable.InsertAt(n, i);
                        break;
                    }
                }
            }
            else vtable.Append(n);
            Add(name, fn);
        }
    }
    size = offset;
}

void ClassDecl::Emit()
{
    for (int i = 0; i < members->NumElements(); ++i) members->Nth(i)->Emit();
    CG.GenVTable(GetName(), &vtable);
}

int ClassDecl::GetOffset(const char *label)
{
    for (int i = 0; i < vtable.NumElements(); ++i)
        if (!strcmp(label, vtable.Nth(i))) return 4 * i;
    return -1;
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
    formalLocs = new List<Location*>;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void FnDecl::Emit()
{
    int para = 4;
    CG.GenLabel(GetLabel());
    BeginFunc *bf = CG.GenBeginFunc(this);
    ClassDecl *cla = GetClass();
    VarDecl *var = NULL;
    if (cla)
    {
        var = new VarDecl(new Identifier(yyltype(), "this"), cla->GetType());
        var->SetLoc(new Location(Segment::fpRelative, para, "this"));
        para += 4;
        Insert("this", var);
        formalLocs->Append(var->GetLoc());
    }
    for (int i = 0; i < formals->NumElements(); ++i)
    {
        var = formals->Nth(i);
        const char *name = var->GetName();
        var->SetLoc(new Location(Segment::fpRelative, para, name));
        para += 4;
        Insert(name, var);
        formalLocs->Append(var->GetLoc());
    }
    if (body) body->Emit();
    bf->SetFrameSize(-8 - offset);
    CG.GenEndFunc();
}

bool FnDecl::ifLCall()
{
    return label.find(".") == std::string::npos;
}