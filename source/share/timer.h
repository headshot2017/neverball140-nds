#ifndef TIMER_H
#define TIMER_H

/*---------------------------------------------------------------------------*/

void timer_init(void);
void timer_free(void);
uint64_t timer_get(void);
int timer_freq(void);

/*---------------------------------------------------------------------------*/

#endif
