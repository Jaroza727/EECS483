/* File: parser.y
 * --------------
 * Yacc input file to generate the parser for the compiler.
 *
 * pp2: your job is to write a parser that will construct the parse tree
 *      and if no parse errors were found, print it.  The parser should 
 *      accept the language as described in specification, and as augmented 
 *      in the pp2 handout.
 */

%{

/* Just like lex, the text within this first region delimited by %{ and %}
 * is assumed to be C/C++ code and will be copied verbatim to the y.tab.c
 * file ahead of the definitions of the yyparse() function. Add other header
 * file inclusions or C++ variable declarations/prototypes that are needed
 * by your code here.
 */
#include "scanner.h" // for yylex
#include "parser.h"
#include "errors.h"

void yyerror(const char *msg); // standard error-handling routine

%}

/* The section before the first %% is the Definitions section of the yacc
 * input file. Here is where you declare tokens and types, add precedence
 * and associativity options, and so on.
 */
 
/* yylval 
 * ------
 * Here we define the type of the yylval global variable that is used by
 * the scanner to store attibute information about the token just scanned
 * and thus communicate that information to the parser. 
 *
 * pp2: You will need to add new fields to this union as you add different 
 *      attributes to your non-terminal symbols.
 */
%union {
    int integerConstant;
    bool boolConstant;
    char *stringConstant;
    double doubleConstant;
    char identifier[MaxIdentLen+1]; // +1 for terminating null
    Decl *decl;
    List<Decl*> *declList;
    Identifier *ident;
    Type *type;
    VarDecl *varDecl;
    FnDecl *fnDecl;
    List<VarDecl*> *varList;
    StmtBlock *stmtBlock;
    List<Stmt*> *stmts;
    Stmt *stmt;
    Expr *expr;
    LValue *lValue;
    Call *call;
    List<Expr*> *exprs;
    IfStmt *ifStmt;
    WhileStmt *whileStmt;
    ForStmt *forStmt;
    BreakStmt *breakStmt;
    ReturnStmt *returnStmt;
    PrintStmt *printStmt;
    ClassDecl *classDecl;
    List<NamedType*> *idents;
    InterfaceDecl *interfaceDecl;
    SwitchStmt *switchStmt;
    Case *_case;
    List<Case*> *cases;
    Default *_default;
}


/* Tokens
 * ------
 * Here we tell yacc about all the token types that we are using.
 * Yacc will assign unique numbers to these and export the #define
 * in the generated y.tab.h header file.
 */
%token   T_Void T_Bool T_Int T_Double T_String T_Class 
%token   T_LessEqual T_GreaterEqual T_Equal T_NotEqual T_Dims
%token   T_And T_Or T_Null T_Extends T_This T_Interface T_Implements
%token   T_While T_For T_If T_Else T_Return T_Break
%token   T_New T_NewArray T_Print T_ReadInteger T_ReadLine

%token   <identifier> T_Identifier
%token   <stringConstant> T_StringConstant 
%token   <integerConstant> T_IntConstant
%token   <doubleConstant> T_DoubleConstant
%token   <boolConstant> T_BoolConstant

%token   T_Increm T_Decrem T_Switch T_Case T_Default


/* Non-terminal types
 * ------------------
 * In order for yacc to assign/access the correct field of $$, $1, we
 * must to declare which field is appropriate for the non-terminal.
 * As an example, this first type declaration establishes that the DeclList
 * non-terminal uses the field named "declList" in the yylval union. This
 * means that when we are setting $$ for a reduction for DeclList ore reading
 * $n which corresponds to a DeclList nonterminal we are accessing the field
 * of the union named "declList" which is of type List<Decl*>.
 * pp2: You'll need to add many of these of your own.
 */
%type <declList>  DeclList 
%type <decl>      Decl
%type <ident>     Ident
%type <type>      Type
%type <varDecl>   Variable
%type <varDecl>   VarDecl
%type <fnDecl>    FnDecl
%type <varList>   Formals
%type <varList>   _Formals
%type <stmtBlock> StmtBlock
%type <varList>   VarDecls
%type <stmts>     Stmts
%type <stmt>      Stmt
%type <expr>      Expr
%type <lValue>    LValue
%type <expr>      Constant
%type <call>      Call
%type <exprs>     Actuals
%type <exprs>     Exprs
%type <ifStmt>    IfStmt
%type <whileStmt> WhileStmt
%type <forStmt>   ForStmt
%type <breakStmt> BreakStmt
%type <returnStmt>ReturnStmt
%type <printStmt> PrintStmt
%type <classDecl> ClassDecl
%type <idents>    Idents
%type <decl>      Field
%type <declList>  Fields
%type <interfaceDecl> InterfaceDecl
%type <decl>      Prototype
%type <declList>  Prototypes
%type <switchStmt>SwitchStmt
%type <_case>     Case
%type <cases>     Cases
%type <_default>  Default

