/*
	surface.c
*/

#include <tools.h>
#include <camera.h>
#include <surface.h>
#include <userinterface.h>


#ifdef LIBRODT
static List _surface_list = {0};
List* surface_list() { return &_surface_list; }
#endif


static bool surface_shootPixel (ObjectPaint op, int xp, int yp);

static void surface_paint (Object *obj, void *camera)
{ object_paint(obj, camera, surface_shootPixel); }

void surface_process (Object *obj, bool update)
{
	if(!obj) return;
	object_process(obj, update);
	if(!update && obj->variable[0] < 1) obj->variable[0] = 1;
}


static bool surface_destroy (Object *obj)
{
	Surface *surface = (Surface*)obj;
	if(obj==NULL) return false;
	if(obj->container && !inherits_remove(obj->container)) return false;
	list_delete(surface_list(), surface);
	memory_freed("Surface");
	return true;
}


bool surface_set (value stack, Container* container)
{
	Component *origin_comp, *axes_comp, *boundary_comp, *variable_comp, *function, *colour;
	Surface *surface = (Surface*)list_find(surface_list(), NULL, object_find, container);

	Component *comp;
	GET_COMPONENT ("origin"  , origin_comp  ,   0  , VST31)
	GET_COMPONENT ("axes"    , axes_comp    ,   0  , VST33)
	GET_COMPONENT ("boundary", boundary_comp,   0  , VST61)
	GET_COMPONENT ("accuracy", variable_comp,   0  , VST11)
	GET_COMPONENT ("function", function     , VST31, VST11)
	GET_COMPONENT ("colour"  , colour       , VST31, VST41)

	Object* obj = (Object*)surface;
	if(surface==NULL)
	{
		memory_alloc("Surface");
		surface = (Surface*)list_new(NULL, sizeof(Surface));
		list_head_push(surface_list(), surface);

		obj = (Object*)surface;
		obj->container = container;
		container->owner = &obj->container;

		obj->destroy = surface_destroy;
		obj->process = surface_process;
		obj->paint   = surface_paint;
	}
	obj->origin_comp   = origin_comp;
	obj->axes_comp     = axes_comp;
	obj->boundary_comp = boundary_comp;
	obj->variable_comp = variable_comp;
	surface->function  = function;
	surface->colour    = colour;
	return true;
}


#define CHECK_BOUNDARY(x,a,b,c) \
	do{ \
		s=A[a][x]; \
		if(cmrPixelDir[b]<0) { if(A[b][0]<s || s<A[b][1]) break; } else { if(A[b][0]>s || s>A[b][1]) break; } \
		if(cmrPixelDir[c]<0) { if(A[c][0]<s || s<A[c][1]) break; } else { if(A[c][0]>s || s>A[c][1]) break; } \
		if(s<t) { u=t; t=s; } else u=s; \
	}while(0);

#define ComputeFOFT \
	PO.point[0] = op.origin[0] + t * cmrPixelDir[0]; \
	PO.point[1] = op.origin[1] + t * cmrPixelDir[1]; \
	PO.point[2] = op.origin[2] + t * cmrPixelDir[2]; \
	v = op.stack+1; \
	v = setSmaFlt(v, PO.point[0]); \
	v = setSmaFlt(v, PO.point[1]); \
	v = setSmaFlt(v, PO.point[2]); \
	v = operations_evaluate(P, oper_function); \
	if(VERROR(v)) i=0; \
	else { i=1; f_of_t = getSmaFlt(vGetPrev(toFlt(v))); }

#define LIM (1/(SmaFlt)(1<<10))

#ifdef WIN32
// using (a==b) sometimes fails for an optimized compilation on MinGW
#define equal(a,b) (*(uint64_t*)(&a) == *(uint64_t*)(&b))
#else
#define equal(a,b) (a==b)
#endif


