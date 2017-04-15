/*
    object.c
*/

#include <_stdio.h>
#include <timer.h>
#include <mouse.h>
#include <object.h>
#include <camera.h>
#include <surface.h>
#include <textobj.h>
#include <userinterface.h>
#include <tools.h>


int object_find (void* key1, void* key2, void* arg)
{
    Container* c1 = (Container*)key1;
    Container* c2 = ((Object*)key2)->container;
    return ((c1 < c2) ? -1 : (c1 > c2) ? +1 : 0);
}


void object_process (Object *obj, bool update)
{
    int x, y;
    Camera *cmr;
    Mouse *mouse;
    Component *component;

    if(!obj) return;
    assert(obj->container!=NULL);
    mouse = headMouse;

    GeneralArg garg = {
        .caller = obj->container,
        .message = errorMessage(),
        .argument = NULL
    };
    ExprCallArg eca = {
        .garg = &garg,
        .expression = NULL,
        .stack = mainStack()
    };

    if(update)
    {
        if(obj != mouse->clickedObject)
        { obj->isClicked = false; return; }

        if(!obj->isClicked || mouse->buttonPressed)
        {   obj->isClicked = true;

            cmr = mouse->clickedCamera;
            if(cmr!=NULL && cmr!=(Camera*)obj)
            {
                x = mouse->currentPosition[0];
                y = mouse->currentPosition[1];
                if(PixelInCamera(cmr,x,y))
                {
                    const PixelObject* po = cmr->pixelObject[y*cmr->XSize+x];
                    if(po) obj->valueOfT = po->distance;
                }
            }
        }
        if(CancelUserInput(mouse))
        {
            if(obj->container->replacement_count > 0)
            {  obj->container->replacement_count = 0;
               tools_do_eval(NULL);
               graphplotter_update_repeat = true;
            }
        }
        else if(user_input_allowed)
        {
            replacement_container = obj->container;
            component = (Component*)avl_min(&obj->container->inners);
            for( ; component != NULL; component = (Component*)avl_next(component))
                if(!component_evaluate(eca, component, 0)) ;//display_message(eca.garg->errormessage); // TODO: error ignored due to call without proper argument value structure
            replacement_container = NULL;

            if(obj->container->replacement_count > 0)
            {
                if(mouse->buttonPressed)
                {
                    mouse->showSignature = 1;
                    replacement_commit(obj->container);
                }
                graphplotter_update_repeat = true;
            }
        }
    }
    else
    {
        eca.stack += stackSize()/2;
        eca.garg->message += stackSize()/2;
        mchar* errmsg = eca.garg->message;

        if(!component_evaluate(eca, obj->origin_comp, VST31)
        || !floatFromVst (obj->position, eca.stack, 3, errmsg, "object position"))
            display_message (errmsg);

        if(!component_evaluate (eca, obj->axes_comp, VST33)
        || !floatFromVst (obj->axes[0], MVE(eca.stack,0,3), 3, errmsg, "object x-axis")
        || !floatFromVst (obj->axes[1], MVE(eca.stack,1,3), 3, errmsg, "object y-axis")
        || !floatFromVst (obj->axes[2], MVE(eca.stack,2,3), 3, errmsg, "object z-axis"))
            display_message (errmsg);

        if(!component_evaluate (eca, obj->boundary_comp, VST61)
        || !floatFromVst (obj->boundary, eca.stack, 6, errmsg, "object boundary"))
            display_message (errmsg);

        if(!component_evaluate (eca, obj->variable_comp, VST11))
            display_message (errmsg);
        else floatFromVst (obj->variable , eca.stack, 1, errmsg, NULL);
    }
}


/* ALL THE BELOW IS ABOUT PAINTING TO CAMERA */

static void checkarray_initialise (Camera *cmr)
{
    int i;
    cmr->check++;
    if(cmr->check!=1) return;
    for(i=0; i < cmr->XSize; i++) cmr->checkX[i] = 0;
    for(i=0; i < cmr->YSize; i++) cmr->checkY[i] = 0;
}



