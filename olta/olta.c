#include "olta.h"


static uint64_t *allocate_sync(litmus_t *lt) {
    // XXX: enable configuration?
    const int sync_size = 1024 * 1024;
    uint64_t *sync = (uint64_t *) malloc(sync_size);
    assert(sync != NULL);
    memset(sync, 1, sync_size);
    memset(sync, 0, sync_size);
    return sync;
}

static void release_sync(litmus_t *lt, uint64_t *ptr) {
    assert(ptr != NULL);
    free(ptr);
}

static unsigned int thread_buffer_size(litmus_t *lt, tthread_t *tt) {
    unsigned int bits, bytes;

    if (tt) {
        bytes = config_thread_var_int(lt, tt, "buffer-size", 64 * 1024 * 1024);
    } else {
        bytes = config_lookup_var_int(lt, "shared-buffer-size", 64 * 1024 * 1024);
    }
    
    if (bytes < 64) {
        bytes = 64;
        if (tt) {
            log_warning("%s buffer-size must be at least %d bytes, adjusting size", tt->name, bytes);
        } else {
            log_warning("shared-buffer-size must be at least %d bytes, adjusting size", bytes);
        }
    }

    bits = ((sizeof(unsigned int) * 8) - __builtin_clz(bytes)) - 1;
    if ((1 << bits) != bytes) {
        unsigned int dst = 1 << bits;
        if (dst < bytes)
            dst = 1 << (bits + 1); 
        if (tt) {
            log_warning("%s buffer-size (%d) must be power of two, rounding to %d", tt->name, bytes, dst);
        } else {
            log_warning("shared-buffer-size (%d) must be power of two, rounding up %d", bytes, dst);
        }
        bytes = dst;
    }
    
    return bytes;
}

static uint64_t *allocate_buffer(litmus_t *lt, tthread_t *tt, size_t bytes) {
    // FIXME: implement configuration: e.g. mmap, numa, etc
    uint64_t *buf = (uint64_t *) malloc(bytes);
    return buf;
}

static void release_buffer(litmus_t *lt, uint64_t *ptr, size_t bytes) {
    if (ptr)
        free(ptr);
}

static void update_mem_loc_config(litmus_t *lt, mem_loc_t *ml) {
    int stride = config_mem_loc_var_int(lt, NULL, ml, "stride", ml->stride);
    ml->stride = stride;
}

static int prepare_output_format(litmus_t *lt, char **outfmt) {
    const int n_threads = lt->n_tthread;
    char fbuf[BUFFER_LEN];
    int n_out = 0;
    int i;
    
    for (i = 0; i < lt->n_mem_loc; ++i) {
        mem_loc_t *ml = &(lt->mem_loc[i]);
        sprintf(fbuf, 
            "%s%s = 0x%%llx",
            (n_out == 0) ? "" : ", ",
            ml->name
        );
        outfmt[n_out++] = strdup(fbuf);
    }
    for (i = 0; i < n_threads; ++i) {
        tthread_t *tt = &(lt->tthread[i]);
        int j;

        for (j = 0; j < tt->n_reg; ++j) {
            if (tt->reg[j].flags & R_OUTPUT) {
                if (tt->reg[j].name == NULL || strlen(tt->reg[j].name) == 0) {
                    // FIXME: the X here is armv8 specific
                    sprintf(fbuf, 
                        "%s%s:X%d = 0x%%llx",
                        (n_out == 0) ? "" : ", ",
                        tt->name,
                        tt->reg[j].n
                    );
                } else {
                    sprintf(fbuf, 
                        "%s%s:%s = %%llu",
                        (n_out == 0) ? "" : ", ",
                        tt->name,
                        tt->reg[j].name
                    );
                }
                outfmt[n_out++] = strdup(fbuf);
            }
        }
    }

    return n_out;
}

static void fill_memory(uint64_t *mem, int size, uint64_t v, size_t bytes) {
    unsigned int cnt = bytes;

    switch (size) {
        case 1:
            memset(mem, v & 0xff, bytes);
            break;
        case 2: {
                uint16_t *p = (uint16_t *)mem;
                cnt /= 2;
                while (cnt--) *(p++) = (v & 0xff);
            }
            break;
        case 4: {
                uint32_t *p = (uint32_t *)mem;
                cnt /= 4;
                while (cnt--) *(p++) = v;
            }
            break;
        case 8: {
                uint64_t *p = (uint64_t *)mem;
                cnt /= 8;
                while (cnt--) *(p++) = v;
            }
            break;
        default:
            assert(0);
    }
}

