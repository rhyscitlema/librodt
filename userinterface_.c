/*
    userinterface_.c
*/

#include <_stdio.h>
#include <rfet.h>
#include <timer.h>
#include <tools.h>
#include <mouse.h>
#include <surface.h>
#include <userinterface.h>


void display_main_text (const wchar* text)
{
    if(text==NULL) text = userMessage();
    userinterface_set_text (UI_MAIN_TEXT, text);
    userinterface_set_text (UI_PATH_TEXT, NULL);
    mouse_clear_pointers(headMouse);
}

void display_message (const wchar* message)
{
    if(message) puts2(message);
    userinterface_set_text (UI_MESG_TEXT, message);
}


static value calculator_evaluate (value stack, const wchar* source, Container** rfet_ptr, enum UI_ITEM input, enum UI_ITEM output)
{
    assert(rfet_ptr && stack);
    Container* rfet;
    const wchar* out = NULL;
    if(!strEnd2(source))
    {
        const wchar* entry = userinterface_get_text(input);
        Str3 text = astrcpy32(C37(NULL), entry, source);

        rfet = (Container*)rfet_parse(stack, *rfet_ptr, text); text=C37(NULL);
        if(rfet)
        {
            *rfet_ptr = rfet;
            if(!rfet->owner)
                rfet->owner = rfet_ptr;

            // skip rfet_evaluate(), as it will later be called via
            // calculator_evaluate_main(0) in userinterface_process().
            if(input==UI_MAIN_TEXT) rfet=NULL;

            stack = setBool(stack, true);
        }
        else
        {   out = getMessage(vGet(stack));
            stack = vnext(stack); // go to after VT_MESSAGE
            assert(VERROR(stack));
        }
    }
    else rfet = *rfet_ptr;

    if(rfet)
    {   stack = rfet_evaluate(stack, rfet, NULL);
        if(VERROR(stack))
            out = getMessage(vGetPrev(stack)); // get error message
        else
        {
            if(rfet_commit_replacement(stack, rfet))
            {
                rfet_get_container_text(stack, rfet);
                out = getStr2(vGet(stack));
                userinterface_set_text(input, out);
            }
            stack = VstToStr(stack, PUT_NEWLINE|0, -1, -1); // see _math.h
            out = getStr2(vGetPrev(stack)); // get final result
        }
    }
    if(out) userinterface_set_text(output, out);
    return stack;
}

       Container* main_entry_rfet = NULL;
static Container* calculator_rfet = NULL;

value calculator_evaluate_main (value stack, const wchar* source)
{ return calculator_evaluate (stack, source, &main_entry_rfet, UI_MAIN_TEXT, UI_MESG_TEXT); }

value calculator_evaluate_calc (value stack, bool parse)
{
  if(stack==NULL) stack = stackArray();
  const wchar* source = C21(parse ? TEXT_CALC : NULL);
  return calculator_evaluate (stack, source, &calculator_rfet, UI_CALC_INPUT, UI_CALC_RESULT);
}


void display_time_text()
{
    uint32_t V[1000], *v=V;
    v = setSmaInt(v, timer_get_period());
    v = timer_get_time(v);
    v = tovector(v, 2);
    v = VstToStr(v, 0,-1,-1);
    const wchar* out = getStr2(vGetPrev(v));
    userinterface_set_text(UI_TIME_TEXT, out);
}

static void userinterface_process (bool skipmain)
{
    switch(headMouse->showSignature)
    {
        case 1: select_container(headMouse->clickedObject->container); break;
        case 2: display_main_text(NULL); break;
    }
    headMouse->showSignature = 0;
    value stack = stackArray();

    if(!skipmain)
     calculator_evaluate_main(stack, NULL);
    calculator_evaluate_calc(stack, false);
    display_time_text();

    if(!headMouse->ButtonBefore[Key_Space] && headMouse->Button[Key_Space]) tools_do_pause(!timer_paused());

    if(headMouse->Button[Key_Ctrl] && headMouse->Button[Key_Shift])
    {
             if(!headMouse->ButtonBefore[Key_R] && headMouse->Button[Key_R]) tools_go_forward(timer_get_period()<0);
        else if(!headMouse->ButtonBefore[Key_F] && headMouse->Button[Key_F]) tools_lower_period();
        else if(!headMouse->ButtonBefore[Key_S] && headMouse->Button[Key_S]) tools_higher_period();
    }
    mouse_record(headMouse);
    mouse_clear_motion(headMouse);
}


/***************************************************************************************************/


