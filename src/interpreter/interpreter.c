#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../include/interpreter.h"
#include "../../include/classfile.h"
#include "../../include/heap.h"
#include "../../include/gc.h"
#include "../../include/threads.h"

/* Sentinel pushed by getstatic for System.out (never dereferenced) */
#define SYSTEM_OUT_SENTINEL ((int64_t)0x7EFEFEFEFEFEFEFEL)

/* ── descriptor helpers ──────────────────────────────────────────────────── */

static int count_args(const char *desc) {
    if (!desc || desc[0] != '(') return 0;
    int count = 0;
    const char *p = desc + 1;
    while (*p && *p != ')') {
        if (*p == 'L') { while (*p && *p != ';') p++; }
        else if (*p == '[') { while (*p == '[') p++;
            if (*p == 'L') { while (*p && *p != ';') p++; }
        }
        count++;
        p++;
    }
    return count;
}

static int has_return_value(const char *desc) {
    if (!desc) return 0;
    const char *p = strchr(desc, ')');
    return p && *(p + 1) != 'V';
}

/* ── static field storage ────────────────────────────────────────────────── */

#define MAX_STATIC_FIELDS 64
static int64_t static_fields[MAX_STATIC_FIELDS];

/* Return 0-based slot index among instance (is_static=0) or static
   (is_static=1) fields for the field named by fieldref_idx. -1 = not found. */
static int get_field_index(const class_file *cf, uint16_t fieldref_idx, int is_static) {
    if (fieldref_idx == 0 || fieldref_idx >= cf->constant_pool_count) return -1;
    cp_info *fref = &cf->constant_pool[fieldref_idx - 1];
    if (fref->tag != CP_FIELDREF) return -1;

    uint16_t nat_idx  = (uint16_t)((fref->info[2] << 8) | fref->info[3]);
    cp_info *nat      = &cf->constant_pool[nat_idx - 1];
    uint16_t name_idx = (uint16_t)((nat->info[0] << 8) | nat->info[1]);
    const char *fname = classfile_get_utf8(cf, name_idx);
    if (!fname) return -1;

    int slot = 0;
    for (uint16_t i = 0; i < cf->fields_count; i++) {
        int field_static = (cf->fields[i].access_flags & 0x0008) != 0;
        if (field_static != is_static) continue;
        const char *fn = classfile_get_utf8(cf, cf->fields[i].name_index);
        if (fn && strcmp(fn, fname) == 0) return slot;
        slot++;
    }
    return -1;
}

/* ── class name helpers ──────────────────────────────────────────────────── */

/* Return the class name string for a CONSTANT_Methodref entry */
static const char *get_methodref_class(const class_file *cf, const cp_info *mref) {
    if (!mref || mref->tag != CP_METHODREF) return NULL;
    uint16_t ci = (uint16_t)((mref->info[0] << 8) | mref->info[1]);
    if (ci == 0 || ci > cf->constant_pool_count) return NULL;
    cp_info *cls = &cf->constant_pool[ci - 1];
    if (cls->tag != CP_CLASS) return NULL;
    uint16_t ni = (uint16_t)((cls->info[0] << 8) | cls->info[1]);
    return classfile_get_utf8(cf, ni);
}

/* Return the name of the class being executed */
static const char *get_this_class_name(const class_file *cf) {
    if (cf->this_class == 0 || cf->this_class > cf->constant_pool_count) return NULL;
    cp_info *cls = &cf->constant_pool[cf->this_class - 1];
    if (cls->tag != CP_CLASS) return NULL;
    uint16_t ni = (uint16_t)((cls->info[0] << 8) | cls->info[1]);
    return classfile_get_utf8(cf, ni);
}

/* ── thread entry ────────────────────────────────────────────────────────── */

typedef struct {
    class_file *cf;
    object     *this_obj;
} thread_args_t;

static void *thread_entry(void *arg) {
    thread_args_t *ta = (thread_args_t *)arg;
    threads_register_self();

    method_info *run_method = classfile_find_method(ta->cf, "run", "()V");
    if (run_method) {
        int64_t self_arg = (int64_t)(uintptr_t)ta->this_obj;
        interpreter_execute(ta->cf, run_method, &self_arg, 1);
    }

    threads_unregister_self();
    free(ta);
    return NULL;
}

