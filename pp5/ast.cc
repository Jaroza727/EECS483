/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_stmt.h"
#include <string.h> // strdup
#include <stdio.h>  // printf

CodeGenerator CG;

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}

Program *Node::GetProgram()
{
	Node *p = this;
	while (p->parent) p = p->parent;
	return dynamic_cast<Program*>(p);
}

FnDecl *Node::GetFn()
{
	Node *p = this;
	while (p && !dynamic_cast<FnDecl*>(p)) p = p->parent;
	return dynamic_cast<FnDecl*>(p);
}

ClassDecl *Node::GetClass()
{
	Node *p = this;
	while (p && !dynamic_cast<ClassDecl*>(p)) p = p->parent;
	return dynamic_cast<ClassDecl*>(p);
}
	 
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

