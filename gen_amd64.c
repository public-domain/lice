#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "lice.h"
#include "gen.h"

#define REGISTER_AREA_SIZE     304
#define REGISTER_MULT_SIZE_XMM 8
#define REGISTER_MULT_SIZE     6

#define SRDI "rdi"
#define SRSI "rsi"
#define SRDX "rdx"
#define SRCX "rcx"
#define SR8  "r8"
#define SR9  "r9"
#define SEDI "edi"
#define SESI "esi"
#define SEDX "edx"
#define SECX "ecx"
#define SR8D "r8d"
#define SR9D "r9d"
#define SDIL "dil"
#define SSIL "sil"
#define SDL  "dl"
#define SCL  "cl"
#define SR8B "r8b"
#define SR9B "r9b"
#define SRAX "rax"
#define SRBX "rbx"
#define SR11 "r11"

static const char *register_table[][REGISTER_MULT_SIZE] = {
    { SRDI,    SRSI,    SRDX,    SRCX,    SR8,     SR9  },
    { SEDI,    SESI,    SEDX,    SECX,    SR8D,    SR9D },
    { SDIL,    SSIL,    SDL,     SCL,     SR8B,    SR9B }
};

#define NREG(I) register_table[0][I]
#define SREG(I) register_table[1][I]
#define MREG(I) register_table[2][I]

static int stack = 0;
static int gp    = 0;
static int fp    = 0;

static void gen_push(const char *reg) {
    gen_emit("push %%%s", reg);
    stack += 8;
}
static void gen_pop(const char *reg) {
    gen_emit("pop %%%s", reg);
    stack -= 8;
}
static void gen_push_xmm(int r) {
    gen_emit("sub $8, %%rsp");
    gen_emit("movsd %%xmm%d, (%%rsp)", r);
    stack += 8;
}
static void gen_pop_xmm(int r) {
    gen_emit("movsd (%%rsp), %%xmm%d", r);
    gen_emit("add $8, %%rsp");
    stack -= 8;
}

/*
 * Technically not the safest, but also can't legally be optimized with
 * strict aliasing optimizations. Volatile will mark the construction
 * of the literal from being directly delt with in the optimizer. Plus
 * aliasing though the use of a union, while it isn't technically legal,
 * all compilers do deal with it to some extent. Restrict on want will
 * prevent the compiler from emitting two loads for the same address, since
 * it is likely already in an register.
 */
#define TYPEPUN(TYPE, VALUE) \
    *(((volatile union { __typeof__(VALUE) *have; TYPE *restrict want; }) { &(VALUE) }).want)

static void *gen_mapping_table(const void **table, size_t index, size_t length, const char *func) {
    const unsigned char **ptr = (const unsigned char **)table;
    const unsigned char **end = &ptr[length];
    const unsigned char **ret = &ptr[index];

    if (ret < ptr || ret >= end || !*ret)
        compile_ice("gen_mapping_table from %s (index: %zu, length: %zu)", func, index, length);

    return *((void **)ret);
}

#define gen_mapping(TABLE, INDEX, LENGTH) \
    gen_mapping_table((const void **)(TABLE), (INDEX), (LENGTH), __func__)

static const char *gen_register_integer(data_type_t *type, char r) {
    static const char *items[] = {
        "cl", "cx", 0, "ecx", 0, 0, 0, "rcx",
        "al", "ax", 0, "eax", 0, 0, 0, "rax"
    };
    static const size_t length = sizeof(items)/sizeof(*items);
    return gen_mapping(items, (type->size - 1) + !!(r == 'a') * 8, length);
}

static const char *gen_load_instruction(data_type_t *type) {
    static const char *items[] = {
        "movsbq", "movswq", 0,
        "movslq", 0,        0, 0,
        "mov"
    };
    return gen_mapping(items, type->size - 1, sizeof(items)/sizeof(*items));
}

static void gen_shift_load(data_type_t *type) {
    if (type->bitfield.size <= 0)
        return;
    gen_emit("shr $%d, %%rax", type->bitfield.offset);
    gen_push(SRCX);
    gen_emit("mov $0x%" PRIx64 ", %%rcx", (1 << (uint64_t)type->bitfield.size) - 1);
    gen_emit("and %%rcx, %%rax");
    gen_pop(SRCX);
}

static void gen_shift_save(data_type_t *type, char *address) {
    if (type->bitfield.size <= 0)
        return;
    gen_push(SRCX);
    gen_push(SRDI);

    gen_emit("mov $0x%" PRIx64 ", %%rdi", (1 << (uint64_t)type->bitfield.size) - 1);
    gen_emit("and %%rdi, %%rax");
    gen_emit("shl $%d, %%rax", type->bitfield.offset);
    gen_emit("mov %s, %%%s", address, gen_register_integer(type, 'c'));
    gen_emit("mov $0x%" PRIx64 ", %%rdi", ~(((1 << (uint64_t)type->bitfield.size) - 1) << type->bitfield.offset));
    gen_emit("and %%rdi, %%rcx");
    gen_emit("or %%rcx, %%rax");

    gen_pop(SRDI);
    gen_pop(SRCX);
}

static void gen_load_global(data_type_t *type, char *label, int offset) {
    if (type->type == TYPE_ARRAY) {
        if (offset)
            gen_emit("lea %s+%d(%%rip), %%rax", label, offset);
        else
            gen_emit("lea %s(%%rip), %%rax", label);
        return;
    }
    gen_emit("%s %s+%d(%%rip), %%rax", gen_load_instruction(type), label, offset);
    gen_shift_load(type);
}

