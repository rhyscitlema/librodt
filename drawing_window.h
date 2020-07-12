#ifndef _DRAWING_WINDOW_H
#define _DRAWING_WINDOW_H
/*
	drawing_window.h

	This is used only by camera.h
*/

#include <_stddef.h>

typedef void* DrawingWindow;

#define drawing_window_equal(dw1, dw2) ((dw1)==(dw2))

extern DrawingWindow drawing_window_new ();

extern void drawing_window_name   (DrawingWindow dw, const wchar* name);

extern void drawing_window_move   (DrawingWindow dw);

extern void drawing_window_draw   (DrawingWindow dw);

extern void drawing_window_remove (DrawingWindow dw);

#endif
