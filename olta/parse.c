#include "olta.h"

static int next_id = 42;

static void wssplit2(const char *src, char *left, char *right) {
    int p = 0;
    int n;

    while (isspace(src[p]) && (src[p] != '\0')) p++;

    for (n = 0; src[p] != '\0'; ++p, ++n) {
        if (isspace(src[p]))
            break;
        left[n] = src[p];
    }
    left[p] = '\0';

    while (isspace(src[p]) && (src[p] != '\0')) p++;
    
    /* FIXME: copy until end of string whitespace, not first whitespace */
    for (n = 0; src[p] != '\0'; ++p, ++n) {
        if (isspace(src[p]))
            break;
        right[n] = src[p];
    }
    right[n] = '\0';
}

static int find_char(const char *str, char c) {
    int p;
    for (p = 0; str[p] != '\0'; ++p) {
        if (str[p] == c)
            return p;
    }
    return -1;
}

static int eqsplit2(const char *src, char *left, char *right) {
    int p = find_char(src, '=');
    int sl = strlen(src);
    int n, ls, le, rs, re;

    if (p < 0) {
        left[0] = '\0';
        right[0] = '\0';
        return -1;
    }

    /* find left end */
    le = p - 1;
    while (isspace(src[le]) && le > 0) le--;
    /* find left start */
    ls = 0;
    while (isspace(src[ls]) && ls < le) ls++;

    /* find right start */
    rs = p + 1;
    while (isspace(src[rs]) && rs < sl) rs++;
    /* find right end */
    re = sl - 1;
    while (isspace(src[re]) && re > rs) re--;

    if (ls > le || rs > re) {
        left[0] = '\0';
        right[0] = '\0';
        return -1;
    }

    /* copy */
    for (n = 0, p = ls; p <= le; ++p, ++n)
        left[n] = src[p];
    left[n] = '\0';
    for (n = 0, p = rs; p <= re; ++p, ++n)
        right[n] = src[p];
    right[n] = '\0';
    return 0;
}

static unsigned int var_hash(const char *str) {
    unsigned int hash = 1;
    int i;
    for (i = 0; str[i] != '\0'; ++i) {
        hash += (hash + str[i]) * 37;
    }
    return hash;
}

static spec_var_t *alloc_spec_var(const char *name, const char *value) {
    spec_var_t *r = (spec_var_t *) malloc(sizeof(spec_var_t));
    r->name = strdup(name);
    r->value = strdup(value);
    r->next = NULL;
    return r;
}

void config_add_var(litmus_t *cfg, const char *name, const char *value) {
    unsigned int hash = var_hash(name);
    unsigned int idx = hash % cfg->spec_hash_size;
    spec_var_t *var = alloc_spec_var(name, value); 

    if (cfg->spec_hash[idx] == NULL) {
        cfg->spec_hash[idx] = var;
    } else {
        spec_var_t *vp = cfg->spec_hash[idx];
        while (vp->next != NULL)
            vp = vp->next;
        vp->next = var;
    }
}

spec_var_t *config_lookup_var(litmus_t *cfg, const char *name) {
    unsigned int hash = var_hash(name);
    unsigned int idx = hash % cfg->spec_hash_size;
    spec_var_t *var = cfg->spec_hash[idx];

    if (var == NULL)
        return NULL;

    while (strcmp(var->name, name) != 0) {
        if (var->next == NULL)
            return NULL;
        var = var->next;
    }

    return var;
}

const char *config_lookup_var_str(litmus_t *cfg, const char *name, const char *fallback) {
    spec_var_t *var = config_lookup_var(cfg, name);
    if (var)
        return var->value;
    else
        return fallback;
}

static int config_var_to_int(spec_var_t *var, int fallback) {
    if (var) {
        char *ep = NULL;
        long v = strtol(var->value, &ep, 0);
        if ((*ep == '\0') || ((ep != var->value) && isspace(*ep))) // successful parse
            return (int) v; 
        else
            return fallback;
    } else {
        return fallback;
    }
}

int config_lookup_var_int(litmus_t *cfg, const char *name, int fallback) {
    spec_var_t *var = config_lookup_var(cfg, name);
    return config_var_to_int(var, fallback);
}

spec_var_t *config_thread_var(litmus_t *cfg, tthread_t *th, const char *name) {
    spec_var_t *r = NULL;
    char buffer[BUFFER_LEN];
    snprintf(buffer, sizeof(buffer) - 1, "%s-%s", th->name, name);
    buffer[sizeof(buffer) - 1] = '\0';
    r = config_lookup_var(cfg, buffer);
    if (!r) {
        snprintf(buffer, sizeof(buffer) - 1, "*-%s", name);
        buffer[sizeof(buffer) - 1] = '\0';
        r = config_lookup_var(cfg, buffer);
    }
    return r;   
}

