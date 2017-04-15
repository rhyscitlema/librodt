/*
    tools.h
*/
#ifndef _TOOLS_H
#define _TOOLS_H

#include <_stddef.h>

extern bool tools_uidt_eval (int layout[][4], void* uidt);

extern void tools_init (int stack_size, const mchar* uidt_mfet);

extern void tools_clean(); // to free memory, best called upon software exit


extern void tools_do_eval (const mchar* source);

extern void tools_do_select();

extern void tools_do_clear();

extern void tools_do_delete();

extern void tools_get_next();

extern void tools_get_prev();


extern void tools_do_pause (bool pause);

extern void tools_go_forward (bool forward);

extern void tools_lower_period();

extern void tools_higher_period();

extern bool tools_set_time (const mchar* entry);


extern void tools_remove_all_objects (bool ask_confirmation);

#endif