%right            T_Else THEN
%nonassoc         '='
%left             T_Or
%left             T_And
%nonassoc         T_Equal T_NotEqual
%nonassoc         '<' '>' T_LessEqual T_GreaterEqual
%left             '+' '-'
%left             '*' '/' '%'
%nonassoc         '!' UMINUS //Not sure about the associativity
%nonassoc         '[' '.' //Not sure about the associativity

%%
/* Rules
 * -----
 * All productions and actions should be placed between the start and stop
 * %% markers which delimit the Rules section.
	 
 */
Program   :    DeclList             { 
                                      Program *program = new Program($1);
                                      // if no errors, advance to next phase
                                      if (ReportError::NumErrors() == 0) 
                                          program->Print(0);
                                    }
          ;

DeclList  :    DeclList Decl        { ($$=$1)->Append($2); }
          |    Decl                 { ($$ = new List<Decl*>)->Append($1); }
          ;

Decl      :    VarDecl              { $$ = $1; }
          |    FnDecl               { $$ = $1; }
          |    ClassDecl            { $$ = $1; }
          |    InterfaceDecl        { $$ = $1; }
          ;

VarDecl   :    Variable ';'         { $$ = $1; }
          ;

Variable  :    Type Ident           { $$ = new VarDecl($2, $1); }
          ;

Ident     :    T_Identifier         { $$ = new Identifier(@1, $1); }
          ;

Idents    :    Ident                { ($$ = new List<NamedType*>)->Append(new NamedType($1)); }
          |    Idents ',' Ident     { ($$ = $1)->Append(new NamedType($3)); }
          ;

Type      :    T_Int                { $$ = Type::intType; }
          |    T_Double             { $$ = Type::doubleType; }
          |    T_Bool               { $$ = Type::boolType; }
          |    T_String             { $$ = Type::stringType; }
          |    Ident                { $$ = new NamedType($1); }
          |    Type T_Dims          { $$ = new ArrayType(@1, $1); }
          ;

FnDecl    :    Type Ident '(' Formals ')' StmtBlock
                                    { ($$ = new FnDecl($2, $1, $4))->SetFunctionBody($6); }
          |    T_Void Ident '(' Formals ')' StmtBlock
                                    { ($$ = new FnDecl($2, Type::voidType, $4))->SetFunctionBody($6); }
          ;

Formals   :                         { $$ = new List<VarDecl*>; }
          |    _Formals             { $$ = $1; }
          ;

_Formals  :    Variable             { ($$ = new List<VarDecl*>)->Append($1); }
          |    _Formals ',' Variable{ ($$ = $1)->Append($3); }
          ;

ClassDecl :    T_Class Ident T_Extends Ident T_Implements Idents '{' Fields '}'
                                    { $$ = new ClassDecl($2, new NamedType($4), $6, $8); }
          |    T_Class Ident T_Implements Idents '{' Fields '}'
                                    { $$ = new ClassDecl($2, NULL, $4, $6); }
          |    T_Class Ident T_Extends Ident '{' Fields '}'
                                    { $$ = new ClassDecl($2, new NamedType($4), new List<NamedType*>, $6); }
          |    T_Class Ident '{' Fields '}'
                                    { $$ = new ClassDecl($2, NULL, new List<NamedType*>, $4); }
          ;

Field     :    VarDecl              { $$ = $1; }
          |    FnDecl               { $$ = $1; }
          ;

Fields    :                         { $$ = new List<Decl*>; }
          |    Fields Field         { ($$ = $1)->Append($2); }
          ;

InterfaceDecl : T_Interface Ident '{' Prototypes '}'
                                    { $$ = new InterfaceDecl($2, $4); }
          ;