const char *config_thread_var_str(litmus_t *cfg, tthread_t *th, const char *name, const char *fallback) {
    spec_var_t *var = config_thread_var(cfg, th, name);
    if (var)
        return var->value;
    else
        return fallback;
}

int config_thread_var_int(litmus_t *cfg, tthread_t *th, const char *name, int fallback) {
    spec_var_t *var = config_thread_var(cfg, th, name);
    return config_var_to_int(var, fallback);
}

const char *config_ins_var(litmus_t *test, tthread_t *th, int i_n, const char *name) {
    char buffer[BUFFER_LEN];
    assert(th != NULL);
    snprintf(buffer, sizeof(buffer) - 1, "%s-i%d-%s", th->name, i_n, name);
    buffer[sizeof(buffer) - 1] = '\0';
    return config_lookup_var_str(test, buffer, NULL);
}

spec_var_t *config_mem_loc_var(litmus_t *cfg, tthread_t *th, mem_loc_t *ml, const char *name) {
    char buffer[BUFFER_LEN];
    assert(ml != NULL);
    if (th && ml) {
        snprintf(buffer, sizeof(buffer) - 1, "%s-%s-%s", th->name, ml->name, name);
    } else if (ml) {
        snprintf(buffer, sizeof(buffer) - 1, "%s-%s", ml->name, name);
    }
    buffer[sizeof(buffer) - 1] = '\0';
    return config_lookup_var(cfg, buffer);
}

const char *config_mem_loc_var_str(litmus_t *cfg, tthread_t *th, mem_loc_t *ml, const char *name, const char *fallback) {
    spec_var_t *var = config_mem_loc_var(cfg, th, ml, name);
    if (var)
        return var->value;
    else
        return fallback;
}

int config_mem_loc_var_int(litmus_t *cfg, tthread_t *th, mem_loc_t *ml, const char *name, int fallback) {
    spec_var_t *var = config_mem_loc_var(cfg, th, ml, name);
    return config_var_to_int(var, fallback);
}

static int match_spec_start(const char *line) {
    if (strncmp(line, "-=-=-", 5) == 0)
        return 1;
    return 0;
}

static int match_config_end(const char *line) {
    if (strncmp(line, "=-=-=", 5) == 0)
        return 1;
    return 0;
}

static litmus_t *read_file(FILE *fh) {
    const int buflen = BUFFER_LEN;
    litmus_t *d;
    char buffer[buflen];
    char **lines;
    int max_lines = 100;
    int ln;

    lines = (char **) malloc(sizeof(char *) * max_lines);
    assert(lines != NULL);

    ln = 0;
    while (!feof(fh)) {
        int p = 0;

        do {
            int c = fgetc(fh);
            if (c < 0 || c == '\n')
                break;
            if (c != '\r')
                buffer[p++] = (char) c;
        } while (p < (buflen - 1));

        // XXX: log if buflen reached

        if (p > 0) {
            buffer[p] = '\0';
            if (match_config_end(buffer))
                break;
            lines[ln++] = strdup(buffer);
        }
        
        if (ln >= max_lines) {
            max_lines *= 2;
            lines = (char **) realloc(lines, sizeof(char *) * max_lines);
            assert(lines != NULL);
        }
    }

    d = (litmus_t *) malloc(sizeof(litmus_t));
    
    d->id = next_id++;
    d->lines = lines;
    d->n_lines = ln;
    d->name = NULL;
    d->arch = NULL;
    d->var.ptr = 0;
    d->var.lines = 0;
    d->thread.ptr = 0;
    d->thread.lines = 0;
    d->spec_hash_size = 0;
    d->spec_hash = NULL;
    d->n_mem_loc = 0;
    d->n_tthread = 0;

    return d;
}

static litmus_t *load_test_file(const char *fn) {
    FILE *fh = fopen(fn, "r");
    litmus_t *cfg;
    if (!fh)
        return NULL;
    cfg = read_file(fh);
    fclose(fh);
    return cfg;
}

void print_test_file(litmus_t *d) {
    int i;
    log_debug("name = \"%s\"", d->name);
    log_debug("arch = \"%s\"", d->arch);
    for (i = 0; i < d->var.lines; ++i)
        log_debug("vars %d   : %s", i, d->lines[d->var.ptr + i]);
    for (i = 0; i < d->thread.lines; ++i)
        log_debug("threads %d: %s", i, d->lines[d->thread.ptr + i]);
    /*
    for (i = 0; i < d->n_lines; ++i) {
        fprintf(stdout, "line % 3d: %s\n", i, d->lines[i]);
    }
    */
}

