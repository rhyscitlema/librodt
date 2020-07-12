/*
    getimage.c
*/

#include <getimage.h>
#include <rwif.h>
#include <avl.h>
#include <_stdio.h>
#include <_string.h>
#include <expression.h>



typedef struct _Image
{   ImageData data;
    const wchar* fileName;
} Image;

static AVLT images = {0};

static int node_compare (const void* a, const void* b, const void* arg)
{ return strcmp22( ((const Image*)a)->fileName, ((const Image*)b)->fileName ); }



/* on success: return Image* of loaded image file.
 * on failure: update errormessage and return NULL.
 */
static Image* load_image_file (value stack, const wchar* fileName)
{
    Image img={0}; img.fileName = fileName;
    Image* image = avl_do(AVL_FIND, &images, &img, 0, NULL, node_compare);
    if(image) return image;

    const char* filename = C12(add_path_to_file_name(fileName, (Str2)stack));
    if(!read_image_file(filename, &img.data))
    {
        setError(stack, C21(rwif_errormessage));
        return NULL;
    }
    else memory_alloc("Image");

    image = (Image*)avl_new(NULL, sizeof(Image) + (strlen2(fileName)+1)*sizeof(wchar) );
    strcpy22((wchar*)(image+1), fileName); // copy fileName to Image structure
    image->fileName = (wchar*)(image+1);
    image->data = img.data;

    avl_do(AVL_PUT, &images, image, 0, NULL, node_compare);
    return image;
}

void unload_images()
{
    Image* image = (Image*)avl_min(&images);
    for( ; image; image = (Image*)avl_next(image))
    {
        memory_freed("Image");
        clear_image_data(&image->data);
    }
    avl_free(&images);
}



value get_image_width_height (value v)
{
    value s = vPrev(v);
    const_value n = vGet(s);
    if(!(*n >> 28)) return v;
    if(!isStr2(n)) return setError(s, L"Argument must evaluate to a string.");

    const Image* image;
    image = load_image_file(v, getStr2(n));
    if(!image) return vnext(v); // go to after error message

    v = setSmaInt(s, image->data.width); // starts with s,
    v = setSmaInt(v, image->data.height);
    v = tovector(v, 2);
    return v;
}



static long getLong (value v, const_value n)
{ return getSmaInt(vPrev(toInt(setRef(v,n)))); }

value get_image_pixel_colour (value v)
{
    long x, y, w, h, B;
    const unsigned char* colour;

    value s = vPrev(v);
    const_value n = vGet(s);

    if(!(*n >> 28)) return v;
    value e = check_arguments(s,*n,3); if(e) return e;

    n += 2; // skip vector header
    if(!isStr2(n)) setError(s, L"First argument must evaluate to a string.");

    const Image* image;
    image = load_image_file(v, getStr2(n));
    if(!image) return vnext(v); // go to after error message

    colour = image->data.pixelArray;
    h      = image->data.height;
    w      = image->data.width;
    B      = image->data.bpp/8;

    n = vNext(n); x = getLong(v,n); x %= w; if(x<0) x += w;
    n = vNext(n); y = getLong(v,n); y %= h; if(y<0) y += h;
    colour += (y*w + x)*B;

    const SmaFlt f255 = (SmaFlt)255;
    v = setSmaFlt (s, colour[0]/f255); // starts with s,
    v = setSmaFlt (v, colour[1]/f255);
    v = setSmaFlt (v, colour[2]/f255);
    v = setSmaFlt (v, (B==4 ? colour[3] : 255)/f255);
    return tovector(v, 4);
}



value get_image_pixel_matrix (value v)
{
    return setError(vPrev(v), L"Operation not yet available!");
}

