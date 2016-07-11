#ifndef __OLTA_H
#define __OLTA_H 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>

#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#include <sys/mman.h>
#include <errno.h>

#define BUFFER_LEN (4096)

#if defined(__arm__) || defined(__aarch64__)
#define ARM_HOST 1
#endif

#include "litmus.h"
#include "parse.h"
#include "affinity.h"
#include "arm.h"
#include "results.h"
#include "log.h"
#include "timing.h"

#endif /* __OLTA_H */
