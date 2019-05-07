#ifndef _ftime_h
#define _ftime_h


extern volatile uint64_t current_cycle_count;

extern cvmx_wqe_t *g_timer_event;

static void timer_handler(int irq, uint64_t regs[32], void *arg);

int InitPIT(void);




#endif
