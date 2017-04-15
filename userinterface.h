#ifndef _USERINTERFACE_H
#define _USERINTERFACE_H
/*
    userinterface.h
*/

#include <structures.h>


#define TEXT_PAUSE "||"
#define TEXT_RESUME ">>"
#define TEXT_FORWARD "-->"
#define TEXT_BACKWARD "<--"
#define TEXT_LOWER "[ ]"
#define TEXT_HIGHER "[   ]"
#define TEXT_CALC "Calc"

enum UI_ITEM
{
    UI_main_text,
    UI_mesg_text,
    UI_path_text,
    UI_time_text,
    UI_pause_button,
    UI_forward_button,
    UI_calc_input,
    UI_calc_result,
    UI_ITEM_TOTAL
};


//---- the below are provided by the platform ----

const mchar* userinterface_get_text (enum UI_ITEM ui_item);
void userinterface_set_text (enum UI_ITEM ui_item, const mchar* text);
void userinterface_clean ();

bool main_window_resize ();
extern int main_window_width;
extern int main_window_height;

// also those from _stdio.h

// also those from timer.h

//---- the above are provided by the platform ----


void userinterface__init ();
void userinterface__clean ();
void wait_for_draw_to_finish ();

void display_main_text (const mchar* text);
void display_message (const mchar* message);
void display_time_text();
const mchar* userMessage();

extern Container* main_entry_mfet;
void select_container (Container* container);

bool calculator_evaluate_main (const mchar* source);
bool calculator_evaluate_calc (bool parse);
void userinterface_update ();

extern volatile int draw_request_count;
extern bool graphplotter_update_repeat;


#endif