static int match_line(const char *line, const char *content) {
    int ll = strlen(line);
    int cl = strlen(content);
    int p;

    if (ll < cl)
        return 0;
    
    for (p = 0; isspace(line[p]) && p < ll; p += 1) ;
    
    if ((ll - p) < cl)
        return 0;
    
    if (strncmp(line + p, content, cl) == 0) {
        p += cl;
        while (p < ll) {
            if (!isspace(line[p]))
                return 0;
        }
        return 1;
    } else {
        return 0;
    }
}

/* Return >0 if line is of thread format.
 * If >0, the value returned indicates number of threads.
 */
static int match_thread_line(const char *line) {
    int p = 0;
    int pipes = 0;
    char last_ns = '\0';
    while (line[p] != '\0') {
        if (line[p] == '|')
            pipes += 1;
        if (!isspace(line[p]))
            last_ns = line[p];
        p += 1;
    }
    if (pipes >= 1 && last_ns == ';')
        return pipes + 1;
    else
        return 0;
}

static char *strdup_tolower(const char *src, int len) {
    char *dst = (char *) malloc(sizeof(char) * (len + 1));
    int i;
    for (i = 0; i < len; ++i) {
        dst[i] = tolower(src[i]);
    }
    dst[i] = '\0';
    return dst;
}

static int extract_reg_n(const char *name) {
    int p = 0;
    while (!isdigit(name[p])) {
        if (name[p] == '\0')
            return -1;
        p += 1;
    }
    return atoi(name + p);
}

/* Select the part of a line that is specific to a numbered thread. */
static char *extract_thread(const char *line, int thread_n, int lc) {
    int p = 0;
    int s;
    
    if (thread_n > 0) {
        int pipes = 0;
        while (line[p] != '\0') {
            if (line[p] == '|')
                pipes += 1;
            p += 1;
            if (pipes >= thread_n)
                break;
        }
        if (pipes < thread_n) {
            // XXX: log parse error insufficient threads
            return NULL;
        }
    }

    while (isspace(line[p]) && (line[p] != '\0')) p++;
    s = p;
    while ((line[p] != '|') && (line[p] != ';') && (line[p] != '\0')) p++;
    if (s == p) {
        return strdup("");
    }
    p -= 1;
    while (isspace(line[p]) && (p > s)) p--;
    
    if (lc) {
        return strdup_tolower(line + s, (p - s) + 1);
    } else {
        return strndup(line + s, (p - s) + 1);
    }
}

/* Perform first parse of test identifying key features.
 * e.g. name, architecture
 *      which lines refer to threads and variables
 */
static int prepare_config(litmus_t *d) {
    char *line;
    int i;

    if (d->n_lines < 1)
        return -1;

    /* look for architecture and name on line 0 */
    line = d->lines[0];
    for (i = 0; (line[i] != '\0') && (!isspace(line[i])); ++i) ;
    if (line[i] == '\0')
        return -2;
    d->arch = strndup(line, i);
    while (isspace(line[i])) i++;
    d->name = strdup(line + i);

    /* look for spec block */
    d->spec.ptr = 0;
    d->spec.lines = 0;
    i = 1;
    while (i < d->n_lines) {
        if (match_spec_start(d->lines[i])) {
            d->spec.ptr = i + 1;
            d->spec.lines = (d->n_lines - i) - 1;
            break;
        }
        i += 1;
    }

    /* look for variable configuration block */
    i = 1;
    while (i < d->n_lines) {
        if (match_line(d->lines[i], "{"))
            break;
        i += 1;
    }
    if (i >= (d->n_lines - 1))
        return -3;
    
    d->var.ptr = i + 1;
    i += 1;
    while (i < d->n_lines) {
        if (match_line(d->lines[i], "}"))
            break;
        i += 1;
    }
    if (i >= d->n_lines)
        return -4;
    d->var.lines = i - d->var.ptr;

    /* look for thread instructions block */
    while (i < d->n_lines) {
        if (match_thread_line(d->lines[i])) {
            d->thread.ptr = i;
            break;
        } 
        i += 1;
    }
    if (i >= d->n_lines)
        return -5;
    while (i < d->n_lines) {
        if (match_thread_line(d->lines[i])) {
            d->thread.lines += 1;
            i += 1;
        } else {
            break;
        }
    }

    /* parse spec */
    d->spec_hash_size = DEFAULT_SPEC_HASH_SIZE;
    d->spec_hash = (spec_var_t **) malloc(sizeof(spec_var_t *) * d->spec_hash_size);
    for (i = 0; i < d->spec_hash_size; ++i)
        d->spec_hash[i] = NULL;

    i = d->spec.ptr;
    while (i < (d->spec.ptr + d->spec.lines)) {
        char name[BUFFER_LEN], value[BUFFER_LEN];
        int ret = eqsplit2(d->lines[i], name, value);
        if (ret >= 0)
            config_add_var(d, name, value);
        i += 1;
    }

    return 0;
}

