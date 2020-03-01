/*
 * File: gen.c
 *  Common code generator facilities.
 */
#include <stdarg.h>
#include <stdio.h>

#include "gen.h"
#include "lice.h"

char *gen_label_break           = NULL;
char *gen_label_continue        = NULL;
char *gen_label_switch          = NULL;
char *gen_label_break_backup    = NULL;
char *gen_label_continue_backup = NULL;
char *gen_label_switch_backup   = NULL;

static void gen_emit_emitter(bool indent, const char *fmt, va_list list) {
    if (indent)
        fputc('\t', stdout);

    va_list va;
    va_copy(va, list);
    vprintf(fmt, va);
    va_end(va);

    fputc('\n', stdout);
}

void gen_emit(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    gen_emit_emitter(true, fmt, va);
    va_end(va);
}

void gen_emit_inline(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    gen_emit_emitter(false, fmt, va);
    va_end(va);
}

void gen_jump_backup(void) {
    gen_label_break_backup    = gen_label_break;
    gen_label_continue_backup = gen_label_continue;
}

void gen_jump_save(char *lbreak, char *lcontinue) {
    gen_jump_backup();

    gen_label_break           = lbreak;
    gen_label_continue        = lcontinue;
}

void gen_jump_restore(void) {
    gen_label_break    = gen_label_break_backup;
    gen_label_continue = gen_label_continue_backup;
}

void gen_jump(const char *label) {
    if (!label)
        compile_ice("gen_jump");

    gen_emit("jmp %s", label);
}

void gen_label(const char *label) {
    gen_emit("%s:", label);
}

/*
 * Some expressions are architecture-independent thanks to generic generation
 * functions.
 */
static void gen_statement_switch(ast_t *ast) {
    gen_label_switch_backup = gen_label_switch;
    gen_label_break_backup  = gen_label_break;
    gen_expression(ast->switchstmt.expr);
    gen_label_switch = ast_label();
    gen_label_break  = ast_label();
    gen_jump(gen_label_switch);
    if (ast->switchstmt.body)
        gen_expression(ast->switchstmt.body);
    gen_label(gen_label_switch);
    gen_label(gen_label_break);
    gen_label_switch = gen_label_switch_backup;
    gen_label_break  = gen_label_break_backup;
}

static void gen_statement_do(ast_t *ast) {
    char *begin = ast_label();
    char *end   = ast_label();
    gen_jump_save(end, begin);
    gen_label(begin);
    gen_expression(ast->forstmt.body);
    gen_expression(ast->forstmt.cond);
    gen_je(end);
    gen_jump(begin);
    gen_label(end);
    gen_jump_restore();
}

static void gen_statement_compound(ast_t *ast) {
    for (list_iterator_t *it = list_iterator(ast->compound); !list_iterator_end(it); )
        gen_expression(list_iterator_next(it));
}

static void gen_statement_goto(ast_t *ast) {
    gen_jump(ast->gotostmt.where);
}

static void gen_statement_label(ast_t *ast) {
    if (ast->gotostmt.where)
        gen_label(ast->gotostmt.where);
}

static void gen_statement_cond(ast_t *ast) {
    gen_expression(ast->ifstmt.cond);
    char *ne = ast_label();
    gen_je(ne);
    if (ast->ifstmt.then)
        gen_expression(ast->ifstmt.then);
    if (ast->ifstmt.last) {
        char *end = ast_label();
        gen_jump(end);
        gen_label(ne);
        gen_expression(ast->ifstmt.last);
        gen_label(end);
    } else {
        gen_label(ne);
    }
}

static void gen_statement_for(ast_t *ast) {
    if (ast->forstmt.init)
        gen_expression(ast->forstmt.init);
    char *begin = ast_label();
    char *step  = ast_label();
    char *end   = ast_label();
    gen_jump_save(end, step);
    gen_label(begin);
    if (ast->forstmt.cond) {
        gen_expression(ast->forstmt.cond);
        gen_je(end);
    }
    gen_expression(ast->forstmt.body);
    gen_label(step);
    if (ast->forstmt.step)
        gen_expression(ast->forstmt.step);
    gen_jump(begin);
    gen_label(end);
    gen_jump_restore();
}

static void gen_statement_while(ast_t *ast) {
    char *begin = ast_label();
    char *end   = ast_label();
    gen_jump_save(end, begin);
    gen_label(begin);
    gen_expression(ast->forstmt.cond);
    gen_je(end);
    gen_expression(ast->forstmt.body);
    gen_jump(begin);
    gen_label(end);
    gen_jump_restore();
}

static void gen_statement_return(ast_t *ast) {
    if (ast->returnstmt) {
        gen_expression(ast->returnstmt);
        gen_boolean_maybe(ast->returnstmt->ctype);
    }
    gen_return();
}

static void gen_statement_break(void) {
    gen_jump(gen_label_break);
}

static void gen_statement_continue(void) {
    gen_jump(gen_label_continue);
}

static void gen_statement_default(void) {
    gen_label(gen_label_switch);
    gen_label_switch = ast_label();
}

static void gen_comma(ast_t *ast) {
    gen_expression(ast->left);
    gen_expression(ast->right);
}

static void gen_data_bss(ast_t *ast) {
    gen_emit(".data");
    if (!ast->decl.var->ctype->isstatic)
        gen_emit(".global %s", ast->decl.var->variable.name);
    gen_emit(".lcomm %s, %d", ast->decl.var->variable.name, ast->decl.var->ctype->size);
}