/* ── interpreter ─────────────────────────────────────────────────────────── */

int64_t interpreter_execute(class_file *cf, method_info *method,
                             int64_t *args, uint16_t args_count) {
    if (!method || !method->code) {
        fprintf(stderr, "Error: method has no bytecode\n");
        return 0;
    }

    frame f;
    f.code        = method->code;
    f.code_length = method->code_length;
    f.pc          = 0;
    f.max_locals  = method->max_locals;
    f.max_stack   = method->max_stack;
    f.stack_top   = 0;

    f.locals        = calloc(f.max_locals ? f.max_locals : 1, sizeof(int64_t));
    f.operand_stack = calloc(f.max_stack  ? f.max_stack  : 1, sizeof(int64_t));
    if (!f.locals || !f.operand_stack) {
        free(f.locals); free(f.operand_stack);
        return 0;
    }

    for (uint16_t i = 0; i < args_count && i < f.max_locals; i++)
        f.locals[i] = args[i];

    /* Register this frame so gc_collect() can find live object references */
    int my_depth = -1;
    if (gc_call_depth < GC_MAX_CALL_DEPTH) {
        my_depth = gc_call_depth++;
        gc_call_stack[my_depth].stack         = f.operand_stack;
        gc_call_stack[my_depth].stack_top_ptr = &f.stack_top;
        gc_call_stack[my_depth].locals        = f.locals;
        gc_call_stack[my_depth].num_locals    = f.max_locals;
    }

#define PUSH(v)  (f.operand_stack[f.stack_top++] = (int64_t)(v))
#define POP()    (f.operand_stack[--f.stack_top])
#define PEEK()   (f.operand_stack[f.stack_top - 1])

    int64_t return_val = 0;

    while (f.pc < f.code_length) {
        uint8_t op = f.code[f.pc++];

        switch (op) {
        case OP_NOP:        break;
        case OP_ACONST_NULL: PUSH(0LL); break;
        case OP_ICONST_M1: PUSH(-1); break;
        case OP_ICONST_0:  PUSH(0);  break;
        case OP_ICONST_1:  PUSH(1);  break;
        case OP_ICONST_2:  PUSH(2);  break;
        case OP_ICONST_3:  PUSH(3);  break;
        case OP_ICONST_4:  PUSH(4);  break;
        case OP_ICONST_5:  PUSH(5);  break;

        case OP_BIPUSH: { int8_t v = (int8_t)f.code[f.pc++]; PUSH((int32_t)v); break; }
        case OP_SIPUSH: {
            int16_t v = (int16_t)((f.code[f.pc] << 8) | f.code[f.pc + 1]);
            f.pc += 2; PUSH((int32_t)v); break;
        }
        case OP_LDC: {
            uint8_t idx = f.code[f.pc++];
            cp_info *cp = &cf->constant_pool[idx - 1];
            if (cp->tag == CP_INTEGER) {
                int32_t v = (int32_t)((cp->info[0] << 24) | (cp->info[1] << 16)
                                     | (cp->info[2] << 8)  |  cp->info[3]);
                PUSH(v);
            } else if (cp->tag == CP_STRING) {
                uint16_t str_idx = (uint16_t)((cp->info[0] << 8) | cp->info[1]);
                const char *str = classfile_get_utf8(cf, str_idx);
                PUSH((int64_t)(uintptr_t)str);
            } else { PUSH(0); }
            break;
        }

        /* int loads/stores */
        case OP_ILOAD:   PUSH((int32_t)f.locals[f.code[f.pc++]]); break;
        case OP_ILOAD_0: PUSH((int32_t)f.locals[0]); break;
        case OP_ILOAD_1: PUSH((int32_t)f.locals[1]); break;
        case OP_ILOAD_2: PUSH((int32_t)f.locals[2]); break;
        case OP_ILOAD_3: PUSH((int32_t)f.locals[3]); break;

        case OP_ISTORE:   f.locals[f.code[f.pc++]] = (int32_t)POP(); break;
        case OP_ISTORE_0: f.locals[0] = (int32_t)POP(); break;
        case OP_ISTORE_1: f.locals[1] = (int32_t)POP(); break;
        case OP_ISTORE_2: f.locals[2] = (int32_t)POP(); break;
        case OP_ISTORE_3: f.locals[3] = (int32_t)POP(); break;

        /* object ref loads/stores — same as iload/istore but full 64-bit */
        case OP_ALOAD:   PUSH(f.locals[f.code[f.pc++]]); break;
        case OP_ALOAD_0: PUSH(f.locals[0]); break;
        case OP_ALOAD_1: PUSH(f.locals[1]); break;
        case OP_ALOAD_2: PUSH(f.locals[2]); break;
        case OP_ALOAD_3: PUSH(f.locals[3]); break;

        case OP_ASTORE:   f.locals[f.code[f.pc++]] = POP(); break;
        case OP_ASTORE_0: f.locals[0] = POP(); break;
        case OP_ASTORE_1: f.locals[1] = POP(); break;
        case OP_ASTORE_2: f.locals[2] = POP(); break;
        case OP_ASTORE_3: f.locals[3] = POP(); break;

        case OP_DUP:  PUSH(PEEK()); break;
        case OP_POP:  POP();        break;
        case OP_POP2: POP(); POP(); break;

        /* array loads — pop index, pop arrayref, bounds check, push element */
        case OP_IALOAD:
        case OP_AALOAD:
        case OP_BALOAD:
        case OP_CALOAD:
        case OP_SALOAD: {
            int32_t index = (int32_t)POP();
            object *arr = (object *)(uintptr_t)POP();
            if (!arr) { fprintf(stderr, "NullPointerException\n"); goto done; }
            int32_t len = (int32_t)arr->fields[0];
            if (index < 0 || index >= len) {
                fprintf(stderr, "ArrayIndexOutOfBoundsException: %d\n", index);
                goto done;
            }
            int64_t elem = arr->fields[index + 1];
            if      (op == OP_BALOAD) PUSH((int32_t)(int8_t)elem);
            else if (op == OP_CALOAD) PUSH((int32_t)(uint16_t)elem);
            else if (op == OP_SALOAD) PUSH((int32_t)(int16_t)elem);
            else                      PUSH(elem); /* IALOAD, AALOAD */
            break;
        }

        /* array stores — pop value, pop index, pop arrayref, bounds check, store */
        case OP_IASTORE:
        case OP_AASTORE:
        case OP_BASTORE:
        case OP_CASTORE:
        case OP_SASTORE: {
            int64_t val   = POP();
            int32_t index = (int32_t)POP();
            object *arr   = (object *)(uintptr_t)POP();
            if (!arr) { fprintf(stderr, "NullPointerException\n"); goto done; }
            int32_t len = (int32_t)arr->fields[0];
            if (index < 0 || index >= len) {
                fprintf(stderr, "ArrayIndexOutOfBoundsException: %d\n", index);
                goto done;
            }
            if      (op == OP_BASTORE) arr->fields[index + 1] = (int8_t)val;
            else if (op == OP_CASTORE) arr->fields[index + 1] = (uint16_t)val;
            else if (op == OP_SASTORE) arr->fields[index + 1] = (int16_t)val;
            else                       arr->fields[index + 1] = val;
            break;
        }

        /* integer arithmetic */
        case OP_IADD: { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a+b); break; }
        case OP_ISUB: { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a-b); break; }
        case OP_IMUL: { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a*b); break; }
        case OP_IDIV: {
            int32_t b=(int32_t)POP(),a=(int32_t)POP();
            if (b==0){fprintf(stderr,"ArithmeticException: / by zero\n");goto done;}
            PUSH(a/b); break;
        }
        case OP_IREM: {
            int32_t b=(int32_t)POP(),a=(int32_t)POP();
            if (b==0){fprintf(stderr,"ArithmeticException: / by zero\n");goto done;}
            PUSH(a%b); break;
        }
        case OP_INEG:  { int32_t a=(int32_t)POP(); PUSH(-a); break; }
        case OP_ISHL:  { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a<<(b&0x1f)); break; }
        case OP_ISHR:  { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a>>(b&0x1f)); break; }
        case OP_IUSHR: { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH((int32_t)((uint32_t)a>>(b&0x1f))); break; }
        case OP_IAND:  { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a&b); break; }
        case OP_IOR:   { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a|b); break; }
        case OP_IXOR:  { int32_t b=(int32_t)POP(),a=(int32_t)POP(); PUSH(a^b); break; }

        case OP_IINC: {
            uint8_t idx = f.code[f.pc++];
            int8_t  c   = (int8_t)f.code[f.pc++];
            f.locals[idx] += c;
            break;
        }

        /* type conversions */
        case OP_I2B: PUSH((int32_t)(int8_t)  POP()); break;
        case OP_I2C: PUSH((int32_t)(uint16_t) POP()); break;
        case OP_I2S: PUSH((int32_t)(int16_t)  POP()); break;
        case OP_I2L: PUSH((int64_t)(int32_t)  POP()); break;
        case OP_L2I: PUSH((int32_t)           POP()); break;
        case OP_I2F: {
            int32_t ival = (int32_t)POP();
            float   fval = (float)ival;
            int32_t bits; memcpy(&bits, &fval, 4);
            PUSH((int64_t)bits);
            break;
        }
        case OP_I2D: {
            int32_t ival = (int32_t)POP();
            double  dval = (double)ival;
            int64_t bits; memcpy(&bits, &dval, 8);
            PUSH(bits);
            break;
        }
        case OP_F2I: {
            int32_t bits = (int32_t)POP();
            float   fval; memcpy(&fval, &bits, 4);
            PUSH((int32_t)fval);
            break;
        }
        case OP_D2I: {
            int64_t bits = POP();
            double  dval; memcpy(&dval, &bits, 8);
            PUSH((int32_t)dval);
            break;
        }

        /* unary conditional branches */
        case OP_IFEQ: case OP_IFNE:
        case OP_IFLT: case OP_IFGE:
        case OP_IFGT: case OP_IFLE: {
            int16_t offset = (int16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            int32_t val = (int32_t)POP(), take = 0;
            switch (op) {
                case OP_IFEQ: take=(val==0); break; case OP_IFNE: take=(val!=0); break;
                case OP_IFLT: take=(val< 0); break; case OP_IFGE: take=(val>=0); break;
                case OP_IFGT: take=(val> 0); break; case OP_IFLE: take=(val<=0); break;
            }
            if (take) f.pc = (uint32_t)((int32_t)(f.pc-3) + offset);
            break;
        }

        /* binary conditional branches */
        case OP_IF_ICMPEQ: case OP_IF_ICMPNE:
        case OP_IF_ICMPLT: case OP_IF_ICMPGE:
        case OP_IF_ICMPGT: case OP_IF_ICMPLE: {
            int16_t offset = (int16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            int32_t b=(int32_t)POP(), a=(int32_t)POP(), take=0;
            switch (op) {
                case OP_IF_ICMPEQ: take=(a==b); break; case OP_IF_ICMPNE: take=(a!=b); break;
                case OP_IF_ICMPLT: take=(a< b); break; case OP_IF_ICMPGE: take=(a>=b); break;
                case OP_IF_ICMPGT: take=(a> b); break; case OP_IF_ICMPLE: take=(a<=b); break;
            }
            if (take) f.pc = (uint32_t)((int32_t)(f.pc-3) + offset);
            break;
        }

        case OP_GOTO: {
            int16_t offset = (int16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc = (uint32_t)((int32_t)(f.pc-1) + offset);
            break;
        }

        /* switch statements */
        case OP_TABLESWITCH: {
            uint32_t insn_pc = f.pc - 1;
            /* skip 0-3 padding bytes to reach 4-byte alignment */
            while (f.pc % 4 != 0) f.pc++;

#define RS32(p) ((int32_t)(((uint32_t)f.code[p]<<24)|((uint32_t)f.code[p+1]<<16) \
                           |((uint32_t)f.code[p+2]<<8)|(uint32_t)f.code[p+3]))

            int32_t default_off = RS32(f.pc); f.pc += 4;
            int32_t low         = RS32(f.pc); f.pc += 4;
            int32_t high        = RS32(f.pc); f.pc += 4;

            int32_t val = (int32_t)POP();
            int32_t jump_off;

            if (val < low || val > high) {
                jump_off = default_off;
            } else {
                uint32_t entry = f.pc + (uint32_t)(val - low) * 4;
                jump_off = RS32(entry);
            }
            f.pc = (uint32_t)((int32_t)insn_pc + jump_off);
#undef RS32
            break;
        }

        case OP_LOOKUPSWITCH: {
            uint32_t insn_pc = f.pc - 1;
            while (f.pc % 4 != 0) f.pc++;

#define RS32(p) ((int32_t)(((uint32_t)f.code[p]<<24)|((uint32_t)f.code[p+1]<<16) \
                           |((uint32_t)f.code[p+2]<<8)|(uint32_t)f.code[p+3]))

            int32_t default_off = RS32(f.pc); f.pc += 4;
            int32_t npairs      = RS32(f.pc); f.pc += 4;

            int32_t val      = (int32_t)POP();
            int32_t jump_off = default_off;

            for (int32_t i = 0; i < npairs; i++) {
                int32_t match  = RS32(f.pc); f.pc += 4;
                int32_t offset = RS32(f.pc); f.pc += 4;
                if (val == match) { jump_off = offset; break; }
            }
            f.pc = (uint32_t)((int32_t)insn_pc + jump_off);
