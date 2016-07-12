#include "olta.h"

static void _nop(asm_ctx_t *ctx) {
    ctx->buf[ctx->idx++] = 0xd503201f;
}

static void _add_const(asm_ctx_t *ctx, reg_t reg, int val) {
    ctx->buf[ctx->idx++] = 0x91000000 | (reg & 0x1f) | ((reg & 0x1f) << 5) | ((val & 0xfff) << 10);
}

static void _sub_const(asm_ctx_t *ctx, reg_t reg, int val) {
    ctx->buf[ctx->idx++] = 0xd1000000 | (reg & 0x1f) | ((reg & 0x1f) << 5) | ((val & 0xfff) << 10);
}

static void _ldr_ind_by_size(asm_ctx_t *ctx, int size, reg_t dst_r, reg_t src_addr_r, int offset_idx) {
    uint32_t base = 0;
    switch (size) {
        case 1: base = 0x39400000; break;
        case 2: base = 0x79400000; break;
        case 4: base = 0xb9400000; break;
        case 8: base = 0xf9400000; break;
        default: assert(0);
    }
    ctx->buf[ctx->idx++] = base | (dst_r & 0x1f) | ((src_addr_r & 0x1f) << 5) | ((offset_idx & 0xff) << 10); 
}

static void _ldr_ind_by_size_offset_r(asm_ctx_t *ctx, int size, reg_t dst_r, reg_t src_addr_r, reg_t offset_r, int scale) {
    uint32_t base = 0;
    switch (size) {
        case 1: base = 0x38606800; break;
        case 2: base = 0x79606800; break;
        case 4: base = 0xb8606800; break;
        case 8: base = 0xf8606800; break;
        default: assert(0);
    }
    if (scale)
        base |= 0x00001000;
    ctx->buf[ctx->idx++] = base | (dst_r & 0x1f) | ((src_addr_r & 0x1f) << 5) | ((offset_r & 0x1f) << 16); 
}

static void _ldr_ind(asm_ctx_t *ctx, reg_t dst_r, reg_t src_addr_r) {
    _ldr_ind_by_size(ctx, 8, dst_r, src_addr_r, 0);
}

static void _ldr_ind_idx(asm_ctx_t *ctx, reg_t dst_r, reg_t src_addr_r, int offset_idx) {
    _ldr_ind_by_size(ctx, 8, dst_r, src_addr_r, offset_idx);
}

static void _str_ind_by_size(asm_ctx_t *ctx, int size, reg_t src_r, reg_t dst_addr_r, int offset_idx) {
    uint32_t base = 0;
    switch (size) {
        case 1: base = 0x39000000; break;
        case 2: base = 0x79000000; break;
        case 4: base = 0xb9000000; break;
        case 8: base = 0xf9000000; break;
        default: assert(0);
    }
    ctx->buf[ctx->idx++] = base | (src_r & 0x1f) | ((dst_addr_r & 0x1f) << 5) | ((offset_idx & 0xff) << 10); 
}

static void _str_ind_by_size_offset_r(asm_ctx_t *ctx, int size, reg_t src_r, reg_t dst_addr_r, reg_t offset_r, int scale) {
    uint32_t base = 0;
    switch (size) {
        case 1: base = 0x38206800; break;
        case 2: base = 0x79206800; break;
        case 4: base = 0xb8206800; break;
        case 8: base = 0xf8206800; break;
        default: assert(0);
    }
    if (scale)
        base |= 0x00001000;
    ctx->buf[ctx->idx++] = base | (src_r & 0x1f) | ((dst_addr_r & 0x1f) << 5) | ((offset_r & 0x1f) << 16); 
}

static void _str_ind(asm_ctx_t *ctx, reg_t src_r, reg_t dst_addr_r) {
    _str_ind_by_size(ctx, 8, src_r, dst_addr_r, 0);
}

static void _str_ind_r(asm_ctx_t *ctx, reg_t src_r, reg_t dst_addr_r, reg_t offset_r) {
    _str_ind_by_size_offset_r(ctx, 8, src_r, dst_addr_r, offset_r, 1);
}

__attribute__ ((unused))
static void _str_ind_idx(asm_ctx_t *ctx, reg_t src_r, reg_t dst_addr_r, int offset_idx) {
    _str_ind_by_size(ctx, 8, src_r, dst_addr_r, offset_idx);
}

static void _movz(asm_ctx_t *ctx, reg_t reg, int val, int shift) {
    const int shift_v = shift / 16;
    assert(shift == 0 || shift == 16 || shift == 32 || shift == 48);
    ctx->buf[ctx->idx++] = 0xd2800000 | (reg & 0x1f) | ((val & 0xffff) << 5) | ((shift_v & 0x3) << 21);
}

static void _movk(asm_ctx_t *ctx, reg_t reg, int val, int shift) {
    const int shift_v = shift / 16;
    assert(shift == 0 || shift == 16 || shift == 32 || shift == 48);
    ctx->buf[ctx->idx++] = 0xf2800000 | (reg & 0x1f) | ((val & 0xffff) << 5) | ((shift_v & 0x3) << 21);
}

