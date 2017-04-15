#ifndef _CAMERA_H
#define _CAMERA_H
/*
    camera.h
*/

#include "object.h"
#include <drawing_window.h>


typedef struct _PixelObject
{
    Object* object;     // the object
    SmaFlt distance;    // the vertical distance from camera to object
	value point[1+3];   // the viewed point on the object
    value colour[1+4];  // the colour of the point
    struct _PixelObject *next;
} PixelObject;


typedef struct _Camera
{
    Object obj;                         // this must be the first declaration

    int XPost, YPost, XSize, YSize;     // Camera as a rectangle on the main screen
    int dXPost, dYPost, dXSize, dYSize; // Resize of the camera rectangle
    int XS, YS, notfirst, memorysize;

    DrawingWindow drawing_window;       // Utility to draw to device screen
    PixelObject** pixelObject;          // Array of objects infront of the camera pixels
    int* pixelColour;                   // Array of colours for the camera pixels

    /* The below are used only inside object_paint()
       and all function calls as from object_paint()
       They are for temporary storage. */
    SmaFlt paintAxes[3][3];
    SmaFlt paintPost[3];
    SmaFlt (*storeX)[3];
    SmaFlt (*storeY)[3];
    int check, *checkX, *checkY;
} Camera;


#ifdef LIBRODT
extern List* camera_list;
#else
#define camera_list NULL
#endif

bool camera_set (Container* container);

bool camera_putpixel (Camera *camera, int pixel, const PixelObject* pixelObject);

void camera_paint_initialise (Camera *camera);
void camera_paint_finalise (Camera *camera);


/* Note, the below are:
 * - declared only in camera.h
 * - defined only in camera.c
 * - used only in drawing_window.c
 */
Camera *findCameraFromDW (DrawingWindow dw);
void drawing_window_close_do (DrawingWindow dw);
void drawing_window_resize_do (DrawingWindow dw);


extern SmaFlt PIXELSIZE;

#define PixelToPointX(pixel,cmr) (((pixel) - (cmr)->XSize/2)*PIXELSIZE)
#define PixelToPointY(pixel,cmr) (((pixel) - (cmr)->YSize/2)*PIXELSIZE)

#define PointToPixelX(point,cmr) (((point)/PIXELSIZE + (cmr)->XSize/2))
#define PointToPixelY(point,cmr) (((point)/PIXELSIZE + (cmr)->YSize/2))

#define PixelInCamera(cmr,x,y) ((x)>=0 && (x) < (cmr)->XSize && (y)>=0 && (y) < (cmr)->YSize)


#endif

