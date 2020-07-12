#ifndef _MOUSE_H
#define _MOUSE_H
/*
	mouse.h
*/

#include <camera.h>


enum UserInputButton
{
	UserInputButtonStart = 0,

	Mouse_Left      = 1,
	Mouse_Middle    = 2,
	Mouse_Right     = 3,
	Mouse_Fourth    = 4,
	Mouse_Fifth     = 5,

	Key_Up          = 0x100,
	Key_Down        = 0x101,
	Key_Left        = 0x102,
	Key_Right       = 0x103,

	Key_NumLock     = 0x110,
	Key_ScrollLock  = 0x111,
	Key_CapsLock    = 0x112,

	Key_F1          = 0x121,
	Key_F2          = 0x122,
	Key_F3          = 0x123,
	Key_F4          = 0x124,
	Key_F5          = 0x125,
	Key_F6          = 0x126,
	Key_F7          = 0x127,
	Key_F8          = 0x128,
	Key_F9          = 0x129,
	Key_F10         = 0x12A,
	Key_F11         = 0x12B,
	Key_F12         = 0x12C,

	Key_Print       = 0x130,
	Key_Break       = 0x131,
	Key_Insert      = 0x132,
	Key_Home        = 0x133,
	Key_PageUp      = 0x134,
	Key_PageDown    = 0x135,
	Key_End         = 0x136,
	Key_Tab         = 0x137,
	Key_Menu        = 0x138,

	Key_Ctrl        = 0x130,
	Key_Ctrl_Left   = 0x131,
	Key_Ctrl_Right  = 0x132,
	Key_Alt         = 0x133,
	Key_Alt_Left    = 0x134,
	Key_Alt_Right   = 0x135,
	Key_Shift       = 0x136,
	Key_Shift_Left  = 0x137,
	Key_Shift_Right = 0x138,

	Key_Enter       = '\n',
	Key_Escape      = 0x1B,
	Key_Delete      = 0x7F,
	Key_Space       = ' ',
	Key_Backspace   = 0x08,

	Key_0 = '0',
	Key_1 = '1',
	Key_2 = '2',
	Key_3 = '3',
	Key_4 = '4',
	Key_5 = '5',
	Key_6 = '6',
	Key_7 = '7',
	Key_8 = '8',
	Key_9 = '9',

	Key_A = 'A',
	Key_B = 'B',
	Key_C = 'C',
	Key_D = 'D',
	Key_E = 'E',
	Key_F = 'F',
	Key_G = 'G',
	Key_H = 'H',
	Key_I = 'I',
	Key_J = 'J',
	Key_K = 'K',
	Key_L = 'L',
	Key_M = 'M',
	Key_N = 'N',
	Key_O = 'O',
	Key_P = 'P',
	Key_Q = 'Q',
	Key_R = 'R',
	Key_S = 'S',
	Key_T = 'T',
	Key_U = 'U',
	Key_V = 'V',
	Key_W = 'W',
	Key_X = 'X',
	Key_Y = 'Y',
	Key_Z = 'Z',

	Key_a = 'a',
	Key_b = 'b',
	Key_c = 'c',
	Key_d = 'd',
	Key_e = 'e',
	Key_f = 'f',
	Key_g = 'g',
	Key_h = 'h',
	Key_i = 'i',
	Key_j = 'j',
	Key_k = 'k',
	Key_l = 'l',
	Key_m = 'm',
	Key_n = 'n',
	Key_o = 'o',
	Key_p = 'p',
	Key_q = 'q',
	Key_r = 'r',
	Key_s = 's',
	Key_t = 't',
	Key_u = 'u',
	Key_v = 'v',
	Key_w = 'w',
	Key_x = 'x',
	Key_y = 'y',
	Key_z = 'z',

	UserInputButtonStop = 0x200
};


typedef struct _Mouse
{
	int ID;

	int motion[3];
	int previousPosition[3];
	int currentPosition[3];

	int showSignature;
	bool enabled;
	bool allow;
	bool moved;

	bool buttonPressed;
	//bool buttonBefore;

	bool Button[UserInputButtonStop];
	bool ButtonBefore[UserInputButtonStop];

	Object *pointedObject;
	Object *clickedObject;
	Camera *pointedCamera;
	Camera *clickedCamera;
	//Container* selected;

	struct _Mouse *prev, *next;
} Mouse;


extern Mouse *headMouse;

// clear mouse dx, dy and dz values to zeros
extern void mouse_clear_motion (Mouse *mouse);

// clear all mouse buttons to false
extern void mouse_clear_buttons (Mouse *mouse);

// clear all mouse pointer variables
extern void mouse_clear_pointers (Mouse *mouse);

// enable or disable mouse
extern void mouse_enable (Mouse *mouse, bool enable);

// process mouse input
extern void mouse_process (Mouse *mouse);

// record mouse input
extern void mouse_record (Mouse *mouse);

// install or initialise mouse
extern void mouse_init ();

// free memory, best called upon software exit
extern void mouse_clean ();


enum BUTTON_STATE {
	BUTTON_SAME,
	BUTTON_PRESS,
	BUTTON_RELEASE
};

bool on_key_event (int key, bool pressed);

bool on_mouse_event (
	int px, int py, int dz, int button,
	enum BUTTON_STATE state, DrawingWindow dw);


#define CancelUserInput(m) (m->ButtonBefore[Key_Escape] || m->Button[Key_Escape])

#endif