static void _mov_const(asm_ctx_t *ctx, reg_t reg, int val) {
    _movz(ctx, reg, val, 0);
}

static void _mrs_pmccntr_el0(asm_ctx_t *ctx, reg_t dst_r) {
    /* mrs %[dst_r], pmccntr_el0 */
    ctx->buf[(ctx->idx)++] = 0xd53b9d00 | (dst_r & 0x1f);
}

static void _udiv32(asm_ctx_t *ctx, reg_t dst_r, reg_t val_r, reg_t div_r) {
    // dst_r = val_r / div_r
    ctx->buf[(ctx->idx)++] = 0x1ac00800 | (dst_r & 0x1f) | ((val_r & 0x1f) << 5) | ((div_r & 0x1f) << 16);
}

static void _msub32(asm_ctx_t *ctx, reg_t dst_r, reg_t src0_r, reg_t src1_r, reg_t acc_r) {
    // dst_r = acc_r - (src0_r * src1_r)
    ctx->buf[(ctx->idx)++] = 0x1b008000 | (dst_r & 0x1f) | ((src0_r & 0x1f) << 5) | ((src1_r & 0x1f) << 16) | ((acc_r & 0x1f) << 10);
}

__attribute__ ((unused))
static void _mov(asm_ctx_t *ctx, reg_t dst_r, reg_t src_r) {
    ctx->buf[(ctx->idx)++] = 0xaa0003e0 | (dst_r) | ((src_r & 0x1f) << 16);
}

static void _ret(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd65f03c0;
}

static void _isb(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd5033fdf;
}

static void _dsb_ish(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd5033b9f;
}

static void _dsb_ishld(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd503399f;
}

static void _dmb_sy(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd5033fbf;
}

static void _dmb_ish(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd5033bbf;
}

static void _dmb_ishst(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd5033abf;
}

__attribute__ ((unused))
static void _dsb_sy(asm_ctx_t *ctx) {
    ctx->buf[(ctx->idx)++] = 0xd5033f9f;
}

static void _nop_alignment(asm_ctx_t *ctx, int align) {
    if (align) {
        while ((ctx->idx % align) != 0) {
            _nop(ctx);
        }
    }
}

__attribute__ ((unused))
static void _nop_pad_to(asm_ctx_t *ctx, int target) {
    while (ctx->idx < target) {
        _nop(ctx);
    }
}

__attribute__ ((unused))
static void _atomic_add_const(asm_ctx_t *ctx, reg_t addr_r, int val) {
    ctx->buf[(ctx->idx)++] = 0xc85f7c00 | (addr_r << 5) | (ctx->r_tmp0); /* ldxr t0, addr_r */
    _add_const(ctx, ctx->r_tmp0, val);
    ctx->buf[(ctx->idx)++] = 0xc8007c00 | (ctx->r_tmp1 << 16) | (addr_r << 5) | (ctx->r_tmp0); /* stxr tw1, t0, addr_r  */
    ctx->buf[(ctx->idx)++] = 0x35ffffa0 | (ctx->r_tmp1); /* cbnz tw1, - */
}

static void _atomic_sub_const(asm_ctx_t *ctx, reg_t addr_r, int val) {
    ctx->buf[(ctx->idx)++] = 0xc85f7c00 | (addr_r << 5) | (ctx->r_tmp0); /* ldxr t0, addr_r */
    _sub_const(ctx, ctx->r_tmp0, val);
    ctx->buf[(ctx->idx)++] = 0xc8007c00 | (ctx->r_tmp1 << 16) | (addr_r << 5) | (ctx->r_tmp0); /* stxr tw1, t0, addr_r  */
    ctx->buf[(ctx->idx)++] = 0x35ffffa0 | (ctx->r_tmp1); /* cbnz tw1, - */
}

static void _flush_dcache(asm_ctx_t *ctx, reg_t addr_r) {
    ctx->buf[(ctx->idx)++] = 0xd50b7e20 | addr_r;
}

static void _cbz(asm_ctx_t *ctx, reg_t reg, int dist) {
    if (dist < 0) {
        dist = 0x7ffff + (dist + 1);
    }
    ctx->buf[(ctx->idx)++] = 0xb4000000 | reg | (dist << 5);
}

static void _cbnz(asm_ctx_t *ctx, reg_t reg, int dist) {
    if (dist < 0) {
        dist = 0x7ffff + (dist + 1);
    }
    ctx->buf[(ctx->idx)++] = 0xb5000000 | reg | (dist << 5);
}

static void _cmp(asm_ctx_t *ctx, reg_t reg0, reg_t reg1) {
    ctx->buf[(ctx->idx)++] = 0xeb00001f | (reg0 << 5) | (reg1 << 16);
}