static int prefetch_flags(int type, int level) {
    int flags = 0;
    if (type == 1)
        flags |= R_PREFETCH_KEEP;
    else if (type == 2)
        flags |= R_PREFETCH_STRM;
    if (level <= 0)
        level = 3;
    flags |= (level & 0x3) << 2;
    return flags;
}

static void run_test(litmus_t *lt, result_set_t *rs) {
    const int n_threads = lt->n_tthread;
    const int n_iterations = config_lookup_var_int(lt, "iterations", 100000);
    const int record_timing = timing_enabled(lt);
    thread_ctx_t *threads[MAX_TTHREAD];
    test_ctx_t *ctx = (test_ctx_t *) malloc(sizeof(test_ctx_t));
    uint64_t *mem_loc[MAX_MEM_LOC];
    uint64_t *sync_a = allocate_sync(lt);
    uint64_t *sync_b = allocate_sync(lt);
    size_t buffer_size = thread_buffer_size(lt, NULL);
    uint64_t *buffer = allocate_buffer(lt, NULL, buffer_size);
    uint64_t out_vs[MAX_MEM_LOC + MAX_THREAD_REG * MAX_TTHREAD];
    int i, n;
    int problems = 0;
    
    //log_sep("allocate and assemble");
   
    // create shared test context
    ctx->sync_a = sync_a;
    ctx->sync_b = sync_b;
    ctx->buffer = buffer;
    ctx->buffer_size = buffer_size;
    ctx->n_threads = n_threads;
    ctx->n_iterations = n_iterations;

    // force buffer page allocation
    if (buffer) {
        memset(buffer, 0xff, buffer_size);
        memset(buffer, 0, buffer_size);
    } else {
        log_error("buffer allocation failed");
        problems++;
    }

    // set initial sync state
    *sync_a = n_threads;
    *sync_b = n_threads;

    // configure memory locations
    for (i = 0; i < lt->n_mem_loc; ++i) {
        mem_loc_t *ml = &(lt->mem_loc[i]);
        update_mem_loc_config(lt, ml);
    }

    // allocate memory locations
    for (i = 0; i < lt->n_mem_loc; ++i) {
        mem_loc_t *ml = &(lt->mem_loc[i]);
        size_t bytes;
       
        bytes = ml->size * ml->stride * ctx->n_iterations;
        bytes += (1 << 20) - (bytes & ((1 << 20) - 1)); // round to nearest MiB

        mem_loc[i] = (uint64_t *) malloc(bytes);
        if (mem_loc[i] != NULL) {
            memset(mem_loc[i], 0xff, bytes);
            if (ml->v == 0) {
                memset(mem_loc[i], 0, bytes);
            } else {
                fill_memory(mem_loc[i], ml->size, ml->v, bytes);
            }
        } else {
            log_error("memory location allocation failed (%d bytes)", bytes);
            problems++;
        }
    }

    // create per-thread structures
    for (i = 0; i < n_threads; ++i) {
        tthread_t *tt = &(lt->tthread[i]);
        thread_ctx_t *t = (thread_ctx_t *) malloc(sizeof(thread_ctx_t));
        int j;

        threads[i] = t;
        t->affinity = config_thread_var_int(lt, tt, "affinity", -1);
        t->name = tt->name;
        t->test = ctx;
        t->buffer_size = thread_buffer_size(lt, tt);
        t->buffer = allocate_buffer(lt, tt, t->buffer_size);
        if (t->buffer == NULL) {
            log_error("thread buffer allocation failed");
            problems++;
        }
        if (record_timing) {
            t->timing = (uint64_t *) malloc(sizeof(uint64_t) * ctx->n_iterations);
        } else {
            t->timing = NULL;
        }

        for (j = 0; j < MAX_THREAD_REG; ++j) {
            t->reg[j] = 0;
            t->out[j] = NULL;
        }

        // prepare register values
        for (j = 0; j < tt->n_reg; ++j) {
            
            if (tt->reg[j].t == T_PTR) {
                const int mem_loc_n = tt->reg[j].v;
                mem_loc_t *ml = &(lt->mem_loc[mem_loc_n]);
                const int preload = config_mem_loc_var_int(lt, tt, ml, "preload", 0);
                const int flush = config_mem_loc_var_int(lt, tt, ml, "flush", 0);
                const int prefetch_ldr_type = config_mem_loc_var_int(lt, tt, ml, "prefetch-ldr-type", 0);
                const int prefetch_ldr_level = config_mem_loc_var_int(lt, tt, ml, "prefetch-ldr-level", 0);
                const int prefetch_str_type = config_mem_loc_var_int(lt, tt, ml, "prefetch-str-type", 0);
                const int prefetch_str_level = config_mem_loc_var_int(lt, tt, ml, "prefetch-str-level", 0);
                
                // update flags
                if (preload == 1)
                    tt->reg[j].flags |= R_PRELOAD;
                if (flush)
                    tt->reg[j].flags |= interpret_flush_type(flush);
                if (prefetch_ldr_type)
                    tt->reg[j].flags |= prefetch_flags(prefetch_ldr_type, prefetch_ldr_level) << R_PREFETCH_LDR_SHIFT;
                if (prefetch_str_type)
                    tt->reg[j].flags |= prefetch_flags(prefetch_str_type, prefetch_str_level) << R_PREFETCH_STR_SHIFT;

                // set memory location pointer
                t->reg[j] = (uint64_t) mem_loc[mem_loc_n];
            } else {
                t->reg[j] = tt->reg[j].v;
            }

            if (tt->reg[j].flags & R_OUTPUT) {
                t->out[j] = (uint64_t *) malloc(sizeof(uint64_t) * ctx->n_iterations);
            }
        }
        
        t->code = build_thread_code(lt, tt, t);
        if (t->code == NULL) {
            log_error("assembly failed");
            problems++;
        }
    }

    // check we are good to go
    if (problems) {
        goto out;
    }
    
    // start threads
    log_sep("run threads");
    for (i = 0; i < n_threads; ++i) {
        thread_ctx_t *t = threads[i];
        pthread_create(&(t->handle), NULL, boot_thread, t);
    }

    // wait for threads to finish
    for (i = 0; i < n_threads; ++i) {
        thread_ctx_t *t = threads[i];
        void *retval = NULL;
        pthread_join(t->handle, &retval);
    }

    // serialise results
    log_sep("correlate results");
    for (n = 0; n < ctx->n_iterations; ++n) {
        int idx = 0;
        
        for (i = 0; i < lt->n_mem_loc; ++i) {
            out_vs[idx++] = mem_loc[i][n];
        }

        for (i = 0; i < n_threads; ++i) {
            tthread_t *tt = &(lt->tthread[i]);
            thread_ctx_t *t = threads[i];
            int j;
            
            for (j = 0; j < tt->n_reg; ++j) {
                if (tt->reg[j].flags & R_OUTPUT) {
                    out_vs[idx++] = t->out[j][n];
                }
            }
        }
        result_set_add(rs, out_vs);
    }

    // timing
    if (record_timing) {
        log_sep("timing");
        for (i = 0; i < n_threads; ++i) {
            thread_ctx_t *t = threads[i];
            timing_report(lt, t);
        }
    }

out:
    // tidy up
    //log_sep("release");
    release_sync(lt, sync_a);
    release_sync(lt, sync_b);
    release_buffer(lt, buffer, buffer_size);

    for (i = 0; i < n_threads; ++i) {
        thread_ctx_t *t = threads[i];
        int j;
        
        for (j = 0; j < MAX_THREAD_REG; ++j) {
            if (t->out[j] != NULL) {
                free(t->out[j]);
            }
        }

        if (t->timing)
            free(t->timing);

        release_buffer(lt, t->buffer, t->buffer_size);
        free_thread_code(t->code);
        free(t);
    }
    free(ctx);
}