static int parse_ins_args(ins_desc_t *ins, const char *line, char *err) {
    char buffer[BUFFER_LEN];
    int arg_n = 0;
    int arg_p = 0;
    int p = 0;
    
    while (isspace(line[p]) && line[p] != '\0') p++;

    for (;;) {
        char c = line[p++];
        if (c == '[' || c == ']') {
            ins->flags |= I_INDIRECT;
        } else if (c == ',' || c == '\0') {
            if (arg_p > 0) {
                if (arg_n >= MAX_INS_ARGS) {
                    snprintf(err, BUFFER_LEN - 1, "too many arguments \"%s\"", buffer);
                    return -1;
                }
                    
                buffer[arg_p] = '\0';

                if (isdigit(buffer[0])) {
                    ins->arg[arg_n].size = -1;
                    ins->arg[arg_n].n = strtol(buffer, NULL, 0);
                } else if (buffer[0] == '#') {
                    ins->arg[arg_n].size = -1;
                    ins->arg[arg_n].n = strtol(buffer + 1, NULL, 0);
                } else if (buffer[0] == 'w' || buffer[0] == 'r' || buffer[0] == 'x') {
                    switch (buffer[0]) {
                        case 'w': ins->arg[arg_n].size = 4; break;
                        case 'x': ins->arg[arg_n].size = 8; break;
                        case 'r': ins->arg[arg_n].size = 0; break;
                    }
                    ins->arg[arg_n].n = strtol(buffer + 1, NULL, 10);
                } else {
                    snprintf(err, BUFFER_LEN - 1, "unparsable argument \"%s\"", buffer);
                    return -1;
                }
                
                arg_n += 1;
                arg_p = 0;
            }
            if (c == '\0')
                break;
        } else if (!isspace(c)) {
            buffer[arg_p++] = c;
        }
        assert(arg_p < BUFFER_LEN);
    }

    ins->n_arg = arg_n;
    
    return arg_n;
}

static int parse_ins(tthread_t *thread, const char *line, char *err) {
    ins_desc_t *d;

    if (line[0] == '\0') return 0;

    d = &(thread->ins[thread->n_ins]);

    if ((strncmp(line, "ldr", 3) == 0) || (strncmp(line, "str", 3) == 0)) {
        int ret;
        int p = 3;

        if (strncmp(line, "ldr", 3) == 0)
            d->ins = I_LDR;
        else
            d->ins = I_STR;

        d->size = 8;

        if (line[p] == 'h') {
            d->size = 2;
            p += 1;
        } else if (line[p] == 'b') {
            d->size = 1;
            p += 1;
        } else if (!isspace(line[p])) {
            snprintf(err, BUFFER_LEN - 1, "unknown instruction \"%s\"", line);
            return -1;
        }

        ret = parse_ins_args(d, line + p, err);
        if (ret < 0)
            return ret;
        if (ret < 2) {
            snprintf(err, BUFFER_LEN - 1, "insufficient arguments \"%s\"", line);
            return -1;
        }

        if ((d->size == 8) && (d->arg[0].size == 4))
            d->size = 4;

        if ((d->arg[0].size < 0) || (d->arg[1].size < 0)) {
            snprintf(err, BUFFER_LEN - 1, "invalid arguments \"%s\"", line);
            return -1;
        }

        if (ret >= 3) {
            if (d->arg[2].size == -1)
                d->flags |= I_OFFSET_CONST;
            else
                d->flags |= I_OFFSET_REG;
        }

        return 1;
    } else if (strncmp(line, "mov", 3) == 0) {
        int p = 3;
        int ret = parse_ins_args(d, line + p, err);
        
        d->ins = I_MOV;
        
        if (ret < 0)
            return ret;

        if (ret < 2) {
            snprintf(err, BUFFER_LEN - 1, "insufficient arguments \"%s\"", line);
            return -1;
        }
        // FIXME: movk movz support
        if (ret > 2) {
            snprintf(err, BUFFER_LEN - 1, "insufficient arguments \"%s\"", line);
            return -1;
        }

        d->size = d->arg[0].size;
        if (d->size == 0)
            d->size = 8;
        if (d->arg[1].size == -1)
            d->flags |= I_CONST;

        return 1;
    } else if (strncmp(line, "eor", 3) == 0 || strncmp(line, "add", 3) == 0) {
        int p = 3;
        int ret = parse_ins_args(d, line + p, err);
        
        if (line[0] == 'e')
            d->ins = I_EOR;
        else if (line[0] == 'a')
            d->ins = I_ADD;
        else
            assert(0);
        
        if (ret < 0)
            return ret;

        if (ret < 3) {
            snprintf(err, BUFFER_LEN - 1, "insufficient arguments \"%s\"", line);
            return -1;
        }
        
        d->size = d->arg[0].size;
        if (d->size == 0)
            d->size = 8;
        if (d->arg[2].size == -1)
            d->flags |= I_CONST;

        return 1;
    } else if ((line[0] == 'd' || line[0] == 'i') && (line[1] == 'm' || line[1] == 's') && (line[2] == 'b')) {
        d->size = 0;
        d->n_arg = 0;
        
        if (line[0] == 'i') {
            d->ins = I_ISB;
        } else if (line[1] == 's') {
            d->ins = I_DSB;
        } else {
            d->ins = I_DMB;
        }

        if (line[3] == ' ') {
            int p = 4;
            if (strncmp(line + p, "ish", 3) == 0) {
                d->flags |= I_BAR_ISH;
                p += 3;
            } else if (strncmp(line + p, "osh", 3) == 0) {
                d->flags |= I_BAR_OSH;
                p += 3;
            } else if (strncmp(line + p, "nsh", 3) == 0) {
                d->flags |= I_BAR_NSH;
                p += 3;
            }
            if (strcmp(line + p, "st") == 0) {
                d->flags |= I_BAR_ST;
            } else if (strcmp(line + p, "ld") == 0) {
                d->flags |= I_BAR_LD;
            }
        }
            
        return 1;
    } else {
        snprintf(err, BUFFER_LEN - 1, "unknown instruction \"%s\"", line);
        return -1;
    } 
}

