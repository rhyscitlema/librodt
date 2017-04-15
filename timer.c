/*
    timer.c
*/

#include <_math.h>
#include <userinterface.h>
#include <timer.h>


static value timer_time;

static int timer_period = 100; // in milliseconds, 1000/100ms = 10 fps

static bool timer_enabled = false;


bool timer_handler_do()
{
    if(!timer_enabled) return false;
    timer_time = add(timer_time, setSmaRa2(timer_period, 1000));
    userinterface_update();
    return timer_enabled;
}


bool timer_set_period (int period)
{
    int t = timer_period;
    timer_period = period;
    if(period<0) period = -period;

    if(!timer_enabled)
    {   timer_set_period_do(period);
        return true;
    }
    else if((t<0?-t:t)==period)
        return true;
    else
    {   timer_period = t;
        strcpy21(errorMessage(), "Pause first!");
        return false;
    }
}


void timer_install_do ()
{ timer_time = setSmaInt(0); }

void timer_pause (bool pause)
{
    if(pause)
    {
        timer_enabled = false;
        timer_pause_do();
    }
    else
    {   timer_set_period(timer_period);
        timer_enabled = true;
    }
}

bool timer_paused() { return !timer_enabled; }

int timer_get_period () { return timer_period; }

void timer_set_time (value time) { timer_time = time; }

value timer_get_time () { return timer_time; }



//*************************
//*** Platform-specific ***
//*************************

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
    SetTimer ( (HWND)timer_window,          // handle to main window 
               IDT_TIMER,                   // timer identifier
               period,              // interval in milliseconds
               (TIMERPROC) timer_handler);  // timer callback
    #endif
    #ifdef GTK_API
    g_timeout_add ( period,
                    (GSourceFunc) timer_handler_do,
                    (gpointer) timer_window );
    #endif
}


#endif
