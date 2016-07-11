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
    AO_MASK             = 0x00ff,
    AO_UNQ              = 0x1000,
    AO_SHR              = 0x2000,
    AO_STR              = 0x0100,
    AO_LDR              = 0x0200,
    AO_INC              = 0x0400,
    AO_DEC              = 0x0800
} ao_t;

typedef struct _aopt_t aopt_t;

struct _aopt_t {
    aopt_t *next;
    ao_t opt;
    long n;
    long v;
    long s;
};

aopt_t *parse_ancillary(const char *s);
void release_ancillary(aopt_t *ao);

#endif /* __PARSE_H */
