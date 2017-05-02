#ifndef _USERINTERFACE_H
#define _USERINTERFACE_H
/*
    userinterface.h
*/

#include <structures.h>


#define TEXT_EVAL "="
#define TEXT_PREV "Prev"
#define TEXT_NEXT "Next"
#define TEXT_DELE "Dele"
#define TEXT_CLEAR "Clear"
#define TEXT_PAUSE "||"
#define TEXT_RESUME "\u25BA"
#define TEXT_FORWARD "-->"
#define TEXT_BACKWARD "<--"
#define TEXT_LOWER "[ ]"
#define TEXT_HIGHER "[   ]"
#define TEXT_CALC "Calc"

enum UI_ITEM
{
    UI_MAIN_TEXT,
    UI_MESG_TEXT,
    UI_PATH_TEXT,
    UI_TIME_TEXT,
    UI_PAUSE_BUTTON,
    UI_FORWARD_BUTTON,
    UI_CALC_INPUT,
    UI_CALC_RESULT,
    UI_ITEM_TOTAL
};


/*** the below are provided by the platform ***/

const wchar* userinterface_get_text (enum UI_ITEM ui_item);
void userinterface_set_text (enum UI_ITEM ui_item, const wchar* text);
void userinterface_clean ();

bool main_window_resize ();
extern int main_window_width;
extern int main_window_height;

// also those from _stdio.h

// also those from timer.h

/*** the above are provided by the platform ***/


void userinterface__init ();
void userinterface__clean ();
void wait_for_draw_to_finish ();

void display_main_text (const wchar* text);
void display_message (const wchar* message);
void display_time_text();
const wchar* userMessage();

extern Container* main_entry_mfet;
void select_container (Container* container);

bool calculator_evaluate_main (const wchar* source);
bool calculator_evaluate_calc (bool parse);
void userinterface_update ();

extern volatile int draw_request_count;
extern bool graphplotter_update_repeat;


#endif
