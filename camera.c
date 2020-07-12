/*
	camera.c
*/

#include <_malloc.h>
#include <mouse.h>
#include <timer.h>
#include <tools.h>
#include <camera.h>
#include <userinterface.h>


static List _camera_list = {0};
List* camera_list() { return &_camera_list; }


void camera_paint_initialise (Camera *camera)
{
	if(!camera) return;
	PixelObject* PO = camera->pixelObject;
	PixelObject* end = PO + camera->XSize * camera->YSize;
	for( ; PO!=end; PO++) PO->object = NULL;
}



bool camera_putpixel (PixelObject *pObj, const PixelObject PO)
{
	if(!PO.colour[0]
	&& !PO.colour[1]
	&& !PO.colour[2]
	&& !PO.colour[3]) return false;

	if(!pObj->object || pObj->distance > PO.distance)
		*pObj = PO;
	return true;
}



void camera_paint_finalise (Camera *camera)
{
	if(!camera) return;
	uint32_t* PC = camera->pixelColour;
	const uint32_t* end = PC + camera->XSize * camera->YSize;
	const PixelObject* PO = camera->pixelObject;
	for( ; PC!=end; PC++, PO++)
	{
		uint32_t colr;
		if(PO->object)
		{
			colr = (( (int)FltToInt(PO->colour[0] * 0xFF) & 0xFF) <<  0)
			     + (( (int)FltToInt(PO->colour[1] * 0xFF) & 0xFF) <<  8)
			     + (( (int)FltToInt(PO->colour[2] * 0xFF) & 0xFF) << 16)
			     + (( (int)FltToInt(PO->colour[3] * 0xFF) & 0xFF) << 24);
		}
		else colr = 0;
		*PC = colr;
	}
	drawing_window_draw (camera->drawing_window);
}



static void cameraSizeChange (Camera *camera, int XSize, int YSize)
{
	if(camera==NULL || (camera->XSize==XSize && camera->YSize==YSize)) return;
	camera_paint_initialise(camera);

	long size = (XSize * YSize) * (sizeof(uint32_t) + sizeof(PixelObject));
	camera->pixelColour = (uint32_t*) _realloc (camera->pixelColour , size, "cameraSize");
	camera->pixelObject = (PixelObject*) (camera->pixelColour + XSize*YSize);

	memset(camera->pixelColour, 0, size);
	camera->XSize = XSize;
	camera->YSize = YSize;
}



static void camera_process (Object *obj, bool update)
{
	int XPost, YPost, XSize, YSize;
	Camera *temp, *cmr = (Camera*)obj;
	if(!obj) return;
	object_process(obj, update);
	uint32_t stack[10000];

	if(update)
	{
		if((cmr->dXPost != 0 || cmr->dYPost != 0
		||  cmr->dXSize != 0 || cmr->dYSize != 0)
		&& (headMouse->clickedObject != obj))
		{
			evaluation_instance(true);
			user_input_allowed = true;

			temp = headMouse->clickedCamera;
			headMouse->clickedCamera = cmr;
			component_evaluate (stack, obj->container, obj->boundary_comp, NULL);
			headMouse->clickedCamera = temp;

			user_input_allowed = false;
			graphplotter_update_repeat = true;
		}
		cmr->dXPost = cmr->dYPost = cmr->dXSize = cmr->dYSize = 0;

		value v = component_evaluate(stack, obj->container, obj->boundary_comp, NULL);
		if(!floatFromValue(v, 6, 1, obj->boundary, "camera rectangle"))
			display_message(getMessage(stack));
	}
	XPost = (int)FltToInt(obj->boundary[0]);
	YPost = (int)FltToInt(obj->boundary[1]);
	XSize = (int)FltToInt(obj->boundary[2]);
	YSize = (int)FltToInt(obj->boundary[3]);

	if(XSize<0 || XSize>10000
	|| YSize<0 || YSize>10000)
	{
		const_Str2 argv[3];
		value v = stack;
		argv[0] = L"Error: camera size = (%s, %s) is invalid.\n";
		argv[1] = (Str2)v; v+=64; intToStr((Str2)v, XSize);
		argv[2] = (Str2)v; v+=64; intToStr((Str2)v, YSize);
		setMessage(v, 0, 3, argv);
		display_message(getMessage(v));

		tools_do_pause(true);
		if(update)
		{
			// schedule to recover current size
			cmr->dXSize = cmr->XSize - XSize;
			cmr->dYSize = cmr->YSize - YSize;
			graphplotter_update_repeat = true;
		}
	}
	else if(!update)
	{
		// update changes in camera size
		cmr->XPost = XPost;
		cmr->YPost = YPost;
		cameraSizeChange(cmr, XSize, YSize);
		drawing_window_move(cmr->drawing_window);
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
	_free(camera->pixelColour, "cameraSize");
	camera->pixelColour = NULL;

	// drawing_window_remove() may want to call camera_destroy() recursively,
	// for that reason cmr->pixelColour is set to NULL then is later checked.
	drawing_window_remove(camera->drawing_window);

	list_delete(camera_list(), camera);
	memory_freed("Camera");
	return true;
}



bool camera_set (value stack, Container* container)
{
	Component *origin_comp, *axes_comp, *boundary_comp, *variable_comp;
	Camera *camera = (Camera*)list_find(camera_list(), 0, object_find, container);

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
		list_head_push(camera_list(), camera);

		obj = (Object*)camera;
		obj->container = container;
		container->owner = &obj->container;

		obj->destroy = camera_destroy;
		obj->process = camera_process;
		obj->paint   = camera_paint;

		camera->drawing_window = drawing_window_new ();
		if(camera->drawing_window == NULL)
		{
			setError(stack, L"Could not create a new window for the new camera.\r\n");
			camera_destroy((Object*)camera);
			return false;
		}
		else drawing_window_name(camera->drawing_window, C23(c_name(container)));
	}
	obj->origin_comp   = origin_comp;
	obj->axes_comp     = axes_comp;
	obj->boundary_comp = boundary_comp;
	obj->variable_comp = variable_comp;
	return true;
}



/* Note, findCameraFromDW() is:
*  - declared only in camera.h
*  - defined only in camera.c
*  - used only in drawing_window.c
*/
Camera *findCameraFromDW (DrawingWindow dw)
{
	if(dw==NULL) return NULL;
	Camera *camera = (Camera*)list_head(camera_list());
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
	if(camera->container) display_main_text(C23(c_rfet(camera->container)));
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

