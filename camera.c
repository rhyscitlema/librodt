/*
    camera.c
*/

#include <_stdio.h>     // for sprintf1() only
#include <_malloc.h>
#include <mouse.h>
#include <timer.h>
#include <tools.h>
#include <camera.h>
#include <userinterface.h>


static List _camera_list = {0};
List* camera_list = &_camera_list;


void camera_paint_initialise (Camera *camera)
{
    if(!camera) return;
    int i, size = camera->XSize * camera->YSize;
    for(i=0; i<size; i++)
    {
        PixelObject *po, *next;
        po = camera->pixelObject[i];
        for( ; po != NULL; po = next)
        {
            next = po->next;
            _free(po); memory_freed("PO");
        }
        camera->pixelObject[i] = NULL;
    }
}



bool camera_putpixel (Camera *camera, int pixel, const PixelObject* pixelObject)
{
    PixelObject *PO, *prev, *next;
    assert(camera!=NULL);

    const value* colour = pixelObject->colour;
    if(getSmaInt(colour[1+0])==0
    && getSmaInt(colour[1+1])==0
    && getSmaInt(colour[1+2])==0
    && getSmaInt(colour[1+3])==0)
        return false;

    next = camera->pixelObject[pixel];
    prev = NULL;
    while(next!=NULL && next->distance <  pixelObject->distance)
    {
        value v = next->colour[1+3]; // get opacity
        if(IsSmaInt(getType(v)) && getSmaInt(v)==1) return true;
        prev = next;
        next = next->next;
    }

    PO = _malloc(sizeof(PixelObject)); memory_alloc("PO");
    *PO = *pixelObject;
    if(prev==NULL) camera->pixelObject[pixel] = PO;
    else prev->next = PO;
    PO->next = next;

    value v = PO->colour[1+3]; // get opacity
    return (IsSmaInt(getType(v)) && getSmaInt(v)==1);
}



void camera_paint_finalise (Camera *camera)
{
    int i, j;
    value v;
    value bg[1+4];
    const value* fg;
    const PixelObject *PO, *ARR[100];
    if(!camera) return;
    const value one = setSmaInt(1);

    for(i=0; i < camera->XSize * camera->YSize; i++)
    {
        PO = camera->pixelObject[i];
        if(PO==NULL) { camera->pixelColour[i]=0; continue; }

        for(j=0; PO != NULL; PO = PO->next) ARR[j++] = PO;

            fg = ARR[--j]->colour;
            bg[1+0] = multiply(fg[1+0], fg[1+3]);
            bg[1+1] = multiply(fg[1+1], fg[1+3]);
            bg[1+2] = multiply(fg[1+2], fg[1+3]);
            bg[1+3] =          fg[1+3];

        while(j>0)
        {
            fg = ARR[--j]->colour;
            v = subtract(one, fg[1+3]);
            bg[1+0] = add(multiply(fg[1+0], fg[1+3]), multiply(multiply(bg[1+0], bg[1+3]), v));
            bg[1+1] = add(multiply(fg[1+1], fg[1+3]), multiply(multiply(bg[1+1], bg[1+3]), v));
            bg[1+2] = add(multiply(fg[1+2], fg[1+3]), multiply(multiply(bg[1+2], bg[1+3]), v));
            bg[1+3] = add(         fg[1+3]          , multiply(         bg[1+3]          , v));
        }
        v = setSmaInt(0xFF);
        camera->pixelColour[i] = (((int)getSmaInt(_floor(multiply(bg[1+0], v))) & 0xFF) <<  0)
                               + (((int)getSmaInt(_floor(multiply(bg[1+1], v))) & 0xFF) <<  8)
                               + (((int)getSmaInt(_floor(multiply(bg[1+2], v))) & 0xFF) << 16)
                               + (((int)getSmaInt(_floor(multiply(bg[1+3], v))) & 0xFF) << 24);
    }
    drawing_window_draw (camera->drawing_window);
}



static void cameraSizeChange (Camera *camera, int XSize, int YSize)
{
    long size = (XSize * YSize) * (sizeof(int) + sizeof(PixelObject*))
              + (XSize + YSize) * (sizeof(int) + sizeof(SmaFlt)*3);

    if(camera==NULL || (camera->XSize==XSize && camera->YSize==YSize)) return;
    camera_paint_initialise(camera);

    camera->pixelColour = (int*) _realloc (camera->pixelColour , size);
    camera->pixelObject = (PixelObject**) (camera->pixelColour + XSize*YSize);
    camera->storeX      = (SmaFlt (*)[3]) (camera->pixelObject + XSize*YSize);
    camera->storeY      = (SmaFlt (*)[3]) (camera->storeX      + XSize);
    camera->checkX      = (int*)          (camera->storeY      + YSize);
    camera->checkY      = (int*)          (camera->checkX      + XSize);

    memset(camera->pixelColour, 0, size);
    camera->XSize = XSize;
    camera->YSize = YSize;
}



static inline int Flt2Int (SmaFlt n) { return (int)(n + ((n>=0) ? +0.000001 : -0.000001)); }

