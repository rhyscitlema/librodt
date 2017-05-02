/*
    outsider.c
*/

#include <outsider.h>
#include <timer.h>
#include <mouse.h>
#include <userinterface.h>

// from getimage.c
extern bool get_image_width_height (ExprCallArg eca);
extern bool get_image_pixel_colour (ExprCallArg eca);
extern bool get_image_pixel_matrix (ExprCallArg eca);

bool user_input_allowed = false;
#define AUI (user_input_allowed && replacement_container)



enum OUTSIDER_ID
{
    NONE = UserInputButtonStart, //==0
    TIME = UserInputButtonStop,
    TimerPeriod,

    PixelSize,
    MouseMotion,
    MainWindowWidthHeight,

	LocalPointedPixel,
	LocalPointedPoint,
    CameraDistance,
    CameraResize,
    CameraPosition,
    CameraAxes,

    // below are those that are functions
    GetImageWidthHeight = OUTSIDER_ISA_FUNCTION,
    GetImagePixelColour,
    GetImagePixelMatrix,

	ID_END
};



/* Get Outsider ID */
#define GOID(string, ID) if(0==strcmp31(lstr, string)) return ID;


/* Return outsider's ID > 0, else return 0. */
unsigned int outsider_getID (const lchar* lstr)
{
    GOID ("time", TIME)
    GOID ("TimerPeriod", TimerPeriod)

    GOID ("MainWindowWidthHeight", MainWindowWidthHeight)

    #ifdef LIBRODT
    GOID ("PixelSize", PixelSize)

    GOID ("GetImageWidthHeight", GetImageWidthHeight)
    GOID ("GetImagePixelColour", GetImagePixelColour)
    GOID ("GetImagePixelMatrix", GetImagePixelMatrix)

	GOID ("LocalPointedPixel", LocalPointedPixel)
	GOID ("LocalPointedPoint", LocalPointedPoint)
    GOID ("CameraDistance", CameraDistance)
    GOID ("CameraResize", CameraResize)
    GOID ("CameraPosition", CameraPosition)
    GOID ("CameraAxes", CameraAxes)

    GOID ("MouseMotion", MouseMotion)
    GOID ("Mouse_Left", Mouse_Left)
    GOID ("Mouse_Right", Mouse_Right)
    GOID ("Mouse_Middle", Mouse_Middle)
    #endif

    GOID ("Key_Ctrl"       , Key_Ctrl)
    GOID ("Key_Ctrl_Left"  , Key_Ctrl_Left)
    GOID ("Key_Ctrl_Right" , Key_Ctrl_Right)
  //GOID ("Key_Alt"        , Key_Alt)
  //GOID ("Key_Alt_Left"   , Key_Alt_Left)
  //GOID ("Key_Alt_Right"  , Key_Alt_Right)
    GOID ("Key_Shift"      , Key_Shift)
    GOID ("Key_Shift_Left" , Key_Shift_Left)
    GOID ("Key_Shift_Right", Key_Shift_Right)

    GOID ("Key_Up"   , Key_Up)
    GOID ("Key_Down" , Key_Down)
    GOID ("Key_Left" , Key_Left)
    GOID ("Key_Right", Key_Right)

    GOID ("Key_F1" , Key_F1)
    GOID ("Key_F2" , Key_F2)
    GOID ("Key_F3" , Key_F3)
    GOID ("Key_F4" , Key_F4)
    GOID ("Key_F5" , Key_F5)
    GOID ("Key_F6" , Key_F6)
    GOID ("Key_F7" , Key_F7)
    GOID ("Key_F8" , Key_F8)
    GOID ("Key_F9" , Key_F9)
    GOID ("Key_F10", Key_F10)
    GOID ("Key_F11", Key_F11)
    GOID ("Key_F12", Key_F12)

    GOID ("Key_0", Key_0)
    GOID ("Key_1", Key_1)
    GOID ("Key_2", Key_2)
    GOID ("Key_3", Key_3)
    GOID ("Key_4", Key_4)
    GOID ("Key_5", Key_5)
    GOID ("Key_6", Key_6)
    GOID ("Key_7", Key_7)
    GOID ("Key_8", Key_8)
    GOID ("Key_9", Key_9)

    GOID ("Key_A", Key_A)
    GOID ("Key_B", Key_B)
    GOID ("Key_C", Key_C)
    GOID ("Key_D", Key_D)
    GOID ("Key_E", Key_E)
    GOID ("Key_F", Key_F)
    GOID ("Key_G", Key_G)
    GOID ("Key_H", Key_H)
    GOID ("Key_I", Key_I)
    GOID ("Key_J", Key_J)
    GOID ("Key_K", Key_K)
    GOID ("Key_L", Key_L)
    GOID ("Key_M", Key_M)
    GOID ("Key_N", Key_N)
    GOID ("Key_O", Key_O)
    GOID ("Key_P", Key_P)
    GOID ("Key_Q", Key_Q)
    GOID ("Key_R", Key_R)
    GOID ("Key_S", Key_S)
    GOID ("Key_T", Key_T)
    GOID ("Key_U", Key_U)
    GOID ("Key_V", Key_V)
    GOID ("Key_W", Key_W)
    GOID ("Key_X", Key_X)
    GOID ("Key_Y", Key_Y)
    GOID ("Key_Z", Key_Z)

    GOID ("Key_a", Key_a)
    GOID ("Key_b", Key_b)
    GOID ("Key_c", Key_c)
    GOID ("Key_d", Key_d)
    GOID ("Key_e", Key_e)
    GOID ("Key_f", Key_f)
    GOID ("Key_g", Key_g)
    GOID ("Key_h", Key_h)
    GOID ("Key_i", Key_i)
    GOID ("Key_j", Key_j)
    GOID ("Key_k", Key_k)
    GOID ("Key_l", Key_l)
    GOID ("Key_m", Key_m)
    GOID ("Key_n", Key_n)
    GOID ("Key_o", Key_o)
    GOID ("Key_p", Key_p)
    GOID ("Key_q", Key_q)
    GOID ("Key_r", Key_r)
    GOID ("Key_s", Key_s)
    GOID ("Key_t", Key_t)
    GOID ("Key_u", Key_u)
    GOID ("Key_v", Key_v)
    GOID ("Key_w", Key_w)
    GOID ("Key_x", Key_x)
    GOID ("Key_y", Key_y)
    GOID ("Key_z", Key_z)

    return NONE;
}



