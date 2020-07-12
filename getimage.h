#ifndef _GETIMAGE_H
#define _GETIMAGE_H
/*
	getimage.h
*/

#include <structures.h>

extern void unload_images();
extern value get_image_width_height (value v);
extern value get_image_pixel_colour (value v);
extern value get_image_pixel_matrix (value v);

#endif