static void camera_process (Object *obj, bool update)
{
    int XPost, YPost, XSize, YSize;
    Camera *temp, *cmr = (Camera*)obj;
    if(!obj) return;
    object_process(obj, update);

    if(update)
    {
        GeneralArg garg = { .caller = obj->container, .message = errorMessage(), .argument = NULL };
        ExprCallArg eca = { .garg = &garg, .expression = NULL, .stack = mainStack() };

        if((cmr->dXPost != 0 || cmr->dYPost != 0
         || cmr->dXSize != 0 || cmr->dYSize != 0)
         &&(headMouse->clickedObject != obj))
        {
            evaluation_instance++;
            replacement_container = obj->container;
            user_input_allowed = true;

            temp = headMouse->clickedCamera;
            headMouse->clickedCamera = cmr;
            component_evaluate (eca, obj->boundary_comp, NULL);
            headMouse->clickedCamera = temp;

            user_input_allowed = false;
            graphplotter_update_repeat = true;
        }
        cmr->dXPost = cmr->dYPost = cmr->dXSize = cmr->dYSize = 0;

        if(!component_evaluate( eca, obj->boundary_comp, VST61)
        || !floatFromVst( obj->boundary, eca.stack, 6, eca.garg->message, "object boundary"))
            display_message(eca.garg->message);
    }
    XPost = Flt2Int(obj->boundary[0]);
    YPost = Flt2Int(obj->boundary[1]);
    XSize = Flt2Int(obj->boundary[2]);
    YSize = Flt2Int(obj->boundary[3]);

    if(XSize<0 || XSize>10000 || YSize<0 || YSize>10000)
    {
        char errormessage[100];
        sprintf1(errormessage, "Error: camera size = (%d, %d) is invalid.\n", XSize, YSize);
        display_message(CST21(errormessage));
        tools_do_pause(true);

        if(update)
        {
            // schedule to recover current size
            cmr->dXSize = cmr->XSize - XSize;
            cmr->dYSize = cmr->YSize - YSize;
            graphplotter_update_repeat = true;
        }
    }
    else
    {
        if(!update)
        {
            // update changes in camera size
            cmr->XPost = XPost;
            cmr->YPost = YPost;
            cameraSizeChange(cmr, XSize, YSize);
            drawing_window_move(cmr->drawing_window);
        }
    }
}

static void camera_paint (Object *obj, void *camera)
{ obj=NULL; camera=NULL; }



static bool camera_destroy (Object *obj)
{
    Camera *camera = (Camera*)obj;
    if(!camera || !camera->pixelColour) return false;
    if(obj->container && !inherits_remove(obj->container)) return false;

    camera_paint_initialise(camera); // do this before freeing pixelColour
    _free(camera->pixelColour);
    camera->pixelColour = NULL;

    // drawing_window_remove() may want to call camera_destroy() recursively,
    // for that reason cmr->pixelColour is set to NULL then is later checked.
    drawing_window_remove(camera->drawing_window);

    list_delete(camera_list, camera);
    memory_freed("Camera");
    return true;
}



bool camera_set (Container* container)
{
    Component *origin_comp, *axes_comp, *boundary_comp, *variable_comp;
    Camera *camera = (Camera*)list_find(camera_list, 0, object_find, container);

    GeneralArg garg = { .caller = container, .message = errorMessage(), .argument = NULL };
    ExprCallArg eca = { .garg = &garg, .expression = NULL, .stack = mainStack() };

    Component *comp;
    GET_COMPONENT ("origin"   , origin_comp  , 0, VST31)
    GET_COMPONENT ("axes"     , axes_comp    , 0, VST33)
    GET_COMPONENT ("rectangle", boundary_comp, 0, VST61)
    GET_COMPONENT ("zoom"     , variable_comp, 0, VST11)

    Object* obj = (Object*)camera;
    if(camera==NULL)
    {
        memory_alloc("Camera");
        camera = (Camera*)list_new(NULL, sizeof(Camera));
        list_head_push(camera_list, camera);

        obj = (Object*)camera;
        obj->container = container;
        container->owner = &obj->container;

        obj->destroy = camera_destroy;
        obj->process = camera_process;
        obj->paint   = camera_paint;

        camera->drawing_window = drawing_window_new ();
        if(camera->drawing_window == NULL)
        {
            strcpy21 (errorMessage(), "Could not create a new window for the new camera.\r\n");
            camera_destroy((Object*)camera);
            return false;
        }
        else drawing_window_name(camera->drawing_window, CST23(c_name(container)));
    }
    obj->origin_comp   = origin_comp;
    obj->axes_comp     = axes_comp;
    obj->boundary_comp = boundary_comp;
    obj->variable_comp = variable_comp;
    return true;
}



/* Note, findCameraFromDW() is:
 * - declared only in camera.h
 * - defined only in camera.c
 * - used only in drawing_window.c
 */
Camera *findCameraFromDW (DrawingWindow dw)
{
    if(dw==NULL) return NULL;
    Camera *camera = (Camera*)list_head(camera_list);
    for( ; camera != NULL; camera = (Camera*)list_next(camera))
        if(drawing_window_equal(camera->drawing_window, dw))
            break;
    return camera;
}

void drawing_window_close_do (DrawingWindow dw)
{
    wait_for_draw_to_finish();
    Object *camera = (Object*)findCameraFromDW(dw);
    assert(camera!=NULL);
    if(camera->container) display_main_text(CST23(c_rfet(camera->container)));
    camera->destroy(camera);
}

void drawing_window_resize_do (DrawingWindow dw)
{
    Camera *cmr = findCameraFromDW(dw);
    assert(cmr!=NULL);

    if(cmr->dXPost==0 && cmr->dYPost==0
    && cmr->dXSize==0 && cmr->dYSize==0) return;

    headMouse->buttonPressed = true;
    user_input_allowed = true;

    wait_for_draw_to_finish(); // TODO: must come before 'if', not sure why
    if(draw_request_count<=0 && timer_paused()) userinterface_update();
}