bool set_outsider (ExprCallArg eca)
{
    bool b;
    int rows=1, cols=1;
    value* out = eca.stack;
	const Camera* cmr = headMouse->clickedCamera;

    int id = GET_OUTSIDER_ID(eca.expression);
    switch(id)
    {
    case TIME: out[0] = timer_get_time(); break;
    case TimerPeriod: out[0] = setSmaInt(timer_get_period()); break;
    case PixelSize: out[0] = setSmaFlt(PIXELSIZE); break;

    case MainWindowWidthHeight: rows=2;
        out[1+0] = setSmaInt(main_window_width);
        out[1+1] = setSmaInt(main_window_height);
        break;

	case LocalPointedPixel: rows=3;
		out[1+0] = setSmaInt(headMouse->currentPosition[0]);
		out[1+1] = setSmaInt(headMouse->currentPosition[1]);
		out[1+2] = setSmaInt(0);
		break;

	case LocalPointedPoint: rows=3;
		cmr   = headMouse->pointedCamera;
        int i = headMouse->currentPosition[0];
        int j = headMouse->currentPosition[1];

        b = (cmr==NULL || cmr->pixelObject==NULL || !PixelInCamera(cmr, i, j));
        if(!b)
        {   i = j * cmr->XSize + i;
            b = cmr->pixelObject[i]==NULL;
        }
        if(b) vst_shift(out, VST31); // set to zero
        else vst_shift(out, cmr->pixelObject[i]->point);
		break;

    case MouseMotion: rows=3;
        b = !AUI;
        out[1+0] = setSmaInt(b?0: headMouse->motion[0]);
        out[1+1] = setSmaInt(b?0: headMouse->motion[1]);
        out[1+2] = setSmaInt(b?0: headMouse->motion[2]);
        break;

    case CameraDistance:
        b = (!AUI || headMouse->clickedObject == NULL);
        out[0] = setSmaFlt(b?0: headMouse->clickedObject->valueOfT);
        break;

    case CameraResize: rows=6;
        b = (!AUI || !cmr);
        out[1+0] = setSmaInt(b?0: cmr->dXPost);
        out[1+1] = setSmaInt(b?0: cmr->dYPost);
        out[1+2] = setSmaInt(b?0: cmr->dXSize);
        out[1+3] = setSmaInt(b?0: cmr->dYSize);
        out[1+4] = setSmaInt(0);
        out[1+5] = setSmaInt(0);
        break;

    case CameraPosition: rows=3;
        b = (!AUI || !cmr);
        out[1+0] = setSmaFlt(b?0: cmr->obj.position[0]);
        out[1+1] = setSmaFlt(b?0: cmr->obj.position[1]);
        out[1+2] = setSmaFlt(b?0: cmr->obj.position[2]);
        break;

    case CameraAxes: rows=3; cols=3;
        if(!AUI || !cmr) memcpy(out, VST33, MSIZE(rows, cols));
        else valueSt_from_floats (out, rows, cols, (SmaFlt*)cmr->obj.axes);
        break;

    #ifdef LIBRODT
    case GetImageWidthHeight: return get_image_width_height(eca);
    case GetImagePixelColour: return get_image_pixel_colour(eca);
    case GetImagePixelMatrix: return get_image_pixel_matrix(eca);
    #endif

    default:
        if(UserInputButtonStart < id && id < UserInputButtonStop)
            out[0] = setSmaInt((AUI && headMouse->Button[id]) ? 1 : 0);

        else { set_message (eca.garg->message, L"Software Error: in \\1 at \\2:\\3:\r\nOn '\\4': outsider \\5 is unrecognised.", eca.expression->name, TIS2(0,id)); return 0; }
    }

    valueSt_matrix_setSize (eca.stack, rows, cols);
    return 1;
}