static bool surface_shootPixel (ObjectPaint op, int xp, int yp)
{
	bool i;
	SmaFlt s, t, u;

	SmaFlt *bdr;
	SmaFlt A[3][2];
	SmaFlt cmrPixelDir[3];   // the direction vector (d,e,f) of the pixel line
	Object* obj = op.obj;
	Camera* cmr = (Camera*)op.camera;

	if(!PixelInCamera(cmr,xp,yp)) { return 0; }
	PixelObject *pObj = &cmr->pixelObject[yp * cmr->XSize + xp];
	if(pObj->object == obj) return 1;

	PixelObject PO = {0};
	PO.object = obj;

	value v, P = op.stack+2 + 4*3 + 2*3; // see object_paint()
	const_value oper_function = c_oper(((Surface*)obj)->function);
	const_value oper_colour   = c_oper(((Surface*)obj)->colour);

	if(!op.checkX[xp])
	{
		op.checkX[xp] = 1;
		s = PixelToPointX(xp, cmr);
		op.X[xp][0] = op.axes[0][0]*s;
		op.X[xp][1] = op.axes[1][0]*s;
		op.X[xp][2] = op.axes[2][0]*s;
	}

	if(!op.checkY[yp])
	{
		op.checkY[yp] = 1;
		s = PixelToPointY(yp, cmr);
		u = *cmr->obj.variable; // get zoom
		op.Y[yp][0] = op.axes[0][1]*s + op.axes[0][2]*u;
		op.Y[yp][1] = op.axes[1][1]*s + op.axes[1][2]*u;
		op.Y[yp][2] = op.axes[2][1]*s + op.axes[2][2]*u;
	}

	cmrPixelDir[0] = op.X[xp][0] + op.Y[yp][0] + 0; // zero added
	cmrPixelDir[1] = op.X[xp][1] + op.Y[yp][1] + 0; // so to change
	cmrPixelDir[2] = op.X[xp][2] + op.Y[yp][2] + 0; // -0.0 to 0.0


	bdr = obj->boundary;
	A[0][0] = (bdr[0] - op.origin[0]) / cmrPixelDir[0];
	A[0][1] = (bdr[1] - op.origin[0]) / cmrPixelDir[0];
	A[1][0] = (bdr[2] - op.origin[1]) / cmrPixelDir[1];
	A[1][1] = (bdr[3] - op.origin[1]) / cmrPixelDir[1];
	A[2][0] = (bdr[4] - op.origin[2]) / cmrPixelDir[2];
	A[2][1] = (bdr[5] - op.origin[2]) / cmrPixelDir[2];
	t = u = WORLD_LIMIT;
	CHECK_BOUNDARY(0,0,1,2)
	CHECK_BOUNDARY(1,0,1,2)
	CHECK_BOUNDARY(0,1,0,2)
	CHECK_BOUNDARY(1,1,0,2)
	CHECK_BOUNDARY(0,2,0,1)
	CHECK_BOUNDARY(1,2,0,1)
	if(u >= WORLD_LIMIT) return 0;
	if(t<1) t=1;

	/* code below is to profile computation time of boundary box */
	/*PO.distance  = t;
	PO.point[0]  = op.origin[0] + t * cmrPixelDir[0];
	PO.point[1]  = op.origin[1] + t * cmrPixelDir[1];
	PO.point[2]  = op.origin[2] + t * cmrPixelDir[2];
	PO.colour[0] = PO.point[0]/(bdr[1]-bdr[0]);
	PO.colour[1] = PO.point[1]/(bdr[3]-bdr[2]);
	PO.colour[2] = PO.point[2]/(bdr[5]-bdr[4]);
	PO.colour[3] = 1;
	camera_putpixel(pObj, PO);
	if(obj) return 1; // always executes*/


	SmaFlt prev, f_of_t=0, low=0, high=0;
	s = (u-t) / obj->variable[0];
	prev = 1/(SmaFlt)PixelsPUL; // get prev = unit length per pixel
	if(s<prev) s=prev; // 's' cannot be less than a pixel's size
	u += prev; // += so to deal with t+=s accuracy errors

	ComputeFOFT
	prev = f_of_t;
	while(t<=u)
	{
		bool found=false;
		if(i==0);

		else if(f_of_t < -LIM){
			if(prev > +LIM)        // if f_of_t changes sign
			{
				low = t;
				high = t-s;
			}
		}
		else if(f_of_t > +LIM){
			if(prev < -LIM)        // if f_of_t changes sign
			{
				low = t-s;
				high = t;
			}
		}
		else found=true; // if -LIM <= f_of_t <= +LIM

		if(low!=0){
			while(true)
			{
				t = (low+high) / 2.0;
				if(equal(t,low) || equal(t,high))
				{
					if(-1 < f_of_t && f_of_t < 1)
						found=true;
					break;
				}
				ComputeFOFT
				if(i==0) break;
				else if(f_of_t < -LIM) low = t;
				else if(f_of_t > +LIM) high = t;
				else { found=true; break; }
			}
			low=0;
		}

		if(found)
		{
			v = operations_evaluate(P, oper_colour);
			value y = vPrev(v);
			if(!floatFromValue(v, 4, 1, PO.colour, "surface colour"))
			{
				display_message(getMessage(vGet(y)));
				tools_do_pause(true);
				return 0;
			}
			PO.distance = t;
			if(camera_putpixel(pObj, PO)) break;
		}

		prev = f_of_t;
		t += s;
		ComputeFOFT
	}
	return 1;
}

