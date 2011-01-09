#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "breakfast.h"

#if defined(__i386)
#define REGISTER_IP EIP
#define TRAP_LEN    1
#define TRAP_INST   0xCC
#define TRAP_MASK   0xFFFFFF00

#elif defined(__x86_64)
#define REGISTER_IP RIP
#define TRAP_LEN    1
#define TRAP_INST   0xCC
#define TRAP_MASK   0xFFFFFFFFFFFFFF00

#else
#error Unsupported architecture
#endif

void breakfast_attach(pid_t pid) {
  int status;
  ptrace(PTRACE_ATTACH, pid);
  waitpid(pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACEEXIT);
}

target_addr_t breakfast_getip(pid_t pid) {
  long v = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*REGISTER_IP);
  return (target_addr_t) (v - TRAP_LEN);
}

struct breakpoint {
  target_addr_t addr;
  long orig_code;
};

static void enable(pid_t pid, struct breakpoint *bp) {
  long orig = ptrace(PTRACE_PEEKTEXT, pid, bp->addr);
  ptrace(PTRACE_POKETEXT, pid, bp->addr, (orig & TRAP_MASK) | TRAP_INST);
  bp->orig_code = orig;
}

static void disable(pid_t pid, struct breakpoint *bp) {
  ptrace(PTRACE_POKETEXT, pid, bp->addr, bp->orig_code);
}

struct breakpoint *breakfast_break(pid_t pid, target_addr_t addr) {
  struct breakpoint *bp = malloc(sizeof(*bp));
  bp->addr = addr;
  enable(pid, bp);
  return bp;
}

static int run(pid_t pid, int cmd);

int breakfast_run(pid_t pid, struct breakpoint *bp) {
  if (bp) {
    ptrace(PTRACE_POKEUSER, pid, sizeof(long)*REGISTER_IP, bp->addr);

    disable(pid, bp);
    if (!run(pid, PTRACE_SINGLESTEP))
      return 0;
    enable(pid, bp);
  }
  return run(pid, PTRACE_CONT);
}

static int run(pid_t pid, int cmd) {
  int status, last_sig = 0, event;
  while (1) {
    ptrace(cmd, pid, 0, last_sig);
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
      return 0;

    if (WIFSTOPPED(status)) {
      last_sig = WSTOPSIG(status);
      if (last_sig == SIGTRAP) {
        event = (status >> 16) & 0xffff;
        return (event == PTRACE_EVENT_EXIT) ? 0 : 1;
      }
    }
  }
}