static void gen_data_global(ast_t *variable) {
    if (variable->decl.init)
        gen_data(variable, 0, 0);
    else
        gen_data_bss(variable);
}

static void gen_declaration_initialization(list_t *init, int offset) {
    for (list_iterator_t *it = list_iterator(init); !list_iterator_end(it); ) {
        ast_t *node = list_iterator_next(it);
        if (node->init.value->type == AST_TYPE_LITERAL && node->init.type->bitfield.size <= 0)
            gen_literal_save(node->init.value, node->init.type, node->init.offset + offset);
        else {
            gen_expression(node->init.value);
            gen_save_local(node->init.type, node->init.offset + offset);
        }
    }
}

static void gen_declaration(ast_t *ast) {
    if (!ast->decl.init)
        return;

    gen_zero(ast->decl.var->variable.off, ast->decl.var->variable.off + ast->decl.var->ctype->size);
    gen_declaration_initialization(ast->decl.init, ast->decl.var->variable.off);
}

void gen_ensure_lva(ast_t *ast) {
    if (ast->variable.init) {
        gen_zero(ast->variable.off, ast->variable.off + ast->ctype->size);
        gen_declaration_initialization(ast->variable.init, ast->variable.off);
    }
    ast->variable.init = NULL;
}

void gen_expression(ast_t *ast) {
    if (!ast) return;

    switch (ast->type) {
        case AST_TYPE_STATEMENT_IF:             gen_statement_cond(ast);         break;
        case AST_TYPE_EXPRESSION_TERNARY:       gen_statement_cond(ast);         break;
        case AST_TYPE_STATEMENT_FOR:            gen_statement_for(ast);          break;
        case AST_TYPE_STATEMENT_WHILE:          gen_statement_while(ast);        break;
        case AST_TYPE_STATEMENT_DO:             gen_statement_do(ast);           break;
        case AST_TYPE_STATEMENT_COMPOUND:       gen_statement_compound(ast);     break;
        case AST_TYPE_STATEMENT_SWITCH:         gen_statement_switch(ast);       break;
        case AST_TYPE_STATEMENT_GOTO:           gen_statement_goto(ast);         break;
        case AST_TYPE_STATEMENT_LABEL:          gen_statement_label(ast);        break;
        case AST_TYPE_STATEMENT_RETURN:         gen_statement_return(ast);       break;
        case AST_TYPE_STATEMENT_BREAK:          gen_statement_break();           break;
        case AST_TYPE_STATEMENT_CONTINUE:       gen_statement_continue();        break;
        case AST_TYPE_STATEMENT_DEFAULT:        gen_statement_default();         break;
        case AST_TYPE_CALL:                     gen_function_call(ast);          break;
        case AST_TYPE_POINTERCALL:              gen_function_call(ast);          break;
        case AST_TYPE_LITERAL:                  gen_literal(ast);                break;
        case AST_TYPE_STRING:                   gen_literal_string(ast);         break;
        case AST_TYPE_VAR_LOCAL:                gen_variable_local(ast);         break;
        case AST_TYPE_VAR_GLOBAL:               gen_variable_global(ast);        break;
        case AST_TYPE_DECLARATION:              gen_declaration(ast);            break;
        case AST_TYPE_DEREFERENCE:              gen_dereference(ast);            break;
        case AST_TYPE_ADDRESS:                  gen_address(ast->unary.operand); break;
        case AST_TYPE_STATEMENT_CASE:           gen_case(ast);                   break;
        case AST_TYPE_VA_START:                 gen_va_start(ast);               break;
        case AST_TYPE_VA_ARG:                   gen_va_arg(ast);                 break;
        case '!':                               gen_not(ast);                    break;
        case AST_TYPE_NEGATE:                   gen_negate(ast);                 break;
        case AST_TYPE_AND:                      gen_and(ast);                    break;
        case AST_TYPE_OR:                       gen_or(ast);                     break;
        case AST_TYPE_POST_INCREMENT:           gen_postfix(ast, "add");         break;
        case AST_TYPE_POST_DECREMENT:           gen_postfix(ast, "sub");         break;
        case AST_TYPE_PRE_INCREMENT:            gen_prefix (ast, "add");         break;
        case AST_TYPE_PRE_DECREMENT:            gen_prefix (ast, "sub");         break;
        case AST_TYPE_EXPRESSION_CAST:          gen_cast(ast);                   break;
        case AST_TYPE_STRUCT:                   gen_struct(ast);                 break;
        case '&':                               gen_bitandor(ast);               break;
        case '|':                               gen_bitandor(ast);               break;
        case '~':                               gen_bitnot(ast);                 break;
        case ',':                               gen_comma(ast);                  break;
        case '=':                               gen_assign(ast);                 break;
        case AST_TYPE_CONVERT:                  gen_conversion(ast);             break;
        case AST_TYPE_STATEMENT_GOTO_COMPUTED:  gen_goto_computed(ast);          break;
        case AST_TYPE_STATEMENT_LABEL_COMPUTED: gen_address_label(ast);          break;
        default:
            gen_binary(ast);
    }
}

void gen_toplevel(ast_t *ast) {
    gen_function(ast);
    if (ast->type == AST_TYPE_FUNCTION) {
        gen_function_prologue(ast);
        gen_expression(ast->function.body);
        gen_function_epilogue();
    } else if (ast->type == AST_TYPE_DECLARATION) {
        gen_data_global(ast);
    }
}
