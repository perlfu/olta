#include "olta.h"

static int cached_test_id = -1;
static int cached_answer = 0;

int timing_enabled(litmus_t *test) {
    if (test->id != cached_test_id) {
        int record_timing = config_lookup_var_int(test, "record-timing", 0);
        if (record_timing) {
            if (!has_pmu_access()) {
                log_warning("record-timing set but PMU access not available; no timing will be reported");
                record_timing = 0;
            }
        }
        cached_test_id = test->id;
        cached_answer = record_timing;
    }
    return cached_answer;
}

void timing_report(litmus_t *lt, thread_ctx_t *t) {
    int i;
    log_result_start();
    log_result_p("timing,%s", t->name);
    for (i = 0; i < t->test->n_iterations; ++i) {
        log_result_p(",%llu", (long long unsigned) t->timing[i]);
    }
    log_result_end();
}