static void gen_cast_int(data_type_t *type) {
    if (type->type == TYPE_FLOAT)
        gen_emit("cvttss2si %%xmm0, %%eax");
    else if (type->type == TYPE_DOUBLE)
        gen_emit("cvttsd2si %%xmm0, %%eax");
}

static void gen_cast_bool(data_type_t *type) {
    if (ast_type_isfloating(type)) {
        gen_push_xmm(1);
        gen_emit("xorpd %%xmm1, %%xmm1");
        gen_emit("ucomisd %%xmm1, %%xmm0");
        gen_emit("setne %%al");
        gen_pop_xmm(1);
    } else {
        gen_emit("cmp $0, %%rax");
        gen_emit("setne %%al");
    }
    gen_emit("movzb %%al, %%eax");
}

static void gen_load_local(data_type_t *var, const char *base, int offset) {
    if (var->type == TYPE_ARRAY) {
        gen_emit("lea %d(%%%s), %%rax", offset, base);
    } else if (var->type == TYPE_FLOAT) {
        gen_emit("movss %d(%%%s), %%xmm0", offset, base);
    } else if (var->type == TYPE_DOUBLE || var->type == TYPE_LDOUBLE) {
        gen_emit("movsd %d(%%%s), %%xmm0", offset, base);
    } else {
        gen_emit("%s %d(%%%s), %%rax", gen_load_instruction(var), offset, base);
        gen_shift_load(var);
    }
}

void gen_boolean_maybe(data_type_t *type) {
    if (type->type != TYPE_BOOL)
        return;

    gen_emit("test %%rax, %%rax");
    gen_emit("setne %%al");
}

static void gen_save_global(char *name, data_type_t *type, int offset) {
    gen_boolean_maybe(type);

    const char *reg = gen_register_integer(type, 'a');
    string_t   *str = string_create();

    if (offset != 0)
        string_catf(str, "%s+%d(%%rip)", name, offset);
    else
        string_catf(str, "%s(%%rip)", name);

    gen_shift_save(type, string_buffer(str));
    gen_emit("mov %%%s, %s", reg, string_buffer(str));
}

void gen_save_local(data_type_t *type, int offset) {
    if (type->type == TYPE_FLOAT)
        gen_emit("movss %%xmm0, %d(%%rbp)", offset);
    else if (type->type == TYPE_DOUBLE || type->type == TYPE_LDOUBLE)
        gen_emit("movsd %%xmm0, %d(%%rbp)", offset);
    else {
        gen_boolean_maybe(type);

        string_t   *str = string_create();
        const char *reg = gen_register_integer(type, 'a');

        if (offset != 0)
            string_catf(str, "%d(%%rbp)", offset);
        else
            string_catf(str, "(%%rbp)");

        gen_shift_save(type, string_buffer(str));
        gen_emit("mov %%%s, %s", reg, string_buffer(str));
    }
}

static void gen_assignment_dereference_intermediate(data_type_t *type, int offset) {
    gen_emit("mov (%%rsp), %%rcx");

    const char *reg = gen_register_integer(type, 'c');
    if (offset)
        gen_emit("mov %%%s, %d(%%rax)", reg, offset);
    else
        gen_emit("mov %%%s, (%%rax)", reg);
    gen_pop(SRAX);
}

void gen_address(ast_t *ast) {
    switch (ast->type) {
        case AST_TYPE_VAR_LOCAL:
            gen_emit("lea %d(%%rbp), %%rax", ast->variable.off);
            break;
        case AST_TYPE_VAR_GLOBAL:
            gen_emit("lea %s(%%rip), %%rax", ast->variable.label);
            break;
        case AST_TYPE_DEREFERENCE:
            gen_expression(ast->unary.operand);
            break;
        case AST_TYPE_STRUCT:
            gen_address(ast->structure);
            gen_emit("add $%d, %%rax", ast->ctype->offset);
            break;
        default:
            compile_ice("gen_address (%s)", ast_type_string(ast->ctype));
    }
}

void gen_address_label(ast_t *ast) {
    gen_emit("mov $%s, %%rax", ast->gotostmt.where);
}

void gen_goto_computed(ast_t *ast) {
    gen_expression(ast->unary.operand);
    gen_emit("jmp *%%rax");
}

static void gen_structure_copy(int size, const char *base) {
    int i = 0;
    for (; i < size; i += 8) {
        gen_emit("movq %d(%%rcx), %%r11", i);
        gen_emit("movq %%r11, %d(%%%s)", i, base);
    }

    for (; i < size; i += 4) {
        gen_emit("movl %d(%%rcx), %%r11", i);
        gen_emit("movl %%r11d, %d(%%%s)", i, base);
    }

    for (; i < size; i++) {
        gen_emit("movb %d(%%rcx), %%r11", i);
        gen_emit("movb %%r11b, %d(%%%s)", i, base);
    }
}

static void gen_structure_assign(ast_t *left, ast_t *right) {
    gen_push(SRCX);
    gen_push(SR11);
    gen_address(right);
    gen_emit("mov %%rax, %%rcx");
    gen_address(left);
    gen_structure_copy(left->ctype->size, "rax");
    gen_pop(SR11);
    gen_pop(SRCX);
}