Prototype :    Type Ident '(' Formals ')' ';'
                                    { $$ = new FnDecl($2, $1, $4); }
          |    T_Void Ident '(' Formals ')' ';'
                                    { $$ = new FnDecl($2, Type::voidType, $4); }
          ;

Prototypes:                         { $$ = new List<Decl*>; }
          |    Prototypes Prototype { ($$ = $1)->Append($2); }
          ;

StmtBlock :    '{' VarDecls Stmts '}' { $$ = new StmtBlock($2, $3); }
          |    '{' Stmts '}'        { $$ = new StmtBlock(new List<VarDecl*>, $2); }
          |    '{' VarDecls '}'     { $$ = new StmtBlock($2, new List<Stmt*>); }
          |    '{' '}'              { $$ = new StmtBlock(new List<VarDecl*>, new List<Stmt*>); }
          ;

VarDecls  :    VarDecl              { ($$ = new List<VarDecl*>)->Append($1); }
          |    VarDecls VarDecl     { ($$ = $1)->Append($2); }
          ;

Stmts     :    Stmt                 { ($$ = new List<Stmt*>)->Append($1); }
          |    Stmts Stmt           { ($$ = $1)->Append($2); }
          ;

Stmt      :    Expr ';'             { $$ = $1; }
          |    ';'                  { $$ = new EmptyExpr; }
          |    IfStmt               { $$ = $1; }
          |    WhileStmt            { $$ = $1; }
          |    ForStmt              { $$ = $1; }
          |    BreakStmt            { $$ = $1; }
          |    ReturnStmt           { $$ = $1; }
          |    PrintStmt            { $$ = $1; }
          |    StmtBlock            { $$ = $1; }
          |    SwitchStmt           { $$ = $1; }
          ;

SwitchStmt:    T_Switch '(' Expr ')' '{' Cases '}'
                                    { $$ = new SwitchStmt($3, $6, NULL); }
          |    T_Switch '(' Expr ')' '{' Cases Default '}'
                                    { $$ = new SwitchStmt($3, $6, $7); }
          ;

Cases     :    Case                 { ($$ = new List<Case*>)->Append($1); }
          |    Cases Case           { ($$ = $1)->Append($2); }
          ;

Case      :    T_Case T_IntConstant ':' Stmts
                                    { $$ = new Case(new IntConstant(@2, $2), $4); }
          |    T_Case T_IntConstant ':'
                                    { $$ = new Case(new IntConstant(@2, $2), new List<Stmt*>); }
          ;

Default   :    T_Default ':' Stmts  { $$ = new Default($3); }
          |    T_Default ':'        { $$ = new Default(new List<Stmt*>); }
          ;

IfStmt    :    T_If '(' Expr ')' Stmt T_Else Stmt
                                    { $$ = new IfStmt($3, $5, $7); }
          |    T_If '(' Expr ')' Stmt %prec THEN
                                    { $$ = new IfStmt($3, $5, NULL); }
          ;

WhileStmt :    T_While '(' Expr ')' Stmt
                                    { $$ = new WhileStmt($3, $5); }
          ;

ForStmt   :    T_For '(' Expr ';' Expr ';' Expr ')' Stmt
                                    { $$ = new ForStmt($3, $5, $7, $9); }
          |    T_For '(' ';' Expr ';' Expr ')' Stmt
                                    { $$ = new ForStmt(new EmptyExpr(), $4, $6, $8); }
          |    T_For '(' Expr ';' Expr ';' ')' Stmt
                                    { $$ = new ForStmt($3, $5, new EmptyExpr(), $8); }
          |    T_For '(' ';' Expr ';' ')' Stmt
                                    { $$ = new ForStmt(new EmptyExpr(), $4, new EmptyExpr(), $7); }
          ;

ReturnStmt:    T_Return Expr ';'    { $$ = new ReturnStmt(@1, $2); }
          |    T_Return ';'         { $$ = new ReturnStmt(@1, new EmptyExpr()); }
          ;

BreakStmt :    T_Break ';'          { $$ = new BreakStmt(@1); }
          ;

PrintStmt :    T_Print '(' Exprs ')' ';'
                                    { $$ = new PrintStmt($3); }
          ;

Exprs     :    Expr                 { ($$ = new List<Expr*>)->Append($1); }
          |    Exprs ',' Expr       { ($$ = $1)->Append($3); }
          ;

