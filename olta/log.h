#ifndef __LOG_H
#define __LOG_H 1

void log_init(void);
void log_configure_for_test(litmus_t *lt);
void log_sep(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_result_start(void);
void log_result_p(const char *fmt, ...);
void log_result_end(void);
void log_debug_start(void);
void log_debug_p(const char *fmt, ...);
void log_debug_end(void);
void log_debug(const char *fmt, ...);
void log_flush(void);
int log_error_count(void);

#endif /* __LOG_H */