static void cameraLocal_to_World_to_ObjectLocal (Object *obj, Camera *cmr)
{
    SmaFlt temp[3], zoom = *cmr->obj.variable;

    cmr->paintAxes[0][0] = dotproduct (obj->axes[0], cmr->obj.axes[0]);
    cmr->paintAxes[0][1] = dotproduct (obj->axes[0], cmr->obj.axes[1]);
    cmr->paintAxes[0][2] = dotproduct (obj->axes[0], cmr->obj.axes[2]);
    cmr->paintAxes[1][0] = dotproduct (obj->axes[1], cmr->obj.axes[0]);
    cmr->paintAxes[1][1] = dotproduct (obj->axes[1], cmr->obj.axes[1]);
    cmr->paintAxes[1][2] = dotproduct (obj->axes[1], cmr->obj.axes[2]);
    cmr->paintAxes[2][0] = dotproduct (obj->axes[2], cmr->obj.axes[0]);
    cmr->paintAxes[2][1] = dotproduct (obj->axes[2], cmr->obj.axes[1]);
    cmr->paintAxes[2][2] = dotproduct (obj->axes[2], cmr->obj.axes[2]);

    substractvectors(temp, cmr->obj.position, obj->position);
    cmr->paintPost[0] = dotproduct (obj->axes[0], temp) - cmr->paintAxes[0][2]*zoom;
    cmr->paintPost[1] = dotproduct (obj->axes[1], temp) - cmr->paintAxes[1][2]*zoom;
    cmr->paintPost[2] = dotproduct (obj->axes[2], temp) - cmr->paintAxes[2][2]*zoom;
}



// putpixel given a point (x, y, z) of an object in its local coordinates
static bool putLocalPixel (const Object *obj, const Camera *cmr, SmaFlt x, SmaFlt y, SmaFlt z, SmaFlt point[2])
{
    SmaFlt t, temp[3], temp2[3], zoom = *cmr->obj.variable;

    // Get CameraOrigin CaO with an offset of camera_zoom
    temp2[0] = cmr->obj.position[0] - cmr->obj.axes[2][0]*zoom;
    temp2[1] = cmr->obj.position[1] - cmr->obj.axes[2][1]*zoom;
    temp2[2] = cmr->obj.position[2] - cmr->obj.axes[2][2]*zoom;

    // Local_To_World coordinate transformation
    temp[0] = obj->position[0]  +  obj->axes[0][0]*x + obj->axes[1][0]*y + obj->axes[2][0]*z;
    temp[1] = obj->position[1]  +  obj->axes[0][1]*x + obj->axes[1][1]*y + obj->axes[2][1]*z;
    temp[2] = obj->position[2]  +  obj->axes[0][2]*x + obj->axes[1][2]*y + obj->axes[2][2]*z;

    // Get line direction vector, of the line joining CaO and (x,y,z).
    temp[0] = temp[0] - temp2[0];
    temp[1] = temp[1] - temp2[1];
    temp[2] = temp[2] - temp2[2];

    // Get t = distance from Ca0 to camera plane
    t = dotproduct (cmr->obj.axes[2], temp) / zoom;
    if(t<1) return false; // if (x,y,z) is not on the nZ-axis of the
                          // camera, then it cannot be put to screen.

    // Final direction vector from Ca0 to camera plane.
    temp[0] = temp[0]/t;
    temp[1] = temp[1]/t;
    temp[2] = temp[2]/t;

    // Extract the local point about the camera origin (0,0)
    point[0] = dotproduct(cmr->obj.axes[0], temp);
    point[1] = dotproduct(cmr->obj.axes[1], temp);
    return true;
}



