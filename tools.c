/*
    tools.c
*/

#include <_stdio.h>
#include <_texts.h>
#include <files.h>
#include <mfet.h>
#include <outsider.h>
#include <userinterface.h>
#include <surface.h>
#include <camera.h>
#include <mouse.h>
#include <timer.h>
#include <tools.h>

#define errmsg errorMessage()



SmaFlt PIXELSIZE;

static Container* base_surface = NULL;
static Container* base_camera = NULL;
static Container* base_object = NULL;
static Container* base_rodt = NULL;
static Container* base_uidt = NULL;
static Container* curr_uidt = NULL;

static const wchar base_uidt_mfet[];
static const wchar base_rodt_rodt[];

static const char* uidt_name[] = {
    "main_textfield",

    "eval_button",
    "message_textfield",
    "lock_button",

    "prev_button",
    "next_button",
    "delete_button",
    "clear_button",
    "path_textfield",

    "pause_button",
    "forward_button",
    "lower_button",
    "higher_button",
    "time_textfield",

    "calc_button",
    "calc_input",
    "calc_result",

    "status_bar"
};
static const int uidt_count = SIZEOF(uidt_name);

static int Layout[20][4];



bool tools_uidt_eval (int layout[][4], void* _uidt)
{
    if(!curr_uidt) curr_uidt = base_uidt;
    Container* uidt = (Container*)_uidt;
    if(!uidt) uidt = curr_uidt;

    GeneralArg garg = { .caller = uidt, .message = errmsg, .argument = NULL };
    ExprCallArg eca = { .garg = &garg, .expression = NULL, .stack = mainStack() };
    Component* component;

    int i;
    const lchar* name;
    memset(layout, 0, uidt_count*4*sizeof(int));

    for(i=0; i < uidt_count; i++)
    {
        name = CST31(uidt_name[i]);
        component = component_parse(component_find(uidt,name,errmsg,0));
        if(!component_evaluate(eca, component, VST41)
        || !integerFromVst(layout[i], eca.stack, 4, errmsg, uidt_name[i])) break;
    }
    while(i==uidt_count // if there was no error
        && _uidt!=NULL) // if checking for error
    {
        // note: not a loop
        i=0; // assume an error

        name = CST31("lower_period");
        component = component_parse(component_find(uidt,name,errmsg,0));
        if(!component_evaluate(eca, component, VST11)
        || !integerFromVst(NULL, eca.stack, 1, errmsg, "period")) break;

        name = CST31("higher_period");
        component = component_parse(component_find(uidt,name,errmsg,0));
        if(!component_evaluate(eca, component, VST11)
        || !integerFromVst(NULL, eca.stack, 1, errmsg, "period")) break;

        #ifdef LIBRODT
        SmaFlt pixelsize;
        name = CST31("PixelSize");
        component = component_parse(component_find(uidt,name,errmsg,0));
        if(!component_evaluate(eca, component, VST11)
        || !floatFromVst(&pixelsize, eca.stack, 1, errmsg, "PixelSize")) break;
        PIXELSIZE = pixelsize;
        #endif

        i=uidt_count; // no error found
        curr_uidt = uidt;
        uidt->owner = &curr_uidt;
        break;
    }
    return (i==uidt_count);
}


static wchar* usermessage = NULL;
const wchar* userMessage() { return usermessage; }

static bool set_user_message (Container* rodt)
{
    GeneralArg garg = { .caller = rodt, .message = errmsg, .argument = NULL };
    ExprCallArg eca = { .garg = &garg, .expression = NULL, .stack = mainStack() };

    Component* component = component_find(rodt, CST31("message"), errmsg, 0);
    if(component)
    {   component = component_parse(component);
        if(!component_evaluate(eca, component, VST11)) return false;

        if(!isString(eca.stack[0]))
        {
            set_message(eca.garg->message,
                L"Error in \\1 at \\2:\\3:\r\b '\\4' must evaluate to a string.",
                c_name(component));
            return false;
        }
        lchar* lstr = getString(eca.stack[0]);
        astrcpy23(&usermessage, lstr);
        lchar_free(lstr);
    }
    return true;
}


void tools_init (int stack_size, const wchar* uidt_mfet)
{
    lchar* text=NULL;
    bool success;

    mouse_init();
    mfet_init(stack_size);

    // 1) set BASE UIDT
    if(!uidt_mfet || !uidt_mfet[0])
        uidt_mfet = base_uidt_mfet;
    astrcpy32(&text, uidt_mfet);
    set_line_coln_source(text, 1, 1, L"UIDT");

    success=false;
    base_uidt = container_parse(NULL, NULL, text); text=NULL;
    if(base_uidt
    && dependence_parse()
    && tools_uidt_eval(Layout, base_uidt))
        success = true;
    else base_uidt = NULL;
    dependence_finalise(success);
    if(base_uidt==NULL) puts2(errorMessage());
    assert(base_uidt!=NULL);

    #ifdef LIBRODT
    // 2) set BASE RODT
    astrcpy32(&text, base_rodt_rodt);
    set_line_coln_source(text, 1, 1, L"RODT");
    base_rodt = (Container*)mfet_parse(NULL, text); text=NULL;
    assert(base_rodt!=NULL);
    set_user_message(base_rodt);

    // 3) set base objects
    base_object  = container_find(base_rodt, CST31(".|Object" ), 0,0,0);
    base_camera  = container_find(base_rodt, CST31(".|Camera" ), 0,0,0);
    base_surface = container_find(base_rodt, CST31(".|Surface"), 0,0,0);
    assert(base_object!=NULL);
    assert(base_camera!=NULL);
    assert(base_surface!=NULL);
    #else
    CST12(base_rodt_rodt); // just a dirty way to avoid compiler warning!
    #endif

    userinterface__init();
}



