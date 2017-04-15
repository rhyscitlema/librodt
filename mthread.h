#ifndef _MTHREAD_H
#define _MTHREAD_H
/*
	mthread.h

	interface for using multi-threading
*/
#include <_stddef.h> // for 'bool' only

typedef void* mthread_thread;
mthread_thread mthread_thread_new (void* (*function) (void* argument), void* argument);
void mthread_thread_join (mthread_thread thread);

typedef void* mthread_mutex;
mthread_mutex mthread_mutex_new();
bool mthread_mutex_lock   (mthread_mutex mutex, bool wait);
void mthread_mutex_unlock (mthread_mutex mutex);
void mthread_mutex_free   (mthread_mutex mutex);

typedef void* mthread_signal;
mthread_signal mthread_signal_new();
void mthread_signal_send (mthread_signal signal);
void mthread_signal_wait (mthread_signal signal);
void mthread_signal_free (mthread_signal signal);

#endif

