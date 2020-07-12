/*
    outsider.c
*/

#include <outsider.h>
#include <getimage.h>
#include <timer.h>
#include <mouse.h>
#include <userinterface.h>

bool user_input_allowed = false;
#define AUI user_input_allowed


enum OUTSIDER_ID
{
    ID_NONE = UserInputButtonStart, //==0
    ID_TIME = UserInputButtonStop,
    TimerPeriod,
    TimerPaused,
    MainWindowWidthHeight,

    GetPixelsPUL,
    MouseMotion,
    PointedPixel,
    PointedPoint,
    CameraDistance,
    CameraResize,

    PointedCamera,
    PointedObject,
    FocusedCamera,
    FocusedObject,

    // below are those that are functions
    GetImageWidthHeight = OUTSIDER_ISA_FUNCTION,
    GetImagePixelColour,
    GetImagePixelMatrix,

    ID_END
};


/* Get Outsider ID */
#define GOID(string, ID) if(0==strcmp31(str, string)) return ID;


/* Return outsider's ID != 0, else return 0 */
int outsider_getID (const_Str3 str)
{
    GOID("time", ID_TIME)
    GOID("TimerPeriod", TimerPeriod)
    GOID("TimerPaused", TimerPaused)
    GOID("MainWindowWidthHeight", MainWindowWidthHeight)

    #ifdef LIBRODT
    GOID("PixelsPUL", GetPixelsPUL)

    GOID("MouseMotion", MouseMotion)
    GOID("Mouse_Left", Mouse_Left)
    GOID("Mouse_Right", Mouse_Right)
    GOID("Mouse_Middle", Mouse_Middle)

    GOID("PointedPixel", PointedPixel)
    GOID("PointedPoint", PointedPoint)
    GOID("CameraDistance", CameraDistance)
    GOID("CameraResize", CameraResize)

    GOID("PointedCamera", PointedCamera)
    GOID("PointedObject", PointedObject)
    GOID("FocusedCamera", FocusedCamera)
    GOID("FocusedObject", FocusedObject)

    GOID("GetImageWidthHeight", GetImageWidthHeight)
    GOID("GetImagePixelColour", GetImagePixelColour)
    GOID("GetImagePixelMatrix", GetImagePixelMatrix)
    #endif

    GOID("Key_Ctrl"       , Key_Ctrl)
    GOID("Key_Ctrl_Left"  , Key_Ctrl_Left)
    GOID("Key_Ctrl_Right" , Key_Ctrl_Right)
    //GOID("Key_Alt"        , Key_Alt)
    //GOID("Key_Alt_Left"   , Key_Alt_Left)
    //GOID("Key_Alt_Right"  , Key_Alt_Right)
    GOID("Key_Shift"      , Key_Shift)
    GOID("Key_Shift_Left" , Key_Shift_Left)
    GOID("Key_Shift_Right", Key_Shift_Right)

    GOID("Key_Up"   , Key_Up)
    GOID("Key_Down" , Key_Down)
    GOID("Key_Left" , Key_Left)
    GOID("Key_Right", Key_Right)

    GOID("Key_F1" , Key_F1)
    GOID("Key_F2" , Key_F2)
    GOID("Key_F3" , Key_F3)
    GOID("Key_F4" , Key_F4)
    GOID("Key_F5" , Key_F5)
    GOID("Key_F6" , Key_F6)
    GOID("Key_F7" , Key_F7)
    GOID("Key_F8" , Key_F8)
    GOID("Key_F9" , Key_F9)
    GOID("Key_F10", Key_F10)
    GOID("Key_F11", Key_F11)
    GOID("Key_F12", Key_F12)

    GOID("Key_0", Key_0)
    GOID("Key_1", Key_1)
    GOID("Key_2", Key_2)
    GOID("Key_3", Key_3)
    GOID("Key_4", Key_4)
    GOID("Key_5", Key_5)
    GOID("Key_6", Key_6)
    GOID("Key_7", Key_7)
    GOID("Key_8", Key_8)
    GOID("Key_9", Key_9)

    GOID("Key_A", Key_A)
    GOID("Key_B", Key_B)
    GOID("Key_C", Key_C)
    GOID("Key_D", Key_D)
    GOID("Key_E", Key_E)
    GOID("Key_F", Key_F)
    GOID("Key_G", Key_G)
    GOID("Key_H", Key_H)
    GOID("Key_I", Key_I)
    GOID("Key_J", Key_J)
    GOID("Key_K", Key_K)
    GOID("Key_L", Key_L)
    GOID("Key_M", Key_M)
    GOID("Key_N", Key_N)
    GOID("Key_O", Key_O)
    GOID("Key_P", Key_P)
    GOID("Key_Q", Key_Q)
    GOID("Key_R", Key_R)
    GOID("Key_S", Key_S)
    GOID("Key_T", Key_T)
    GOID("Key_U", Key_U)
    GOID("Key_V", Key_V)
    GOID("Key_W", Key_W)
    GOID("Key_X", Key_X)
    GOID("Key_Y", Key_Y)
    GOID("Key_Z", Key_Z)

    GOID("Key_a", Key_a)
    GOID("Key_b", Key_b)
    GOID("Key_c", Key_c)
    GOID("Key_d", Key_d)
    GOID("Key_e", Key_e)
    GOID("Key_f", Key_f)
    GOID("Key_g", Key_g)
    GOID("Key_h", Key_h)
    GOID("Key_i", Key_i)
    GOID("Key_j", Key_j)
    GOID("Key_k", Key_k)
    GOID("Key_l", Key_l)
    GOID("Key_m", Key_m)
    GOID("Key_n", Key_n)
    GOID("Key_o", Key_o)
    GOID("Key_p", Key_p)
    GOID("Key_q", Key_q)
    GOID("Key_r", Key_r)
    GOID("Key_s", Key_s)
    GOID("Key_t", Key_t)
    GOID("Key_u", Key_u)
    GOID("Key_v", Key_v)
    GOID("Key_w", Key_w)
    GOID("Key_x", Key_x)
    GOID("Key_y", Key_y)
    GOID("Key_z", Key_z)

    return ID_NONE;
}


