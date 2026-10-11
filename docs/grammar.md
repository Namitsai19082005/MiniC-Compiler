# MiniC Language Grammar (EBNF)

```ebnf
program        → (function_def | global_decl)*
function_def   → type_spec IDENT '(' param_list? ')' (block | ';')
param_list     → param (',' param)*
param          → type_spec IDENT
type_spec      → base_type '*'*
base_type      → 'int' | 'char' | 'void'

block          → '{' statement* '}'
statement      → decl_stmt | expr_stmt | if_stmt | while_stmt
               | for_stmt | return_stmt | block

decl_stmt      → type_spec IDENT ('[' INT_LIT ']')? ('=' expr)? ';'
expr_stmt      → expr ';'
if_stmt        → 'if' '(' expr ')' statement ('else' statement)?
while_stmt     → 'while' '(' expr ')' statement
for_stmt       → 'for' '(' (decl_stmt | expr_stmt | ';')
                           expr? ';' expr? ')' statement
return_stmt    → 'return' expr? ';'

expr           → assignment
assignment     → unary assign_op assignment | logic_or
assign_op      → '=' | '+=' | '-=' | '*=' | '/='
logic_or       → logic_and ('||' logic_and)*
logic_and      → bitwise_or ('&&' bitwise_or)*
bitwise_or     → bitwise_xor ('|' bitwise_xor)*
bitwise_xor    → bitwise_and ('^' bitwise_and)*
bitwise_and    → equality ('&' equality)*
equality       → comparison (('==' | '!=') comparison)*
comparison     → shift (('<' | '>' | '<=' | '>=') shift)*
shift          → addition (('<<' | '>>') addition)*
addition       → multiplication (('+' | '-') multiplication)*
multiplication → unary (('*' | '/' | '%') unary)*
unary          → ('!' | '-' | '~' | '*' | '&') unary | postfix
postfix        → primary ('[' expr ']' | '(' arg_list? ')')*
primary        → INT_LIT | CHAR_LIT | STRING_LIT | IDENT | '(' expr ')'
arg_list       → expr (',' expr)*
```

## Supported Types
- `int` — 64-bit signed integer
- `char` — 8-bit character
- `void` — return type only
- `T*`, `T**`, ... — pointer types of arbitrary depth

## Supported Operators (by precedence, low→high)
| Prec | Operators | Assoc |
|------|-----------|-------|
| 1 | `= += -= *= /=` | Right |
| 2 | `\|\|` | Left |
| 3 | `&&` | Left |
| 4 | `\|` | Left |
| 5 | `^` | Left |
| 6 | `&` | Left |
| 7 | `== !=` | Left |
| 8 | `< > <= >=` | Left |
| 9 | `<< >>` | Left |
| 10 | `+ -` | Left |
| 11 | `* / %` | Left |
| 12 | `! - ~ * &` (unary) | Right |
| 13 | `() []` (postfix) | Left |
