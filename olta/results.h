#ifndef __RESULTS_H
#define __RESULTS_H 1

typedef struct _result_ent_t result_ent_t;
struct _result_ent_t {
    unsigned long count;
    result_ent_t *left;
    result_ent_t *right;
    uint64_t *key;
};

typedef struct _result_set_t {
    int key_len;
    result_ent_t *root;
} result_set_t;

void result_set_init(result_set_t *rs, int key_len);
void result_set_add(result_set_t *rs, uint64_t *vs);
void result_set_output(result_set_t *rs, const char **fmt);
void result_set_release(result_set_t *rs);

#endif /* __RESULTS_H */