__attribute__ ((unused))
static void _b(asm_ctx_t *ctx, int dist) {
    if (dist < 0) {
        dist = 0x7ffff + (dist + 1);
    }
    ctx->buf[(ctx->idx)++] = 0x14000000 | (dist << 5);
}

static void _bne(asm_ctx_t *ctx, int dist) {
    if (dist < 0) {
        dist = 0x7ffff + (dist + 1);
    }
    ctx->buf[(ctx->idx)++] = 0x54000001 | (dist << 5);
}

__attribute__ ((unused))
static void _orr(asm_ctx_t *ctx, reg_t dst_r, reg_t reg0, reg_t reg1) {
    ctx->buf[(ctx->idx)++] = 0xaa000000 | (dst_r) | (reg0 << 5) | (reg1 << 16);
}

__attribute__ ((unused))
static void _and(asm_ctx_t *ctx, reg_t dst_r, reg_t src_r, reg_t mask_r) {
    ctx->buf[(ctx->idx)++] = 0x8a000000 | (dst_r) | (src_r << 5) | (mask_r << 16);
}

__attribute__ ((unused))
static void _add(asm_ctx_t *ctx, reg_t dst_r, reg_t reg0, reg_t reg1) {
    ctx->buf[(ctx->idx)++] = 0x8b000000 | (dst_r) | (reg0 << 5) | (reg1 << 16);
}

__attribute__ ((unused))
static void _sub(asm_ctx_t *ctx, reg_t dst_r, reg_t reg0, reg_t reg1) {
    ctx->buf[(ctx->idx)++] = 0xcb000000 | (dst_r) | (reg0 << 5) | (reg1 << 16);
}

__attribute__ ((unused))
static void _lsl(asm_ctx_t *ctx, reg_t dst_r, reg_t src_r, int shift) {
    int imms = 63 - shift;
    int immr = 64 - (shift % 64);
    assert(shift != 0);
    ctx->buf[(ctx->idx)++] = 0xd3400000 | (dst_r) | (src_r << 5) | (imms << 10) | (immr << 16);
}

static void _stall(asm_ctx_t *ctx, int size) {
    int i;
    for (i = 0; i < size; ++i) {
        _add_const(ctx, ctx->r_stall, 1);
    }
}

__attribute__ ((unused))
static void _wait_for_zero(asm_ctx_t *ctx, reg_t addr_r, int stall) {
    _stall(ctx, stall);
    _dsb_ishld(ctx);
    _ldr_ind(ctx, ctx->r_tmp0, addr_r);
    _cbnz(ctx, ctx->r_tmp0, -(2 + stall));
}

static void _wait_for_not_zero(asm_ctx_t *ctx, reg_t addr_r, int stall) {
    _stall(ctx, stall);
    _dsb_ishld(ctx);
    _ldr_ind(ctx, ctx->r_tmp0, addr_r);
    _cbz(ctx, ctx->r_tmp0, -(2 + stall));
}

static void _wait_for_val(asm_ctx_t *ctx, reg_t addr_r, reg_t val_r, int stall) {
    _stall(ctx, stall);
    _dsb_ishld(ctx);
    _ldr_ind(ctx, ctx->r_tmp0, addr_r);
    _cmp(ctx, val_r, ctx->r_tmp0);
    _bne(ctx, -(3 + stall));
}

static void _data_dep(asm_ctx_t *ctx, reg_t dst_r, reg_t src_r) {
    _add(ctx, dst_r, dst_r, src_r);
}

static void _addr_dep(asm_ctx_t *ctx, reg_t dst_r, reg_t src_r) {
    if (ctx->addr_dep == ADDR_DEP_ADDSUB) {
        _add(ctx, dst_r, dst_r, src_r);
        _sub(ctx, dst_r, dst_r, src_r);
    } else {
        assert(0);
    }
}

typedef struct _bootrec_t {
    uint64_t *sync_a;
    uint64_t *sync_b;
    uint64_t threads;
    uint64_t iterations;
    uint64_t *buf_unq;
    uint64_t *buf_shr;
    uint64_t *ts_ptr;
    uint64_t reg[MAX_THREAD_REG];
    uint64_t *out[MAX_THREAD_REG];
} bootrec_t;

// word (64b) offsets in bootrec_t
enum {
    BOOTREC_SYNC_A      = 0,
    BOOTREC_SYNC_B      = 1,
    BOOTREC_THREADS     = 2,
    BOOTREC_ITERATIONS  = 3,
    BOOTREC_BUF_UNQ     = 4,
    BOOTREC_BUF_SHR     = 5,
    BOOTREC_TS_PTR      = 6,
    BOOTREC_REG         = 7,
    BOOTREC_OUT         = 7 + MAX_THREAD_REG
};


typedef struct _thread_env_t {
    int32_t *code;
    bootrec_t boot;
} thread_env_t;