Expr      :    LValue '=' Expr      { $$ = new AssignExpr($1, new Operator(@2, "="), $3); }
          |    Constant             { $$ = $1; }
          |    LValue               { $$ = $1; }
          |    T_This               { $$ = new This(@1); }
          |    Call                 { $$ = $1; }
          |    '(' Expr ')'         { $$ = $2; }
          |    Expr '+' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "+"), $3); }
          |    Expr '-' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "-"), $3); }
          |    Expr '*' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "*"), $3); }
          |    Expr '/' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "/"), $3); }
          |    Expr '%' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "%"), $3); }
          |    '-' Expr %prec UMINUS{ $$ = new ArithmeticExpr(new Operator(@1, "-"), $2); }
          |    Expr '<' Expr        { $$ = new RelationalExpr($1, new Operator(@2, "<"), $3); }
          |    Expr '>' Expr        { $$ = new RelationalExpr($1, new Operator(@2, ">"), $3); }
          |    Expr T_LessEqual Expr{ $$ = new RelationalExpr($1, new Operator(@2, "<="), $3); }
          |    Expr T_GreaterEqual Expr
                                    { $$ = new RelationalExpr($1, new Operator(@2, ">="), $3); }
          |    Expr T_Equal Expr    { $$ = new EqualityExpr($1, new Operator(@2, "=="), $3); }
          |    Expr T_NotEqual Expr { $$ = new EqualityExpr($1, new Operator(@2, "!="), $3); }
          |    Expr T_And Expr      { $$ = new LogicalExpr($1, new Operator(@2, "&&"), $3); }
          |    Expr T_Or Expr       { $$ = new LogicalExpr($1, new Operator(@2, "||"), $3); }
          |    '!' Expr             { $$ = new LogicalExpr(new Operator(@1, "!"), $2); }
          |    T_ReadInteger '(' ')'{ $$ = new ReadIntegerExpr(@1); }
          |    T_ReadLine '(' ')'   { $$ = new ReadLineExpr(@1); }
          |    T_New '(' Ident ')'  { $$ = new NewExpr(@1, new NamedType($3)); }
          |    T_NewArray '(' Expr ',' Type ')'
                                    { $$ = new NewArrayExpr(@1, $3, $5); }
          |    LValue T_Increm      { $$ = new PostfixExpr($1, new Operator(@2, "++")); }
          |    LValue T_Decrem      { $$ = new PostfixExpr($1, new Operator(@2, "--")); }
          ;

LValue    :    Ident                { $$ = new FieldAccess(NULL, $1); }
          |    Expr '.' Ident       { $$ = new FieldAccess($1, $3); }
          |    Expr '[' Expr ']'    { $$ = new ArrayAccess(@1, $1, $3); }
          ;

Call      :    Ident '(' Actuals ')'{ $$ = new Call(@1, NULL, $1, $3); }
          |    Expr '.' Ident '(' Actuals ')'
                                    { $$ = new Call(@1, $1, $3, $5); }
          ;

Actuals   :                         { $$ = new List<Expr*>; }
          |    Exprs                { $$ = $1; }
          ;

Constant  :    T_IntConstant        { $$ = new IntConstant(@1, $1); }
          |    T_DoubleConstant     { $$ = new DoubleConstant(@1, $1); }
          |    T_BoolConstant       { $$ = new BoolConstant(@1, $1); }
          |    T_StringConstant     { $$ = new StringConstant(@1, $1); }
          |    T_Null               { $$ = new NullConstant(@1); }
          ;

%%

/* The closing %% above marks the end of the Rules section and the beginning
 * of the User Subroutines section. All text from here to the end of the
 * file is copied verbatim to the end of the generated y.tab.c file.
 * This section is where you put definitions of helper functions.
 */

/* Function: InitParser
 * --------------------
 * This function will be called before any calls to yyparse().  It is designed
 * to give you an opportunity to do anything that must be done to initialize
 * the parser (set global variables, configure starting state, etc.). One
 * thing it already does for you is assign the value of the global variable
 * yydebug that controls whether yacc prints debugging information about
 * parser actions (shift/reduce) and contents of state stack during parser.
 * If set to false, no information is printed. Setting it to true will give
 * you a running trail that might be helpful when debugging your parser.
 * Please be sure the variable is set to false when submitting your final
 * version.
 */
void InitParser()
{
   PrintDebug("parser", "Initializing parser");
   yydebug = false;
}
