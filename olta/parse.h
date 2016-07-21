#ifndef __PARSE_H
#define __PARSE_H 1

litmus_t *load_test(const char *fn);
litmus_t *read_test(FILE *fh);
void print_test_file(litmus_t *d);
void print_test(litmus_t *lt);
void free_litmus_t(litmus_t *test);

void config_add_var(litmus_t *cfg, const char *name, const char *value);
spec_var_t *config_lookup_var(litmus_t *cfg, const char *name);
const char *config_lookup_var_str(litmus_t *cfg, const char *name, const char *fallback);
int config_lookup_var_int(litmus_t *cfg, const char *name, int fallback);

spec_var_t *config_thread_var(litmus_t *cfg, tthread_t *th, const char *name);
const char *config_thread_var_str(litmus_t *test, tthread_t *th, const char *name, const char *fallback);
int config_thread_var_int(litmus_t *cfg, tthread_t *th, const char *name, int fallback);

spec_var_t *config_mem_loc_var(litmus_t *cfg, tthread_t *th, mem_loc_t *ml, const char *name);
const char *config_mem_loc_var_str(litmus_t *test, tthread_t *th, mem_loc_t *ml, const char *name, const char *fallback);
int config_mem_loc_var_int(litmus_t *cfg, tthread_t *th, mem_loc_t *ml, const char *name, int fallback);

const char *config_ins_var(litmus_t *test, tthread_t *th, int i_n, const char *name);


typedef enum _ao_t {
    AO_UNKNOWN          = 0,
    AO_STALL_ADD_DEP    = 1,
    AO_STALL_ADD        = 2,
    AO_ALIGN            = 4,
    AO_NOP              = 8,
    AO_ISB              = 9,
    AO_BUF_OP           = 10,
    AO_MASK             = 0x000ff,
    AO_UNQ              = 0x01000,
    AO_SHR              = 0x02000,
    AO_STR              = 0x00100,
    AO_LDR              = 0x00200,
    AO_UPDATE_SR        = 0x00800,
    AO_INC              = 0x04000,
    AO_DEC              = 0x08000,
    AO_SET              = 0x00400,
    AO_PREFETCH         = 0x10000,
    AO_FLUSH            = 0x20000
} ao_t;

typedef struct _aopt_t aopt_t;

struct _aopt_t {
    aopt_t *next;
    ao_t opt;
    long n;
    long v;
    long s;
    int n_arg;
};

int interpret_flush_type(int n);
aopt_t *parse_ancillary(const char *s);
void release_ancillary(aopt_t *ao);

#endif /* __PARSE_H */
