#include <stddef.h>

#include "sched.h"
#include "timer.h"

#include "cmdline.h"
#include "dhcp.h"

#define TASK_RUN  1
#define TASK_STOP 0

typedef struct {
   void (*task_f)(void *);
   void *arg;
   int sched;
   unsigned long long next;
} task_t;

static task_t tasks[] = {
   { cmdline_task,   NULL, 0,          TASK_RUN  },
   { dhcp_task,      NULL, SCHED_STOP, TASK_STOP },
   { NULL,           NULL, SCHED_STOP, TASK_STOP }
};

void scheduler(void) {
   unsigned long long next;
   for (task_t *t = tasks; t->task_f != NULL; t++) {
      next = t->next;
      if (next == TASK_STOP || ticks < next)
         continue;
      t->task_f(t->arg);
      /* If the next time has not been overwritten, calculate it. */
      if (t->next == next) {
         if (t->sched < 0)
            t->next = TASK_STOP;
         else
            t->next = ticks + t->sched;
      }
   }
}

void sched(unsigned int tid, void *arg, int delay) {
   if (tid >= SCHED_TASK_COUNT)
      return;
   tasks[tid].arg = arg;
   if (delay < 0)
      tasks[tid].next = TASK_STOP;
   else
      tasks[tid].next = ticks + delay;
}