enum CONT_TYPE {
    CONT_NONE,
    UIDT,
    RODT,
    CAMERA,
    SURFACE,
    FMTTEXT
};
static inline bool ISOBJECT(enum CONT_TYPE type)
{ return (type==CAMERA || type==SURFACE || type==FMTTEXT); }

static enum CONT_TYPE get_type (const Container* c)
{
    if(c_isaf(c)) return 0;
    for( ; c!=NULL; c = c_type(c))
    {
        if(c==base_uidt) return UIDT;
        if(c==base_rodt) return RODT;
        if(c==base_camera) return CAMERA;
        if(c==base_surface) return SURFACE;
    }
    return CONT_NONE;
    // NOTE: no Object as an inner container to another object
}

static inline bool is_base (const Container* c)
{
    return (c==base_uidt
         || c==base_rodt
         || c==base_object
         || c==base_camera
         || c==base_surface
         || c->parent==NULL);
}



static bool rodt_load (Container* rodt);

static bool process_container (Container* c)
{
    switch(get_type(c))
    {
    case UIDT: return (tools_uidt_eval(Layout,c) && main_window_resize());
    case RODT: return rodt_load(c);
    #ifdef LIBRODT
    case CAMERA: return camera_set(c);
    case SURFACE: return surface_set(c);
    #endif
    default:  return true;
    }
}

static bool rodt_load (Container* rodt)
{
    void* c = avl_min(&rodt->inners);
    for( ; c != NULL; c = avl_next(c))
    {
        if(!process_container((Container*)c))
        {
            wchar* message = errmsg+2000;
            sprintf2(message, L"%s\r\n\r\nContinue loading?", errmsg);
            if(!wait_for_confirmation(L"Error", message)) return false;
        }
    }
    if(!set_user_message(rodt)) return false;
    return true;
}



static Container* get_selected ()
{
    lchar* path=NULL;
    astrcpy32(&path, userinterface_get_text(UI_PATH_TEXT));
    set_line_coln_source(path, 1, 1, L"path name");

    Container* container = NULL;
    if(path && path->wchr!=0 && path->wchr!='|')
        strcpy21(errmsg, "Error: path name must start with a '|'.");
    else container = container_find(NULL, path, errmsg, 0, true);

    if(!container) display_message(errmsg);
    lchar_free(path);
    return container;
}



#define MAIN_ENTRY_NAME L"entry"
static bool g_check = false;
bool checkfail (const Container* c, const lchar* name)
{
    if(!g_check) return false;
    g_check = false;
    if(name)
    {   if(0!=strcmp32(name, MAIN_ENTRY_NAME))
        {
            sprintf2(errmsg, L"A new container:\r\n\"%s|%s\"\r\nwill be created.", container_path_name(c), CST23(name));
            if(!wait_for_confirmation(L"Confirm to continue", errmsg))
            { strcpy21(errmsg, "Container creation is cancelled."); return true; }
        }
    }
    else if(is_base(c)) { strcpy21(errmsg, "This container cannot be updated."); return true; }
    return false;
}



void tools_do_eval (const wchar* source)
{
    wait_for_draw_to_finish();
    bool success = false;
    while(true) // not a loop
    {
        Container* container = get_selected();
        if(container==NULL) break;

        Object* obj = NULL;
        if(ISOBJECT(get_type(container)))
            obj = (Object*)container->owner;

        const lchar* name = c_name(container);
        main_entry_mfet = name ? container : NULL;
        // only the RootContainer has no name

        if(!source)
        {   g_check = true;
            if(name)
                source = CST23(c_name(container));
            else if(file_exists_get())
                source = get_name_from_path_name(NULL,file_name_get());
            else source = MAIN_ENTRY_NAME;
        }
        bool b = calculator_evaluate_main(source);
        g_check = false;
        if(!b) break;

        container = main_entry_mfet;
        if(!process_container(container)) break;
        select_container(container);

        enum CONT_TYPE type = get_type(container);
        if(obj && !ISOBJECT(type))
        {
            assert(obj->container == container);
            obj->container->owner = NULL;
            obj->container = NULL;
            obj->destroy(obj);
        }
        else if(container==curr_uidt && type!=UIDT)
        {
            strcpy21(errmsg, "Container is no more of type User Interface Definition Text (UIDT). The used UIDT will change back to the original. If this was not intended then provide a container name.");
            wait_for_user_first(L"Warning", errmsg);
            tools_uidt_eval(Layout, base_uidt);
            main_window_resize();
        }
        success = true;
        errmsg[0]=0;
        userinterface_update();
        break;
    }
    if(!success) display_message(errmsg);
}



void tools_do_delete()
{
    wait_for_draw_to_finish();
    Container* container = get_selected();
    if(!container) return;

    if(is_base(container))
    { display_message(L"This container cannot be deleted."); return; }

    if(container==curr_uidt)
    {
        strcpy21(errmsg, "Container of type User Interface Definition Text (UIDT) will be deleted. The used UIDT will change back to the original. Is that OK?");
        if(!wait_for_confirmation(L"Warning", errmsg)) return;
        tools_uidt_eval(Layout, base_uidt);
        main_window_resize();
    }

    wchar* text = (wchar*)mainStack()+2000; // not errorMessage(), better use a local array!
    strcpy23(text, c_mfet(container));

    if(ISOBJECT(get_type(container)))
    {   Object* obj = (Object*)container->owner;
        if(!obj->destroy(obj)) return;
    } else if(!inherits_remove(container)) return;

    main_entry_mfet = NULL;
    display_message(NULL);
    display_main_text(text);

    if(timer_paused()) userinterface_update();
}



