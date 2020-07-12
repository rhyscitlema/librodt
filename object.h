#ifndef _OBJECT3D_H
#define _OBJECT3D_H
/*
    object.h
*/

#include <expression.h>
#include <component.h>
#include <outsider.h>


#define WORLD_LIMIT 1000000000      // maximum distance between any two points

typedef struct _Object
{
    Container *container;
    Component *origin_comp;
    Component *axes_comp;
    Component *boundary_comp;
    Component *variable_comp;

    SmaFlt origin[3];       // Origin position vector (in world coordinates)
    SmaFlt axes[3][3];      // xyz-axes unit vectors (in world coordinates)
    SmaFlt boundary[6];     // Box enclosing the object (in local coordinates)
    SmaFlt variable[1];     // additional variable used by the object

    bool (*destroy) (struct _Object *obj);
    void (*process) (struct _Object *obj, bool update);
    void (*paint)   (struct _Object *obj, void *camera);

    SmaFlt valueOfT;
    bool isClicked;

    struct _Object *prev, *next;
} Object;


int object_find (const void* key1, const void* key2, const void* arg);

void object_process (Object *obj, bool update);

void object_paint (Object *obj, void* camera, bool (*shootPixel) (Object *obj, void* camera, int xp, int yp));


#define setvector(out,x,y,z) { (out)[0]=x; (out)[1]=y; (out)[2]=z; }

#define copyvector(out,in) { (out)[0]=(in)[0]; (out)[1]=(in)[1]; (out)[2]=(in)[2]; }

#define addvectors(out,u,v) { out[0]=u[0]+v[0]; out[1]=u[1]+v[1]; out[2]=u[2]+v[2]; }

#define substractvectors(out,u,v) { out[0]=u[0]-v[0]; out[1]=u[1]-v[1]; out[2]=u[2]-v[2]; }

#define dotproduct(u,v) (u[0]*v[0] + u[1]*v[1] + u[2]*v[2])

#define crossproduct(out, u, v) {       \
    out[0] = u[1]*v[2] - u[2]*v[1];     \
    out[1] = u[2]*v[0] - u[0]*v[2];     \
    out[2] = u[0]*v[1] - u[1]*v[0];     \
}

#define pointOnLine(out, o, t, d) {     \
    out[0] = o[0] + t*d[0];             \
    out[1] = o[1] + t*d[1];             \
    out[2] = o[2] + t*d[2];             \
}


#define CHECK_COMPONENT(comp, component, args, rvst) \
{   assert(comp!=NULL); \
    comp = component_parse(stack, comp); \
    if(VERROR(component_evaluate(stack, container, comp, args))) return false; \
    component = comp; \
}

#define GET_COMPONENT(comp_name, component, args, rvst) \
{   comp = component_find(stack, container, C31(comp_name), 0); \
    CHECK_COMPONENT(comp, component, args, rvst) \
}


#endif