static reg_t assign_register(uint32_t *map, int offset) {
    int i;
    for (i = 0; i < 28; ++i) {
        int idx = (i + offset) % 32;
        int bit;
        if (idx == 31)
            idx = 0;
        bit = 1 << idx;
        if (((*map) & bit) == 0) {
            *map |= bit;
            return idx;
        }
    }
    log_error("ran out of registers");
    assert (0);
    return X0;
}

static void build_ms_sync(asm_ctx_t *ctx, const int master, int stall) {
    if (!master) {
        _mov_const(ctx, ctx->r_tmp0, 0);
        _str_ind(ctx, ctx->r_tmp0, ctx->r_sync_b);
        _dsb_ish(ctx);
    }
    
    _atomic_sub_const(ctx, ctx->r_sync_a, 1);
    _dsb_ish(ctx);

    if (master) {
        _wait_for_zero(ctx, ctx->r_sync_a, stall);
        
        _str_ind(ctx, ctx->r_threads, ctx->r_sync_a);
        _dsb_ish(ctx);
        
        _mov_const(ctx, ctx->r_tmp0, 1);
        _str_ind(ctx, ctx->r_tmp0, ctx->r_sync_b);
        _dsb_ish(ctx);
    } else {
        _wait_for_not_zero(ctx, ctx->r_sync_b, stall);
    }
}

static void build_rr_sync(asm_ctx_t *ctx, reg_t reg, int this_n, int stall) {
    // r_tmp0 = expected sync thread, r_tmp1 = our thread id

    // compute current sync thread from iteration number
    // r_iterations must be less than 2^32
    _dsb_ish(ctx);
    _mov_const(ctx, ctx->r_tmp0, 0);
    _mov_const(ctx, ctx->r_tmp1, this_n);
    _udiv32(ctx, ctx->r_tmp0, ctx->r_iterations, ctx->r_threads);
    _msub32(ctx, ctx->r_tmp0, ctx->r_tmp0, ctx->r_threads, ctx->r_iterations);
    
    // are we the sync thread?
    _cmp(ctx, ctx->r_tmp0, ctx->r_tmp1);
    _bne(ctx, 2); // no? -> skip code
    
    // yes? -> notify sync
    _str_ind(ctx, ctx->r_tmp1, reg);

    // wait for sync (sync == sync thread #)
    _dmb_ish(ctx);
    _wait_for_val(ctx, reg, ctx->r_tmp0, stall);
}

static void build_cd_sync(asm_ctx_t *ctx, reg_t reg, int stall) {
    _atomic_sub_const(ctx, reg, 1);
    _dsb_ish(ctx);
    _wait_for_zero(ctx, reg, stall);
    _str_ind(ctx, ctx->r_threads, reg);
    _dmb_ishst(ctx);
}

static void build_sync_start(asm_ctx_t *ctx) {
    const char *sync = config_lookup_var_str(ctx->test, "sync", "rr");
    if (strcmp(sync, "ms") == 0) {
        const char *master = config_lookup_var_str(ctx->test, "sync-master", ctx->test->tthread[0].name);
        int is_master = (strcmp(master, ctx->thread->name) == 0);
        build_ms_sync(ctx, is_master, 0);
    } else if (strcmp(sync, "cd") == 0) {
        build_cd_sync(ctx, ctx->r_sync_a, 0);
    } else {
        if (strcmp(sync, "rr") != 0) {
            log_warning("unknown sync type: \"%s\", using rr instead.", sync);
        }
        build_rr_sync(ctx, ctx->r_sync_a, ctx->thread->id, 0);
    }
}

static void build_sync_end(asm_ctx_t *ctx) {
    litmus_t *test = ctx->test;
    tthread_t *th = ctx->thread;
    const char *sync = config_lookup_var_str(test, "sync", "rr");
    int sync_stall = config_thread_var_int(test, th, "sync-stall", -1);
    if (sync_stall == -1)
        sync_stall = config_lookup_var_int(test, "sync-stall", 0);
    if (sync_stall < 0)
        sync_stall = 0;

    if (strcmp(sync, "ms") == 0) {
        const char *master = config_lookup_var_str(test, "sync-master", test->tthread[0].name);
        int is_master = (strcmp(master, th->name) == 0);
        build_ms_sync(ctx, is_master, sync_stall);
    } else if (strcmp(sync, "cd") == 0) {
        build_cd_sync(ctx, ctx->r_sync_b, sync_stall);
    } else {
        if (strcmp(sync, "rr") != 0) {
            log_warning("unknown sync type: \"%s\", using rr instead.", sync);
        }
        build_rr_sync(ctx, ctx->r_sync_b, th->id, sync_stall);
    }
}