void select_container (Container* container)
{
    if(!container) return;
    const lchar* mfet = c_mfet(container);
    assert(mfet!=NULL);

    void* node = list_find(containers_list, NULL, pointer_compare, &container);
    assert(node!=NULL);
    list_head_push(containers_list, list_node_pop(containers_list, node));

    enum CONT_TYPE type = get_type(container);
    if(ISOBJECT(type))
    {
        Object* obj = (Object*)container->owner;
        userinterface_set_text (UI_MAIN_TEXT, CST23(mfet));
        headMouse->clickedObject = obj;
        if(type==CAMERA) headMouse->clickedCamera = (Camera*)obj;
    }
    else display_main_text (CST23(mfet));
    userinterface_set_text(UI_PATH_TEXT, container_path_name(container));
    display_message(NULL);
}

void tools_do_select()
{
    Container* container = get_selected();
    if(!container) return;
    if(c_mfet(container))
        select_container(container);
    else display_main_text(NULL);
}

void tools_get_prev()
{
    void* node = list_head_pop(containers_list);
    list_tail_push(containers_list, node);
    select_container(*(Container**)list_head(containers_list));
}

void tools_get_next()
{
    void* node = list_tail_pop(containers_list);
    list_head_push(containers_list, node);
    select_container(*(Container**)node);
}

void tools_do_clear()
{
    main_entry_mfet = NULL;
    userinterface_set_text(UI_MESG_TEXT, NULL);
    userinterface_set_text(UI_PATH_TEXT, NULL);
}



void tools_do_pause (bool pause)
{
    timer_pause(pause);
    const wchar* text = CST21(pause ? TEXT_RESUME : TEXT_PAUSE);
    userinterface_set_text(UI_PAUSE_BUTTON, text);
}

static void time_reverse_set_text()
{
    const wchar* text = CST21(timer_get_period()<0 ? TEXT_FORWARD : TEXT_BACKWARD);
    userinterface_set_text(UI_FORWARD_BUTTON, text);
}

void tools_go_forward (bool forward)
{
    int period = timer_get_period();
    bool positive = period>0;
    if(forward ^ positive)
    {
        period = -period;
        timer_set_period(period);
    }
    time_reverse_set_text();
    display_time_text();
}

static void edit_period (const char* name)
{
    if(!curr_uidt) curr_uidt = base_uidt;
    GeneralArg garg = { .caller = curr_uidt, .message = errmsg, .argument = NULL };
    ExprCallArg eca = { .garg = &garg, .expression = NULL, .stack = mainStack() };

    int period;
    Component* component = component_find(curr_uidt,CST31(name),errmsg,0);
    assert(component != NULL);

    if(component_evaluate(eca, component, VST11)
    && integerFromVst(&period, eca.stack, 1, errmsg, "period")
    && timer_set_period(period))
    {
        time_reverse_set_text();
        display_time_text();
    }
    else display_message(errorMessage());
}
void tools_lower_period()  { edit_period("lower_period"); }
void tools_higher_period() { edit_period("higher_period"); }


bool tools_set_time (const wchar* entry)
{
    if(!entry) entry = userinterface_get_text(UI_TIME_TEXT);
    while(true) // not a loop
    {
        const value* vst = mfet_parse_and_evaluate(entry, NULL, VST21);
        if(!vst) break;

        int period;
        if(!integerFromVst(&period, vst+1, 1, errmsg, "period")) break;

        value time = vst[2];
        if(!IsNumber(getType(time)))
        { strcpy21(errmsg, "Error: given time is not a number."); break; }

        if(!timer_set_period(period)) break;
        timer_set_time(time);
        userinterface_update();

        time_reverse_set_text();
        errmsg[0]=0;
        break;
    }
    if(errmsg[0]) display_message(errmsg);
    return errmsg[0]==0;
}



static void object_remove_all (List* list)
{
    Object *prev, *obj = (Object*)list_tail(list);
    for( ; obj != NULL; obj = prev)
    {
        prev = (Object*)list_prev(obj);
        // obj->destroy() is not called directly so to avoid
        // the dependence check performed by inherits_remove()
        component_destroy(obj->container);
        assert(obj->container==NULL);
        obj->destroy(obj);
    }
}

void tools_remove_all_objects (bool ask_confirmation)
{
    wait_for_draw_to_finish();
    if(ask_confirmation)
    {
        wchar title[50], message[50];
        strcpy21(title, "Confirm");
        strcpy21(message, "Remove All Objects?");
        if(!wait_for_confirmation(title, message)) return;
    }
    mouse_clear_pointers(headMouse);
    display_main_text(NULL);
    display_message(NULL);
    userinterface_set_text(UI_PATH_TEXT, NULL);
    object_remove_all(camera_list);
    object_remove_all(surface_list);
}

void tools_clean()
{
    tools_remove_all_objects(false);
    userinterface__clean();
    userinterface_clean();
    mouse_clean();
    mfet_clean();
    mchar_free(usermessage); usermessage=NULL;
}



