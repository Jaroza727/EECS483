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

    Decl             *decl;
    List<Decl*>      *declList;
    VarDecl          *varDecl;
    List<VarDecl*>   *varDeclList;
    ClassDecl        *classDecl;
    InterfaceDecl    *interfaceDecl;
    FnDecl           *fnDecl;
    List<FnDecl*>    *fnDeclList;
    Type             *type;
    Stmt             *stmt;
    List<Stmt*>      *stmtList;
    StmtBlock        *stmtBlock;
    Expr             *expr;
    List<Expr*>      *exprList;
    IfStmt           *ifStmt;
    WhileStmt        *whileStmt;
    ForStmt          *forStmt;
    ReturnStmt       *returnStmt;
    BreakStmt        *breakStmt;
    PrintStmt        *printStmt;
    LValue           *lValue;
    Call             *call;
    NamedType        *namedType;
    List<NamedType*> *namedTypeList;
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
%type <declList>      DeclList 
%type <decl>          Decl
%type <varDecl>       VarDecl
%type <varDeclList>   VarDeclList
%type <classDecl>     ClassDecl
%type <interfaceDecl> InterfaceDecl
%type <fnDecl>        FnDecl
%type <varDecl>       Variable
%type <type>          Type
%type <varDeclList>   Formals
%type <varDeclList>   NonEmptyFormals
%type <stmt>          Stmt
%type <stmtList>      StmtList
%type <stmtBlock>     StmtBlock
%type <expr>          Expr
%type <expr>          ExprStmt
%type <exprList>      ExprList
%type <ifStmt>        IfStmt
%type <whileStmt>     WhileStmt
%type <forStmt>       ForStmt
%type <returnStmt>    ReturnStmt
%type <breakStmt>     BreakStmt
%type <printStmt>     PrintStmt
%type <lValue>        LValue
%type <expr>          Constant
%type <call>          Call
%type <exprList>      Actuals
%type <namedType>     Extends
%type <namedTypeList> Implements
%type <namedTypeList> IdentList
%type <declList>      FieldList
%type <decl>          Field
%type <declList>      PrototypeList
%type <fnDecl>        Prototype

%right    '='
%left     T_Or
%left     T_And
%left     T_Equal T_NotEqual
%left     '<' T_LessEqual '>' T_GreaterEqual
%left     '+' '-'
%left     '*' '/' '%'
%nonassoc '!' '-'
%left     '[' '.'

%start Program

%%
/* Rules
 * -----
 * All productions and actions should be placed between the start and stop
 * %% markers which delimit the Rules section.
	 
 */
Program           :    DeclList             { 
                                              Program *program = new Program($1);
                                              // if no errors, advance to next phase
                                              if (ReportError::NumErrors() == 0) 
                                                  program->Print(0);
                                            }
                  ;

DeclList          :    DeclList Decl        { ($$ = $1)->Append($2); }
                  |    Decl                 { ($$ = new List<Decl*>)->Append($1); }
                  ;

Decl              :    VarDecl              { $$ = $1; }
                  |    ClassDecl            { $$ = $1; }
                  |    InterfaceDecl        { $$ = $1; }
                  |    FnDecl               { $$ = $1; }
                  ;

ClassDecl         :     T_Class T_Identifier Extends Implements '{' FieldList '}'
                                            { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new ClassDecl(ident, $3, $4, $6); 
                                            }
                  ;

InterfaceDecl     :     T_Interface T_Identifier '{' PrototypeList '}'
                                            { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new InterfaceDecl(ident, $4); 
                                            }
                  ;

FieldList         :     FieldList Field     { ($$ = $1)->Append($2); }
                  |     /*  empty  */       { $$ = new List<Decl*>; }
                  ;

Field             :     VarDecl             { $$ = $1; }
                  |     FnDecl              { $$ = $1; }
                  ;

FnDecl            :    Type T_Identifier '(' Formals ')' StmtBlock 
                                            { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new FnDecl(ident, $1, $4);
                                              $$->SetFunctionBody($6);
                                            }
                  |    T_Void T_Identifier '(' Formals ')' StmtBlock 
                                            { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new FnDecl(ident, Type::voidType, $4);
                                              $$->SetFunctionBody($6);
                                            }
                  ;

PrototypeList     :     PrototypeList Prototype
                                            { ($$ = $1)->Append($2); }
                  |     /*  empty  */       { $$ = new List<Decl*>; }
                  ;

Prototype         :     Type T_Identifier '(' Formals ')' ';'
                                            { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new FnDecl(ident, $1, $4); 
                                            }
                  |     T_Void T_Identifier '(' Formals ')' ';'
                                            { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new FnDecl(ident, Type::voidType, $4); 
                                            }
                  ;

