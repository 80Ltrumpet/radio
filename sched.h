#ifndef _SCHED_H
#define _SCHED_H

#define SCHED_STOP -1

enum {
   CMDLINE_TASK,
   DHCP_TASK,
   SCHED_TASK_COUNT // Must be last
};

void scheduler(void);
void sched(unsigned int tid, void *arg, int delay);

#endif