static const wchar base_uidt_mfet[] = {
#ifdef LIBRODT
   114,101,115,117,108,116, 59, 13, 10, 13, 10,110, 97,109,101, 32, 61, 32, 34, 85,115,101,114, 95, 73,
   110,116,101,114,102, 97, 99,101, 95, 68,101,102,105,110,105,116,105,111,110, 95, 84,101,120,116, 34,
    59, 13, 10, 13, 10, 13, 10,119,104, 32, 61, 32, 77, 97,105,110, 87,105,110,100,111,119, 87,105,100,
   116,104, 72,101,105,103,104,116, 59, 13, 10,119, 32, 61, 32,119,104, 91, 48, 93, 59, 13, 10,104, 32,
    61, 32,119,104, 91, 49, 93, 59, 13, 10, 13, 10,104, 49, 32, 61, 32, 54, 48, 59, 32, 32, 32, 32, 35,
    32,104,101,105,103,104,116, 32,111,102, 32, 49,115,116, 32,108, 97,121,101,114, 13, 10,104, 50, 32,
    61, 32, 51, 48, 59, 32, 32, 32, 32, 35, 32,104,101,105,103,104,116, 32,111,102, 32, 50,110,100, 32,
   108, 97,121,101,114, 13, 10,104, 51, 32, 61, 32, 51, 48, 59, 32, 32, 32, 32, 35, 32,104,101,105,103,
   104,116, 32,111,102, 32, 51,114,100, 32,108, 97,121,101,114, 13, 10,104, 52, 32, 61, 32, 53, 48, 59,
    32, 32, 32, 32, 35, 32,104,101,105,103,104,116, 32,111,102, 32, 52,116,104, 32,108, 97,121,101,114,
    13, 10,104, 53, 32, 61, 32, 50, 48, 59, 32, 32, 32, 32, 35, 32,104,101,105,103,104,116, 32,111,102,
    32, 53,116,104, 32,108, 97,121,101,114, 13, 10, 13, 10,121, 53, 32, 61, 32, 32,104, 45,104, 53, 59,
    32, 35, 32,121, 95,115,116, 97,114,116, 32,111,102, 32, 53,116,104, 32,108, 97,121,101,114, 13, 10,
   121, 52, 32, 61, 32,121, 53, 45,104, 52, 59, 32, 35, 32,121, 95,115,116, 97,114,116, 32,111,102, 32,
    52,116,104, 32,108, 97,121,101,114, 13, 10,121, 51, 32, 61, 32,121, 52, 45,104, 51, 59, 32, 35, 32,
   121, 95,115,116, 97,114,116, 32,111,102, 32, 51,114,100, 32,108, 97,121,101,114, 13, 10,121, 50, 32,
    61, 32,121, 51, 45,104, 50, 59, 32, 35, 32,121, 95,115,116, 97,114,116, 32,111,102, 32, 50,110,100,
    32,108, 97,121,101,114, 13, 10,121, 49, 32, 61, 32,121, 50, 45,104, 49, 59, 32, 35, 32,121, 95,115,
   116, 97,114,116, 32,111,102, 32, 49,115,116, 32,108, 97,121,101,114, 13, 10, 13, 10, 97, 32, 61, 32,
    54, 48, 59, 13, 10, 13, 10, 13, 10,109, 97,105,110, 95,116,101,120,116,102,105,101,108,100, 32, 32,
    32, 32, 32, 32, 61, 32,123, 48, 44, 32, 48, 44, 32,119, 44, 32,121, 49,125, 59, 13, 10, 13, 10,101,
   118, 97,108, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 32, 32, 32,
    48, 44, 32,121, 49, 44, 32, 32, 32, 51, 48, 32, 32, 32, 44, 32,104, 49,125, 59, 13, 10,109,101,115,
   115, 97,103,101, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 61, 32,123, 32, 32, 51, 48, 44,
    32,121, 49, 44, 32,119, 45, 51, 48, 45, 50, 48, 44, 32,104, 49,125, 59, 13, 10,108,111, 99,107, 95,
    98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123,119, 45, 50, 48, 44, 32,121,
    49, 44, 32, 32, 32, 32, 32, 32, 50, 48, 44, 32,104, 49,125, 59, 13, 10, 13, 10,112,114,101,118, 95,
    98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 48, 42, 97, 44, 32,121, 50,
    44, 32, 32, 97, 32, 32, 44, 32,104, 50,125, 59, 13, 10,110,101,120,116, 95, 98,117,116,116,111,110,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 49, 42, 97, 44, 32,121, 50, 44, 32, 32, 97, 32, 32,
    44, 32,104, 50,125, 59, 13, 10,100,101,108,101,116,101, 95, 98,117,116,116,111,110, 32, 32, 32, 32,
    32, 32, 32, 61, 32,123, 50, 42, 97, 44, 32,121, 50, 44, 32, 32, 97, 32, 32, 44, 32,104, 50,125, 59,
    13, 10, 99,108,101, 97,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123,
    51, 42, 97, 44, 32,121, 50, 44, 32, 32, 97, 32, 32, 44, 32,104, 50,125, 59, 13, 10,112, 97,116,104,
    95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 32, 32, 61, 32,123, 52, 42, 97, 44, 32,121,
    50, 44,119, 45, 52, 42, 97, 44, 32,104, 50,125, 59, 13, 10, 13, 10,112, 97,117,115,101, 95, 98,117,
   116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 48, 42, 97, 44, 32,121, 51, 44, 32, 32,
    97, 32, 32, 44, 32,104, 51,125, 59, 13, 10,102,111,114,119, 97,114,100, 95, 98,117,116,116,111,110,
    32, 32, 32, 32, 32, 32, 61, 32,123, 49, 42, 97, 44, 32,121, 51, 44, 32, 32, 97, 32, 32, 44, 32,104,
    51,125, 59, 13, 10,108,111,119,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32,
    61, 32,123, 50, 42, 97, 44, 32,121, 51, 44, 32, 32, 97, 32, 32, 44, 32,104, 51,125, 59, 13, 10,104,
   105,103,104,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 51, 42, 97,
    44, 32,121, 51, 44, 32, 32, 97, 32, 32, 44, 32,104, 51,125, 59, 13, 10,116,105,109,101, 95,116,101,
   120,116,102,105,101,108,100, 32, 32, 32, 32, 32, 32, 61, 32,123, 52, 42, 97, 44, 32,121, 51, 44,119,
    45, 52, 42, 97, 44, 32,104, 51,125, 59, 13, 10, 13, 10, 99, 97,108, 99, 95, 98,117,116,116,111,110,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 32, 48, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 44,
    32,121, 52, 44, 32, 32, 32, 32, 97, 32, 32, 32, 32, 32, 44, 32,104, 52,125, 59, 13, 10, 99, 97,108,
    99, 95,105,110,112,117,116, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 32, 97, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 44, 32,121, 52, 44, 32, 40,119, 45, 97, 41, 42, 51, 47, 53, 44, 32,104,
    52,125, 59, 13, 10, 99, 97,108, 99, 95,114,101,115,117,108,116, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    61, 32,123, 32, 97, 43, 40,119, 45, 97, 41, 42, 51, 47, 53, 44, 32,121, 52, 44, 32, 40,119, 45, 97,
    41, 42, 50, 47, 53, 44, 32,104, 52,125, 59, 13, 10, 13, 10,115,116, 97,116,117,115, 95, 98, 97,114,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 48, 44, 32,121, 53, 44, 32,119, 44, 32,104, 53,
   125, 59, 13, 10, 13, 10, 13, 10,112,114,105,118, 97,116,101, 32,114,101,115,117,108,116, 32, 61, 13,
    10, 32, 32, 32, 32, 40, 34,109, 97,105,110, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32,
    34, 32, 44, 32,109, 97,105,110, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 41, 32, 44,
    13, 10, 13, 10, 32, 32, 32, 32, 40, 34,101,118, 97,108, 95, 98,117,116,116,111,110, 32, 32, 32, 32,
    32, 32, 32, 34, 32, 44, 32,101,118, 97,108, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32,
    41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,109,101,115,115, 97,103,101, 95,116,101,120,116,102,105,
   101,108,100, 32, 34, 32, 44, 32,109,101,115,115, 97,103,101, 95,116,101,120,116,102,105,101,108,100,
    32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,108,111, 99,107, 95, 98,117,116,116,111,110, 32, 32,
    32, 32, 32, 32, 32, 34, 32, 44, 32,108,111, 99,107, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32,
    32, 32, 41, 32, 44, 13, 10, 13, 10, 32, 32, 32, 32, 40, 34,112,114,101,118, 95, 98,117,116,116,111,
   110, 32, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,112,114,101,118, 95, 98,117,116,116,111,110, 32, 32,
    32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,110,101,120,116, 95, 98,117,116,116,
   111,110, 32, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,110,101,120,116, 95, 98,117,116,116,111,110, 32,
    32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,100,101,108,101,116,101, 95, 98,
   117,116,116,111,110, 32, 32, 32, 32, 32, 34, 32, 44, 32,100,101,108,101,116,101, 95, 98,117,116,116,
   111,110, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34, 99,108,101, 97,114, 95, 98,
   117,116,116,111,110, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32, 99,108,101, 97,114, 95, 98,117,116,116,
   111,110, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,112, 97,116,104, 95,116,
   101,120,116,102,105,101,108,100, 32, 32, 32, 32, 34, 32, 44, 32,112, 97,116,104, 95,116,101,120,116,
   102,105,101,108,100, 32, 32, 32, 32, 41, 32, 44, 13, 10, 13, 10, 32, 32, 32, 32, 40, 34,112, 97,117,
   115,101, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,112, 97,117,115,101, 95,
    98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,102,111,
   114,119, 97,114,100, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 34, 32, 44, 32,102,111,114,119, 97,
   114,100, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,108,
   111,119,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,108,111,119,101,
   114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,
   104,105,103,104,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 34, 32, 44, 32,104,105,103,
   104,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40,
    34,116,105,109,101, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 34, 32, 44, 32,116,105,
   109,101, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 41, 32, 44, 13, 10, 13, 10, 32, 32,
    32, 32, 40, 34, 99, 97,108, 99, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 34, 32, 44,
    32, 99, 97,108, 99, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32,
    32, 32, 32, 40, 34, 99, 97,108, 99, 95,105,110,112,117,116, 32, 32, 32, 32, 32, 32, 32, 32, 34, 32,
    44, 32, 99, 97,108, 99, 95,105,110,112,117,116, 32, 32, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10,
    32, 32, 32, 32, 40, 34, 99, 97,108, 99, 95,114,101,115,117,108,116, 32, 32, 32, 32, 32, 32, 32, 34,
    32, 44, 32, 99, 97,108, 99, 95,114,101,115,117,108,116, 32, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13,
    10, 13, 10, 32, 32, 32, 32, 40, 34,115,116, 97,116,117,115, 95, 98, 97,114, 32, 32, 32, 32, 32, 32,
    32, 32, 34, 32, 44, 32,115,116, 97,116,117,115, 95, 98, 97,114, 32, 32, 32, 32, 32, 32, 32, 32, 41,
    32, 59, 13, 10, 13, 10, 13, 10,108,111,119,101,114, 95,112,101,114,105,111,100, 32, 32, 61, 32, 32,
    84,105,109,101,114, 80,101,114,105,111,100, 32, 45, 32, 50, 53, 59, 13, 10,104,105,103,104,101,114,
    95,112,101,114,105,111,100, 32, 61, 32, 32, 84,105,109,101,114, 80,101,114,105,111,100, 32, 43, 32,
    50, 53, 59, 13, 10, 13, 10, 80,105,120,101,108, 83,105,122,101, 32, 61, 32, 49, 47, 49, 48, 48, 48,
    32, 59, 32, 35, 32, 61, 62, 32, 49, 48, 48, 48, 32,112,105,120,101,108,115, 32,112,101,114, 32,117,
   110,105,116, 32,108,101,110,103,116,104, 13, 10, 13, 10, 0
#else
   114,101,115,117,108,116, 59, 13, 10, 13, 10,110, 97,109,101, 32, 61, 32, 34, 85,115,101,114, 95, 73,
   110,116,101,114,102, 97, 99,101, 95, 68,101,102,105,110,105,116,105,111,110, 95, 84,101,120,116, 34,
    59, 13, 10, 13, 10, 13, 10,119,104, 32, 61, 32, 77, 97,105,110, 87,105,110,100,111,119, 87,105,100,
   116,104, 72,101,105,103,104,116, 59, 13, 10,119, 32, 61, 32,119,104, 91, 48, 93, 59, 13, 10,104, 32,
    61, 32,119,104, 91, 49, 93, 59, 13, 10, 13, 10,104, 49, 32, 61, 32, 54, 48, 59, 32, 32, 32, 32, 35,
    32,104,101,105,103,104,116, 32,111,102, 32, 49,115,116, 32,108, 97,121,101,114, 13, 10,104, 50, 32,
    61, 32, 32, 48, 59, 32, 32, 32, 32, 35, 32,104,101,105,103,104,116, 32,111,102, 32, 50,110,100, 32,
   108, 97,121,101,114, 13, 10,104, 51, 32, 61, 32, 51, 48, 59, 32, 32, 32, 32, 35, 32,104,101,105,103,
   104,116, 32,111,102, 32, 51,114,100, 32,108, 97,121,101,114, 13, 10,104, 52, 32, 61, 32, 32, 48, 59,
    32, 32, 32, 32, 35, 32,104,101,105,103,104,116, 32,111,102, 32, 52,116,104, 32,108, 97,121,101,114,
    13, 10,104, 53, 32, 61, 32, 50, 48, 59, 32, 32, 32, 32, 35, 32,104,101,105,103,104,116, 32,111,102,
    32, 53,116,104, 32,108, 97,121,101,114, 13, 10, 13, 10,121, 53, 32, 61, 32, 32,104, 45,104, 53, 59,
    32, 35, 32,121, 95,115,116, 97,114,116, 32,111,102, 32, 53,116,104, 32,108, 97,121,101,114, 13, 10,
   121, 52, 32, 61, 32,121, 53, 45,104, 52, 59, 32, 35, 32,121, 95,115,116, 97,114,116, 32,111,102, 32,
    52,116,104, 32,108, 97,121,101,114, 13, 10,121, 51, 32, 61, 32,121, 52, 45,104, 51, 59, 32, 35, 32,
   121, 95,115,116, 97,114,116, 32,111,102, 32, 51,114,100, 32,108, 97,121,101,114, 13, 10,121, 50, 32,
    61, 32,121, 51, 45,104, 50, 59, 32, 35, 32,121, 95,115,116, 97,114,116, 32,111,102, 32, 50,110,100,
    32,108, 97,121,101,114, 13, 10,121, 49, 32, 61, 32,121, 50, 45,104, 49, 59, 32, 35, 32,121, 95,115,
   116, 97,114,116, 32,111,102, 32, 49,115,116, 32,108, 97,121,101,114, 13, 10, 13, 10, 97, 32, 61, 32,
    54, 48, 59, 13, 10, 13, 10, 13, 10,109, 97,105,110, 95,116,101,120,116,102,105,101,108,100, 32, 32,
    32, 32, 32, 32, 61, 32,123, 48, 44, 32, 48, 44, 32,119, 44, 32,121, 49,125, 59, 13, 10, 13, 10,101,
   118, 97,108, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 32, 32, 32,
    48, 44, 32,121, 49, 44, 32, 32, 32, 51, 48, 32, 32, 32, 44, 32,104, 49,125, 59, 13, 10,109,101,115,
   115, 97,103,101, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 61, 32,123, 32, 32, 51, 48, 44,
    32,121, 49, 44, 32,119, 45, 51, 48, 45, 50, 48, 44, 32,104, 49,125, 59, 13, 10,108,111, 99,107, 95,
    98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123,119, 45, 50, 48, 44, 32,121,
    49, 44, 32, 32, 32, 32, 32, 32, 50, 48, 44, 32,104, 49,125, 59, 13, 10, 13, 10,112,114,101,118, 95,
    98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 48, 42, 97, 44, 32,121, 50,
    44, 32, 32, 97, 32, 32, 44, 32,104, 50,125, 59, 13, 10,110,101,120,116, 95, 98,117,116,116,111,110,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 49, 42, 97, 44, 32,121, 50, 44, 32, 32, 97, 32, 32,
    44, 32,104, 50,125, 59, 13, 10,100,101,108,101,116,101, 95, 98,117,116,116,111,110, 32, 32, 32, 32,
    32, 32, 32, 61, 32,123, 50, 42, 97, 44, 32,121, 50, 44, 32, 32, 97, 32, 32, 44, 32,104, 50,125, 59,
    13, 10, 99,108,101, 97,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123,
    51, 42, 97, 44, 32,121, 50, 44, 32, 32, 97, 32, 32, 44, 32,104, 50,125, 59, 13, 10,112, 97,116,104,
    95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 32, 32, 61, 32,123, 52, 42, 97, 44, 32,121,
    50, 44,119, 45, 52, 42, 97, 44, 32,104, 50,125, 59, 13, 10, 13, 10,112, 97,117,115,101, 95, 98,117,
   116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 48, 42, 97, 44, 32,121, 51, 44, 32, 32,
    97, 32, 32, 44, 32,104, 51,125, 59, 13, 10,102,111,114,119, 97,114,100, 95, 98,117,116,116,111,110,
    32, 32, 32, 32, 32, 32, 61, 32,123, 49, 42, 97, 44, 32,121, 51, 44, 32, 32, 97, 32, 32, 44, 32,104,
    51,125, 59, 13, 10,108,111,119,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 32,
    61, 32,123, 50, 42, 97, 44, 32,121, 51, 44, 32, 32, 97, 32, 32, 44, 32,104, 51,125, 59, 13, 10,104,
   105,103,104,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 51, 42, 97,
    44, 32,121, 51, 44, 32, 32, 97, 32, 32, 44, 32,104, 51,125, 59, 13, 10,116,105,109,101, 95,116,101,
   120,116,102,105,101,108,100, 32, 32, 32, 32, 32, 32, 61, 32,123, 52, 42, 97, 44, 32,121, 51, 44,119,
    45, 52, 42, 97, 44, 32,104, 51,125, 59, 13, 10, 13, 10, 99, 97,108, 99, 95, 98,117,116,116,111,110,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 32, 48, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 44,
    32,121, 52, 44, 32, 32, 32, 32, 97, 32, 32, 32, 32, 32, 44, 32,104, 52,125, 59, 13, 10, 99, 97,108,
    99, 95,105,110,112,117,116, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 32, 97, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 44, 32,121, 52, 44, 32, 40,119, 45, 97, 41, 42, 51, 47, 53, 44, 32,104,
    52,125, 59, 13, 10, 99, 97,108, 99, 95,114,101,115,117,108,116, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    61, 32,123, 32, 97, 43, 40,119, 45, 97, 41, 42, 51, 47, 53, 44, 32,121, 52, 44, 32, 40,119, 45, 97,
    41, 42, 50, 47, 53, 44, 32,104, 52,125, 59, 13, 10, 13, 10,115,116, 97,116,117,115, 95, 98, 97,114,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 61, 32,123, 48, 44, 32,121, 53, 44, 32,119, 44, 32,104, 53,
   125, 59, 13, 10, 13, 10, 13, 10,112,114,105,118, 97,116,101, 32,114,101,115,117,108,116, 32, 61, 13,
    10, 32, 32, 32, 32, 40, 34,109, 97,105,110, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32,
    34, 32, 44, 32,109, 97,105,110, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 41, 32, 44,
    13, 10, 13, 10, 32, 32, 32, 32, 40, 34,101,118, 97,108, 95, 98,117,116,116,111,110, 32, 32, 32, 32,
    32, 32, 32, 34, 32, 44, 32,101,118, 97,108, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32,
    41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,109,101,115,115, 97,103,101, 95,116,101,120,116,102,105,
   101,108,100, 32, 34, 32, 44, 32,109,101,115,115, 97,103,101, 95,116,101,120,116,102,105,101,108,100,
    32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,108,111, 99,107, 95, 98,117,116,116,111,110, 32, 32,
    32, 32, 32, 32, 32, 34, 32, 44, 32,108,111, 99,107, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32,
    32, 32, 41, 32, 44, 13, 10, 13, 10, 32, 32, 32, 32, 40, 34,112,114,101,118, 95, 98,117,116,116,111,
   110, 32, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,112,114,101,118, 95, 98,117,116,116,111,110, 32, 32,
    32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,110,101,120,116, 95, 98,117,116,116,
   111,110, 32, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,110,101,120,116, 95, 98,117,116,116,111,110, 32,
    32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,100,101,108,101,116,101, 95, 98,
   117,116,116,111,110, 32, 32, 32, 32, 32, 34, 32, 44, 32,100,101,108,101,116,101, 95, 98,117,116,116,
   111,110, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34, 99,108,101, 97,114, 95, 98,
   117,116,116,111,110, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32, 99,108,101, 97,114, 95, 98,117,116,116,
   111,110, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,112, 97,116,104, 95,116,
   101,120,116,102,105,101,108,100, 32, 32, 32, 32, 34, 32, 44, 32,112, 97,116,104, 95,116,101,120,116,
   102,105,101,108,100, 32, 32, 32, 32, 41, 32, 44, 13, 10, 13, 10, 32, 32, 32, 32, 40, 34,112, 97,117,
   115,101, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,112, 97,117,115,101, 95,
    98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,102,111,
   114,119, 97,114,100, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 34, 32, 44, 32,102,111,114,119, 97,
   114,100, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,108,
   111,119,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 34, 32, 44, 32,108,111,119,101,
   114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40, 34,
   104,105,103,104,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 34, 32, 44, 32,104,105,103,
   104,101,114, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32, 32, 32, 32, 40,
    34,116,105,109,101, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 34, 32, 44, 32,116,105,
   109,101, 95,116,101,120,116,102,105,101,108,100, 32, 32, 32, 32, 41, 32, 44, 13, 10, 13, 10, 32, 32,
    32, 32, 40, 34, 99, 97,108, 99, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 34, 32, 44,
    32, 99, 97,108, 99, 95, 98,117,116,116,111,110, 32, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10, 32,
    32, 32, 32, 40, 34, 99, 97,108, 99, 95,105,110,112,117,116, 32, 32, 32, 32, 32, 32, 32, 32, 34, 32,
    44, 32, 99, 97,108, 99, 95,105,110,112,117,116, 32, 32, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13, 10,
    32, 32, 32, 32, 40, 34, 99, 97,108, 99, 95,114,101,115,117,108,116, 32, 32, 32, 32, 32, 32, 32, 34,
    32, 44, 32, 99, 97,108, 99, 95,114,101,115,117,108,116, 32, 32, 32, 32, 32, 32, 32, 41, 32, 44, 13,
    10, 13, 10, 32, 32, 32, 32, 40, 34,115,116, 97,116,117,115, 95, 98, 97,114, 32, 32, 32, 32, 32, 32,
    32, 32, 34, 32, 44, 32,115,116, 97,116,117,115, 95, 98, 97,114, 32, 32, 32, 32, 32, 32, 32, 32, 41,
    32, 59, 13, 10, 13, 10, 13, 10,108,111,119,101,114, 95,112,101,114,105,111,100, 32, 32, 61, 32, 32,
    84,105,109,101,114, 80,101,114,105,111,100, 32, 45, 32, 50, 53, 59, 13, 10,104,105,103,104,101,114,
    95,112,101,114,105,111,100, 32, 61, 32, 32, 84,105,109,101,114, 80,101,114,105,111,100, 32, 43, 32,
    50, 53, 59, 13, 10, 13, 10, 0
#endif
};