Formals           :    NonEmptyFormals      { $$ = $1; }
                  |    /*  empty  */        { $$ = new List<VarDecl*>; }
                  ;

NonEmptyFormals   :    NonEmptyFormals ',' Variable
                                            { ($$ = $1)->Append($3); }
                  |    Variable             { ($$ = new List<VarDecl*>)->Append($1); }
                  ;

StmtList          :    StmtList Stmt        { ($$ = $1)->Append($2); }
                  |    /*  empty  */        { $$ = new List<Stmt*>; }
                  ;

ExprStmt          :     ';'                 { $$ = new EmptyExpr; }
                  |     Expr ';'            { $$ = $1; }
                  ;

IfStmt            :     T_If '(' Expr ')' Stmt T_Else Stmt
                                            { $$ = new IfStmt($3, $5, $7); }
                  |     T_If '(' Expr ')' Stmt
                                            { $$ = new IfStmt($3, $5, NULL); }
                  ;

WhileStmt         :     T_While '(' Expr ')' Stmt
                                            { $$ = new WhileStmt($3, $5); }
                  ;

ForStmt           :     T_For '(' ExprStmt Expr ';' ExprStmt ')' Stmt
                                            { $$ = new ForStmt($3, $4, $6, $8); }
                  ;

ReturnStmt        :     T_Return ExprStmt   { $$ = new ReturnStmt(@2, $2); }
                  ;

BreakStmt         :     T_Break ';'         { $$ = new BreakStmt(@1); }
                  ;

PrintStmt         :     T_Print '(' ExprList ')' ';'
                                            { $$ = new PrintStmt($3); }
                  ;

Stmt              :    ExprStmt             { $$ = $1; }
                  |    IfStmt               { $$ = $1; }
                  |    WhileStmt            { $$ = $1; }
                  |    ForStmt              { $$ = $1; }
                  |    BreakStmt            { $$ = $1; }
                  |    ReturnStmt           { $$ = $1; }
                  |    PrintStmt            { $$ = $1; }
                  |    StmtBlock            { $$ = $1; }
                  ;

StmtBlock         :     '{' VarDeclList StmtList '}'
                                            { $$ = new StmtBlock($2, $3); }
                  ;

VarDeclList       :     VarDeclList VarDecl { ($$ = $1)->Append($2); }
                  |     /*  empty  */       { $$ = new List<VarDecl*>; }
                  ;

Call              :     Expr '.' T_Identifier '(' Actuals ')'
                                            { 
                                              Identifier *ident = new Identifier(@3, $3);
                                              $$ = new Call(@1, $1, ident, $5); 
                                            }
                  |     T_Identifier '(' Actuals ')'
                                            { 
                                              Identifier *ident = new Identifier(@1, $1);
                                              $$ = new Call(@1, NULL, ident, $3); 
                                            }
                  ;

Actuals           :     ExprList            { $$ = $1; }
                  |                         { $$ = new List<Expr*>; }
                  ;

ExprList          :     ExprList ',' Expr   { ($$ = $1)->Append($3); }
                  |     Expr                { ($$ = new List<Expr*>)->Append($1); }
                  ;