static int gen_alignment(int n, int align) {
    int remainder = n % align;
    return (remainder == 0)
                ? n
                : n - remainder + align;
}

static int gen_structure_push(int size) {
    compile_error("cannot pass structure of size: %d bytes by copy (unimplemented)", size);
}

void gen_zero(int start, int end) {
    for (; start <= end - 8; start += 8)
      gen_emit("movq $0, %d(%%rbp)", start);
    for (; start <= end - 4; start += 4)
        gen_emit("movl $0, %d(%%rbp)", start);
    for (; start < end; start ++)
        gen_emit("movb $0, %d(%%rbp)", start);
}

static void gen_assignment_dereference(ast_t *var) {
    gen_push(SRAX);
    gen_expression(var->unary.operand);
    gen_assignment_dereference_intermediate(var->unary.operand->ctype->pointer, 0);
}

static void gen_pointer_arithmetic(char op, ast_t *left, ast_t *right) {
    gen_expression(left);
    gen_push(SRCX);
    gen_push(SRAX);
    gen_expression(right);

    int size = left->ctype->pointer->size;
    if (size > 1)
        gen_emit("imul $%d, %%rax", size);

    gen_emit("mov %%rax, %%rcx");
    gen_pop(SRAX);

    switch (op) {
        case '+': gen_emit("add %%rcx, %%rax"); break;
        case '-': gen_emit("sub %%rcx, %%rax"); break;
    }
    gen_pop(SRCX);
}

static void gen_assignment_structure(ast_t *structure, data_type_t *field, int offset) {
    switch (structure->type) {
        case AST_TYPE_VAR_LOCAL:
            gen_ensure_lva(structure);
            gen_save_local(field, structure->variable.off + field->offset + offset);
            break;
        case AST_TYPE_VAR_GLOBAL:
            gen_save_global(structure->variable.name, field, field->offset + offset);
            break;
        case AST_TYPE_STRUCT:
            gen_assignment_structure(structure->structure, field, offset + structure->ctype->offset);
            break;
        case AST_TYPE_DEREFERENCE:
            gen_push(SRAX);
            gen_expression(structure->unary.operand);
            gen_assignment_dereference_intermediate(field, field->offset + offset);
            break;
        default:
            compile_ice("gen_assignment_structure");
            break;
    }
}

static void gen_load_structure(ast_t *structure, data_type_t *field, int offset) {
    switch (structure->type) {
        case AST_TYPE_VAR_LOCAL:
            gen_ensure_lva(structure);
            gen_load_local(field, "rbp", structure->variable.off + field->offset + offset);
            break;
        case AST_TYPE_VAR_GLOBAL:
            gen_load_global(field, structure->variable.name, field->offset + offset);
            break;
        case AST_TYPE_STRUCT:
            gen_load_structure(structure->structure, field, structure->ctype->offset + offset);
            break;
        case AST_TYPE_DEREFERENCE:
            gen_expression(structure->unary.operand);
            gen_load_local(field, SRAX, field->offset + offset);
            break;
        default:
            compile_ice("gen_assignment_structure");
            break;
    }
}

static void gen_store(ast_t *var) {
    switch (var->type) {
        case AST_TYPE_DEREFERENCE:
            gen_assignment_dereference(var);
            break;
        case AST_TYPE_STRUCT:
            gen_assignment_structure(var->structure, var->ctype, 0);
            break;
        case AST_TYPE_VAR_LOCAL:
            gen_ensure_lva(var);
            gen_save_local(var->ctype, var->variable.off);
            break;
        case AST_TYPE_VAR_GLOBAL:
            gen_save_global(var->variable.name, var->ctype, 0);
            break;
        default:
            compile_ice("gen_assignment");
    }
}

static void gen_comparision(char *operation, ast_t *ast) {
    if (ast_type_isfloating(ast->left->ctype)) {
        gen_expression(ast->left);
        gen_push_xmm(0);
        gen_expression(ast->right);
        gen_pop_xmm(1);
        if (ast->left->ctype->type == TYPE_FLOAT)
            gen_emit("ucomiss %%xmm0, %%xmm1");
        else
            gen_emit("ucomisd %%xmm0, %%xmm1");
    } else {
        gen_expression(ast->left);
        gen_push(SRAX);
        gen_expression(ast->right);
        gen_pop(SRCX);

        int type = ast->left->ctype->type;
        if (type == TYPE_LONG || type == TYPE_LLONG)
            gen_emit("cmp %%rax, %%rcx");
        else
            gen_emit("cmp %%eax, %%ecx");
    }
    gen_emit("%s %%al", operation);
    gen_emit("movzb %%al, %%eax");
}

static const char *gen_binary_instruction(ast_t *ast) {
    string_t *string = string_create();
    if (ast_type_isfloating(ast->ctype)) {
        switch (ast->type) {
            case '+': string_catf(string, "adds"); break;
            case '-': string_catf(string, "subs"); break;
            case '*': string_catf(string, "muls"); break;
            case '/': string_catf(string, "divs"); break;
        }
        if (ast->ctype->type == TYPE_DOUBLE || ast->ctype->type == TYPE_LDOUBLE)
            string_cat(string, 'd');
        else
            string_cat(string, 's');
        if (!string_length(string))
            goto error;

        return string_buffer(string);
    }
    /* integer */
    switch (ast->type) {
        case '+':              string_catf(string, "add");  break;
        case '-':              string_catf(string, "sub");  break;
        case '*':              string_catf(string, "imul"); break;
        case '^':              string_catf(string, "xor");  break;
        case AST_TYPE_LSHIFT:  string_catf(string, "sal");  break;
        case AST_TYPE_RSHIFT:  string_catf(string, "sar");  break;
        case AST_TYPE_LRSHIFT: string_catf(string, "shr");  break;

        /* need to be handled specially */
        case '/': return "@/";
        case '%': return "@%";
    }
    return string_buffer(string);
error:
    compile_ice("gen_binary_instruction");
}