static const wchar base_rodt_rodt[] = {
    48, 59, 13, 10, 13, 10,110, 97,109,101, 32, 61, 32, 34, 82,104,121,115, 99,105,116,108,101,109, 97,
    95, 79, 98,106,101, 99,116,115, 95, 68,101,102,105,110,105,116,105,111,110, 95, 84,101,120,116, 34,
    32, 59, 13, 10, 13, 10,112,114,105,118, 97,116,101, 13, 10,109,101,115,115, 97,103,101, 32, 61, 32,
    34, 13, 10, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,108,101,102,116, 45, 99,108,105, 99,107, 32,
   111,110, 32, 97, 32,103,114, 97,112,104, 13, 10, 13, 10, 32, 32, 32, 32, 32, 32,111,114, 32, 32,114,
   105,103,104,116, 45, 99,108,105, 99,107, 32,111,110, 32, 97, 32, 99, 97,109,101,114, 97, 13, 10, 13,
    10, 32, 77,111,114,101, 32, 97,116, 32,104,116,116,112, 58, 47, 47,114,104,121,115, 99,105,116,108,
   101,109, 97, 46, 99,111,109, 47, 97,112,112,108,105, 99, 97,116,105,111,110,115, 13, 10, 34, 59, 13,
    10, 13, 10,112,114,105,118, 97,116,101, 13, 10, 92,109,102,101,116,123, 48, 59, 13, 10, 32,110, 97,
   109,101, 32, 61, 32, 34, 79, 98,106,101, 99,116, 34, 59, 13, 10, 32,111,114,105,103,105,110, 32, 61,
    32, 32, 40, 48, 44, 48, 44, 48, 41, 32, 59, 13, 10, 32, 97,120,101,115, 32, 61, 32, 32, 40, 49, 44,
    48, 44, 48, 41, 44, 40, 48, 44, 49, 44, 48, 41, 44, 40, 48, 44, 48, 44, 49, 41, 32, 59, 13, 10,125,
    13, 10, 13, 10, 92,109,102,101,116,123, 48, 59, 13, 10, 32,116,121,112,101, 32, 61, 32, 34, 79, 98,
   106,101, 99,116, 34, 59, 13, 10, 32,110, 97,109,101, 32, 61, 32, 34, 67, 97,109,101,114, 97, 34, 59,
    13, 10, 32,114,101, 99,116, 97,110,103,108,101, 32, 61, 32,123, 48, 44, 48, 44, 52, 48, 48, 44, 51,
    48, 48, 44, 48, 44, 48,125, 59, 13, 10, 32,122,111,111,109, 32, 61, 32, 32, 49, 59, 13, 10,125, 13,
    10, 13, 10, 92,109,102,101,116,123, 48, 59, 13, 10, 32,116,121,112,101, 32, 61, 32, 34, 79, 98,106,
   101, 99,116, 34, 59, 13, 10, 32,110, 97,109,101, 32, 61, 32, 34, 83,117,114,102, 97, 99,101, 34, 59,
    13, 10, 32,102,117,110, 99,116,105,111,110, 40,120, 44,121, 44,122, 41, 32, 61, 32, 32, 48, 59, 13,
    10, 32, 99,111,108,111,117,114, 40,120, 44,121, 44,122, 41, 32, 61, 32, 32, 40,120, 44,121, 44,122,
    44, 49, 41, 59, 13, 10, 32, 98,111,117,110,100, 97,114,121, 32, 61, 32, 32,123, 48, 44, 49, 44, 48,
    44, 49, 44, 48, 44, 49,125, 59, 13, 10, 32, 97, 99, 99,117,114, 97, 99,121, 32, 61, 32, 32, 49, 59,
    13, 10,125, 13, 10, 13, 10, 0
};
