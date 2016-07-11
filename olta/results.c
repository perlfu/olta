#include "olta.h"

static result_ent_t *result_alloc_ent(result_set_t *rs, uint64_t *key) {
    result_ent_t *e = (result_ent_t *) malloc(sizeof(result_ent_t) + sizeof(uint64_t) * rs->key_len);
    int i;

    e->count = 1;
    e->left = NULL;
    e->right = NULL;
    e->key = (uint64_t *)(((uint8_t *)e) + sizeof(result_ent_t));
    for (i = 0; i < rs->key_len; ++i) {
        e->key[i] = key[i];
    }

    return e;
}

static void result_free_ent(result_set_t *rs, result_ent_t *ent) {
    if (ent->left)
        result_free_ent(rs, ent->left);
    if (ent->right)
        result_free_ent(rs, ent->right);
    free(ent);
}

void result_set_init(result_set_t *rs, int key_len) {
    rs->key_len = key_len;
    rs->root = NULL;
}

void result_set_add(result_set_t *rs, uint64_t *vs) {
    result_ent_t *ent = rs->root;

    if (ent == NULL) {
        rs->root = result_alloc_ent(rs, vs);
        return;
    }

    for (;;) {
        int ki;

        for (ki = 0; ki < rs->key_len; ++ki) {
            if (ent->key[ki] > vs[ki]) {
                if (ent->left) {
                    ent = ent->left;
                    break;
                } else {
                    ent->left = result_alloc_ent(rs, vs);
                    return;
                }
            } else if (ent->key[ki] < vs[ki]) {
                if (ent->right) {
                    ent = ent->right;
                    break;
                } else {
                    ent->right = result_alloc_ent(rs, vs);
                    return;
                }
            }
        }
        
        // key match
        if (ki == rs->key_len) {
            ent->count += 1;
            return;
        }
    }
}

static void result_output_ent(result_set_t *rs, result_ent_t *ent, const char **fmt) {
    int i;

    if (ent->left)
        result_output_ent(rs, ent->left, fmt);

    log_result_start();
    //log_result_p("end-state ");
    for (i = 0; i < rs->key_len; ++i) {
        log_result_p(fmt[i], ent->key[i]);
    }
    log_result_p(" -- count = %lu", ent->count);
    log_result_end();

    if (ent->right)
        result_output_ent(rs, ent->right, fmt);
}

void result_set_output(result_set_t *rs, const char **fmt) {
    if (rs->root)
        result_output_ent(rs, rs->root, fmt);
}

void result_set_release(result_set_t *rs) {
    if (rs->root)
        result_free_ent(rs, rs->root);
}

