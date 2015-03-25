/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf

CodeGenerator *g_code_generator_ptr;

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}
     
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

Node *Node::GetRoot()
{
    Node *p = this;
    while (p->GetParent()) p = p->GetParent();
    return p;
}

Decl *Node::Insert(Decl *decl, bool override)
{
    Identifier *id = decl->GetId();
    Decl *prev = Lookup(id);
    if (prev && !override) return prev;
    table.Enter(id->GetName(), decl);
    return decl;
}

Decl *Node::Lookup(Identifier *id)
{
    return table.Lookup(id->GetName());
}