static void gen_binary_arithmetic_integer(ast_t *ast) {
    const char *op = gen_binary_instruction(ast);
    gen_expression(ast->left);
    gen_push(SRAX);
    gen_expression(ast->right);
    gen_emit("mov %%rax, %%rcx");
    gen_pop(SRAX);

    if (*op == '@') {
        gen_emit("cqto");
        gen_emit("idiv %%rcx");
        if (op[1] == '%')
            gen_emit("mov %%edx, %%eax");
    } else if (ast->type == AST_TYPE_LSHIFT
        ||     ast->type == AST_TYPE_RSHIFT
        ||     ast->type == AST_TYPE_LRSHIFT
    ) {
        gen_emit("%s %%cl, %%%s", op, gen_register_integer(ast->left->ctype, 'a'));
    } else {
        gen_emit("%s %%rcx, %%rax", op);
    }
}

static void gen_binary_arithmetic_floating(ast_t *ast) {
    const char *op = gen_binary_instruction(ast);
    gen_expression(ast->left);
    gen_push_xmm(0);
    gen_expression(ast->right);
    if (ast->ctype->type == TYPE_DOUBLE)
        gen_emit("movsd %%xmm0, %%xmm1");
    else
        gen_emit("movss %%xmm0, %%xmm1");
    gen_pop_xmm(0);
    gen_emit("%s %%xmm1, %%xmm0", op);
}

void gen_load_convert(data_type_t *to, data_type_t *from) {
    if (ast_type_isinteger(from) && to->type == TYPE_FLOAT)
        gen_emit("cvtsi2ss %%eax, %%xmm0");
    else if (ast_type_isinteger(from) && to->type == TYPE_DOUBLE)
        gen_emit("cvtsi2sd %%eax, %%xmm0");
    else if (from->type == TYPE_FLOAT && to->type == TYPE_DOUBLE)
        gen_emit("cvtps2pd %%xmm0, %%xmm0");
    else if (from->type == TYPE_DOUBLE && to->type == TYPE_FLOAT)
        gen_emit("cvtpd2ps %%xmm0, %%xmm0");
    else if (to->type == TYPE_BOOL)
        gen_cast_bool(from);
    else if (ast_type_isinteger(to))
        gen_cast_int(from);
}

void gen_conversion(ast_t *ast) {
    gen_expression(ast->unary.operand);
    gen_load_convert(ast->ctype, ast->unary.operand->ctype);
}

void gen_binary(ast_t *ast) {
    if (ast->ctype->type == TYPE_POINTER) {
        gen_pointer_arithmetic(ast->type, ast->left, ast->right);
        return;
    }

    switch (ast->type) {
        case '<':             gen_comparision("setl",  ast); return;
        case '>':             gen_comparision("setg",  ast); return;
        case AST_TYPE_EQUAL:  gen_comparision("sete",  ast); return;
        case AST_TYPE_GEQUAL: gen_comparision("setge", ast); return;
        case AST_TYPE_LEQUAL: gen_comparision("setle", ast); return;
        case AST_TYPE_NEQUAL: gen_comparision("setne", ast); return;
    }

    if (ast_type_isinteger(ast->ctype))
        gen_binary_arithmetic_integer(ast);
    else if (ast_type_isfloating(ast->ctype))
        gen_binary_arithmetic_floating(ast);
    else
        compile_ice("gen_binary");
}

void gen_literal_save(ast_t *ast, data_type_t *type, int offset) {
    uint64_t load64  = ((uint64_t)ast->integer);
    uint32_t load32  = ast->integer;
    float    loadf32 = ast->floating.value;
    double   loadf64 = ast->floating.value;

    gen_emit("# literal save {");
    switch (type->type) {
        case TYPE_BOOL:  gen_emit("movb $%d, %d(%%rbp)", !!ast->integer, offset); break;
        case TYPE_CHAR:  gen_emit("movb $%d, %d(%%rbp)", load32, offset); break;
        case TYPE_SHORT: gen_emit("movw $%d, %d(%%rbp)", load32, offset); break;
        case TYPE_INT:   gen_emit("movl $%d, %d(%%rbp)", load32, offset); break;
        case TYPE_LONG:
        case TYPE_LLONG:
        case TYPE_POINTER:
            gen_emit("movl $0x%" PRIx64 ", %d(%%rbp)", load64 & 0xFFFFFFFF, offset);
            gen_emit("movl $0x%" PRIx64 ", %d(%%rbp)", load64 >> 32, offset + 4);
            break;
        case TYPE_FLOAT:
            load32 = TYPEPUN(uint32_t, loadf32);
            gen_emit("movl $0x%" PRIx32 ", %d(%%rbp)", load32, offset);
            break;
        case TYPE_DOUBLE:
            load64 = TYPEPUN(uint64_t, loadf64);
            gen_emit("movl $0x%" PRIx64 ", %d(%%rbp)", load64 & 0xFFFFFFFF, offset);
            gen_emit("movl $0x%" PRIx64 ", %d(%%rbp)", load64 >> 32, offset + 4);
            break;

        default:
            compile_ice("gen_literal_save");
    }
    gen_emit("# }");
}