#ifdef LIBRODT
static inline value getCPN (value v, const void* obj)
{
    const Container* c = obj ? ((const Object*)obj)->container : NULL;
    container_path_name(v, c);
    return vnext(v);
}

static value getPoint (value v, const Camera* cmr)
{
    int i  = headMouse->currentPosition[0];
    int j  = headMouse->currentPosition[1];
    do{
        if(cmr==NULL || !PixelInCamera(cmr, i, j)) break;
        i = j * cmr->XSize + i;
        if(cmr->pixelObject[i].object==NULL) break;
        return floatToValue(v, 3, 1, cmr->pixelObject[i].point);
    }while(0);
    return vcopy(v, VST31); // set to zero
}
#endif


/* Evaluate outsider expression */
value set_outsider (value v, int ID)
{
#ifdef LIBRODT
    bool b;
    const Camera *fc;
#endif

    switch(ID)
    {
    case ID_TIME: v = timer_get_time(v); break;
    case TimerPeriod: v = setSmaInt(v, timer_get_period()); break;
    case TimerPaused: v = setSmaInt(v, timer_paused()); break;

    case MainWindowWidthHeight:
        v = setSmaInt(v, main_window_width);
        v = setSmaInt(v, main_window_height);
        v = tovector(v, 2);
        break;

#ifdef LIBRODT
    case GetPixelsPUL: v = setSmaInt(v, PixelsPUL); break;

    case PointedPixel:
        v = setSmaInt(v, headMouse->currentPosition[0]);
        v = setSmaInt(v, headMouse->currentPosition[1]);
        v = setSmaInt(v, 0);
        v = tovector(v, 3);
        break;

    case PointedPoint: return getPoint(v, headMouse->pointedCamera);
    //case FocusedPoint: return getPoint(v, headMouse->clickedCamera); // must save FocusedPoint

    case MouseMotion:
        b = !AUI;
        v = setSmaInt(v, b?0: headMouse->motion[0]);
        v = setSmaInt(v, b?0: headMouse->motion[1]);
        v = setSmaInt(v, b?0: headMouse->motion[2]);
        v = tovector(v, 3);
        break;

    case CameraDistance:
        b = (!AUI || headMouse->clickedObject == NULL);
        v = setSmaFlt(v, b?0: headMouse->clickedObject->valueOfT);
        break;

    case CameraResize:
        fc = headMouse->clickedCamera;
        b = (!AUI || !fc);
        v = setSmaInt(v, b?0: fc->dXPost);
        v = setSmaInt(v, b?0: fc->dYPost);
        v = setSmaInt(v, b?0: fc->dXSize);
        v = setSmaInt(v, b?0: fc->dYSize);
        v = setSmaInt(v, 0);
        v = setSmaInt(v, 0);
        v = tovector(v, 6);
        break;

    case PointedCamera: return getCPN(v, headMouse->pointedCamera);
    case PointedObject: return getCPN(v, headMouse->pointedObject);
    case FocusedCamera: return getCPN(v, headMouse->clickedCamera);
    case FocusedObject: return getCPN(v, headMouse->clickedObject);

    case GetImageWidthHeight: return get_image_width_height(v);
    case GetImagePixelColour: return get_image_pixel_colour(v);
    case GetImagePixelMatrix: return get_image_pixel_matrix(v);
#endif

    default:
        if(UserInputButtonStart < ID && ID < UserInputButtonStop)
            v = setBool(v, (AUI && headMouse->Button[ID]));

        else{
            const_Str2 argv[2];
            v = VstToStr(setSmaInt(v, ID), 0,-1,-1);
            argv[0] = L"Software outsider ID = %s is unrecognised.";
            argv[1] = getStr2(vGetPrev(v));
            v = setMessage(v, 0, 2, argv);
        }
        break;
    }
    return v;
}

