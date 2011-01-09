#ifndef _BREAKFAST_H
#define _BREAKFAST_H

#include <sys/types.h>  /* for pid_t */

typedef void *target_addr_t;
struct breakpoint;

void breakfast_attach(pid_t pid);
target_addr_t breakfast_getip(pid_t pid);
struct breakpoint *breakfast_break(pid_t pid, target_addr_t addr);
int breakfast_run(pid_t pid, struct breakpoint *bp);

#endif