static void do_test(litmus_t *lt) {
    if (lt) {
        const int runs = config_lookup_var_int(lt, "runs", 1);
        char *outfmt[MAX_MEM_LOC + MAX_THREAD_REG * MAX_TTHREAD];
        result_set_t rs;
        int n_out;
        int i;

        log_configure_for_test(lt);

        print_test_file(lt);
        print_test(lt);

        n_out = prepare_output_format(lt, outfmt);
        result_set_init(&rs, n_out);

        for (i = 0; i < runs; ++i)
            run_test(lt, &rs);
        
        log_sep("results");
        result_set_output(&rs, (const char **)outfmt);
        result_set_release(&rs);
        
        for (i = 0; i < n_out; ++i) {
            free(outfmt[i]);
        }
        
        free_litmus_t(lt);
        log_sep("end");
    }
}

int main(int argc, char *argv[]) {
    log_init();
    affinity_init();

    if (argc >= 2) {
        const char *fn = argv[1];
        litmus_t *lt;
        
        log_sep("loading %s", fn);
        lt = load_test(fn);
        
        if (lt) {
            do_test(lt);
        } else {
            log_error("unable to load %s", fn);
        }
    } else {
        while (!feof(stdin)) {
            litmus_t *lt;

            log_sep("waiting for test");
            lt = read_test(stdin);
            do_test(lt);
        }
    }

    return log_error_count() > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
