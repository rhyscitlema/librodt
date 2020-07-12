/*
	timer.c
*/

#include <_math.h>
#include <userinterface.h>
#include <timer.h>


static uint32_t timer_time[100];

static int timer_period = 100; // in milliseconds, 1000/100ms = 10 fps

static bool timer_enabled = false;


bool timer_handler_do()
{
	if(!timer_enabled) return false;
	value v = vnext(timer_time);
	v = setSmaInt(v, timer_period);
	v = setSmaInt(v, 1000);
	v = _add(_div(v));
	userinterface_update();
	return timer_enabled;
}


value timer_set_period (value stack)
{
	if(VERROR(stack)) return stack;
	value y = vPrev(stack);

	if(value_type(vGet(y))!=aSmaInt)
		return setError(y, L"Error: timer period must be an integer.");
	int period = getSmaInt(vGet(y));

	int t = timer_period;
	timer_period = period;
	if(period<0) period = -period;

	if(!timer_enabled)
	{
		timer_set_period_do(period);
		return setBool(y, true);
	}
	else if((t<0?-t:t)==period)
		return setBool(y, true);
	else{
		timer_period = t;
		return setError(y, L"Pause first!");
	}
}


void timer_install_do ()
{ setSmaInt(timer_time, 0); }

void timer_pause (bool pause)
{
	if(pause){
		timer_enabled = false;
		timer_pause_do();
	}
	else{
		int t = timer_period;
		timer_set_period_do(t<0?-t:t);
		timer_enabled = true;
	}
}

bool timer_paused() { return !timer_enabled; }

value timer_set_time (value stack)
{
	value y = vPrev(stack);
	const_value n = vGet(y);
	enum ValueType t = value_type(n);
	if(t!=aSmaInt && t!=aSmaFlt)
		return setError(y, L"Error: given time is not a valid number.");
	vCopy(timer_time, n);
	return setBool(y, true);
}

int timer_get_period () { return timer_period; }

value timer_get_time (value stack) { return vCopy(stack, timer_time); }



//*** Platform-specific ***
/*
#if defined(WIN32) || defined(GTK_API)

#ifdef WIN32
#include <windows.h>
#define IDT_TIMER 0
#endif
#ifdef GTK_API
#include <gtk/gtk.h>
#endif

static void* timer_window = NULL;

void timer_install (void* window)
{
	timer_window = window;
	timer_install_do();
}

void timer_pause_do ()
{
	#ifdef WIN32
	KillTimer((HWND)timer_window, IDT_TIMER);
	#endif
}

#ifdef WIN32
static void timer_handler (HWND hWnd, UINT message, UINT_PTR idEvent, DWORD dwTime)
{
	if(message !=  WM_TIMER) return;
	if(idEvent != IDT_TIMER) return;
	timer_handler_do();
}
#endif


void timer_set_period_do (int period)
{
	#ifdef WIN32
	SetTimer (
		(HWND)timer_window,          // handle to main window
		IDT_TIMER,                   // timer identifier
		period,                      // interval in milliseconds
		(TIMERPROC) timer_handler);  // timer callback
	#endif

	#ifdef GTK_API
	g_timeout_add (
		period,
		(GSourceFunc) timer_handler_do,
		(gpointer) timer_window );
	#endif
}

#endif
*/
