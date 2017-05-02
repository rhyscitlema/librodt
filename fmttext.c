/*
    textobj.c
*/

#include <_stdio.h>
#include <_texts.h>
#include <userinterface.h>
#include <camera.h>
#include <textobj.h>



static number text_shootPixel (Object3D *obj, void* camera, int x, int y)
{ return 0; }

static void text_paint (Object3D *obj, void *camera)
{ object_paint (obj, camera, text_shootPixel); }

void text_process (Object3D *obj)
{ object_process (obj); }



static bool text_edit (Object3D *obj)
{
    lchar _name[20], *name;
    Container *container;
    Component *comp;
    Text *text;

    text = (Text*)obj;
    container = obj->container;
    name = lchar_array (_name, 20);

    GET_COMPONENT ("object_position", obj->position_comp, 0, VST31)
    GET_COMPONENT ("object_xy_axes" , obj->xy_axes_comp , 0, VST23)
    GET_COMPONENT ("text_boundary"  , obj->boundary_comp, 0, VST61)
    GET_COMPONENT ("text_scale"     , obj->variable_comp, 0, VST11)
    GET_COMPONENT ("text_start_line", text->start_line  , 0, VST11)
    GET_COMPONENT ("text_content"   , text->content     , 0,   0  )

    if(text->content->root2->ID != SET_STRING)
    { strcpy21 (errormessage, "text_content is not a user defined string."); return false; }

    return true;
}



static Structure text_structure;

static bool text_destroy (Object3D *obj)
{
    Text *text = (Text*)obj;
    if(obj==NULL) return false;

    /* remove text from linked-list */
    DOUBLE_LINKED_LIST_NODE_REMOVE(head_object, obj);

    /* remove text resources */
    container_remove (obj->container);

    STRUCT_DEL (Text, text, text_structure)

    return true;
}



Object3D *text_new (const lchar* definition)
{
    Text *text;
    STRUCT_NEW (Text, text, text_structure, "Text")

    text->obj.edit = text_edit;
    text->obj.destroy = text_destroy;
    text->obj.process = text_process;
    text->obj.paint = text_paint;

    // add at head of text linked-list
    DOUBLE_LINKED_LIST_NODE_ADD(head_object, (Object3D*)text);

    if(!object_edit ((Object3D*)text, definition))
    {
        display_error_message (errormessage);
        text_destroy((Object3D*)text);
        return NULL;
    }
    return (Object3D*)text;
}

