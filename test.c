#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

// only needed for reading fact() arguments
#include <sys/ptrace.h>
#include <sys/reg.h>

#include "breakfast.h"

void child();
void parent(pid_t);

int main() {
  pid_t pid;
  if ((pid = fork()) == 0)
    child();
  else
    parent(pid);
  return 0;
}

int fact(int n) {
  if (n <= 1)
    return 1;
  return n * fact(n-1);
}

void child() {
  kill(getpid(), SIGSTOP);
  printf("fact(5) = %d\n", fact(5));
}

void parent(pid_t pid) {
  struct breakpoint *fact_break, *last_break = NULL;
  void *fact_ip = fact, *last_ip;
  breakfast_attach(pid);
  fact_break = breakfast_break(pid, fact_ip);
  while (breakfast_run(pid, last_break)) {
    last_ip = breakfast_getip(pid);
    if (last_ip == fact_ip) {
#if defined(__amd64)
      int arg = ptrace(PTRACE_PEEKUSER, pid, sizeof(long)*RDI);
      printf("Break at fact(%d)\n", arg);
#else
      printf("Break at fact()\n");
#endif
      last_break = fact_break;
    } else {
      printf("Unknown trap at %p\n", last_ip);
      last_break = NULL;
    }
  }
}