void object_paint (Object *obj, void* camera, bool (*shootPixel) (Object *obj, void* camera, int xp, int yp))
{
    int x, y;
    int a, b;
    int pX, nX, pY, nY;
    SmaFlt marray[8][2];
    const SmaFlt* boundary;
    Camera *cmr = (Camera*)camera;

    if(!obj || !cmr || !shootPixel) return;
    checkarray_initialise(cmr);
    cameraLocal_to_World_to_ObjectLocal(obj,cmr);
    boundary = obj->boundary;

    while(true)
    {
        x=y=-1;

        // attempt the middle of the object boundary-box
        if(putLocalPixel (obj,cmr,
                          (boundary[0]+boundary[1])/2,
                          (boundary[2]+boundary[3])/2,
                          (boundary[4]+boundary[5])/2,
                          marray[0]))
        {
            a = (int)PointToPixelX (marray[0][0], cmr);
            b = (int)PointToPixelY (marray[0][1], cmr);

            if(PixelInCamera(cmr,a,b)) { x=a; y=b; break; }
        }

        a = cmr->XSize/2;
        b = cmr->YSize/2;
        if(shootPixel(obj, cmr, a, b)) { x=a; y=b; break; }

        b = 0;
        for(a=0; a < cmr->XSize; a++)
            if(shootPixel(obj, cmr, a, b)) { x=a; y=b; break; }
        if(x>=0) break;

        b = cmr->YSize-1;
        for(a=0; a < cmr->XSize; a++)
            if(shootPixel(obj, cmr, a, b)) { x=a; y=b; break; }
        if(x>=0) break;

        a = 0;
        for(b=0; b < cmr->YSize; b++)
            if(shootPixel(obj, cmr, a, b)) { x=a; y=b; break; }
        if(x>=0) break;

        a = cmr->XSize-1;
        for(b=0; b < cmr->YSize; b++)
            if(shootPixel(obj, cmr, a, b)) { x=a; y=b; break; }

        break;
    }

    if(x<0 || y<0) return;

    nX=pX=x;
    nY=pY=y;

    /* Do upward pixel-propagation */
    while(pY >= 0)      // while there is a pY to process
    {
        y = pY;
        pY = -1;        // mark that there is no more pY

        for(x=pX-0; x >= 0; x--) // go from pixel (x-1,y) down to (0,y)
        {
            if(shootPixel(obj, cmr, x, y)==0) { x--; if(x>=0 && shootPixel(obj, cmr, x, y)==0) break; }
            if(pY<0 && shootPixel(obj, cmr, x, y+1)>0) { pY = y+1; pX = x; }
        }
        for(x=pX+1; x < cmr->XSize; x++) // go from pixel (x+1,y) up to (XSize-1, y)
        {
            if(shootPixel(obj, cmr, x, y)==0) { x++; if(x<cmr->XSize && shootPixel(obj, cmr, x, y)==0) break; }
            if(pY<0 && shootPixel(obj, cmr, x, y+1)>0) { pY = y+1; pX = x; }
        }
    }

    /* Do downward pixel-propagation */
    while(nY >= 0)      // while there is an nY to process
    {
        y = nY;
        nY = -1;        // mark that there is no more nY

        for(x=nX-0; x >= 0; x--) // go from pixel (x-1,y) down to (0,y)
        {
            if(shootPixel(obj, cmr, x, y)==0) { x--; if(x>=0 && shootPixel(obj, cmr, x, y)==0) break; }
            if(nY<0 && shootPixel(obj, cmr, x, y-1)>0) { nY = y-1; nX = x; }
        }
        for(x=nX+1; x < cmr->XSize; x++) // go from pixel (x+1,y) up to (XSize-1, y)
        {
            if(shootPixel(obj, cmr, x, y)==0) { x++; if(x<cmr->XSize && shootPixel(obj, cmr, x, y)==0) break; }
            if(nY<0 && shootPixel(obj, cmr, x, y-1)>0) { nY = y-1; nX = x; }
        }
    }
}



/* update world coordinate values of local X and Y axes,
   by using the local Zaxis and the world z-axis (0,0,1).
   They must NOT be parallel to each other.

   Then rotate the local dimension by 'assigning'
   the Xaxis to become the rotation unit vector Rot.
   Therefore z-component Rot[2] is NOT used at all.
*/


/*
    Origin = O = (Ox, Oy, Oz)
    X-axis = U = (Ux, Uy, Uz)
    Y-axis = V = (Vx, Vy, Vz)
    Z-axis = W = (Wx, Wy, Wz)

    LOCAL_TO_WORLD coordinate transformation
    Xw     Ox     Ux Vx Wx   Xl
    Yw  =  Oy  +  Uy Vy Wy x Yl
    Zw     Oz     Uz Vz Wz   Zl

    WORLD_TO_LOCAL coordinate transformation
    Xl     Ux Uy Uz     Xw - Ox
    Yl  =  Vx Vy Vz  x  Yw - Oy
    Zl     Wx Wy Wz     Zw - Oz


    LOCAL_TO_WORLD_TO_LOCAL coordinate transformation

    X2l     U2x U2y U2z       O1x - O2x     U1x V1x W1x   X1l
    Y2l  =  V2x V2y V2z  x  ( O1y - O2y  +  U1y V1y W1y x Y1l  )
    Z2l     W2x W2y W2z       O1z - O2z     U1z V1z W1z   Z1l
*/