#ifdef MTHREAD
#include <mthread.h>
static bool threaded = true;
#else
static bool threaded = false;

typedef void* mthread_thread;
static inline mthread_thread mthread_thread_new (void* (*function) (void* argument), void* argument){return NULL;}
static inline void mthread_thread_join (mthread_thread thread){}

typedef void* mthread_mutex;
static inline mthread_mutex mthread_mutex_new(){return NULL;}
static inline bool mthread_mutex_lock   (mthread_mutex mutex, bool wait){return true;}
static inline void mthread_mutex_unlock (mthread_mutex mutex){}
static inline void mthread_mutex_free   (mthread_mutex mutex){}

typedef void* mthread_signal;
static inline mthread_signal mthread_signal_new(){return NULL;}
static inline void mthread_signal_send (mthread_signal signal){}
static inline void mthread_signal_wait (mthread_signal signal){}
static inline void mthread_signal_free (mthread_signal signal){}
#endif


volatile int draw_request_count;
bool graphplotter_update_repeat;

enum DRAW_STATE
{   WAITING,
    DRAWING,
    DO_DRAW
};
static enum DRAW_STATE draw_state = WAITING;

static bool inform_on_finish = false;

static mthread_thread thread = NULL;
static mthread_mutex  mutex  = NULL;
static mthread_signal signal = NULL;
static mthread_signal finish = NULL;


static bool objects_do_process (bool update)
{
    Object *object, *next;

    object = (Object*)list_head(camera_list());
    for( ; object != NULL; object = next)
    {
        next = (Object*)list_next(object);
        if(!object->container)
            object->destroy(object);
        else object->process(object, update);
    }
    object = (Object*)list_head(surface_list());
    for( ; object != NULL; object = next)
    {
        next = (Object*)list_next(object);
        if(!object->container)
            object->destroy(object);
        else object->process(object, update);
    }
    return true;
}


static void* graphplotter_draw (void* argument)
{
    finish = mthread_signal_new();
    while(true)
    {
        if(draw_state != DO_DRAW)
        {
            draw_state = WAITING;
            if(!threaded) break;
            if(inform_on_finish)
             mthread_signal_send(finish);
            mthread_signal_wait(signal);
            continue; // check again
        }
        draw_state = DRAWING;

    #ifdef LIBRODT
        mthread_mutex_lock(mutex,true);
        objects_do_process(false);
        mthread_mutex_unlock(mutex);

        Object* object;
        void* camera;
        draw_request_count=0;

        for(camera = list_head(camera_list()); camera != NULL; camera = list_next(camera))
        {   draw_request_count++;
            camera_paint_initialise((Camera*)camera);
        }

        for(camera = list_head(camera_list()); camera != NULL; camera = list_next(camera))
        {
            object = (Object*)list_head(surface_list());
            for( ; object != NULL; object = (Object*)list_next(object))
                object->paint(object, (Camera*)camera);

            object = (Object*)list_head(camera_list());
            for( ; object != NULL; object = (Object*)list_next(object))
                object->paint(object, (Camera*)camera);
        }

        for(camera = list_head(camera_list()); camera != NULL; camera = list_next(camera))
            camera_paint_finalise((Camera*)camera);
    #endif
    }
    mthread_signal_free(finish);
    finish = NULL;
    return NULL;
}


void userinterface_update()
{
    if(!mthread_mutex_lock(mutex,false)) return;
    evaluation_instance(true);
    while(true)
    {
        graphplotter_update_repeat = false;
        bool ok = objects_do_process(true); // will 'use' user inputs
        userinterface_process(!ok);         // will clear user inputs
        if(!graphplotter_update_repeat) break;
    }
    if(draw_request_count<=0)
    {
        draw_state = DO_DRAW;
        if(!threaded) graphplotter_draw(NULL);
        else mthread_signal_send(signal);
    }
    mthread_mutex_unlock(mutex);
}


void wait_for_draw_to_finish()
{
    while(draw_state != WAITING)
    {   inform_on_finish = true;
        mthread_signal_wait(finish);
    } inform_on_finish = false;
}

void userinterface__init()
{
    mutex  = mthread_mutex_new();
    signal = mthread_signal_new();
    thread = mthread_thread_new(graphplotter_draw, NULL);
    // note: thread creation must be done last
}

void userinterface__clean()
{
    threaded = false; // set before sending signal
    mthread_signal_send(signal);
    // note: thread termination must be done first
    mthread_signal_free(signal);
    mthread_mutex_free(mutex);
    thread = mutex = signal = NULL;
}

