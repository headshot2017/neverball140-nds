#include <nds.h>

#include "timer.h"

// the speed of the timer when using ClockDivider_1024
#define TIMER_SPEED (BUS_CLOCK/1024)

static u64 time;

void timer_init(void)
{
	timerStart(0, ClockDivider_1024, 0, NULL);
	time = 0;
}

void timer_free(void)
{
	timerStop(0);
}

uint64_t timer_get(void)
{
	uint32_t elapsed = timerElapsed(0);
	uint32_t ms = f32toint(mulf32(divf32(inttof32(elapsed), inttof32(TIMER_SPEED)), inttof32(1000)));

	time += ms;

	return time;
}

int timer_freq(void)
{
	return 1000;
}