Expr              :     LValue '=' Expr     { 
                                              Operator *op = new Operator(@2, "=");
                                              $$ = new AssignExpr($1, op, $3);
                                            }
                  |     Constant            { $$ = $1; }
                  |     LValue              { $$ = $1; }
                  |     T_This              { $$ = new This(@1); }
                  |     Call                { $$ = $1; }
                  |     '(' Expr ')'        { $$ = $2; }
                  |     Expr '+' Expr       { 
                                              Operator *op = new Operator(@2, "+");
                                              $$ = new ArithmeticExpr($1, op, $3);
                                            }
                  |     Expr '-' Expr       { 
                                              Operator *op = new Operator(@2, "-");
                                              $$ = new ArithmeticExpr($1, op, $3);
                                            }
                  |     Expr '*' Expr       { 
                                              Operator *op = new Operator(@2, "*");
                                              $$ = new ArithmeticExpr($1, op, $3);
                                            }
                  |     Expr '%' Expr       { 
                                              Operator *op = new Operator(@2, "%");
                                              $$ = new ArithmeticExpr($1, op, $3);
                                            }
                  |     '-' Expr            { 
                                              Operator *op = new Operator(@1, "-");
                                              $$ = new ArithmeticExpr(op, $2);
                                            }
                  |     Expr '<' Expr       { 
                                              Operator *op = new Operator(@2, "<");
                                              $$ = new RelationalExpr($1, op, $3);
                                            }
                  |     Expr '>' Expr       { 
                                              Operator *op = new Operator(@2, ">");
                                              $$ = new RelationalExpr($1, op, $3);
                                            }
                  |     Expr T_LessEqual Expr       
                                            { 
                                              Operator *op = new Operator(@2, "<=");
                                              $$ = new RelationalExpr($1, op, $3);
                                            }
                  |     Expr T_GreaterEqual Expr       
                                            { 
                                              Operator *op = new Operator(@2, ">=");
                                              $$ = new RelationalExpr($1, op, $3);
                                            }
                  |     Expr T_Equal Expr       
                                            { 
                                              Operator *op = new Operator(@2, "==");
                                              $$ = new EqualityExpr($1, op, $3);
                                            }
                  |     Expr T_NotEqual Expr       
                                            { 
                                              Operator *op = new Operator(@2, "!=");
                                              $$ = new EqualityExpr($1, op, $3);
                                            }
                  |     Expr T_And Expr       
                                            { 
                                              Operator *op = new Operator(@2, "&&");
                                              $$ = new LogicalExpr($1, op, $3);
                                            }
                  |     Expr T_Or Expr       
                                            { 
                                              Operator *op = new Operator(@2, "||");
                                              $$ = new LogicalExpr($1, op, $3);
                                            }
                  |     '!' Expr       
                                            { 
                                              Operator *op = new Operator(@1, "!");
                                              $$ = new LogicalExpr(op, $2);
                                            }
                  |     T_ReadInteger '(' ')'
                                            { 
                                              $$ = new ReadIntegerExpr(@1); 
                                            }
                  |     T_ReadLine '(' ')'
                                            { 
                                              $$ = new ReadLineExpr(@1); 
                                            }
                  |     T_New '(' T_Identifier ')'
                                            { 
                                              Identifier *ident = new Identifier(@3, $3);
                                              $$ = new NewExpr(@1, new NamedType(ident)); 
                                            }
                  |     T_NewArray '(' Expr ',' Type ')'
                                            { $$ = new NewArrayExpr(@1, $3, $5); }
                  ;

LValue            :     Expr '[' Expr ']'
                                            { $$ = new ArrayAccess(@1, $1, $3); }
                  |     Expr '.' T_Identifier
                                            { 
                                              Identifier *ident = new Identifier(@3, $3);
                                              $$ = new FieldAccess($1, ident); 
                                            }
                  |     T_Identifier        { 
                                              Identifier *ident = new Identifier(@1, $1);
                                              $$ = new FieldAccess(NULL, ident); 
                                            }
                  ;

Constant          :     T_IntConstant       { $$ = new IntConstant(@1, $1); }
                  |     T_DoubleConstant    { $$ = new DoubleConstant(@1, $1); }
                  |     T_BoolConstant      { $$ = new BoolConstant(@1, $1); }
                  |     T_StringConstant    { $$ = new StringConstant(@1, $1); }
                  |     T_Null              { $$ = new NullConstant(@1); }
                  ;

Extends           :     T_Extends T_Identifier          
                                            { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new NamedType(ident); 
                                            }
                  |     /*  empty  */       { $$ = NULL; }
                  ;

Implements        :     T_Implements IdentList          
                                            { $$ = $2; }
                  |     /*  empty  */       { $$ = new List<NamedType*>; }
                  ;

IdentList         :     IdentList ',' T_Identifier      
                                            { 
                                              Identifier *ident = new Identifier(@3, $3);
                                              NamedType *type = new NamedType(ident);
                                              ($$ = $1)->Append(type); 
                                            }
                  |     T_Identifier                    
                                            { 
                                              Identifier *ident = new Identifier(@1, $1);
                                              NamedType *type = new NamedType(ident);
                                              ($$ = new List<NamedType*>)->Append(type);
                                            }

VarDecl           :    Variable ';'         { $$ = $1; }
                  ;

Variable          :    Type T_Identifier    { 
                                              Identifier *ident = new Identifier(@2, $2);
                                              $$ = new VarDecl(ident, $1); 
                                            }
                  ;

Type              :    Type '[' ']'         { 
                                              $$ = new ArrayType(@1, $1); 
                                            }
                  |    T_Int                { $$ = Type::intType; }
                  |    T_Double             { $$ = Type::doubleType; }
                  |    T_Bool               { $$ = Type::boolType; }
                  |    T_String             { $$ = Type::stringType; }
                  |    T_Identifier         { 
                                              Identifier *ident = new Identifier(@1, $1);
                                              $$ = new NamedType(ident); 
                                            }
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
   yydebug = true;
   // yydebug = false;
}