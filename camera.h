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
	SmaFlt point[3];    // the viewed point on the object
	SmaFlt colour[4];   // the colour of the point
	//struct _PixelObject *next;
} PixelObject;


typedef struct _Camera
{
	Object obj;                         // this must be the first declaration

	int XPost, YPost, XSize, YSize;     // Camera as a rectangle on the main screen
	int dXPost, dYPost, dXSize, dYSize; // Resize of the camera rectangle
	int XS, YS, notfirst, memorysize;

	DrawingWindow drawing_window;       // Utility to draw to device screen
	PixelObject* pixelObject;           // Array of objects painted by camera
	uint32_t* pixelColour;              // Array of colours for the camera pixels
} Camera;


#ifdef LIBRODT
extern List* camera_list();
#else
#define camera_list() NULL
#endif

bool camera_set (value stack, Container* container);

void camera_paint_initialise (Camera *camera);
void camera_paint_finalise (Camera *camera);

bool camera_putpixel (PixelObject *pObj, const PixelObject PO);


/* Note, the below are:
*  - declared only in camera.h
*  - defined only in camera.c
*  - used only in drawing_window.c
*/
Camera *findCameraFromDW (DrawingWindow dw);
void drawing_window_close_do (DrawingWindow dw);
void drawing_window_resize_do (DrawingWindow dw);


extern uint32_t PixelsPUL;

#define PixelToPointX(pixel,cmr) (((pixel) - (cmr)->XSize/2.0)/PixelsPUL)
#define PixelToPointY(pixel,cmr) (((pixel) - (cmr)->YSize/2.0)/PixelsPUL)

#define PointToPixelX(point,cmr) (((point)*PixelsPUL + (cmr)->XSize/2.0))
#define PointToPixelY(point,cmr) (((point)*PixelsPUL + (cmr)->YSize/2.0))

#define PixelInCamera(cmr,x,y) ((x)>=0 && (x) < (cmr)->XSize && (y)>=0 && (y) < (cmr)->YSize)


#endif