#undef RS32
            break;
        }

        /* returns */
        case OP_IRETURN: return_val = (int32_t)POP(); goto done;
        case OP_ARETURN: return_val = POP();           goto done;
        case OP_RETURN:                                goto done;

        /* ── object creation ─────────────────────────────────────────────── */
        case OP_NEW: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            /* Count instance (non-static) fields for this class */
            uint16_t nfields = 0;
            for (uint16_t i = 0; i < cf->fields_count; i++) {
                if (!(cf->fields[i].access_flags & 0x0008)) nfields++;
            }
            object *obj = heap_alloc(idx, nfields);
            if (!obj) goto done;
            PUSH((int64_t)(uintptr_t)obj);
            break;
        }

        case OP_NEWARRAY: {
            f.pc++; /* skip atype (4=bool,5=char,6=float,7=double,8=byte,9=short,10=int,11=long) */
            int32_t count = (int32_t)POP();
            if (count < 0) { fprintf(stderr, "NegativeArraySizeException\n"); goto done; }
            object *arr = heap_alloc(0xFFFF, (size_t)count + 1);
            if (!arr) goto done;
            arr->fields[0] = count;
            PUSH((int64_t)(uintptr_t)arr);
            break;
        }

        case OP_ANEWARRAY: {
            f.pc += 2; /* skip 2-byte element type index */
            int32_t count = (int32_t)POP();
            if (count < 0) { fprintf(stderr, "NegativeArraySizeException\n"); goto done; }
            object *arr = heap_alloc(0xFFFE, (size_t)count + 1);
            if (!arr) goto done;
            arr->fields[0] = count;
            PUSH((int64_t)(uintptr_t)arr);
            break;
        }

        case OP_ARRAYLENGTH: {
            object *arr = (object *)(uintptr_t)POP();
            if (!arr) { fprintf(stderr, "NullPointerException\n"); goto done; }
            PUSH((int32_t)arr->fields[0]);
            break;
        }

        /* ── field/method invocation ─────────────────────────────────────── */
        case OP_GETSTATIC: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            cp_info *fref = &cf->constant_pool[idx - 1];
            if (fref->tag == CP_FIELDREF) {
                uint16_t ci = (uint16_t)((fref->info[0] << 8) | fref->info[1]);
                cp_info *cls = &cf->constant_pool[ci - 1];
                if (cls->tag == CP_CLASS) {
                    uint16_t ni = (uint16_t)((cls->info[0] << 8) | cls->info[1]);
                    const char *cname = classfile_get_utf8(cf, ni);
                    if (cname && strcmp(cname, "java/lang/System") == 0) {
                        PUSH(SYSTEM_OUT_SENTINEL);
                        break;
                    }
                }
            }
            int slot = get_field_index(cf, idx, 1);
            PUSH(slot >= 0 ? static_fields[slot] : 0LL);
            break;
        }

        case OP_PUTSTATIC: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            int64_t val = POP();
            int slot = get_field_index(cf, idx, 1);
            if (slot >= 0 && slot < MAX_STATIC_FIELDS) static_fields[slot] = val;
            break;
        }

        case OP_GETFIELD: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            object *obj = (object *)(uintptr_t)POP();
            if (!obj) { fprintf(stderr, "NullPointerException\n"); goto done; }
            int slot = get_field_index(cf, idx, 0);
            PUSH(slot >= 0 ? obj->fields[slot] : 0LL);
            break;
        }

        case OP_PUTFIELD: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            int64_t val = POP();
            object *obj = (object *)(uintptr_t)POP();
            if (!obj) { fprintf(stderr, "NullPointerException\n"); goto done; }
            int slot = get_field_index(cf, idx, 0);
            if (slot >= 0) obj->fields[slot] = val;
            break;
        }

        case OP_INVOKESPECIAL: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            cp_info *mref = &cf->constant_pool[idx - 1];
            if (mref->tag != CP_METHODREF) break;

            uint16_t nat_idx  = (uint16_t)((mref->info[2] << 8) | mref->info[3]);
            cp_info *nat      = &cf->constant_pool[nat_idx - 1];
            uint16_t name_idx = (uint16_t)((nat->info[0] << 8) | nat->info[1]);
            uint16_t desc_idx = (uint16_t)((nat->info[2] << 8) | nat->info[3]);
            const char *mname = classfile_get_utf8(cf, name_idx);
            const char *mdesc = classfile_get_utf8(cf, desc_idx);

            int nargs = count_args(mdesc);
            int64_t call_args[17] = {0}; /* [0]=this, [1..n]=args */
            for (int i = nargs; i >= 1; i--) call_args[i] = POP();
            call_args[0] = POP(); /* receiver (this) */

            /* Only dispatch if the method ref targets our own class.
               This prevents super.<init>() (e.g. Thread.<init>) from
               resolving to our own <init> and looping infinitely. */
            const char *ref_class  = get_methodref_class(cf, mref);
            const char *this_class = get_this_class_name(cf);
            int same_class = (!ref_class || !this_class ||
                              strcmp(ref_class, this_class) == 0);
            if (same_class) {
                method_info *target = classfile_find_method(cf, mname, mdesc);
                if (target) {
                    int64_t result = interpreter_execute(cf, target, call_args,
                                                         (uint16_t)(nargs + 1));
                    if (has_return_value(mdesc)) PUSH(result);
                }
            }
            /* if not found or different class (e.g. Object.<init>, Thread.<init>): skip */
            break;
        }

        case OP_INVOKEVIRTUAL: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            cp_info *mref = &cf->constant_pool[idx - 1];
            if (mref->tag != CP_METHODREF) break;

            uint16_t nat_idx  = (uint16_t)((mref->info[2] << 8) | mref->info[3]);
            cp_info *nat      = &cf->constant_pool[nat_idx - 1];
            uint16_t name_idx = (uint16_t)((nat->info[0] << 8) | nat->info[1]);
            uint16_t desc_idx = (uint16_t)((nat->info[2] << 8) | nat->info[3]);
            const char *mname = classfile_get_utf8(cf, name_idx);
            const char *mdesc = classfile_get_utf8(cf, desc_idx);

            int nargs = count_args(mdesc);
            int64_t call_args[16] = {0};
            for (int i = nargs - 1; i >= 0; i--) call_args[i] = POP();
            int64_t receiver = POP();

            if (mname && strcmp(mname, "println") == 0) {
                if (mdesc && strcmp(mdesc, "(I)V") == 0)
                    printf("%d\n", (int32_t)call_args[0]);
                else if (mdesc && strcmp(mdesc, "(Z)V") == 0)
                    printf("%s\n", call_args[0] ? "true" : "false");
                else if (mdesc && strcmp(mdesc, "(Ljava/lang/String;)V") == 0)
                    printf("%s\n", call_args[0] ? (const char *)(uintptr_t)call_args[0] : "null");
                else if (mdesc && strcmp(mdesc, "()V") == 0)
                    printf("\n");
            } else if (mname && strcmp(mname, "start") == 0 &&
                       mdesc && strcmp(mdesc, "()V") == 0) {
                /* Thread.start() — spawn a pthread that calls run() */
                object *thread_obj = (object *)(uintptr_t)receiver;
                if (thread_obj) {
                    thread_args_t *ta = malloc(sizeof(thread_args_t));
                    ta->cf       = cf;
                    ta->this_obj = thread_obj;
                    pthread_create(&thread_obj->thread_id, NULL, thread_entry, ta);
                    thread_obj->is_thread = 1;
                }
            } else if (mname && strcmp(mname, "join") == 0 &&
                       mdesc && strcmp(mdesc, "()V") == 0) {
                /* Thread.join() — wait for the thread to finish */
                object *thread_obj = (object *)(uintptr_t)receiver;
                if (thread_obj && thread_obj->is_thread) {
                    pthread_join(thread_obj->thread_id, NULL);
                }
            } else {
                /* dispatch to user-defined instance method */
                method_info *target = classfile_find_method(cf, mname, mdesc);
                if (target) {
                    int64_t virt_args[17] = {0};
                    virt_args[0] = receiver; /* this → locals[0] */
                    for (int i = 0; i < nargs; i++) virt_args[i + 1] = call_args[i];
                    int64_t result = interpreter_execute(cf, target, virt_args,
                                                         (uint16_t)(nargs + 1));
                    if (has_return_value(mdesc)) PUSH(result);
                }
            }
            break;
        }

        case OP_INVOKESTATIC: {
            uint16_t idx = (uint16_t)((f.code[f.pc] << 8) | f.code[f.pc+1]);
            f.pc += 2;
            cp_info *mref = &cf->constant_pool[idx - 1];
            if (mref->tag != CP_METHODREF) break;

            uint16_t nat_idx  = (uint16_t)((mref->info[2] << 8) | mref->info[3]);
            cp_info *nat      = &cf->constant_pool[nat_idx - 1];
            uint16_t name_idx = (uint16_t)((nat->info[0] << 8) | nat->info[1]);
            uint16_t desc_idx = (uint16_t)((nat->info[2] << 8) | nat->info[3]);
            const char *mname = classfile_get_utf8(cf, name_idx);
            const char *mdesc = classfile_get_utf8(cf, desc_idx);

            method_info *target = classfile_find_method(cf, mname, mdesc);
            if (!target) {
                fprintf(stderr, "Error: method not found: %s%s\n",
                        mname ? mname : "?", mdesc ? mdesc : "");
                break;
            }
            int nargs = count_args(mdesc);
            int64_t call_args[16] = {0};
            for (int i = nargs - 1; i >= 0; i--) call_args[i] = POP();

            int64_t result = interpreter_execute(cf, target, call_args, (uint16_t)nargs);
            if (has_return_value(mdesc)) PUSH(result);
            break;
        }

        case OP_MONITORENTER: {
            object *obj = (object *)(uintptr_t)POP();
            if (!obj) { fprintf(stderr, "NullPointerException\n"); goto done; }
            pthread_mutex_lock(&obj->monitor);
            break;
        }
        case OP_MONITOREXIT: {
            object *obj = (object *)(uintptr_t)POP();
            if (!obj) { fprintf(stderr, "NullPointerException\n"); goto done; }
            pthread_mutex_unlock(&obj->monitor);
            break;
        }

        default:
            fprintf(stderr, "Error: unsupported opcode 0x%02x at pc=%u\n", op, f.pc-1);
            goto done;
        }
    }

done:
    /* Deregister frame — restore depth to what it was before this call */
    if (my_depth >= 0) gc_call_depth = my_depth;
    free(f.locals);
    free(f.operand_stack);
#undef PUSH
#undef POP
#undef PEEK
    return return_val;
}