static void build_buffer_op(asm_ctx_t *ctx, aopt_t *ao_p, 
        litmus_t *test, tthread_t *th, int i_n, const char *desc) {
    const ao_t ao = ao_p->opt;
    reg_t base_r = ctx->r_tmp1, data_r = ctx->r_tmp0, limit_r = ctx->r_tmp2; 
    reg_t idx_r;
    int i, bootrec_offset, buf_size;
    int rep = ao_p->n;
    int size = ao_p->v;
    int stride = ao_p->s;
    
    if (ao & AO_UNQ) {
        idx_r = ctx->r_buf_unq_idx;
        bootrec_offset = BOOTREC_BUF_UNQ;
        buf_size = ctx->thread_ctx->buffer_size;
    } else if (ao & AO_SHR) {
        idx_r = ctx->r_buf_shr_idx;
        bootrec_offset = BOOTREC_BUF_SHR;
        buf_size = ctx->thread_ctx->test->buffer_size;
    } else {
        log_error("unknown buffer ancillary in %s:%d sequence \"%s\"", th->name, i_n, desc);
        return;
    }

    buf_size = ((sizeof(unsigned int) * 8) - __builtin_clz(buf_size)) - 1;
    
    if (size == 0) {
        size = 8;
    } else if (!(size == 1 || size == 2 || size == 4 || size == 8)) {
        log_warning("illegal size %d in ancillary in %s:%d sequence \"%s\"", size, th->name, i_n, desc);
        size = 8;
    }
    if (stride == 0) {
        stride = size;
    }
    
    if ((ao & AO_LDR) || (ao & AO_STR)) {
        _ldr_ind_idx(ctx, base_r, ctx->r_bootrec, bootrec_offset);
        _mov_const(ctx, limit_r, 1);
        _lsl(ctx, limit_r, limit_r, buf_size);
        _sub_const(ctx, limit_r, 1);
        
        for (i = 0; i < rep; ++i) {
            // FIXME: add loop rolling
            if (ao & AO_LDR) {
                if (size < 8)
                    _mov_const(ctx, data_r, 0);
                _ldr_ind_by_size_offset_r(ctx, size, data_r, base_r, idx_r, 0);
                if (ao & AO_UPDATE_SR)
                    _add(ctx, ctx->r_stall, ctx->r_stall, data_r); 
            } else {
                _str_ind_by_size_offset_r(ctx, size, ctx->r_stall, base_r, idx_r, 0);
                if (ao & AO_UPDATE_SR)
                    _add_const(ctx, ctx->r_stall, 1);
            }
            if ((ao & AO_INC) || (ao & AO_DEC)) {
                if (ao & AO_INC)
                    _add_const(ctx, idx_r, stride);
                else
                    _sub_const(ctx, idx_r, stride);
                _and(ctx, idx_r, idx_r, limit_r);
            } 
        }
    } else if ((ao & AO_INC) || (ao & AO_DEC)) {
        if (ao_p->n_arg > 0) 
            size = rep;
        else
            size = 8;
        
        if (size & (~0xfff))
            log_warning("constant overflow in ancillary %s:%d sequence \"%s\"", th->name, i_n, desc);
        
        if (ao & AO_INC)
            _add_const(ctx, idx_r, size);
        else
            _sub_const(ctx, idx_r, size);
        
        _mov_const(ctx, limit_r, 1);
        _lsl(ctx, limit_r, limit_r, buf_size);
        _sub_const(ctx, limit_r, 1);
        _and(ctx, idx_r, idx_r, limit_r);
    } else if (ao & AO_SET) {
        unsigned int ptr = ao_p->n_arg == 0 ? 0 : rep;
        
        ptr &= ((1 << buf_size) - 1); // pre-limit new ptr
        _movz(ctx, idx_r, (ptr >> 16) & 0xffff, 16);
        _movk(ctx, idx_r, (ptr & 0xffff), 0);
    }
}

static void build_ancillary(asm_ctx_t *ctx, litmus_t *test, tthread_t *th,
        ins_desc_t *pre, ins_desc_t *post, 
        int i_n, const char *desc) {
    aopt_t *root = parse_ancillary(desc);
    aopt_t *ao = root;
    int i;

    // for clarity:
    //  pre is the instruction this is an ancillary before
    //  post is the instruction this is an ancillary after

    while (ao != NULL) {
        switch (ao->opt & AO_MASK) {
            case AO_STALL_ADD_DEP:
                if (post) {
                    // try to create a data dependency between instruction and stall
                    if (post->ins == I_LDR) {
                        assert(post->n_arg >= 2);
                        _data_dep(ctx, ctx->r_stall, post->arg[1].n);
                    } else {
                        log_error("unable to add data dependency requested by %s:%d post", th->name, i_n);
                    }
                }
                _stall(ctx, ao->n);
                if (pre) {
                    // try to create an address dependency between stall and instruction
                    if (pre->ins == I_LDR || pre->ins == I_STR) {
                        assert(pre->n_arg >= 2);
                        _addr_dep(ctx, pre->arg[1].n, ctx->r_stall);
                    } else {
                        log_error("unable to add address dependency requested by %s:%d pre", th->name, i_n);
                    }
                }
                break;
            case AO_STALL_ADD:
                _stall(ctx, ao->n);
                break;
            case AO_ALIGN:
                _nop_alignment(ctx, ao->n);
                break;
            case AO_NOP:
                for (i = 0; i < ao->n; ++i)
                    _nop(ctx);
                break;
            case AO_ISB:
                for (i = 0; i < ao->n; ++i)
                    _isb(ctx);
                break;
            case AO_BUF_OP:
                build_buffer_op(ctx, ao, test, th, i_n, desc);
                break;
            default:
                log_error("unknown ancillary in %s:%d sequence \"%s\"", th->name, i_n, desc);
                break;
        }

        ao = ao->next;
    }

    release_ancillary(ao);
}

