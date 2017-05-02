#ifndef _TEXTOBJ_H
#define _TEXTOBJ_H
/*
    textobj.h
*/

#include "object.h"

typedef struct _TextObj
{
    Object obj; // this must be declared first
    Component *start_line;
    Component *content;
} TextObj;

extern List* textobj_list;

bool textobj_set (Container* container);

#endif

