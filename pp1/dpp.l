/*
 * file:  dpp.l
 * ------------
 * Lex inupt file to generate the scanner for the preprocessor.
 * If you decide to use lex for the preprocesor, put your rules
 * here, otherwise the file can remain empty.
 */
%{
#include <string.h>
#include "scanner.h"
#include "errors.h"
#include <unordered_map>

void printNewLine(char * text);
std::unordered_map<std::string, std::string> defineTable;
int linenum = 1;
bool inComment = false;
%}

STRING              \"[^\"\n]*\"
ONELINE_COMMENT     \/\/.*
MULTILINE_COMMENT   \/\*([^*]|[\r\n]|(\*+([^*\/]|[\r\n])))*\*+\/
UNTERM_COMMENT      \/\*([^*]|[\r\n]|(\*+([^*\/]|[\r\n])))*\**[^\/]?
DEFINE              #define\ .*
DEFINED_VAR         #[A-Za-z]+
INVALID_HASH        #[^A-Za-z]*
NEWLINE             \n
OTHER               .

%%

{STRING} {
    ECHO;
}

{MULTILINE_COMMENT} {
    printNewLine(yytext);
}

{UNTERM_COMMENT} {
    printNewLine(yytext);
    ReportError::UntermComment();
}

{ONELINE_COMMENT} ;

{DEFINE} {
    size_t start = strlen("#define ");
    size_t end = start;
    while (yytext[end] && 'A' <= yytext[end] && yytext[end] <= 'Z')
        end ++;
    if (yytext[end] != ' ' || end == start) {
        ReportError::InvalidDirective(linenum);
    } else {
        std::string key(yytext + start, end - start);
        std::string value(yytext + end + 1, strlen(yytext) - end - 1);
        defineTable[key] = value;
    }
}

{DEFINED_VAR} {
    std::string key(yytext + 1, strlen(yytext) - 1);
    std::unordered_map<std::string,std::string>::const_iterator value_i = defineTable.find(key);
    if (value_i != defineTable.end()) {
        printf("%s", value_i->second.c_str());
    } else {
        ReportError::InvalidDirective(linenum);
    }
}

{INVALID_HASH} {
    ReportError::InvalidDirective(linenum);
}

{NEWLINE} {
    linenum ++;
    ECHO;
}

{OTHER} {
    ECHO;
}

%%
void printNewLine(char * text) {
    while (*text) {
        if (*text++ == '\n')
            printf("\n");
    }
}