static int build_ldr(asm_ctx_t *ctx, ins_desc_t *desc) {
    if ((desc->flags & I_INDIRECT) && ((desc->n_arg == 2) || (desc->n_arg == 3))) {
        ins_arg_t *dst_a = &(desc->arg[0]);
        ins_arg_t *addr_a = &(desc->arg[1]);
        int size = dst_a->size;
        if (size == 1 || size == 2 || size == 4 || size == 8) {
            if (desc->n_arg == 2) {
                _ldr_ind_by_size(ctx, size, dst_a->n, addr_a->n, 0);
                return 0;
            } else if (desc->n_arg == 3) {
                ins_arg_t *offset_a = &(desc->arg[2]);
                if (desc->flags & I_OFFSET_CONST) {
                    _ldr_ind_by_size(ctx, size, dst_a->n, addr_a->n, offset_a->n);
                    return 0;
                } else if (desc->flags & I_OFFSET_REG) {
                    // FIXME: check if this should be scaled or not
                    _ldr_ind_by_size_offset_r(ctx, size, dst_a->n, addr_a->n, offset_a->n, 0);
                    return 0;
                }
            }
        }
    }
    return -1;
}

static int build_str(asm_ctx_t *ctx, ins_desc_t *desc) {
    if ((desc->flags & I_INDIRECT) && ((desc->n_arg == 2) || (desc->n_arg == 3))) {
        ins_arg_t *src_a = &(desc->arg[0]);
        ins_arg_t *addr_a = &(desc->arg[1]);
        int size = src_a->size;
        if (size == 1 || size == 2 || size == 4 || size == 8) {
            if (desc->n_arg == 2) {
                _str_ind_by_size(ctx, size, src_a->n, addr_a->n, 0);
                return 0;
            } else if (desc->n_arg == 3) {
                ins_arg_t *offset_a = &(desc->arg[2]);
                if (desc->flags & I_OFFSET_CONST) {
                    _str_ind_by_size(ctx, size, src_a->n, addr_a->n, offset_a->n);
                    return 0;
                } else if (desc->flags & I_OFFSET_REG) {
                    // FIXME: check if this should be scaled or not
                    _str_ind_by_size_offset_r(ctx, size, src_a->n, addr_a->n, offset_a->n, 0);
                    return 0;
                }
            }
        }
    }
    return -1;
}

static int build_instruction(asm_ctx_t *ctx, ins_desc_t *desc) {
    switch (desc->ins) {
        case I_LDR:
            return build_ldr(ctx, desc);
        case I_STR:
            return build_str(ctx, desc);
        case I_DMB_SY:
            _dmb_sy(ctx);
            return 0;
        default:
            return -1;
    }
}

static void build_test(asm_ctx_t *ctx) {
    litmus_t *test = ctx->test;
    tthread_t *th = ctx->thread;
    const char *th_pre = config_thread_var_str(test, th, "pre", NULL);
    const char *th_post = config_thread_var_str(test, th, "post", NULL);
    int i_n;

    if (th_pre)
        build_ancillary(ctx, test, th, NULL, NULL, -1, th_pre);
    for (i_n = 0; i_n < th->n_ins; ++i_n) {
        const char *pre = config_ins_var(test, th, i_n, "pre"); 
        const char *post = config_ins_var(test, th, i_n, "post");
        ins_desc_t *ins = &(th->ins[i_n]);
        int ret;
        if (pre)
            build_ancillary(ctx, test, th, ins, NULL, i_n, pre);
        ret = build_instruction(ctx, ins);
        if (post)
            build_ancillary(ctx, test, th, NULL, ins, i_n, post);

        if (ret != 0) {
            // FIXME: expand reporting
            log_error("unable to build instruction %s:%d", th->name, i_n);
        }
    }
    if (th_post)
        build_ancillary(ctx, test, th, NULL, NULL, -2, th_post);
}

static void report_code(const char *name, uint32_t *ins, int len) {
    int i;
    log_debug("/* %s = %d instructions */", name, len);
    log_debug("void _%s(void) {", name);
    log_debug("  asm volatile (\"\\n\"");
    for (i = 0; i < len; ++i) {
        log_debug("    \".word 0x%08x\\n\"", ins[i]);
    }
    log_debug("  );");
    log_debug("}");
}