static void tag_ins_args(tthread_t *thread, ins_desc_t *ins) {
    int i, j;

    for (i = 0; i < ins->n_arg; ++i) {
        treg_t *reg = NULL;
        int n = ins->arg[i].n;

        if (ins->arg[i].size < 0)
            continue;

        for (j = 0; j < thread->n_reg && reg == NULL; ++j) {
            if (thread->reg[j].n == n)
                reg = &(thread->reg[j]);
        }

        if (reg == NULL) {
            reg = &(thread->reg[thread->n_reg]);
            thread->n_reg += 1;
            
            reg->t = T_UNKNOWN;
            reg->n = n;
            reg->v = 0;
            reg->flags = 0;
        }

        if ((ins->ins == I_LDR || ins->ins == I_STR) && (i == 1)) {
                reg->flags |= R_ADDRESS;
        } else if ((ins->ins == I_LDR) && (i == 0)) {
            reg->flags |= R_OUTPUT;
        }
    }
}


static int tag_reg_mem_loc(tthread_t *thread, const char *name, int ml_n) {
    int reg_n = extract_reg_n(name);
    int i;
    for (i = 0; i < thread->n_reg; ++i) {
        treg_t *reg = &(thread->reg[i]);
        if (reg->n == reg_n) {
            reg->t = T_PTR;
            reg->v = ml_n;
            strncpy(reg->name, name, MAX_TREG_NAME);
            reg->name[MAX_TREG_NAME] = '\0';
            return 0;
        }
    }
    return -1;
}

static int tag_reg_val(tthread_t *thread, const char *name, uint64_t v) {
    int reg_n = extract_reg_n(name);
    int i;
    for (i = 0; i < thread->n_reg; ++i) {
        treg_t *reg = &(thread->reg[i]);
        if (reg->n == reg_n) {
            if ((v & 0xffff) == v)
                reg->t = T_VAL2;
            else if ((v & 0xffffffff))
                reg->t = T_VAL4;
            else
                reg->t = T_VAL8;
            reg->v = v;
            strncpy(reg->name, name, MAX_TREG_NAME);
            reg->name[MAX_TREG_NAME] = '\0';
            return 0;
        }
    }
    return -1;
}

