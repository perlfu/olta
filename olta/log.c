#include "olta.h"
#include <stdarg.h>

static pthread_mutex_t lock;
static int debug_mode = 0;

void log_init(void) {
    int r = pthread_mutex_init(&lock, NULL);
    assert(r == 0);
}

void log_configure_for_test(litmus_t *lt) {
    pthread_mutex_lock(&lock);
    debug_mode = config_lookup_var_int(lt, "log-debug", 1);
    pthread_mutex_unlock(&lock);
}

void log_sep(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "--- ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void log_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "ERROR: ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void log_warning(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "WARNING: ");
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void log_result_start(void) { }
void log_result_p(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}
void log_result_end(void) {
    fprintf(stdout, "\n");
}

void log_debug_start(void) {
    if (debug_mode >= 1) {
        pthread_mutex_lock(&lock);
    }
}
void log_debug_p(const char *fmt, ...) {
    if (debug_mode >= 1) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stdout, fmt, ap);
        va_end(ap);
    }
}
void log_debug_end(void) {
    if (debug_mode >= 1) {
        fprintf(stdout, "\n");
        pthread_mutex_unlock(&lock);
    }
}
void log_debug(const char *fmt, ...) {
    if (debug_mode >= 1) {
        va_list ap;
        va_start(ap, fmt);
        pthread_mutex_lock(&lock);
        vfprintf(stdout, fmt, ap);
        fprintf(stdout, "\n");
        pthread_mutex_unlock(&lock);
        va_end(ap);
    }
}

void log_flush(void) {
    fflush(stdout);
}
