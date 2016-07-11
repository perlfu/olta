#ifndef __TIMING_H
#define __TIMING_H 1

int timing_enabled(litmus_t *lt);
void timing_report(litmus_t *lt, thread_ctx_t *t);

#endif /* __TIMING_H */
