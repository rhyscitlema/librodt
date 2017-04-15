/*
    getimage.c
*/

#include <_string.h>
#include <_stdio.h>
#include <expression.h>

#include <rwif.h>
#include <malloc.h> // used by free()



typedef struct _Image
{   mchar fileName[300];
    ImageData imagedata;
} Image;

#define MAX 100
static Image ImageArray[MAX];



static unsigned int checkID (unsigned int ID, mchar* errormessage)
{
    if(ID==0) return MAX;
    ID--;
    if(ID >= MAX)
    {
        strcpy21(errormessage, "Software Error: ID of image file is invalid.");
        return MAX;
    }
    if(ImageArray[ID].fileName[0]==0)
    {
        strcpy21(errormessage, "Error: image file not loaded.");
        return MAX;
    }
    return ID;
}



/* free all resources used to store image */
/*static*/ void unloadImageFile (unsigned int ID, mchar* errormessage)
{
    ID = checkID(ID, errormessage);
    if(ID>=MAX) return;
    free(ImageArray[ID].imagedata.pixelArray);
    ImageArray[ID].fileName[0]=0;
}



/* on success: return ID of loaded image file. ID > 0.
 * on failure: update errormessage and return 0. */
static unsigned int loadImageFile (const mchar* fileName, mchar* errormessage)
{
    unsigned int i, ID;
    const char* filename;

    if(isEmpty2(fileName,0)) { strcpy21(errormessage, "Software Error: In loadImageFile(): No file name specified."); return 0; }

    i=MAX;
    for(ID=0; ID<MAX; ID++)
    {
        /* look for the first free slot */
        if(i==MAX && ImageArray[ID].fileName[0]==0) i = ID;

        /* check if file was already loaded */
        if(0==strcmp22 (ImageArray[ID].fileName, fileName)) return ID+1;
    }
    if(i==MAX) { sprintf2(errormessage, CST21("Error: Cannot load more than %s files."), TIS2(0,MAX)); return 0; }
    ID = i;

    filename = CST12(add_path_to_file_name (NULL, fileName));

    if(!read_image_file(filename, &ImageArray[ID].imagedata))
    {
        strcpy21(errormessage, rwif_error_message);
        return 0;
    }
    strcpy22( ImageArray[ID].fileName, fileName);
    return ID+1;
}

static bool load_image_file (Expression* expression, const lchar* file_name, mchar* errormessage)
{
    mchar fileName[MAX_PATH_SIZE];
    unsigned int ID = expression->info;
    if(ID!=0) return ID;
    strcpy23(fileName, file_name);
    ID = loadImageFile (fileName, errormessage);
    expression->info = ID;
    return ID;
}



bool get_image_width_height (ExprCallArg eca)
{
    Expression* head;
    bool b=false;
    value* stack = eca.stack;
    Expression* expression = eca.expression;
    unsigned int ID = expression->info;
    int ind = expression->independent;

    if(ID==0 || ind==0) // if parameter is not constant
    {
        head = expression->headChild;
        eca.expression = head;
        if(!expression_evaluate(eca)) return 0;

        else if(!isString(stack[0]))
            set_message (eca.garg->message, CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nArgument must evaluate to a file name."), expression->name);

        ID = load_image_file (expression, getString(stack[0]), eca.garg->message);
        b=true;
    }
    if(ID==0) return 0; // if failed to read the image file
    ID--;
    stack[0] = setSepto2(3, 2);
    stack[1] = setSmaInt(ImageArray[ID].imagedata.width);
    stack[2] = setSmaInt(ImageArray[ID].imagedata.height);
    if(b) INDEPENDENT(expression, stack, head->independent);
    return 1;
}



bool get_image_pixel_colour (ExprCallArg eca)
{
    value* stack = eca.stack;
    long x, y, w, h, B;
    const unsigned char* colour;

    Expression* expression = eca.expression;
    Expression* head = expression->headChild;
    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;

    int ind = (head && head->headChild) ? head->headChild->independent : 0;
    unsigned int ID = expression->info;

    if(ID==0 || ind==0)
    {
        mchar* message = eca.garg->message;

        if(!check_first_level(stack, 3, message, expression->name)) return 0;

        else if(!isString(stack[1]))
            set_message (message, CST21("Error in \\1 at \\2:\\3 on '\\4':\r\nFirst argument must evaluate to a file name."), expression->name);

        ID = load_image_file (expression, getString(stack[1]), message);
    }
    if(ID==0) return 0; // if failed to read the image file
    ID--;

    colour = ImageArray[ID].imagedata.pixelArray;
    h      = ImageArray[ID].imagedata.height;
    w      = ImageArray[ID].imagedata.width;
    B      = ImageArray[ID].imagedata.bpp/8;

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
    set_message (eca.garg->message, CST21("Error in \\1 at \\2:\\3:\r\nOn '\\4': operation not yet available!"), eca.expression->name); return 0;
}

