#ifndef LICE_GEN_HDR
#define LICE_GEN_HDR
#include "ast.h"

extern char *gen_label_break;
extern char *gen_label_continue;
extern char *gen_label_switch;
extern char *gen_label_break_backup;
extern char *gen_label_continue_backup;
extern char *gen_label_switch_backup;

/* emitters */
void gen_emit(const char *fmt, ...);
void gen_emit_inline(const char *fmt, ...);

/* jump */
void gen_jump(const char *label);
void gen_jump_backup(void);
void gen_jump_save(char *lbreak, char *lcontinue);
void gen_jump_restore(void);
void gen_jump_break(void);
void gen_jump_continue(void);

/* label */
void gen_label(const char *label);
void gen_label_default(void);

/* expression */
void gen_expression(ast_t *ast);

/* semantics */
void gen_ensure_lva(ast_t *ast);

/* need to implement */
void gen_return(void);
void gen_literal(ast_t *ast);
void gen_literal_string(ast_t *ast);
void gen_literal_save(ast_t *ast, data_type_t *, int);
void gen_save_local(data_type_t *type, int offset);
void gen_variable_local(ast_t *ast);
void gen_variable_global(ast_t *ast);
void gen_dereference(ast_t *ast);
void gen_address(ast_t *ast);
void gen_binary(ast_t *ast);
void gen_zero(int start, int end);
void gen_je(const char *label);
void gen_data(ast_t *, int, int);
void gen_function_call(ast_t *ast);
void gen_case(ast_t *ast);
void gen_va_start(ast_t *ast);
void gen_va_arg(ast_t *ast);
void gen_not(ast_t *ast);
void gen_and(ast_t *ast);
void gen_or(ast_t *ast);
void gen_postfix(ast_t *, const char *);
void gen_prefix(ast_t *, const char *);
void gen_cast(ast_t *ast);
void gen_struct(ast_t *);
void gen_bitandor(ast_t *ast);
void gen_bitnot(ast_t *ast);
void gen_assign(ast_t *ast);
void gen_function(ast_t *ast);
void gen_function_prologue(ast_t *ast);
void gen_function_epilogue(void);
void gen_boolean_maybe(data_type_t *);
void gen_negate(ast_t *ast);
void gen_conversion(ast_t *ast);
void gen_address_label(ast_t *ast);
void gen_goto_computed(ast_t *ast);

/* entry */
void gen_toplevel(ast_t *);
#endif
