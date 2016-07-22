#ifndef __LITMUS_H
#define __LITMUS_H 1

typedef enum {
    I_INVALID = 0,
    I_LDR,
    I_STR,
    I_MOV,
    I_DSB,
    I_DMB,
    I_ISB,
    I_EOR,
    I_ADD,
    I_SUB,
    I_CMP,
    I_LABEL,
    I_BNE,
    I_BEQ,
    I_AND,
    I_NOP
} ins_t;

typedef enum _val_t {
    T_UNKNOWN = 0,
    T_PTR,
    T_VAL2,
    T_VAL4,
    T_VAL8
} val_t;

#define R_PREFETCH_MASK 0xf
#define R_PREFETCH_KEEP 0x1
#define R_PREFETCH_STRM 0x2
#define R_PREFETCH_L1   0x4
#define R_PREFETCH_L2   0x8
#define R_PREFETCH_L3   0xc
#define R_PREFETCH_LDR_SHIFT 8
#define R_PREFETCH_STR_SHIFT 12
typedef enum _reg_flag_t {
    R_OUTPUT            = 0x1,
    R_ADDRESS           = 0x2,
    R_PRELOAD           = 0x4,
    R_FLUSH             = 0x30,
    R_FLUSH_INV         = 0x10,
    R_FLUSH_CLEAN       = 0x20,
    R_FLUSH_POU         = 0x40,
    R_FLUSH_DSB         = 0x80,
    R_PREFETCH_LDR_MASK = R_PREFETCH_MASK << R_PREFETCH_LDR_SHIFT,
    R_PREFETCH_LDR_KEEP = R_PREFETCH_KEEP << R_PREFETCH_LDR_SHIFT,
    R_PREFETCH_LDR_STRM = R_PREFETCH_STRM << R_PREFETCH_LDR_SHIFT,
    R_PREFETCH_LDR_L1   = R_PREFETCH_L1 << R_PREFETCH_LDR_SHIFT,
    R_PREFETCH_LDR_L2   = R_PREFETCH_L2 << R_PREFETCH_LDR_SHIFT,
    R_PREFETCH_LDR_L3   = R_PREFETCH_L3 << R_PREFETCH_LDR_SHIFT,
    R_PREFETCH_STR_MASK = R_PREFETCH_MASK << R_PREFETCH_STR_SHIFT,
    R_PREFETCH_STR_KEEP = R_PREFETCH_KEEP << R_PREFETCH_STR_SHIFT,
    R_PREFETCH_STR_STRM = R_PREFETCH_STRM << R_PREFETCH_STR_SHIFT,
    R_PREFETCH_STR_L1   = R_PREFETCH_L1 << R_PREFETCH_STR_SHIFT,
    R_PREFETCH_STR_L2   = R_PREFETCH_L2 << R_PREFETCH_STR_SHIFT,
    R_PREFETCH_STR_L3   = R_PREFETCH_L3 << R_PREFETCH_STR_SHIFT
} reg_flag_t;

typedef enum _ins_flag_t {
    I_INDIRECT      = 0x1,
    I_OFFSET_REG    = 0x2,
    I_OFFSET_CONST  = 0x4,
    I_CONST         = 0x8,
    I_BAR_LD        = 0x0100,
    I_BAR_ST        = 0x0200,
    I_BAR_ISH       = 0x0400,
    I_BAR_OSH       = 0x0800,
    I_BAR_NSH       = 0x1000,
} ins_flag_t;

typedef struct _ins_arg_t {
    int size;
    long n;
} ins_arg_t;

#define MAX_INS_ARGS (3)
typedef struct _ins_desc_t {
    ins_t ins;
    char *label;
    int size;
    int flags;
    int n_arg;
    ins_arg_t arg[MAX_INS_ARGS];
} ins_desc_t;

#define MAX_TREG_NAME (3)
typedef struct _treg_t {
    val_t t;
    int n;
    uint64_t v;
    int flags;
    char name[MAX_TREG_NAME + 1];
} treg_t;

#define MAX_THREAD_REG (10)
#define MAX_THREAD_INS (16)
typedef struct _tthread_t {
    int id;
    char *name;

    int n_reg;
    treg_t reg[MAX_THREAD_REG];

    int n_ins;
    ins_desc_t ins[MAX_THREAD_INS];
} tthread_t;

typedef struct _mem_loc_t {
    char *name;
    int size;
    int stride;
    int offset;
    uint64_t v;
} mem_loc_t;

typedef struct _cfg_idx_t {
    int ptr;
    int lines;
} cfg_idx_t;

#define DEFAULT_SPEC_HASH_SIZE (128)
typedef struct _spec_var_t spec_var_t;
struct _spec_var_t {
    const char *name;
    const char *value;
    spec_var_t *next;
};

#define MAX_MEM_LOC (4)
#define MAX_TTHREAD (8)
typedef struct _litmus_t {
    int id;

    char **lines;
    int n_lines;

    char *arch;
    char *name;

    cfg_idx_t var;
    cfg_idx_t thread;
    cfg_idx_t spec;

    unsigned int spec_hash_size;
    spec_var_t **spec_hash;
    
    int n_mem_loc;
    mem_loc_t mem_loc[MAX_MEM_LOC];

    int n_tthread;
    tthread_t tthread[MAX_TTHREAD];
} litmus_t;

typedef struct _test_ctx_t {
    uint64_t *sync_a;
    uint64_t *sync_b;
    uint64_t buffer_size;
    uint64_t *buffer; /* shared buffer */
    uint64_t n_threads;
    uint64_t n_iterations;
} test_ctx_t;

typedef struct _thread_ctx_t {
    pthread_t handle;
    const char *name;
    test_ctx_t *test;
    int affinity;
    void *code;
    uint64_t buffer_size;
    uint64_t *buffer; /* unique buffer */
    uint64_t *timing;
    uint64_t reg[MAX_THREAD_REG];
    uint64_t *out[MAX_THREAD_REG];
} thread_ctx_t;

#endif /* __LITMUS_H */
