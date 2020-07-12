/*
    mouse.c
*/

#include <_malloc.h>
#include <userinterface.h>
#include <timer.h>
#include <mouse.h>

Mouse *headMouse = NULL;



void mouse_clear_motion (Mouse *mouse)
{
    if(!mouse) return;
    mouse->motion[0] = mouse->motion[1] = mouse->motion[2] = 0;
    mouse->previousPosition[0] = mouse->currentPosition[0];
    mouse->previousPosition[1] = mouse->currentPosition[1];
    mouse->previousPosition[2] = mouse->currentPosition[2];
}

void mouse_clear_buttons (Mouse *mouse)
{
    if(!mouse) return;
    memset (mouse->Button, 0, sizeof(mouse->Button));
    memset (mouse->ButtonBefore, 0, sizeof(mouse->ButtonBefore));
}

void mouse_clear_pointers (Mouse *mouse)
{
    if(!mouse) return;
    mouse->clickedObject = mouse->pointedObject = NULL;
    mouse->clickedCamera = mouse->pointedCamera = NULL;
}

void mouse_enable (Mouse *mouse, bool enable)
{
    if(!mouse) return;
    mouse->enabled = enable;
    mouse->allow = enable;
    mouse_clear_motion (mouse);
    mouse_clear_buttons (mouse);
    mouse_clear_pointers (mouse);
}

void mouse_init ()
{
    if(headMouse) return;
    Mouse *mouse = _malloc(sizeof(Mouse), "Mouse");
    memset(mouse, 0, sizeof(Mouse));
    mouse->ID = 1;
    headMouse = mouse;
    mouse_enable(headMouse, true);
}

void mouse_clean ()
{
    _free(headMouse, "Mouse");
    headMouse = NULL;
}



static Object *camera_get_object (Camera *camera, int x, int y)
{
    if(camera && PixelInCamera(camera, x, y))
        return camera->pixelObject[y * camera->XSize + x].object;
    return NULL;
}

void mouse_process (Mouse *m)
{
    if(m->buttonPressed) { m->allow = true; mouse_clear_motion(m); }
    if(CancelUserInput(m)) m->allow = false;
    if(!m->allow) mouse_clear_motion(m);

    if(m->Button[Mouse_Right] && !m->ButtonBefore[Mouse_Right])
    {
        if(m->pointedCamera == NULL)
            m->showSignature = 2;       // '2' for showing null text

        else if(m->clickedObject != (Object*)m->pointedCamera)
            m->showSignature = 1;       // '1' for showing object text

        m->clickedObject = (Object*)m->pointedCamera;
        m->clickedCamera = m->pointedCamera;
    }

    m->pointedObject = camera_get_object (m->pointedCamera, m->currentPosition[0], m->currentPosition[1]);

    if(m->Button[Mouse_Left] && !m->ButtonBefore[Mouse_Left])
    {
        if(m->pointedObject == NULL)
            m->showSignature = 2;       // '2' for showing null text

        else if(m->clickedObject != m->pointedObject)
            m->showSignature = 1;       // '1' for showing object text

        m->clickedObject = m->pointedObject;
        m->clickedCamera = m->pointedCamera;
    }
}

void mouse_record (Mouse *m)
{
    m->buttonPressed = false;
    user_input_allowed = false;
    memcpy (m->ButtonBefore, m->Button, sizeof(m->Button));
}



bool on_key_event (int key, bool pressed)
{
    Mouse *m = headMouse;
    if(!m || !m->enabled) return false;
    m->Button[key] = pressed;

    if((!m->ButtonBefore[key] &&  m->Button[key])  // if a new press or
    || ( m->ButtonBefore[key] && !m->Button[key])) // if a new release
         m->buttonPressed = true;

    m->Button[Key_Ctrl]  = m->Button[Key_Ctrl_Left]  || m->Button[Key_Ctrl_Right];
    m->Button[Key_Alt]   = m->Button[Key_Alt_Left]   || m->Button[Key_Alt_Right];
    m->Button[Key_Shift] = m->Button[Key_Shift_Left] || m->Button[Key_Shift_Right];

    mouse_process(m);
    user_input_allowed = true;
    userinterface_update();
    return true;
}

bool on_mouse_event (int px, int py, int dz, int button, enum BUTTON_STATE state, DrawingWindow dw)
{
    Mouse *m = headMouse;
    if(!m || !m->enabled) return false;

    m->previousPosition[0] = m->currentPosition[0];
    m->previousPosition[1] = m->currentPosition[1];
    m->previousPosition[2] = m->currentPosition[2];

    if(state == BUTTON_SAME) m->moved = true;
    else
    {   m->buttonPressed = true;
        bool pressed = (state == BUTTON_PRESS);
        m->Button[Mouse_Left]   = (button & 0x1) && pressed;
        m->Button[Mouse_Middle] = (button & 0x2) && pressed;
        m->Button[Mouse_Right]  = (button & 0x4) && pressed;
    }
    m->currentPosition[0]  = px;
    m->currentPosition[1]  = py;
    m->currentPosition[2] += dz;

    m->motion[0] += px - m->previousPosition[0];
    m->motion[1] += py - m->previousPosition[1];
    m->motion[2] += dz;

    #ifdef LIBRODT
    m->pointedCamera = findCameraFromDW(dw);
    #endif

    mouse_process(m);
    user_input_allowed = true;
    if(timer_paused()) userinterface_update();
    return true;
}
