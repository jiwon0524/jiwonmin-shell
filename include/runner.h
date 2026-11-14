#ifndef DEU_RUNNER_H
#define DEU_RUNNER_H
#include "plan.h"
int runner_init(void);
void runner_poll(void);
int runner_execute(const Plan *plan, int *exit_requested);
void runner_shutdown(void);
#endif