static int parse_var_token(litmus_t *test, const char *token, char *err) {
    int eq = find_char(token, '=');

    if (eq >= 0) {
        /* variable assignment */
        char reg[BUFFER_LEN], v_name[BUFFER_LEN];
        int colon, thread_n, reg_len, ret;
        
        eqsplit2(token, reg, v_name);
        
        colon = find_char(reg, ':');
        if ((colon < 0) || (strlen(reg) == 0) || (strlen(v_name) == 0)) {
            snprintf(err, BUFFER_LEN - 1, "unknown variable format \"%s\"", token);
            return -1;
        }
        reg_len = strlen(reg);
        reg[colon] = '\0';
        thread_n = atoi(reg);
        memmove(reg, reg + colon + 1, reg_len - colon);

        if (thread_n < 0 || thread_n >= test->n_tthread) {
            snprintf(err, BUFFER_LEN - 1, "unknown thread %d", thread_n);
            return -1;
        }

        if (isdigit(v_name[0])) {
            ret = tag_reg_val(&(test->tthread[thread_n]), reg, strtoull(v_name, NULL, 0));
        } else {
            int ml_n;
            for (ml_n = 0; ml_n < test->n_mem_loc; ++ml_n) {
                if (strcmp(v_name, test->mem_loc[ml_n].name) == 0)
                    break;
            }
            if (ml_n >= test->n_mem_loc) {
                snprintf(err, BUFFER_LEN - 1, "unknown variable \"%s\"", v_name);
                return -1;
            }
            ret = tag_reg_mem_loc(&(test->tthread[thread_n]), reg, ml_n);
        }

        if (ret < 0) {
            snprintf(err, BUFFER_LEN - 1, "register \"%s\" not valid for thread %d", reg, thread_n);
            return -1;
        }

        return 0;
    } else {
        /* variable declaration */
        char v_type[BUFFER_LEN], v_name[BUFFER_LEN];
        mem_loc_t *ml = &(test->mem_loc[test->n_mem_loc]);
        int size;

        wssplit2(token, v_type, v_name);
        if (strlen(v_name) == 0) {
            snprintf(err, BUFFER_LEN - 1, "variable has no name \"%s\"", token);
            return -1;
        }

        if (strcmp(v_type, "uint64_t") == 0) {
            size = 8;
        } else {
            snprintf(err, BUFFER_LEN - 1, "unknown variable type \"%s\"", v_type);
            return -1;
        }
        
        ml->name = strdup(v_name);
        ml->size = size;
        ml->stride = size;
        test->n_mem_loc += 1;
        return 0;
    }
}

void free_litmus_t(litmus_t *test) {
    int i;

    if (test->lines) {
        free(test->lines);
        test->lines = NULL;
    }
    
    if (test->spec_hash) {
        for (i = 0; i < test->spec_hash_size; ++i) {
            spec_var_t *var = test->spec_hash[i];
            while (var) {
                spec_var_t *next = var->next;
                free((void *)var->name);
                free((void *)var->value);
                var = next;
            }
            test->spec_hash[i] = NULL;
        }
        free(test->spec_hash);
        test->spec_hash = NULL;
    } 
    
    for (i = 0; i < test->n_mem_loc; ++i) {
        mem_loc_t *l = &(test->mem_loc[i]);
        free(l->name);
    }

    for (i = 0; i < test->n_tthread; ++i) {
        tthread_t *t = &(test->tthread[i]);
        free(t->name);
    }

    free(test->name);
    free(test);
}

static char *extract_var_token(const char *line, int *offset) {
    int p = *offset;
    int s;

    while (isspace(line[p]) && line[p] != '\0') p++;
    s = p;

    while ((line[p] != ';') && (line[p] != '\0')) p++;
    if (p == s)
        return NULL;
    
    if (line[p] == ';') {
        *offset = p + 1;
        p -= 1;
    } else {
        *offset = p;
    } 

    while ((p > 0) && (isspace(line[p]))) p--;
    if (p <= s) 
        return NULL;
    
    return strndup(line + s, (p - s) + 1);
}

static int parse_test(litmus_t *test) {
    litmus_t *d = test;
    int n_thread = match_thread_line(d->lines[d->thread.ptr]);
    int thread_n;
    int i;

    test->n_tthread = n_thread;
    
    /* parse thread lines to build thread instructions and registers */
    for (thread_n = 0; thread_n < n_thread; ++thread_n) {
        tthread_t *t = &(test->tthread[thread_n]);
        int lptr = d->thread.ptr;
        int i;

        t->id = thread_n;
        t->name = extract_thread(d->lines[lptr], thread_n, 0);
        t->n_reg = 0;
        t->n_ins = 0;
        for (i = 1; i < d->thread.lines; ++i) {
            const char *line = d->lines[lptr + i];
            char err[BUFFER_LEN];
            char *tl = extract_thread(line, thread_n, 1);
            int ret;

            memset(err, 0, sizeof(err));
            ret = parse_ins(t, tl, err);

            if (ret < 0) {
                log_error("parse error, line %d: %s", lptr + i, err);
                goto errout;
            } else if (ret > 0) {
                tag_ins_args(t, &(t->ins[t->n_ins]));
                t->n_ins += 1;
            }

            free(tl);
        }
    }


    /* parse variables, e.g. to set initial values */
    for (i = 0; i < d->var.lines; ++i) {
        const char *line = d->lines[d->var.ptr + i];
        char *token;
        int offset = 0;
        while ((token = extract_var_token(line, &offset)) != NULL) {
            char err[BUFFER_LEN];
            int ret = parse_var_token(test, token, err);
            free(token);
            
            if (ret < 0) {
                log_error("parse error, line %d: %s", d->var.ptr + i, err);
                goto errout;
            }
        }
    }

    return 0;
errout:
    return -1;
}

