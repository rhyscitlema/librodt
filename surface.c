/*
    surface.c
*/

#include <tools.h>
#include <camera.h>
#include <surface.h>
#include <userinterface.h>


static List _surface_list = {0};
List* surface_list = &_surface_list;


static bool surface_shootPixel (Object *obj, void* camera, int xp, int yp);

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
    list_delete(surface_list, surface);
    memory_freed("Surface");
    return true;
}



bool surface_set (Container* container)
{
    Component *origin_comp, *axes_comp, *boundary_comp, *variable_comp, *function, *colour;
    Surface *surface = (Surface*)list_find(surface_list, NULL, object_find, container);

    GeneralArg garg = { .caller = container, .message = errorMessage(), .argument = NULL };
    ExprCallArg eca = { .garg = &garg, .expression = NULL, .stack = mainStack() };

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
        list_head_push(surface_list, surface);

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
    while(true) /* not a loop */ \
    {   s=A[a][x]; \
        if(cmrPixelDir[b]<0) { if(A[b][0]<s || s<A[b][1]) break; } else { if(A[b][0]>s || s>A[b][1]) break; } \
        if(cmrPixelDir[c]<0) { if(A[c][0]<s || s<A[c][1]) break; } else { if(A[c][0]>s || s>A[c][1]) break; } \
        if(s<t) { u=t; t=s; } else u=s; \
        break; \
    }

#define ComputeFOFT \
    PO.point[1+0] = setSmaFlt(cmrPosition[0] + t * cmrPixelDir[0]); \
    PO.point[1+1] = setSmaFlt(cmrPosition[1] + t * cmrPixelDir[1]); \
    PO.point[1+2] = setSmaFlt(cmrPosition[2] + t * cmrPixelDir[2]); \
    if(!expression_evaluate(f_eca)) i=0; \
    else { i=1; f_of_t = getSmaFlt(toFlt(*f_eca.stack)); }

#define LIM (1/(SmaFlt)(1<<10))

// TODO: using (a==b) sometimes fails for an optimized compilation on Windows
#define equal(a,b) (*(unsigned long long*)(&a) == *(unsigned long long*)(&b))



static bool surface_shootPixel (Object *obj, void *camera, int xp, int yp)
{
    bool i;
    int pixel;
    SmaFlt s, t, u;
    SmaFlt prev, f_of_t, low, high;

    SmaFlt *Sx, *Sy;
    SmaFlt A[3][2];
    SmaFlt* cmrPosition;     // the position vector (a,b,c) on the pixel line
    SmaFlt cmrPixelDir[3];   // the direction vector (d,e,f) of the pixel line

    Surface *surface = (Surface*)obj;
    Camera *cmr = (Camera*)camera;
    SmaFlt zoom = *cmr->obj.variable;

    if(!PixelInCamera(cmr,xp,yp)) return 0;
    pixel = yp * cmr->XSize + xp;
    if(cmr->pixelObject[pixel]!=NULL && cmr->pixelObject[pixel]->object == obj) return 1;

    PixelObject PO = {0};
    PO.object = obj;


    GeneralArg garg = {
        .caller = obj->container,
        .message = errorMessage() + stackSize()/2,
        .argument = PO.point
    };
    ExprCallArg f_eca = {
        .garg = &garg,
        .expression = c_root(surface->function),
        .stack = mainStack() + stackSize()/2
    };
    ExprCallArg c_eca = f_eca;
    c_eca.expression = c_root(surface->colour);


    Sx = cmr->storeX[xp];
    Sy = cmr->storeY[yp];

    if(cmr->checkX[xp] != cmr->check)
    {
        cmr->checkX[xp] = cmr->check;
        s = PixelToPointX(xp,cmr);
        Sx[0] = cmr->paintAxes[0][0]*s;
        Sx[1] = cmr->paintAxes[1][0]*s;
        Sx[2] = cmr->paintAxes[2][0]*s;
    }

    if(cmr->checkY[yp] != cmr->check)
    {
        cmr->checkY[yp] = cmr->check;
        s = PixelToPointY(yp,cmr);
        Sy[0] = cmr->paintAxes[0][1]*s + cmr->paintAxes[0][2]*zoom;
        Sy[1] = cmr->paintAxes[1][1]*s + cmr->paintAxes[1][2]*zoom;
        Sy[2] = cmr->paintAxes[2][1]*s + cmr->paintAxes[2][2]*zoom;
    }

    cmrPixelDir[0] = Sx[0] + Sy[0] + 0; // zero added
    cmrPixelDir[1] = Sx[1] + Sy[1] + 0; // so to change
    cmrPixelDir[2] = Sx[2] + Sy[2] + 0; // -0.0 to 0.0


    cmrPosition = cmr->paintPost;
    Sy = obj->boundary;
    A[0][0] = (Sy[0] - cmrPosition[0]) / cmrPixelDir[0];
    A[0][1] = (Sy[1] - cmrPosition[0]) / cmrPixelDir[0];
    A[1][0] = (Sy[2] - cmrPosition[1]) / cmrPixelDir[1];
    A[1][1] = (Sy[3] - cmrPosition[1]) / cmrPixelDir[1];
    A[2][0] = (Sy[4] - cmrPosition[2]) / cmrPixelDir[2];
    A[2][1] = (Sy[5] - cmrPosition[2]) / cmrPixelDir[2];
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
    /*
    PO.distance  = t;
    PO.point[0]  = cmrPosition[0] + t * cmrPixelDir[0];
    PO.point[1]  = cmrPosition[1] + t * cmrPixelDir[1];
    PO.point[2]  = cmrPosition[2] + t * cmrPixelDir[2];
    PO.colour[0] = setSmaFlt(PO.point[0]/(Sy[1]-Sy[0]));
    PO.colour[1] = setSmaFlt(PO.point[1]/(Sy[3]-Sy[2]));
    PO.colour[2] = setSmaFlt(PO.point[2]/(Sy[5]-Sy[4]));
    PO.colour[3] = setSmaInt(1);
    camera_putpixel(cmr, pixel, &PO);
    if(obj) return 1; // always executes
    */

    f_of_t=0;
    low=high=0;
    vst_shift(PO.point, VST31);
    s = (u-t) / obj->variable[0];
    if(s<PIXELSIZE) s=PIXELSIZE;
    ComputeFOFT
    prev = f_of_t;

    while(t<=u+PIXELSIZE) // +0.001 to deal with t+=s accuracy errors
    {
        bool found=false;
        if(i==0);

        else if(f_of_t < -LIM)
        {   if(prev > +LIM)        // if f_of_t changes sign
            {
                low = t;
                high = t-s;
            }
        }
        else if(f_of_t > +LIM)
        {   if(prev < -LIM)        // if f_of_t changes sign
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
        low=0;}

        if(found)
        {
            if(!expression_evaluate(c_eca)) { display_message(c_eca.garg->message); tools_do_pause(true); return 0; }
            PO.distance = t;
            vst_shift(PO.colour, c_eca.stack);
            if(camera_putpixel(cmr, pixel, &PO)) break;
        }

        prev = f_of_t;
        t += s;
        ComputeFOFT
    }
    return 1;
}

