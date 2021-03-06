/*
	tools.c
*/

#include <_stdio.h>
#include <_texts.h>
#include <rfet.h>
#include <outsider.h>
#include "getimage.h"
#include "userinterface.h"
#include "surface.h"
#include "camera.h"
#include "mouse.h"
#include "files.h"
#include "timer.h"
#include "tools.h"


const_Str2 argv[3];

uint32_t PixelsPUL; // Pixels Per Unit Length

static Container* base_surface = NULL;
static Container* base_camera = NULL;
static Container* base_object = NULL;
static Container* base_rodt = NULL;
static Container* base_uidt = NULL;
static Container* curr_uidt = NULL;

static const wchar* get_base_uidt();
static const wchar* get_base_rodt();

static const char* uidt_name[] = {
	"main_textfield",

	"eval_button",
	"mesg_textfield",
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


static bool do_uidt_eval (value stack, Container* uidt, int* output, int count, const char* name)
{
	stack = component_evaluate(stack, uidt, component_parse(stack, component_find(stack, uidt, C31(name), 0)), NULL);
	return integFromValue(stack, count, 1, output, name);
}

bool tools_uidt_eval (value stack, int layout[][4], void* _uidt)
{
	if(!curr_uidt) curr_uidt = base_uidt;
	Container* uidt = (Container*)_uidt;

	if(!uidt) uidt = curr_uidt;
	evaluation_instance(true);

	memset(layout, 0, uidt_count*4*sizeof(int));
	int i;
	for(i=0; i < uidt_count; i++)
		if(!do_uidt_eval(stack, uidt, layout[i], 4, uidt_name[i])) break;

	bool ok = (i == uidt_count);
	do{
		if(!ok      // if there was error, or
		|| !_uidt)  // if not checking for error
			break;
		ok = false; // assume error

		if(!do_uidt_eval(stack, uidt, NULL, 1, "lower_period")) break;
		if(!do_uidt_eval(stack, uidt, NULL, 1, "higher_period")) break;

		#ifdef LIBRODT
		if(!do_uidt_eval(stack, uidt, (int*)&PixelsPUL, 1, "PixelsPUL")) break;
		#endif

		ok = true; // no error found
		curr_uidt = uidt;
		uidt->owner = &curr_uidt;
	}while(0);

	for(i=0; i<CONVERT_TEXT_COUNT; i++)
	{
		wchar text[100] = L"ConvertTextF";
		intToStr(text+strlen2(text), i);

		lchar array[20];
		const_Str3 str = set_lchar_array(array, 20, text, NULL);

		Component* component = component_find(stack, curr_uidt, str, 0);
		if(component)
		{
			// locate the first #"..." after component name and before newline '\n'
			str = C37(c_name(component).end);
			const_Str3 name = {0};
			int size=0;
			while(true)
			{
				str = sNext(str);
				wchar c = sChar(str);
				if(c=='\n' || c=='\0')
					break;
				if(name.ptr){ // if #"... was found before
					if(++size==100)  // if too many characters
						break;
					if(c=='"'){      // if found a matching "
						name.end = str.ptr;
						strcpy23(text, name);
					}
				}
				else if(c=='#'){
					str = sNext(str);
					c = sChar(str);
					if(c=='"'){
						str = sNext(str);
						name.ptr = str.ptr;
					}
				}
			}
		}
		int item = i | (component ? 0 : 0x80);
		setMenuItemTextOfConvertText (item, text);
	}
	return ok;
}


static Str2 usermessage = {0};
const wchar* userMessage() { return usermessage; }

static bool set_user_message (value stack, Container* rodt)
{
	Component* component = container_find(stack, rodt, C31(".|message"), true);
	component = component_parse(stack, component);

	evaluation_instance(true); // is this even needed here?!
	if(VERROR(component_evaluate(stack, rodt, component, NULL)))
		return false; // keep existing usermessage

	if(!isStr2(vGet(stack)))
	{
		argv[0] = L"Error on %s at (%s,%s) in %s:\r\nThis must evaluate to a string.";
		setMessageE(stack, 0, 1, argv, c_name(component));
		return false;
	}
	astrcpy22(&usermessage, getStr2(vGet(stack)));
	return true;
}


void tools_init (size_t stack_size, const wchar* uidt_rfet)
{
	mouse_init();
	rfet_init(stack_size);
	value stack = stackArray();
	bool success=false;

	// 1) set BASE UIDT
	if(strEnd2(uidt_rfet)) uidt_rfet = get_base_uidt();
	Str3 text = astrcpy32(C37(NULL), uidt_rfet, L"BASE_UIDT");
	base_uidt = container_parse(stack, NULL, C37(NULL), text); text=C37(NULL);

	if(base_uidt!=NULL
	&& dependence_parse(stack)
	&& tools_uidt_eval(stack, Layout, base_uidt))
		success = true;
	else base_uidt = NULL;

	dependence_finalise(success);
	if(base_uidt==NULL) { puts1("-----BASE_UIDT_FAIL-----"); puts2(getMessage(vGet(stack))); }
	assert(base_uidt!=NULL);

	#ifdef LIBRODT
	// 2) set BASE RODT
	text = astrcpy32(text, get_base_rodt(), L"BASE_RODT");
	base_rodt = (Container*)rfet_parse(stack, NULL, text); text=C37(NULL);
	if(base_rodt==NULL) { puts1("-----BASE_RODT_FAIL-----"); puts2(getMessage(vGet(stack))); }
	assert(base_rodt!=NULL);
	success = set_user_message(stack, base_rodt);
	assert(success);

	// 3) set base objects
	base_object  = component_find(stack, base_rodt, C31("Object" ), 0);
	base_camera  = component_find(stack, base_rodt, C31("Camera" ), 0);
	base_surface = component_find(stack, base_rodt, C31("Surface"), 0);
	assert(base_object!=NULL);
	assert(base_camera!=NULL);
	assert(base_surface!=NULL);
	#else
	uidt_rfet = get_base_rodt(); // just to skip compiler warning!
	#endif

	userinterface__init();
}



enum RFET_TYPE {
	RFET_OTHER,
	RFET_UIDT,
	RFET_RODT,
	RFET_CAMERA,
	RFET_SURFACE,
	RFET_DOCUMENT,
	RFET_GLOBAL
};

static inline bool ISOBJECT(enum RFET_TYPE type)
{ return
	type==RFET_CAMERA  ||
	type==RFET_SURFACE ||
	type==RFET_DOCUMENT ;
}

static enum RFET_TYPE get_type (const Container* c)
{
	if(!c || !c_iscont(c)) return 0;

	if(0==strcmp31(c_name(c), "GLOBAL.RFET"))
		return RFET_GLOBAL;

	for( ; c!=NULL; c = c_type(c))
	{
		if(c==base_uidt)    return RFET_UIDT;
		if(c==base_rodt)    return RFET_RODT;
		if(c==base_camera)  return RFET_CAMERA;
		if(c==base_surface) return RFET_SURFACE;
	}
	return RFET_OTHER;
	// NOTE: no Object as an inner container to another object
}

static inline bool is_base (const Container* c)
{ return
	c==base_uidt    ||
	c==base_rodt    ||
	c==base_object  ||
	c==base_camera  ||
	c==base_surface ||
	c->parent==NULL ;
}



static bool rodt_load (value stack, Container* rodt);

static bool process_container (value stack, Container* c)
{
	switch(get_type(c))
	{
	case RFET_UIDT: return (tools_uidt_eval(stack,Layout,c) && main_window_resize());
	case RFET_RODT: return rodt_load(stack, c);
	#ifdef LIBRODT
	case RFET_CAMERA: return camera_set(stack, c);
	case RFET_SURFACE: return surface_set(stack, c);
	#endif
	default:  return true;
	}
}

static bool rodt_load (value stack, Container* rodt)
{
	void* c = tree_first(&rodt->inners);
	for( ; c != NULL; c = tree_next(c))
	{
		if(!process_container(stack, (Container*)c))
		{
			value v = vnext(stack);
			argv[0] = L"%s\r\n\r\nContinue loading?";
			argv[1] = getMessage(vGet(stack));
			setMessage(v, 0, 2, argv);

			wchar save[10000+1]; // strlen2(argv[1])
			strcpy22(save, argv[1]);

			if(!user_confirm(L"Error", getMessage(v)))
			{
				setError(stack, save);
				return false;
			}
		}
	}
	return set_user_message(stack, rodt);
}



static Container* get_selected (value stack)
{
	const wchar* wstr = userinterface_get_text(UI_PATH_TEXT);
	long len = strlen2(wstr);
	lchar lstr[10000+1]; //len
	Str3 path = set_lchar_array(lstr, len+1, wstr, L"path name");

	Container* container = NULL;
	if(wstr && *wstr && *wstr!='|')
		setError(stack, L"Error: path name must start with a '|'.");
	else container = container_find(stack, NULL, path, true);

	if(!container) display_message(getMessage(vGet(stack)));
	return container;
}



#define MAIN_ENTRY_NAME L"entry"
static bool g_check = false;

/* Called by container_parse() in component.c */
bool checkfail (value stack, const Container* c, const_Str3 name, bool hasType, bool isNew)
{
	bool isentry = 0==strcmp32(name, MAIN_ENTRY_NAME);

	if(isentry && hasType)
	{
		setError(stack, L"Provide a container name different from \"entry\".");
		user_alert(L"Error", getMessage(stack));
		return true;
	}
	if(isNew && !isentry && g_check)
	{
		argv[0] = L"A new container:\r\n\"%s|%s\"\r\nwill be created.";
		argv[1] = container_path_name(stack, c); // c = parent container
		argv[2] = C23(name);
		value v = vnext(stack);
		setMessage(v, 0, 3, argv);

		if(!user_confirm(L"Confirm to continue", getMessage(v)))
		{
			setError(stack, L"Container creation cancelled.");
			return true;
		}
	}
	if(!isNew && is_base(c))
	{
		setError(stack, L"This container cannot be updated.\r\nUse one that inherits from it.");
		return true;
	}
	g_check = false;
	return false;
}



void tools_do_eval (const wchar* source)
{
	wait_for_draw_to_finish();
	value stack = stackArray();
	bool success = false;
	do{
		Container* container = get_selected(stack);
		if(container==NULL) break;

		Object* obj = NULL;
		if(ISOBJECT(get_type(container)))
			obj = (Object*)container->owner;

		const_Str3 name = c_name(container);
		main_entry_rfet = strEnd3(name) ? NULL : container;
		// only the RootContainer has no name

		if(strEnd2(source))
		{
			g_check = true;
			if(!strEnd3(name))
				source = C23(name);
			else if(!strEnd2(get_file_name()))
				source = get_name_from_path_name(get_file_name());
			else source = MAIN_ENTRY_NAME;
		}
		value v = calculator_evaluate_main(stack, source); // note: parsing only
		g_check = false;
		if(VERROR(v)) break;

		container = main_entry_rfet;
		if(!process_container(stack, container)) break;

		enum RFET_TYPE type = get_type(container);
		if(type!=RFET_UIDT
		&& type!=RFET_GLOBAL) select_container(container);
		else userinterface_set_text(UI_PATH_TEXT, L"");

		if(obj && !ISOBJECT(type))
		{
			assert(obj->container == container);
			obj->container->owner = NULL;
			obj->container = NULL;
			obj->destroy(obj); // TODO: NO, better send error message and tell user to use explicit deletion instead
		}
		else if(container==curr_uidt && type!=RFET_UIDT)
		{
			setError(stack, L"Container is no more of type UIDT. The used UIDT will change back to the original.");
			user_alert(L"Warning", getMessage(stack));
			if(tools_uidt_eval(stack, Layout, base_uidt))
				main_window_resize();
		}
		success = true;
		setBool(stack, true);
		userinterface_update();
	}while(0);
	if(!success) display_message(getMessage(vGet(stack)));
}



void tools_do_delete ()
{
	wait_for_draw_to_finish();
	value stack = stackArray();
	Container* container = get_selected(stack);
	if(!container) return;

	if(is_base(container))
	{ display_message(L"This container cannot be deleted."); return; }

	if(container==curr_uidt)
	{
		setError(stack, L"Container of type UIDT will be deleted. The used UIDT will change back to the original. Is that OK?"); // TODO: maybe change to first one inherited
		if(!user_confirm(L"Warning", getMessage(stack)))
			return;
		if(tools_uidt_eval(stack, Layout, base_uidt))
			main_window_resize();
	}

	setStr23(stack, c_rfet(container));
	const wchar* text = getStr2(stack);

	if(ISOBJECT(get_type(container)))
	{
		Object* obj = (Object*)container->owner;
		if(!obj->destroy(obj)) return;
	}
	else if(!inherits_remove(container)) return;

	main_entry_rfet = NULL;
	display_message(NULL);
	display_main_text(text);

	if(timer_paused()) userinterface_update();
}



void select_container (Container* container)
{
	if(!container) return;
	const_Str3 rfet = c_rfet(container);
	assert(!strEnd3(rfet));

	void* node = list_find(container_list(), NULL, pointer_compare, &container);
	assert(node!=NULL); if(node==NULL) return;
	list_head_push(container_list(), list_node_pop(container_list(), node));

	enum RFET_TYPE type = get_type(container);
	if(ISOBJECT(type))
	{
		Object* obj = (Object*)container->owner;
		userinterface_set_text (UI_MAIN_TEXT, C23(rfet));
		headMouse->clickedObject = obj;
		if(type==RFET_CAMERA) headMouse->clickedCamera = (Camera*)obj;
	}
	else display_main_text (C23(rfet));
	uint32_t stack[1000];
	userinterface_set_text(UI_PATH_TEXT, container_path_name(stack, container));
	display_message(NULL);
	if(container != main_entry_rfet) main_entry_rfet = NULL;
}

void tools_do_select ()
{
	value stack = stackArray();
	Container* container = get_selected(stack);
	if(!container) return;
	if(!strEnd3(c_rfet(container)))
		select_container(container);
	else display_main_text(NULL);
}

void tools_get_prev()
{
	void* node = list_head_pop(container_list());
	list_tail_push(container_list(), node);
	node = list_head(container_list());
	if(node) select_container(*(Container**)node);
}

void tools_get_next()
{
	void* node = list_tail_pop(container_list());
	list_head_push(container_list(), node);
	if(node) select_container(*(Container**)node);
}

void tools_do_clear() // TODO: consider also adding new_file();
{
	main_entry_rfet = NULL;
	userinterface_set_text(UI_MESG_TEXT, NULL);
	userinterface_set_text(UI_PATH_TEXT, NULL);
}



void tools_do_pause (bool pause)
{
	timer_pause(pause);
	const wchar* text = C21(pause ? TEXT_RESUME : TEXT_PAUSE);
	userinterface_set_text(UI_PAUSE_BUTTON, text);
}

static void time_reverse_set_text()
{
	const wchar* text = C21(timer_get_period()<0 ? TEXT_FORWARD : TEXT_BACKWARD);
	userinterface_set_text(UI_FORWARD_BUTTON, text);
}

void tools_go_forward (bool forward)
{
	int period = timer_get_period();
	bool positive = period>0;
	if(forward ^ positive)
	{
		period = -period;
		uint32_t stack[1000];
		timer_set_period(setSmaInt(stack, period));
	}
	time_reverse_set_text();
	display_time_text();
}

static void edit_period (const char* name)
{
	uint32_t V[10000];
	value stack = V;

	if(!curr_uidt) curr_uidt = base_uidt;
	Component* component = component_find(stack, curr_uidt, C31(name), 0);

	assert(component!=NULL);
	if(!component)
		display_message(getMessage(vGet(stack)));
	else{
		evaluation_instance(true);
		stack = component_evaluate(stack, curr_uidt, component, NULL);

		stack = timer_set_period(stack);
		if(VERROR(stack))
			display_message(getMessage(vGetPrev(stack)));
		else{
			time_reverse_set_text();
			display_time_text();
		}
	}
}
void tools_lower_period () { edit_period("lower_period"); }
void tools_higher_period() { edit_period("higher_period"); }


bool tools_set_time (const wchar* entry)
{
	value stack = stackArray();
	if(!entry) entry = userinterface_get_text(UI_TIME_TEXT);
	bool success=false;
	do{
		stack = rfet_parse_and_evaluate(stack, entry, L"set_time", NULL);
		if(VERROR(stack)) break;

		value y = vPrev(stack);
		const_value n = vGet(y);

		y = check_arguments(y,*n,2);
		if(y) { stack = y; break; }

		n += 2; // skip vector header
		stack = timer_set_period(setRef(stack, n));
		if(VERROR(stack)) break;

		n = vNext(n); // skip to second vector element
		stack = timer_set_time(setRef(stack, n));
		if(VERROR(stack)) break;

		time_reverse_set_text();
		userinterface_update();

		success=true;
		stack = setBool(stack, true);
	}while(0);
	if(!success) display_message(getMessage(vGetPrev(stack)));
	return success;
}



const_value tools_convert_text (const wchar* text, int ID)
{
	value stack = stackArray(); // get global array
	const_value n = NULL;

	wchar name[20] = L"ConvertTextF";
	intToStr(name+strlen2(name), ID);

	lchar array[20];
	const_Str3 str = set_lchar_array(array, 20, name, NULL);

	if(!curr_uidt) curr_uidt = base_uidt;
	Component* component = component_find(stack, curr_uidt, str, 0);
	if(component)
	{
		value v = stack;
		stack = setStr22(v, text); // set argument

		evaluation_instance(true);
		v = component_evaluate(stack, curr_uidt, component, v);

		if(!VERROR(v)) n = vGet(stack);
	}
	if(!n) user_alert(L"Error", getMessage(vGet(stack)));
	return n;
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
	if(ask_confirmation
	&& !user_confirm(L"Confirm", L"Remove All Objects?"))
		return;
	mouse_clear_pointers(headMouse);
	display_main_text(NULL);
	display_message(NULL);
	userinterface_set_text(UI_PATH_TEXT, NULL);
	object_remove_all(camera_list());
	object_remove_all(surface_list());
	#ifdef LIBRODT
	unload_images();
	#endif
}

void tools_clean()
{
	tools_remove_all_objects(false);
	userinterface__clean();
	userinterface_clean();
	mouse_clean();
	rfet_clean();
	usermessage = str2_free(usermessage);
}



static const wchar base_uidt_rfet[] =
#ifdef LIBRODT
	L"result;\r\n"
	L"\r\n"
	L"name = \"User_Interface_Definition_Text\";\r\n"
	L"\r\n"
	L"\r\n"
	L"wh = MainWindowWidthHeight;\r\n"
	L"w = wh[0];\r\n"
	L"h = wh[1];\r\n"
	L"\r\n"
	L"h1 =100;    # height of 1st layer\r\n"
	L"h2 = 30;    # height of 2nd layer\r\n"
	L"h3 = 30;    # height of 3rd layer\r\n"
	L"h4 = 50;    # height of 4th layer\r\n"
	L"h5 = 20;    # height of 5th layer\r\n"
	L"\r\n"
	L"y5 = h -h5; # y_start of 5th layer\r\n"
	L"y4 = y5-h4; # y_start of 4th layer\r\n"
	L"y3 = y4-h3; # y_start of 3rd layer\r\n"
	L"y2 = y3-h2; # y_start of 2nd layer\r\n"
	L"y1 = y2-h1; # y_start of 1st layer\r\n"
	L"\r\n"
	L"a = 60;\r\n"
	L"\r\n"
	L"\r\n"
	L"main_textfield  = {0, 0, w, y1};\r\n"
	L"\r\n"
	L"eval_button     = {   0, y1,   30   , h1};\r\n"
	L"mesg_textfield  = {  30, y1, w-30-20, h1};\r\n"
	L"lock_button     = {w-20, y1,      20, h1};\r\n"
	L"\r\n"
	L"prev_button     = { 0a, y2,  a , h2};\r\n"
	L"next_button     = { 1a, y2,  a , h2};\r\n"
	L"delete_button   = { 2a, y2,  a , h2};\r\n"
	L"clear_button    = { 3a, y2,  a , h2};\r\n"
	L"path_textfield  = { 4a, y2,w-4a, h2};\r\n"
	L"\r\n"
	L"pause_button    = { 0a, y3,  a , h3};\r\n"
	L"forward_button  = { 1a, y3,  a , h3};\r\n"
	L"lower_button    = { 2a, y3,  a , h3};\r\n"
	L"higher_button   = { 3a, y3,  a , h3};\r\n"
	L"time_textfield  = { 4a, y3,w-4a, h3};\r\n"
	L"\r\n"
	L"calc_button     = { 0, y4,    a     , h4};\r\n"
	L"calc_input      = { a, y4, (w-a)*3/5, h4};\r\n"
	L"calc_result     = { b, y4, (w-a)*2/5, h4};\r\n"
	L"  private b = a+(w-a)*3/5;\r\n"
	L"\r\n"
	L"status_bar      = {0, y5, w, h5};\r\n"
	L"\r\n"
	L"\r\n"
	L"private result =\r\n"
	L"    (\"main_textfield\" , main_textfield) ,\r\n"
	L"\r\n"
	L"    (\"eval_button   \" , eval_button   ) ,\r\n"
	L"    (\"mesg_textfield\" , mesg_textfield) ,\r\n"
	L"    (\"lock_button   \" , lock_button   ) ,\r\n"
	L"\r\n"
	L"    (\"prev_button   \" , prev_button   ) ,\r\n"
	L"    (\"next_button   \" , next_button   ) ,\r\n"
	L"    (\"delete_button \" , delete_button ) ,\r\n"
	L"    (\"clear_button  \" , clear_button  ) ,\r\n"
	L"    (\"path_textfield\" , path_textfield) ,\r\n"
	L"\r\n"
	L"    (\"pause_button  \" , pause_button  ) ,\r\n"
	L"    (\"forward_button\" , forward_button) ,\r\n"
	L"    (\"lower_button  \" , lower_button  ) ,\r\n"
	L"    (\"higher_button \" , higher_button ) ,\r\n"
	L"    (\"time_textfield\" , time_textfield) ,\r\n"
	L"\r\n"
	L"    (\"calc_button   \" , calc_button   ) ,\r\n"
	L"    (\"calc_input    \" , calc_input    ) ,\r\n"
	L"    (\"calc_result   \" , calc_result   ) ,\r\n"
	L"\r\n"
	L"    (\"status_bar    \" , status_bar    ) ;\r\n"
	L"\r\n"
	L"\r\n"
	L"private change(p,n) = p<0 ? p-n : p+n ;\r\n"
	L"lower_period  =  change(TimerPeriod, -25);\r\n"
	L"higher_period =  change(TimerPeriod, +25);\r\n"
	L"\r\n"
	L"ConvertTextF0(text) = PcnToChr(text) ; #\"Partly Code Number to Character\"\r\n"
	L"ConvertTextF1(text) = ChrToPcn(text) ; #\"Character to Partly Code Number\"\r\n"
	L"ConvertTextF2(text) = ChrToFcn(text) ; #\"Character to Fully Code Number\"\r\n"
	L"ConvertTextF3(text) = SetIsFcn(text) ; #\"Set Partly is Fully Code Number\"\r\n"
	L"\r\n"
	L"PixelsPUL = 1000 ; # Pixels Per Unit Length\r\n";
#else
	L"result;\r\n"
	L"\r\n"
	L"name = \"User_Interface_Definition_Text\";\r\n"
	L"\r\n"
	L"\r\n"
	L"wh = MainWindowWidthHeight;\r\n"
	L"w = wh[0];\r\n"
	L"h = wh[1];\r\n"
	L"\r\n"
	L"h1 =100;    # height of 1st layer\r\n"
	L"h2 =  0;    # height of 2nd layer\r\n"
	L"h3 = 30;    # height of 3rd layer\r\n"
	L"h4 =  0;    # height of 4th layer\r\n"
	L"h5 = 20;    # height of 5th layer\r\n"
	L"\r\n"
	L"y5 = h -h5; # y_start of 5th layer\r\n"
	L"y4 = y5-h4; # y_start of 4th layer\r\n"
	L"y3 = y4-h3; # y_start of 3rd layer\r\n"
	L"y2 = y3-h2; # y_start of 2nd layer\r\n"
	L"y1 = y2-h1; # y_start of 1st layer\r\n"
	L"\r\n"
	L"a = 60;\r\n"
	L"\r\n"
	L"\r\n"
	L"main_textfield  = {0, 0, w, y1};\r\n"
	L"\r\n"
	L"eval_button     = {   0, y1,   30   , h1};\r\n"
	L"mesg_textfield  = {  30, y1, w-30-20, h1};\r\n"
	L"lock_button     = {w-20, y1,      20, h1};\r\n"
	L"\r\n"
	L"prev_button     = { 0a, y2,  a , h2};\r\n"
	L"next_button     = { 1a, y2,  a , h2};\r\n"
	L"delete_button   = { 2a, y2,  a , h2};\r\n"
	L"clear_button    = { 3a, y2,  a , h2};\r\n"
	L"path_textfield  = { 4a, y2,w-4a, h2};\r\n"
	L"\r\n"
	L"pause_button    = { 0a, y3,  a , h3};\r\n"
	L"forward_button  = { 1a, y3,  a , h3};\r\n"
	L"lower_button    = { 2a, y3,  a , h3};\r\n"
	L"higher_button   = { 3a, y3,  a , h3};\r\n"
	L"time_textfield  = { 4a, y3,w-4a, h3};\r\n"
	L"\r\n"
	L"calc_button     = { 0, y4,    a     , h4};\r\n"
	L"calc_input      = { a, y4, (w-a)*3/5, h4};\r\n"
	L"calc_result     = { b, y4, (w-a)*2/5, h4};\r\n"
	L"  private b = a+(w-a)*3/5;\r\n"
	L"\r\n"
	L"status_bar      = {0, y5, w, h5};\r\n"
	L"\r\n"
	L"\r\n"
	L"private result =\r\n"
	L"    (\"main_textfield\" , main_textfield) ,\r\n"
	L"\r\n"
	L"    (\"eval_button   \" , eval_button   ) ,\r\n"
	L"    (\"mesg_textfield\" , mesg_textfield) ,\r\n"
	L"    (\"lock_button   \" , lock_button   ) ,\r\n"
	L"\r\n"
	L"    (\"prev_button   \" , prev_button   ) ,\r\n"
	L"    (\"next_button   \" , next_button   ) ,\r\n"
	L"    (\"delete_button \" , delete_button ) ,\r\n"
	L"    (\"clear_button  \" , clear_button  ) ,\r\n"
	L"    (\"path_textfield\" , path_textfield) ,\r\n"
	L"\r\n"
	L"    (\"pause_button  \" , pause_button  ) ,\r\n"
	L"    (\"forward_button\" , forward_button) ,\r\n"
	L"    (\"lower_button  \" , lower_button  ) ,\r\n"
	L"    (\"higher_button \" , higher_button ) ,\r\n"
	L"    (\"time_textfield\" , time_textfield) ,\r\n"
	L"\r\n"
	L"    (\"calc_button   \" , calc_button   ) ,\r\n"
	L"    (\"calc_input    \" , calc_input    ) ,\r\n"
	L"    (\"calc_result   \" , calc_result   ) ,\r\n"
	L"\r\n"
	L"    (\"status_bar    \" , status_bar    ) ;\r\n"
	L"\r\n"
	L"\r\n"
	L"private change(p,n) = p<0 ? p-n : p+n ;\r\n"
	L"lower_period  =  change(TimerPeriod, -25);\r\n"
	L"higher_period =  change(TimerPeriod, +25);\r\n"
	L"\r\n"
	L"ConvertTextF0(text) = PcnToChr(text) ; #\"Partly Code Number to Character\"\r\n"
	L"ConvertTextF1(text) = ChrToPcn(text) ; #\"Character to Partly Code Number\"\r\n"
	L"ConvertTextF2(text) = ChrToFcn(text) ; #\"Character to Fully Code Number\"\r\n"
	L"ConvertTextF3(text) = SetIsFcn(text) ; #\"Set Partly is Fully Code Number\"\r\n"
	L"\r\n";
#endif

static const wchar base_rodt_rodt[] =
	L"0;\r\n"
	L"\r\n"
	L"name = \"Rhyscitlema_Objects_Definition_Text\";\r\n"
	L"\r\n"
	L"private\r\n"
	L"message = \"\r\n"
	L"           left-click on a graph\r\n"
	L"\r\n"
	L"      or  right-click on a camera\r\n"
	L"\r\n"
	L" http://rhyscitlema.com/applications/graph-plotter-3d\r\n"
	L"\";\r\n"
	L"\r\n"
	L"private\r\n"
	L"\\rfet{0;\r\n"
	L" name = \"Object\";\r\n"
	L" origin =  (0,0,0) ;\r\n"
	L" axes =  (1,0,0),(0,1,0),(0,0,1) ;\r\n"
	L"}\r\n"
	L"\r\n"
	L"\\rfet{0;\r\n"
	L" type = \"Object\";\r\n"
	L" name = \"Camera\";\r\n"
	L" rectangle = {0,0,400,300,0,0};\r\n"
	L" zoom =  1;\r\n"
	L"}\r\n"
	L"\r\n"
	L"\\rfet{0;\r\n"
	L" type = \"Object\";\r\n"
	L" name = \"Surface\";\r\n"
	L" function(x,y,z) =  0;\r\n"
	L" colour(x,y,z) =  {x,y,z,1};\r\n"
	L" boundary =  {0,1,0,1,0,1};\r\n"
	L" accuracy =  1;\r\n"
	L"}\r\n";

static const wchar* get_base_uidt() { return base_uidt_rfet; }
static const wchar* get_base_rodt() { return base_rodt_rodt; }