void gen_prefix(ast_t *ast, const char *op) {
    gen_expression(ast->unary.operand);
    if (ast->ctype->type == TYPE_POINTER)
        gen_emit("%s $%d, %%rax", op, ast->ctype->pointer->size);
    else
        gen_emit("%s $1, %%rax", op);
    gen_store(ast->unary.operand);
}

void gen_postfix(ast_t *ast, const char *op) {
    gen_expression(ast->unary.operand);
    gen_push(SRAX);
    if (ast->ctype->type == TYPE_POINTER)
        gen_emit("%s $%d, %%rax", op, ast->ctype->pointer->size);
    else
        gen_emit("%s $1, %%rax", op);
    gen_store(ast->unary.operand);
    gen_pop(SRAX);
}

static void gen_register_area_calculate(list_t *args) {
    gp = 0;
    fp = 0;
    for (list_iterator_t *it = list_iterator(args); !list_iterator_end(it); )
        (*((ast_type_isfloating(((ast_t*)list_iterator_next(it))->ctype)) ? &fp : &gp)) ++;
}

void gen_je(const char *label) {
    gen_emit("test %%rax, %%rax");
    gen_emit("je %s", label);
}

void gen_cast(ast_t *ast) {
    gen_expression(ast->unary.operand);
    gen_load_convert(ast->ctype, ast->unary.operand->ctype);
}

void gen_literal(ast_t *ast) {
    switch (ast->ctype->type) {
        case TYPE_CHAR:
        case TYPE_BOOL:
            gen_emit("mov $%d, %%rax", ast->integer);
            break;
        case TYPE_INT:
            gen_emit("mov $%d, %%rax", ast->integer);
            break;
        case TYPE_LONG:
        case TYPE_LLONG:
            gen_emit("mov $%" PRIi64 ", %%rax", (uint64_t)ast->integer);
            break;

        case TYPE_FLOAT:
            if (!ast->floating.label) {
                ast->floating.label = ast_label();
                float  fval = ast->floating.value;
                int   *iptr = (int*)&fval;
                gen_emit_inline(".data");
                gen_label(ast->floating.label);
                gen_emit(".long %d", *iptr);
                gen_emit_inline(".text");
            }
            gen_emit("movss %s(%%rip), %%xmm0", ast->floating.label);
            break;

        case TYPE_DOUBLE:
        case TYPE_LDOUBLE:
            if (!ast->floating.label) {
                ast->floating.label = ast_label();
                double dval = ast->floating.value;
                int   *iptr = (int*)&dval;
                gen_emit_inline(".data");
                gen_label(ast->floating.label);
                gen_emit(".long %d", iptr[0]);
                gen_emit(".long %d", iptr[1]);
                gen_emit_inline(".text");
            }
            gen_emit("movsd %s(%%rip), %%xmm0", ast->floating.label);
            break;

        default:
            compile_ice("gen_expression (%s)", ast_type_string(ast->ctype));
    }
}

void gen_literal_string(ast_t *ast) {
    if (!ast->string.label) {
        ast->string.label = ast_label();
        gen_emit_inline(".data");
        gen_label(ast->string.label);
        gen_emit(".string \"%s\"", string_quote(ast->string.data));
        gen_emit_inline(".text");
    }
    gen_emit("lea %s(%%rip), %%rax", ast->string.label);
}

void gen_variable_local(ast_t *ast) {
    gen_ensure_lva(ast);
    gen_load_local(ast->ctype, "rbp", ast->variable.off);
}

void gen_variable_global(ast_t *ast) {
    gen_load_global(ast->ctype, ast->variable.label, 0);
}

void gen_dereference(ast_t *ast) {
    gen_expression(ast->unary.operand);
    gen_load_local(ast->unary.operand->ctype->pointer, SRAX, 0);
    gen_load_convert(ast->ctype, ast->unary.operand->ctype->pointer);
}

static void gen_function_args_classify(list_t *i, list_t *f, list_t *r, list_t *a) {
    int ir = 0;
    int xr = 0;
    int mi = REGISTER_MULT_SIZE;
    int mx = REGISTER_MULT_SIZE_XMM;

    list_iterator_t *it = list_iterator(a);
    while (!list_iterator_end(it)) {
        ast_t *value = list_iterator_next(it);
        if (value->ctype->type == TYPE_STRUCTURE)
            list_push(r, value);
        else if (ast_type_isfloating(value->ctype))
            list_push((xr++ < mx) ? f : r, value);
        else
            list_push((ir++ < mi) ? i : r, value);
    }
}

static void gen_function_args_save(int in, int fl) {
    gen_emit("# function args save {");
    for (int i = 0; i < in; i++)      gen_push(NREG(i));
    for (int i = 1; i < fl; i++)      gen_push_xmm(i);
    gen_emit("# }");
}
static void gen_function_args_restore(int in, int fl) {
    gen_emit("# function args restore {");
    for (int i = fl - 1; i >  0; i--) gen_pop_xmm(i);
    for (int i = in - 1; i >= 0; i--) gen_pop(NREG(i));
    gen_emit("# }");
}
static void gen_function_args_popi(int l) {
    gen_emit("# function args pop {");
    for (int i = l - 1; i >= 0; i--)  gen_pop(NREG(i));
    gen_emit("# }");
}
static void gen_function_args_popf(int l) {
    gen_emit("# function args pop (xmm registers) {");
    for (int i = l - 1; i >= 0; i--)  gen_pop_xmm(i);
    gen_emit("# }");
}

