#ifndef _SURFACE_H
#define _SURFACE_H
/*
	surface.h
*/

#include "object.h"

typedef struct _Surface
{
	Object obj; // this must be declared first
	Component *function;
	Component *colour;
} Surface;

#ifdef LIBRODT
extern List* surface_list();
#else
#define surface_list() NULL
#endif

bool surface_set (value stack, Container* container);

#endif
