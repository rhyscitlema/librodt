#ifndef _TIMER_H
#define _TIMER_H
/*
	timer.h
*/

#include <_stddef.h>


// NOTE: use tools_do_pause(pause) instead
void timer_pause (bool pause);

bool timer_paused ();

value timer_set_period (value stack); // period = vPrev(stack);

int timer_get_period (); // note: period in millisecodns

value timer_set_time (value stack); // time = vPrev(stack);

value timer_get_time (value stack);


void timer_install_do ();

bool timer_handler_do ();


//*** Platform-specific ***

void timer_set_period_do (int period);

void timer_pause_do ();


#endif