static int gen_function_args(list_t *args) {
    gen_emit("# functiona arguments { ");
    int rest = 0;
    list_iterator_t *it = list_iterator(args);
    while (!list_iterator_end(it)) {
        ast_t *value = list_iterator_next(it);
        if (value->ctype->type == TYPE_STRUCTURE) {
            gen_address(value);
            rest += gen_structure_push(value->ctype->size);
        } else if (ast_type_isfloating(value->ctype)) {
            gen_expression(value);
            gen_push_xmm(0);
            rest += 8;
        } else {
            gen_expression(value);
            gen_push(SRAX);
            rest += 8;
        }
    }
    gen_emit("# } ");
    return rest;
}

static void gen_function_call_default(ast_t *ast) {
    int          save = stack;
    bool         fptr = (ast->type == AST_TYPE_POINTERCALL);
    data_type_t *type = fptr ? ast->function.call.functionpointer->ctype->pointer
                             : ast->function.call.type;

    gen_emit("# function call {");

    /* deal with arguments */
    list_t *in = list_create();
    list_t *fl = list_create();
    list_t *re = list_create();

    gen_function_args_classify(in, fl, re, ast->function.call.args);
    gen_function_args_save(list_length(in), list_length(fl));

    bool algn = stack % 16;
    if (algn) {
        gen_emit("sub $8, %%rsp");
        stack += 8;
    }

    int rest = gen_function_args(list_reverse(re));

    if (fptr) {
        gen_expression(ast->function.call.functionpointer);
        gen_push(SRAX);
    }

    gen_function_args(in);
    gen_function_args(fl);
    gen_function_args_popf(list_length(fl));
    gen_function_args_popi(list_length(in));

    if (fptr)
        gen_pop(SR11);

    if (type->hasdots)
        gen_emit("mov $%d, %%eax", list_length(fl));

    if (fptr)
        gen_emit("call *%%r11");
    else
        gen_emit("call %s", ast->function.name);

    gen_boolean_maybe(ast->ctype);

    if (rest > 0) {
        gen_emit("add $%d, %%rsp", rest);
        stack -= rest;
    }

    if (algn) {
        gen_emit("add $8, %%rsp");
        stack -= 8;
    }

    gen_function_args_restore(list_length(in), list_length(fl));

    gen_emit("# }");

    if (stack != save)
        compile_ice("gen_function_call (stack out of alignment)");
}

void gen_function_call(ast_t *ast) {
    char *loopbeg;
    char *loopend;

    if (!ast->function.name || strcmp(ast->function.name, "__builtin_return_address")) {
        gen_function_call_default(ast);
        return;
    }

    /*
     * deal with builtin return address extension. This should be
     * as easy as emitting the expression for the return address
     * argument and using some loops.
     */
    gen_push(SR11);
    gen_expression(list_head(ast->function.call.args));
    loopbeg = ast_label();
    loopend = ast_label();
    gen_emit("mov %%rbp, %%r11");
    gen_label(loopbeg);
    gen_emit("test %%rax, %%rax");
    gen_emit("jz %s", loopend);
    gen_emit("mov (%%r11), %%r11");
    gen_emit("dec %%rax");
    gen_jump(loopbeg);
    gen_label(loopend);
    gen_emit("mov 8(%%r11), %%rax");
    gen_pop(SR11);
}

void gen_case(ast_t *ast) {
    char *skip;
    gen_jump((skip = ast_label()));
    gen_label(gen_label_switch);
    gen_label_switch = ast_label();
    gen_emit("cmp $%d, %%eax", ast->casebeg);
    if (ast->casebeg == ast->caseend)
        gen_emit("jne %s", gen_label_switch);
    else {
        gen_emit("jl %s", gen_label_switch);
        gen_emit("cmp $%d, %%eax", ast->caseend);
        gen_emit("jg %s", gen_label_switch);
    }
    gen_label(skip);
}

void gen_va_start(ast_t *ast) {
    gen_expression(ast->ap);
    gen_push(SRCX);
    gen_emit("movl $%d, (%%rax)", gp * 8);
    gen_emit("movl $%d, 4(%%rax)", 48 + fp * 16);
    gen_emit("lea %d(%%rbp), %%rcx", -REGISTER_AREA_SIZE);
    gen_emit("mov %%rcx, 16(%%rax)");
    gen_pop(SRCX);
}

void gen_va_arg(ast_t *ast) {
    gen_expression(ast->ap);
    gen_emit("nop");
    gen_push(SRCX);
    gen_push("rbx");
    gen_emit("mov 16(%%rax), %%rcx");
    if (ast_type_isfloating(ast->ctype)) {
        gen_emit("mov 4(%%rax), %%ebx");
        gen_emit("add %%rbx, %%rcx");
        gen_emit("add $16, %%ebx");
        gen_emit("mov %%ebx, 4(%%rax)");
        gen_emit("movsd (%%rcx), %%xmm0");
        if (ast->ctype->type == TYPE_FLOAT)
            gen_emit("cvtpd2ps %%xmm0, %%xmm0");
    } else {
        gen_emit("mov (%%rax), %%ebx");
        gen_emit("add %%rbx, %%rcx");
        gen_emit("add $8, %%ebx");
        gen_emit("mov %%rbx, (%%rax)");
        gen_emit("mov (%%rcx), %%rax");
    }
    gen_pop(SRBX);
    gen_pop(SRCX);
}