static litmus_t *prepare_and_parse(litmus_t *td) {
    int ret = prepare_config(td);
    if (ret < 0) {
        log_error("prepare_config failed (%d)", ret);
        free_litmus_t(td);
    } else {
        if (parse_test(td) == 0) {
            return td;
        } else {
            log_error("parse_test failed");
            free_litmus_t(td);
        }
    }
    return NULL;
}

litmus_t *read_test(FILE *fh) {
    litmus_t *td = read_file(fh);
    if (td) {
        return prepare_and_parse(td);
    } else {
        return NULL;
    }
}

litmus_t *load_test(const char *fn) {
    litmus_t *td = load_test_file(fn);
    if (td) {
        return prepare_and_parse(td);
    } else {
        log_error("unable to load %s", fn);
        return NULL;
    }
}

static const char *val_t_name(val_t t) {
    switch (t) {
        case T_PTR: return "PTR";
        case T_VAL2: return "2b";
        case T_VAL4: return "4b";
        case T_VAL8: return "8b";
        default: return "unknown";
    }
}

static const char *ins_name(ins_t ins) {
    switch (ins) {
        case I_LDR: return "LDR";
        case I_STR: return "STR";
        case I_MOV: return "MOV";
        case I_DMB: return "DMB";
        case I_DSB: return "DSB";
        case I_ISB: return "ISB";
        case I_EOR: return "EOR";
        case I_ADD: return "ADD";
        default: return "UNKNOWN";
    }
}

static void print_ins(ins_desc_t *ins, const char *indent) {
    log_debug_start();
    log_debug_p("%sins %s", indent, ins_name(ins->ins));
    if (ins->ins == I_DMB || ins->ins == I_DSB || ins->ins == I_ISB) {
        log_debug_p(" ");
        if (ins->flags & I_BAR_ISH) {
            log_debug_p("ISH");
        } else if (ins->flags & I_BAR_OSH) {
            log_debug_p("OSH");
        } else if (ins->flags & I_BAR_NSH) {
            log_debug_p("NSH");
        }
        if (ins->flags & I_BAR_LD) {
            log_debug_p("LD");
        } else if (ins->flags & I_BAR_ST) {
            log_debug_p("ST");
        }
        if (!(ins->flags & (I_BAR_ISH | I_BAR_OSH | I_BAR_NSH | I_BAR_LD | I_BAR_ST))) {
            log_debug_p("SY");
        }
    } else {
        log_debug_p(" size=%d", ins->size);
    }
    if (ins->flags & I_INDIRECT) {
        log_debug_p(" (indirect)");
    }
    if (ins->flags & I_OFFSET_REG) {
        log_debug_p(" (offset-reg)");
    }
    if (ins->flags & I_OFFSET_CONST) {
        log_debug_p(" (offset-const)");
    }
    if (ins->flags & I_CONST) {
        log_debug_p(" (const)");
    }
    if (ins->n_arg > 0) {
        int i;
        log_debug_p(" args: ");
        for (i = 0; i < ins->n_arg; ++i) {
            log_debug_p("%s%ld (%d)", i > 0 ? ", " : "", ins->arg[i].n, ins->arg[i].size);
        }
    }
    log_debug_end();
}

static void print_thread(tthread_t *th, const char *indent) {
    char x_indent[64];
    int i;

    snprintf(x_indent, sizeof(x_indent) - 1, "%s\t", indent);
    x_indent[sizeof(x_indent) - 1] = '\0';

    log_debug("%sthread %d \"%s\"", indent, th->id, th->name);
    for (i = 0; i < th->n_reg; ++i) {
        treg_t *r = &(th->reg[i]);
        log_debug_start();
        log_debug_p("%sreg %d \"%s\" = ", x_indent, r->n, r->name);
        if (r->t == T_PTR) {
            log_debug_p("%s mem_loc=%d", val_t_name(r->t), (int)r->v);
        } else if (r->t != T_UNKNOWN) {
            log_debug_p("%s val=0x%llx", val_t_name(r->t), (long long)r->v);
        } else {
            log_debug_p("unbound");
        }
        if (r->flags & R_OUTPUT) {
            log_debug_p(" (output)");
        }
        if (r->flags & R_ADDRESS) {
            log_debug_p(" (address)");
        }
        log_debug_end();
    }
    for (i = 0; i < th->n_ins; ++i) {
        print_ins(&(th->ins[i]), x_indent);
    }
}

void print_test(litmus_t *lt) {
    int i;

    log_debug("test = \"%s\"", lt->name);
    log_debug("# memory locations = %d", lt->n_mem_loc);
    for (i = 0; i < lt->n_mem_loc; ++i) {
        log_debug("\t%d: \"%s\" size=%d stride=%d", i, lt->mem_loc[i].name, lt->mem_loc[i].size, lt->mem_loc[i].stride);
    }

    log_debug("# test threads = %d", lt->n_tthread);
    for (i = 0; i < lt->n_tthread; ++i) {
        print_thread(&(lt->tthread[i]), "\t");
    }
}

