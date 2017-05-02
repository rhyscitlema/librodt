/*
    getimage.c
*/

#include <_string.h>
#include <_stdio.h>
#include <expression.h>

#include <avl.h>
#include <rwif.h>
#include <malloc.h> // used by free()



typedef struct _Image
{   ImageData data;
    const wchar* fileName;
} Image;

static AVLT _images = {0};
static AVLT *images = &_images;

static int node_compare (const void* a, const void* b, const void* arg)
{ return strcmp22( ((const Image*)a)->fileName, ((const Image*)b)->fileName ); }



/* on success: return Image* of loaded image file.
 * on failure: update errormessage and return NULL.
 */
static Image* load_image_file (const wchar* fileName, wchar* errormessage)
{
    Image img={0}; img.fileName = fileName;
    Image* image = avl_do(AVL_FIND, images, &img, 0, NULL, node_compare);
    if(image) return image;

    const char* filename = CST12(add_path_to_file_name(NULL, fileName));
    if(!read_image_file(filename, &img.data))
    { strcpy21(errormessage, rwif_errormessage); return NULL; }

    memory_alloc("Image");
    image = (Image*)avl_new(NULL, sizeof(Image) + (strlen2(fileName)+1)*sizeof(wchar) );
    image->fileName = (wchar*)(image+1);
    strcpy22((wchar*)(image+1), fileName);
    image->data = img.data;

    avl_do(AVL_PUT, images, image, 0, NULL, node_compare);
    return image;
}



bool get_image_width_height (ExprCallArg eca)
{
    bool b=false;
    const Image* image;
    value* stack = eca.stack;
    Expression* expression = eca.expression;
    Expression* head;

    image = (Image*)expression->call_comp;
    int ind = expression->independent;
    if(image==NULL || ind==0)
    {
        head = expression->headChild;
        eca.expression = head;
        if(!expression_evaluate(eca)) return 0;

        else if(!isString(stack[0]))
            set_message(eca.garg->message, L"Error in \\1 at \\2:\\3 on '\\4':\r\nArgument must evaluate to a file name.", expression->name);

        image = load_image_file (CST23(getString(stack[0])), eca.garg->message);
        if(image) expression->call_comp = (Component*)(size_t)image;
        b=true;
    }
    if(!image) return 0; // if failed to read the image file
    stack[0] = setSepto2(3, 2);
    stack[1] = setSmaInt(image->data.width);
    stack[2] = setSmaInt(image->data.height);
    if(b) INDEPENDENT(expression, stack, head->independent);
    return 1;
}



bool get_image_pixel_colour (ExprCallArg eca)
{
    long x, y, w, h, B;
    const unsigned char* colour;
    const Image* image;
    value* stack = eca.stack;

    Expression* expression = eca.expression;
    Expression* head = expression->headChild;
    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;

    image = (Image*)expression->call_comp;
    int ind = (head && head->headChild) ? head->headChild->independent : 0;
    if(image==NULL || ind==0)
    {
        wchar* message = eca.garg->message;

        if(!check_first_level(stack, 3, message, expression->name)) return 0;

        else if(!isString(stack[1]))
            set_message(message, L"Error in \\1 at \\2:\\3 on '\\4':\r\nFirst argument must evaluate to a file name.", expression->name);

        image = load_image_file (CST23(getString(stack[1])), message);
        if(image) expression->call_comp = (Component*)(size_t)image;
    }
    if(!image) return 0; // if failed to read the image file

    colour = image->data.pixelArray;
    h      = image->data.height;
    w      = image->data.width;
    B      = image->data.bpp/8;

    x = getSmaInt(_floor(stack[2])); x %= w; if(x<0) x += w;
    y = getSmaInt(_floor(stack[3])); y %= h; if(y<0) y += h;
    colour += (y*w + x)*B;

    stack[0+0] = setSepto2 (5, 4);
    stack[1+0] = setSmaRa2 (colour[0], 255);
    stack[1+1] = setSmaRa2 (colour[1], 255);
    stack[1+2] = setSmaRa2 (colour[2], 255);
    stack[1+3] = setSmaRa2 ((B==4 ? colour[3] : 255), 255);
    return 1;
}



bool get_image_pixel_matrix (ExprCallArg eca)
{
    set_message(eca.garg->message, L"Error in \\1 at \\2:\\3:\r\nOn '\\4': operation not yet available!", eca.expression->name); return 0;
}