void gen_not(ast_t *ast) {
    gen_expression(ast->unary.operand);
    gen_emit("cmp $0, %%rax");
    gen_emit("sete %%al");
    gen_emit("movzb %%al, %%eax");
}

void gen_and(ast_t *ast) {
    char *end = ast_label();
    gen_expression(ast->left);
    gen_emit("test %%rax, %%rax");
    gen_emit("mov $0, %%rax");
    gen_emit("je %s", end);
    gen_expression(ast->right);
    gen_emit("test %%rax, %%rax");
    gen_emit("mov $0, %%rax");
    gen_emit("je %s", end);
    gen_emit("mov $1, %%rax");
    gen_label(end);
}

void gen_or(ast_t *ast) {
    char *end = ast_label();
    gen_expression(ast->left);
    gen_emit("test %%rax, %%rax");
    gen_emit("mov $1, %%rax");
    gen_emit("jne %s", end);
    gen_expression(ast->right);
    gen_emit("test %%rax, %%rax");
    gen_emit("mov $1, %%rax");
    gen_emit("jne %s", end);
    gen_emit("mov $0, %%rax");
    gen_label(end);
}

void gen_struct(ast_t *ast) {
    gen_load_structure(ast->structure, ast->ctype, 0);
}

void gen_bitandor(ast_t *ast) {
    static const char *instruction[] = { "and", "or" };
    gen_expression(ast->left);
    gen_push(SRAX);
    gen_expression(ast->right);
    gen_pop(SRCX);
    gen_emit("%s %%rcx, %%rax", instruction[!!(ast->type == '|')]);
}

void gen_bitnot(ast_t *ast) {
    gen_expression(ast->left);
    gen_emit("not %%rax");
}

void gen_negate(ast_t *ast) {
    gen_expression(ast->unary.operand);
    if (ast_type_isfloating(ast->ctype)) {
        gen_push_xmm(1);
        gen_emit("xorpd %%xmm1, %%xmm1");
        if (ast->ctype->type == TYPE_DOUBLE)
            gen_emit("subsd %%xmm1, %%xmm0");
        else
            gen_emit("subss %%xmm1, %%xmm0");
        gen_pop_xmm(1);
        return;
    }
    gen_emit("neg %%rax");
}

void gen_assign(ast_t *ast) {
    if (ast->left->ctype->type == TYPE_STRUCTURE) {
        if (ast->left->ctype->size > 8) {
            gen_structure_assign(ast->left, ast->right);
            return;
        }
    }
    gen_expression(ast->right);
    gen_load_convert(ast->ctype, ast->right->ctype);
    gen_store(ast->left);
}

int parse_evaluate(ast_t *ast);
static void gen_data_zero(int size) {
    for (; size >= 8; size -= 8) gen_emit(".quad 0");
    for (; size >= 4; size -= 4) gen_emit(".long 0");
    for (; size >  0; size --)   gen_emit(".byte 0");
}

static void gen_data_padding(ast_t *ast, int offset) {
    int d = ast->init.offset - offset;
    if (d < 0)
        compile_ice("gen_data_padding");
    gen_data_zero(d);
}

static void gen_data_intermediate(list_t *inits, int size, int offset, int depth) {
    uint64_t load64;
    uint32_t load32;

    list_iterator_t *it = list_iterator(inits);
    while (!list_iterator_end(it) && 0 < size) {
        ast_t *node = list_iterator_next(it);
        ast_t *v    = node->init.value;

        gen_data_padding(node, offset);
        offset += node->init.type->size;
        size   -= node->init.type->size;

        if (v->type == AST_TYPE_ADDRESS) {
            char *label;
            switch (v->unary.operand->type) {
                case AST_TYPE_VAR_LOCAL:
                    label  = ast_label();
                    gen_emit(".data %d", depth + 1);
                    gen_label(label);
                    gen_data_intermediate(v->unary.operand->variable.init, v->unary.operand->ctype->size, 0, depth + 1);
                    gen_emit(".data %d", depth);
                    gen_emit(".quad %s", label);
                    continue;

                case AST_TYPE_VAR_GLOBAL:
                    gen_emit(".quad %s", v->unary.operand->variable.name);
                    continue;

                default:
                    compile_ice("gen_datat_intermediate");
            }
        }

        if (node->init.value->type == AST_TYPE_VAR_LOCAL && node->init.value->variable.init) {
            gen_data_intermediate(v->variable.init, v->ctype->size, 0, depth);
            continue;
        }

        if (v->ctype->type == TYPE_ARRAY && v->ctype->pointer->type == TYPE_CHAR) {
            char *label = ast_label();
            gen_emit(".data %d", depth + 1);
            gen_label(label);
            gen_emit(".string \"%s\"", string_quote(v->string.data));
            gen_emit(".data %d", depth);
            gen_emit(".quad %s", label);
            continue;
        }


        /* load alias */
        load32 = TYPEPUN(uint32_t, node->init.value->floating.value);
        load64 = TYPEPUN(uint64_t, node->init.value->floating.value);

        switch (node->init.type->type) {
            case TYPE_FLOAT:   gen_emit(".long 0x%"  PRIx32, load32); break;
            case TYPE_DOUBLE:  gen_emit(".quad 0x%"  PRIx64, load64); break;
            case TYPE_CHAR:    gen_emit(".byte %d",  parse_evaluate(node->init.value)); break;
            case TYPE_SHORT:   gen_emit(".short %d", parse_evaluate(node->init.value)); break;
            case TYPE_INT:     gen_emit(".long %d",  parse_evaluate(node->init.value)); break;

            case TYPE_LONG:
            case TYPE_LLONG:
            case TYPE_POINTER:
                if (node->init.value->type == AST_TYPE_VAR_GLOBAL)
                    gen_emit(".quad %s", node->init.value->variable.name);
                else
                    gen_emit(".quad %ld", parse_evaluate(node->init.value));
                break;

            default:
                compile_ice("gen_data_intermediate (%s)", ast_type_string(node->init.type));
        }
    }
    gen_data_zero(size);
}