static aopt_t *alloc_aopt(void) {
    aopt_t *ao = (aopt_t *) malloc(sizeof(aopt_t));
    ao->next = NULL;
    ao->opt = AO_UNKNOWN;
    ao->n = 1;
    ao->v = 0;
    ao->s = 0;
    ao->n_arg = 0;
    return ao;
}

static ao_t parse_ancillary_opt(const char *s) {
    if (strcmp(s, "stall-add-dep") == 0) {
        return AO_STALL_ADD_DEP;
    } else if (strcmp(s, "stall-add") == 0) {
        return AO_STALL_ADD;
    } else if (strcmp(s, "align") == 0) {
        return AO_ALIGN;
    } else if (strcmp(s, "nop") == 0) {
        return AO_NOP;
    } else if (strcmp(s, "isb") == 0) {
        return AO_ISB;
    } else if ((strncmp(s, "unq-", 4) == 0) || (strncmp(s, "shr-", 4) == 0)) {
        int res = AO_BUF_OP | ((strncmp(s, "unq", 3) == 0) ? AO_UNQ : AO_SHR);
        res |= (strncmp(s + 4, "str", 3) == 0 ? AO_STR : 0);
        res |= (strncmp(s + 4, "sts", 3) == 0 ? AO_STR | AO_UPDATE_SR : 0);
        res |= (strncmp(s + 4, "ldr", 3) == 0 ? AO_LDR : 0);
        res |= (strncmp(s + 4, "lds", 3) == 0 ? AO_LDR | AO_UPDATE_SR : 0);
        res |= (strncmp(s + 4, "inc", 3) == 0 ? AO_INC : 0);
        res |= (strncmp(s + 4, "dec", 3) == 0 ? AO_DEC : 0);
        res |= (strncmp(s + 4, "set", 3) == 0 ? AO_SET : 0);
        if (strlen(s) == 11) {
            res |= (strncmp(s + 4, "inc", 3) == 0 ? AO_INC : 0);
            res |= (strncmp(s + 4, "dec", 3) == 0 ? AO_DEC : 0);
        }
        return res;
    } else {
        return AO_UNKNOWN;
    }
}

static int n_args_for_ancillary(const ao_t op) {
    switch (op & AO_MASK) {
        case AO_UNKNOWN:        return 0;
        case AO_STALL_ADD_DEP:  return 2;
        case AO_STALL_ADD:      return 2;
        case AO_ALIGN:          return 1;
        case AO_NOP:            return 1;
        case AO_ISB:            return 1;
        case AO_BUF_OP:         return 3;
        default:                return 0;
    }
}

aopt_t *parse_ancillary(const char *s) {
    aopt_t *root = NULL;
    aopt_t *ao = NULL; 
    int end = strlen(s);
    int p = 0, e = 0;

    while (p < end) {
        char buffer[BUFFER_LEN];
        int bp = 0;
        int arg_n = 0;
        
        e = p;
        while ((s[e] != ';') && (s[e] != '\0'))
            e++;
        
        if (p == e)
            break;
        
        if (ao) {
            ao->next = alloc_aopt();
            ao = ao->next;
        } else {
            ao = root = alloc_aopt();
        }

        while (p <= e) {
            assert(bp < BUFFER_LEN);

            if ((p == e) || (s[p] == ',') || (s[p] == '\0')) {
                buffer[bp] = '\0';
                if ((arg_n == 0) || (arg_n <= n_args_for_ancillary(ao->opt))) {
                    ao->n_arg = arg_n;
                    switch (arg_n) {
                        case 0:
                            ao->opt = parse_ancillary_opt(buffer);
                            break;
                        case 1:
                            ao->n = strtol(buffer, NULL, 0);
                            break;
                        case 2:
                            ao->v = strtol(buffer, NULL, 0);
                            break;
                        case 3:
                            ao->s = strtol(buffer, NULL, 0);
                            break;
                        default:
                            assert(0);
                            break;
                    }
                } else {
                    log_error("extra arguments in ancillary \"%s\", before: %d, arg: \"%s\"", s, e, buffer);
                }
                bp = 0;
                arg_n++;
                p++;
            } else if (isspace(s[p])) {
                p++;
            } else {
                buffer[bp++] = tolower(s[p++]);
            }
        }
        
        p = e + 1;
    }

    return root;
}

void release_ancillary(aopt_t *ao) {
    while (ao != NULL) {
        aopt_t *next = ao->next;
        free(ao);
        ao = next;
    }
}