void *build_thread_code(litmus_t *test, tthread_t *th, thread_ctx_t *thread_ctx) {
    const int align_loop = config_thread_var_int(test, th, "align-loop", 0);
    const int align_test = config_thread_var_int(test, th, "align-test", 0);
    const int register_shift = config_thread_var_int(test, th, "register-shift", 0);
    const int record_timing = timing_enabled(test);
    const int ins_size = 256 * 1024;
    const char *cfg;
    uint32_t *ins;
    uint32_t regmap;
    asm_ctx_t ctx;
    int i;
    
    // map executable memory
    ins = (uint32_t *) mmap(NULL, ins_size * sizeof(int32_t), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(ins != NULL && ins != ((void *)-1));
    
    // prepare context
    memset(&ctx, 0, sizeof(ctx));
    ctx.buf = ins;
    ctx.idx = 0;
    ctx.addr_dep = ADDR_DEP_ADDSUB;
    ctx.test = test;
    ctx.thread = th;
    ctx.thread_ctx = thread_ctx;

    // config context
    if ((cfg = config_lookup_var_str(test, "addr-dep", NULL))) {
        if (strcmp(cfg, "addsub") == 0) {
            ctx.addr_dep = ADDR_DEP_ADDSUB;
        } else {
            log_error("unknown addr-dep type \"%s\"", cfg);
        }
    }

    // fill buffer with NOPs
    for (i = 0; i < ins_size; ++i)
        _nop(&ctx);
    ctx.idx = 0;
    isb();
 
    // map register usage
    regmap = 0;
    for (i = 0; i < th->n_reg; ++i) {
        regmap |= (1 << th->reg[i].n);
    }
    // assign registers
    ctx.r_bootrec = assign_register(&regmap, register_shift);
    ctx.r_tmp0 = assign_register(&regmap, register_shift);
    ctx.r_tmp1 = assign_register(&regmap, register_shift);
    ctx.r_tmp2 = assign_register(&regmap, register_shift);
    ctx.r_stall = assign_register(&regmap, register_shift);
    ctx.r_sync_a = assign_register(&regmap, register_shift);
    ctx.r_sync_b = assign_register(&regmap, register_shift);
    ctx.r_threads = assign_register(&regmap, register_shift);
    ctx.r_iterations = assign_register(&regmap, register_shift);
    ctx.r_idx = assign_register(&regmap, register_shift);
    ctx.r_buf_unq_idx = assign_register(&regmap, register_shift);
    ctx.r_buf_shr_idx = assign_register(&regmap, register_shift);
    if (record_timing) {
        ctx.r_ts_start = assign_register(&regmap, register_shift);
        ctx.r_ts_end = assign_register(&regmap, register_shift);
    }
    for (i = 0; i < th->n_reg; ++i)
        ctx.r_reg[i] = assign_register(&regmap, register_shift);

    // assemble init: boot record unpack
    _mov(&ctx, ctx.r_bootrec, X0); // X0 contains bootrec PTR
    _ldr_ind_idx(&ctx, ctx.r_sync_a, ctx.r_bootrec, BOOTREC_SYNC_A);
    _ldr_ind_idx(&ctx, ctx.r_sync_b, ctx.r_bootrec, BOOTREC_SYNC_B);
    _ldr_ind_idx(&ctx, ctx.r_threads, ctx.r_bootrec, BOOTREC_THREADS);
    _ldr_ind_idx(&ctx, ctx.r_iterations, ctx.r_bootrec, BOOTREC_ITERATIONS);
    for (i = 0; i < th->n_reg; ++i)
        _ldr_ind_idx(&ctx, ctx.r_reg[i], ctx.r_bootrec, (BOOTREC_REG + i));

    // set iteration
    _mov_const(&ctx, ctx.r_idx, 0);
    _mov_const(&ctx, ctx.r_buf_unq_idx, 0);
    _mov_const(&ctx, ctx.r_buf_shr_idx, 0);
    
    // clean temporaries
    _mov_const(&ctx, ctx.r_tmp0, 0);
    _mov_const(&ctx, ctx.r_tmp1, 0);
    
    // prepare registers
    for (i = 0; i < th->n_reg; ++i) {
        _mov(&ctx, th->reg[i].n, ctx.r_reg[i]);
    }
    
    // align loop
    _nop_alignment(&ctx, align_loop);
    
    // store loop head
    ctx.l_head = ctx.idx;

    // implement explicit preload/flush
    for (i = 0; i < th->n_reg; ++i) {
        treg_t *r = &(th->reg[i]);
        if (r->t == T_PTR) {
            if (r->flags & R_PRELOAD) {
                _ldr_ind(&ctx, ctx.r_tmp0, r->n);
                _dsb_ish(&ctx);
            } else if (r->flags & R_FLUSH) {
                _flush_dcache(&ctx, r->n);
                _dsb_ish(&ctx);
            }
        }
    }

    // sync threads
    build_sync_start(&ctx);

    // get start time
    if (record_timing) {
        _isb(&ctx);
        _mrs_pmccntr_el0(&ctx, ctx.r_ts_start);
    }
    
    // align test
    _nop_alignment(&ctx, align_test);

    // run test
    build_test(&ctx);

    // get end time
    if (record_timing) {
        _isb(&ctx);
        _mrs_pmccntr_el0(&ctx, ctx.r_ts_end);
    }

    // sync threads
    build_sync_end(&ctx);

    // inter-iteration
    if (record_timing) {
        _ldr_ind_idx(&ctx, ctx.r_tmp0, ctx.r_bootrec, BOOTREC_TS_PTR);
        _sub(&ctx, ctx.r_tmp1, ctx.r_ts_end, ctx.r_ts_start);
        _str_ind_r(&ctx, ctx.r_tmp1, ctx.r_tmp0, ctx.r_idx);
    }
    for (i = 0; i < th->n_reg; ++i) {
        treg_t *r = &(th->reg[i]);
        // save results
        if (r->flags & R_OUTPUT) {
            _ldr_ind_idx(&ctx, ctx.r_tmp0, ctx.r_bootrec, (BOOTREC_OUT + i));
            _str_ind_r(&ctx, r->n, ctx.r_tmp0, ctx.r_idx);
        }
        // update pointer
        if (r->t == T_PTR) {
            mem_loc_t *ml = &(test->mem_loc[r->v]);
            _add_const(&ctx, ctx.r_reg[i], ml->stride); 
        }
        // prepare register
        _mov(&ctx, th->reg[i].n, ctx.r_reg[i]);
    }
    
    // clean temporaries
    _mov_const(&ctx, ctx.r_tmp0, 0);
    _mov_const(&ctx, ctx.r_tmp1, 0);

    // loop
    _add_const(&ctx, ctx.r_idx, 1);
    _sub_const(&ctx, ctx.r_iterations, 1);
    _cbnz(&ctx, ctx.r_iterations, ctx.l_head - ctx.idx);

    // tidy up
    _ret(&ctx);

    // debug
    if (config_lookup_var_int(test, "debug-asm-dump", 0) == 1) {
        report_code(th->name, ins, ctx.idx);
    } else {
        log_debug("assembled \"%s\", %d instructions", th->name, ctx.idx);
    }

    return ins;
}

void free_thread_code(void *code) {
    const int ins_size = 256 * 1024;
    int ret = munmap(code, ins_size * sizeof(int32_t));
    assert(ret == 0);
}

void *boot_thread(void *t) {
    thread_ctx_t *ctx = (thread_ctx_t *)t;
    bootrec_t br;
    int i;

    log_debug("started \"%s\", affinity: %d", ctx->name, ctx->affinity);
    affinity_set(ctx->affinity);

    // force page allocation of unique buffer after affinity is set
    memset(ctx->buffer, 0xff, ctx->buffer_size);
    memset(ctx->buffer, 0, ctx->buffer_size);

    if (ctx->timing) {
        const int bytes = sizeof(uint64_t) * ctx->test->n_iterations;
        memset(ctx->timing, 0xff, bytes);
        memset(ctx->timing, 0, bytes);
    }

    br.sync_a = ctx->test->sync_a;
    br.sync_b = ctx->test->sync_b;
    br.threads = ctx->test->n_threads;
    br.iterations = ctx->test->n_iterations;
    br.buf_shr = ctx->test->buffer;
    br.buf_unq = ctx->buffer;
    br.ts_ptr = ctx->timing;
    for (i = 0; i < MAX_THREAD_REG; ++i) {
        br.reg[i] = ctx->reg[i];
        br.out[i] = ctx->out[i];
    }

    full_barrier();

#if defined(ARM_HOST)
    asm volatile ("\n\t"
        "sub sp, sp, #16\n\t"
        "stp x26, x27, [sp]\n\t"
        "mov x0, %[brptr]\n\t"
        "mov x1, %[cptr]\n\t"
        "blr x1\n\t"
        "ldp x26, x27, [sp]\n\t"
        "add sp, sp, #16\n\t"
        : 
        : [cptr] "r" (ctx->code), [brptr] "r" (&br)
        : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "memory"
    );
#else
    assert(0);
#endif

    log_debug("finished \"%s\"", ctx->name);

    return NULL;
}

static volatile int caught_signal = 0;

static void signal_action(int sig, siginfo_t *si, void *data) {
#if defined(ARM_HOST)
    ucontext_t *uc = (ucontext_t *)data;
    caught_signal = 1;
    /* skip instruction */
    uc->uc_mcontext.pc += 4;
#endif /* ARM_HOST */
}

int has_pmu_access(void) {
    struct sigaction act, oldact;
    caught_signal = 0;

    memset(&act, 0, sizeof(act));
    act.sa_sigaction = signal_action;
    act.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;

    sigaction(SIGILL, &act, &oldact);
#if defined(ARM_HOST)
    asm volatile (".word 0xd53b9d00" : : : "x0", "memory");
#endif /* ARM_HOST */
    sigaction(SIGILL, &oldact, NULL);

    return caught_signal == 0 ? 1 : 0;
}