void gen_data(ast_t *ast, int offset, int depth) {
    gen_emit(".data %d", depth);
    if (!ast->decl.var->ctype->isstatic)
        gen_emit_inline(".global %s", ast->decl.var->variable.name);
    gen_emit_inline("%s:", ast->decl.var->variable.name);
    gen_data_intermediate(ast->decl.init, ast->decl.var->ctype->size, offset, depth);
}

static int gen_register_area(void) {
    int top = -REGISTER_AREA_SIZE;
    gen_emit("mov %%rdi, %d(%%rsp)", top);
    gen_emit("mov %%rsi, %d(%%rsp)", (top += 8));
    gen_emit("mov %%rdx, %d(%%rsp)", (top += 8));
    gen_emit("mov %%rcx, %d(%%rsp)", (top += 8));
    gen_emit("mov %%r8,  %d(%%rsp)", (top += 8));
    gen_emit("mov %%r9,  %d(%%rsp)", top + 8);

    char *end = ast_label();
    for (int i = 0; i < 16; i++) {
        gen_emit("test %%al, %%al");
        gen_emit("jz %s", end);
        gen_emit("movsd %%xmm%d, %d(%%rsp)", i, (top += 16));
        gen_emit("sub $1, %%al");
    }
    gen_label(end);
    gen_emit("sub $%d, %%rsp", REGISTER_AREA_SIZE);
    return REGISTER_AREA_SIZE;
}

static void gen_function_parameters(list_t *parameters, int offset) {
    gen_emit("# function parameters { ");
    int ir = 0;
    int xr = 0;
    int ar = REGISTER_MULT_SIZE_XMM - REGISTER_MULT_SIZE;

    for (list_iterator_t *it = list_iterator(parameters); !list_iterator_end(it); ) {
        ast_t *value = list_iterator_next(it);
        if (value->ctype->type == TYPE_STRUCTURE) {
            gen_emit("lea %d(%%rbp), %%rax", ar * 8);
            int emit = gen_structure_push(value->ctype->size);
            offset -= emit;
            ar += emit / 8;
        } else if (ast_type_isfloating(value->ctype)) {
            if (xr >= REGISTER_MULT_SIZE_XMM) {
                gen_emit("mov %d(%%rbp), %%rax", ar++ * 8);
                gen_push(SRAX);
            } else {
                gen_push_xmm(xr++);
            }
            offset -= 8;
        } else {
            if (ir >= REGISTER_MULT_SIZE) {
                if (value->ctype->type == TYPE_BOOL) {
                    gen_emit("mov %d(%%rbp), %%al", ar++ * 8);
                    gen_emit("movzb %%al, %%eax");
                } else {
                    gen_emit("mov %d(%%rbp), %%al", ar++ * 8);
                }
                gen_push(SRAX);
            } else {
                if (value->ctype->type == TYPE_BOOL)
                    gen_emit("movsb %%%s, %%%s", SREG(ir), MREG(ir));
                gen_push(NREG(ir++));
            }
            offset -= 8;
        }
        value->variable.off = offset;
    }
    gen_emit("# }");
}

void gen_function_prologue(ast_t *ast) {
    gen_emit("# function prologue {");
    gen_emit_inline(".text");
    if (!ast->ctype->isstatic)
        gen_emit_inline(".global %s", ast->function.name);
    gen_emit_inline("%s:", ast->function.name);
    gen_emit("nop");
    gen_push("rbp");
    gen_emit("mov %%rsp, %%rbp");

    int offset = 0;

    if (ast->ctype->hasdots) {
        gen_register_area_calculate(ast->function.params);
        offset -= gen_register_area();
    }

    gen_function_parameters(ast->function.params, offset);
    offset -= list_length(ast->function.params) * 8;

    int localdata = 0;
    for (list_iterator_t *it = list_iterator(ast->function.locals); !list_iterator_end(it); ) {
        ast_t *value = list_iterator_next(it);
        int    align = gen_alignment(value->ctype->size, 8);

        offset -= align;
        value->variable.off = offset;
        localdata += align;
    }

    if (localdata) {
        gen_emit("sub $%d, %%rsp", localdata);
        stack += localdata;
    }
    gen_emit("# }");
}

void gen_function_epilogue(void) {
    if (stack != 0)
        gen_emit("# stack misalignment: %d\n", stack);
    gen_return();
}

void gen_return(void) {
    gen_emit("leave");
    gen_emit("ret");
}

void gen_function(ast_t *ast) {
    (void)ast;
    stack = 8;
}
