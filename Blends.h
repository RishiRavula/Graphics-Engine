#include "include/GPixel.h"
#include "include/GColor.h"
#include "include/GBlendMode.h"
#include "include/GRect.h"
#include "include/GBitmap.h"

static GPixel colorToPixel(GColor color)
{
    // simple scaling of color values to 0-255 range from 0-1 range
    if (color.a == 1)
    {
        return GPixel_PackARGB(255, color.r * 255 + 0.5, color.g * 255 + 0.5, color.b * 255 + 0.5);
    }
    int a = GRoundToInt(color.a * 255);
    int r = GRoundToInt(color.r * color.a * 255);
    int b = GRoundToInt(color.b * color.a * 255);
    int g = GRoundToInt(color.g * color.a * 255);

    // clamp values to 0-255 range
    return GPixel_PackARGB(a, r, g, b);
}
static unsigned div255(unsigned prod)
{
    return (prod + 128) * 257 >> 16;
}
static GPixel clearPorterDuff(GPixel src, GPixel dst)
{
    return 0;
}
static GPixel dstPorterDuff(GPixel src, GPixel dst)
{
    return dst;
}
static GPixel srcPorterDuff(GPixel src, GPixel dst)
{
    return src;
}
static GPixel srcOverPorterDuff(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    int alpha = 255 - srcA;
    int a = srcA + div255(alpha * GPixel_GetA(dst));
    int r = GPixel_GetR(src) + div255(alpha * GPixel_GetR(dst));
    int g = GPixel_GetG(src) + div255(alpha * GPixel_GetG(dst));
    int b = GPixel_GetB(src) + div255(alpha * GPixel_GetB(dst));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstOverPorterDuff(GPixel src, GPixel dst)
{
    int dstA = GPixel_GetA(dst);
    if (dstA == 255)
    {
        return dst;
    }
    if (dstA == 0)
    {
        return src;
    }
    int alpha = 255 - dstA;
    int a = dstA + div255(alpha * GPixel_GetA(src));
    int r = GPixel_GetR(dst) + div255(alpha * GPixel_GetR(src));
    int g = GPixel_GetG(dst) + div255(alpha * GPixel_GetG(src));
    int b = GPixel_GetB(dst) + div255(alpha * GPixel_GetB(src));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel srcInPorterDuff(GPixel src, GPixel dst)
{
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return 0;
    };
    if (dstA == 255)
    {
        return src;
    };
    int a = div255(dstA * GPixel_GetA(src));
    int r = div255(dstA * GPixel_GetR(src));
    int g = div255(dstA * GPixel_GetG(src));
    int b = div255(dstA * GPixel_GetB(src));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstInPorterDuff(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return 0;
    };
    int a = div255(srcA * GPixel_GetA(dst));
    int r = div255(srcA * GPixel_GetR(dst));
    int g = div255(srcA * GPixel_GetG(dst));
    int b = div255(srcA * GPixel_GetB(dst));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel srcOutPorterDuff(GPixel src, GPixel dst)
{
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return src;
    };
    if (dstA == 255)
    {
        return 0;
    };
    int alpha = 255 - dstA;
    int a = div255(alpha * GPixel_GetA(src));
    int r = div255(alpha * GPixel_GetR(src));
    int g = div255(alpha * GPixel_GetG(src));
    int b = div255(alpha * GPixel_GetB(src));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstOutPorterDuff(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return 0;
    };
    int alpha = 255 - srcA;
    int a = div255(alpha * GPixel_GetA(dst));
    int r = div255(alpha * GPixel_GetR(dst));
    int g = div255(alpha * GPixel_GetG(dst));
    int b = div255(alpha * GPixel_GetB(dst));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel srcATopPorterDuff(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    int dstA = GPixel_GetA(dst);
    int alpha = 255 - srcA;
    int a = dstA;
    int r = div255((dstA * GPixel_GetR(src)) + (alpha * GPixel_GetR(dst)));
    int g = div255((dstA * GPixel_GetG(src)) + (alpha * GPixel_GetG(dst)));
    int b = div255((dstA * GPixel_GetB(src)) + (alpha * GPixel_GetB(dst)));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstATopPorterDuff(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    int dstA = GPixel_GetA(dst);
    int alpha = 255 - dstA;
    int a = srcA;
    int r = div255((srcA * GPixel_GetR(dst)) + (alpha * GPixel_GetR(src)));
    int g = div255((srcA * GPixel_GetG(dst)) + (alpha * GPixel_GetG(src)));
    int b = div255((srcA * GPixel_GetB(dst)) + (alpha * GPixel_GetB(src)));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel xorPorterDuff(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    int dstA = GPixel_GetA(dst);
    int alphaDST = 255 - dstA;
    int alphaSRC = 255 - srcA;
    int a = div255((alphaDST * GPixel_GetA(src)) + (alphaSRC * GPixel_GetA(dst)));
    int r = div255((alphaDST * GPixel_GetR(src)) + (alphaSRC * GPixel_GetR(dst)));
    int g = div255((alphaDST * GPixel_GetG(src)) + (alphaSRC * GPixel_GetG(dst)));
    int b = div255((alphaDST * GPixel_GetB(src)) + (alphaSRC * GPixel_GetB(dst)));
    return GPixel_PackARGB(a, r, g, b);
}

typedef GPixel (*PorterDuffFunc)(GPixel, GPixel);
static PorterDuffFunc porterDuffFuncs[] = {
    clearPorterDuff,
    srcPorterDuff,
    dstPorterDuff,
    srcOverPorterDuff,
    dstOverPorterDuff,
    srcInPorterDuff,
    dstInPorterDuff,
    srcOutPorterDuff,
    dstOutPorterDuff,
    srcATopPorterDuff,
    dstATopPorterDuff,
    xorPorterDuff,
};

// OPTIMIZATION AND DRAWING OF THE PORTER DUFF FUNCTIONS

static void clearPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            *canvas.getAddr(x, y) = 0;
        }
    }
}
static void dstPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            GPixel dst = *canvas.getAddr(x, y);
            *canvas.getAddr(x, y) = dst;
        }
    }
}
static void srcPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            *canvas.getAddr(x, y) = src;
        }
    }
}
static void srcOverPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return;
    }
    if (srcA == 255)
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = src;
            }
        }
        return;
    }
    else
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = srcOverPorterDuff(src, *pixel);
            }
        }
    }
}
static void dstOverPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return;
    }
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            GPixel *pixel = canvas.getAddr(x, y);
            *pixel = dstOverPorterDuff(src, *pixel);
        }
    }
}
static void srcInPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = clearPorterDuff(src, *pixel);
            }
        }
        return;
    }

    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            GPixel *pixel = canvas.getAddr(x, y);
            *pixel = srcInPorterDuff(src, *pixel);
        }
    }
}

static void dstInPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = clearPorterDuff(src, *pixel);
            }
        }
        return;
    }
    if (srcA == 255)
    {
        return;
    }
    else
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = dstInPorterDuff(src, *pixel);
            }
        }
    }
}
static void srcOutPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = clearPorterDuff(src, *pixel);
            }
        }
        return;
    }
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            GPixel *pixel = canvas.getAddr(x, y);
            *pixel = srcOutPorterDuff(src, *pixel);
        }
    }
}
static void dstOutPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return;
    }
    if (srcA == 255)
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = clearPorterDuff(src, *pixel);
            }
        }
        return;
    }
    else
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = dstOutPorterDuff(src, *pixel);
            }
        }
    }
}
static void srcATopPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return;
    }
    if (srcA == 255)
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = srcInPorterDuff(src, *pixel);
            }
        }
        return;
    }
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            GPixel *pixel = canvas.getAddr(x, y);
            *pixel = srcATopPorterDuff(src, *pixel);
        }
    }
}
static void dstATopPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    //Optimized
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = clearPorterDuff(src, *pixel);
            }
        }
        return;
    }
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            GPixel *pixel = canvas.getAddr(x, y);
            *pixel = dstATopPorterDuff(src, *pixel);
        }
    }
}
static void xorPorterDuffDraw(GPixel src, GIRect bounds, const GBitmap &canvas)
{
    //Optimized
    int srcA = GPixel_GetA(src);
    if (srcA == 0){
        return;
    }
    if(srcA == 255){
        for (int y = bounds.fTop; y < bounds.fBottom; y++)
        {
            for (int x = bounds.fLeft; x < bounds.fRight; x++)
            {
                GPixel *pixel = canvas.getAddr(x, y);
                *pixel = srcOutPorterDuff(src, *pixel);
            }
        }
        return;
    }
    for (int y = bounds.fTop; y < bounds.fBottom; y++)
    {
        for (int x = bounds.fLeft; x < bounds.fRight; x++)
        {
            GPixel *pixel = canvas.getAddr(x, y);
            *pixel = xorPorterDuff(src, *pixel);
        }
    }
}

typedef void (*PorterDuffDrawProc)(GPixel src, GIRect bounds, const GBitmap &canvas);
static PorterDuffDrawProc porterDuffDrawProcs[] = {
    clearPorterDuffDraw,
    srcPorterDuffDraw,
    dstPorterDuffDraw,
    srcOverPorterDuffDraw,
    dstOverPorterDuffDraw,
    srcInPorterDuffDraw,
    dstInPorterDuffDraw,
    srcOutPorterDuffDraw,
    dstOutPorterDuffDraw,
    srcATopPorterDuffDraw,
    dstATopPorterDuffDraw,
    xorPorterDuffDraw,
};

static GPixel clearPorterDuffSRC(GPixel src, GPixel dst)
{
    return 0;
}
static GPixel dstPorterDuffSRC(GPixel src, GPixel dst)
{
    return dst;
}
static GPixel srcPorterDuffSRC(GPixel src, GPixel dst)
{
    return src;
}
static GPixel srcOverPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return dst;
    }
    if (srcA == 255)
    {
        return src;
    }
    int alpha = 255 - srcA;
    int a = srcA + div255(alpha * GPixel_GetA(dst));
    int r = GPixel_GetR(src) + div255(alpha * GPixel_GetR(dst));
    int g = GPixel_GetG(src) + div255(alpha * GPixel_GetG(dst));
    int b = GPixel_GetB(src) + div255(alpha * GPixel_GetB(dst));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstOverPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return dst;
    }
    int dstA = GPixel_GetA(dst);
    if (dstA == 255)
    {
        return dst;
    }
    if (dstA == 0)
    {
        return src;
    }
    int alpha = 255 - dstA;
    int a = dstA + div255(alpha * GPixel_GetA(src));
    int r = GPixel_GetR(dst) + div255(alpha * GPixel_GetR(src));
    int g = GPixel_GetG(dst) + div255(alpha * GPixel_GetG(src));
    int b = GPixel_GetB(dst) + div255(alpha * GPixel_GetB(src));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel srcInPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return 0;
    };
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return 0;
    };
    if (dstA == 255)
    {
        return src;
    };
    int a = div255(dstA * GPixel_GetA(src));
    int r = div255(dstA * GPixel_GetR(src));
    int g = div255(dstA * GPixel_GetG(src));
    int b = div255(dstA * GPixel_GetB(src));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstInPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return 0;
    };
    if (srcA == 255)
    {
        return dst;
    };
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return 0;
    };
    int a = div255(srcA * GPixel_GetA(dst));
    int r = div255(srcA * GPixel_GetR(dst));
    int g = div255(srcA * GPixel_GetG(dst));
    int b = div255(srcA * GPixel_GetB(dst));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel srcOutPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return 0;
    };
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return src;
    };
    if (dstA == 255)
    {
        return 0;
    };
    int alpha = 255 - dstA;
    int a = div255(alpha * GPixel_GetA(src));
    int r = div255(alpha * GPixel_GetR(src));
    int g = div255(alpha * GPixel_GetG(src));
    int b = div255(alpha * GPixel_GetB(src));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstOutPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return dst;
    };
    int dstA = GPixel_GetA(dst);
    if (dstA == 0)
    {
        return 0;
    };
    int alpha = 255 - srcA;
    int a = div255(alpha * GPixel_GetA(dst));
    int r = div255(alpha * GPixel_GetR(dst));
    int g = div255(alpha * GPixel_GetG(dst));
    int b = div255(alpha * GPixel_GetB(dst));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel srcATopPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return dst;
    };
    int dstA = GPixel_GetA(dst);
    int alpha = 255 - srcA;
    int a = dstA;
    int r = div255((dstA * GPixel_GetR(src)) + (alpha * GPixel_GetR(dst)));
    int g = div255((dstA * GPixel_GetG(src)) + (alpha * GPixel_GetG(dst)));
    int b = div255((dstA * GPixel_GetB(src)) + (alpha * GPixel_GetB(dst)));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel dstATopPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return 0;
    }
    int dstA = GPixel_GetA(dst);
    int alpha = 255 - dstA;
    int a = srcA;
    int r = div255((srcA * GPixel_GetR(dst)) + (alpha * GPixel_GetR(src)));
    int g = div255((srcA * GPixel_GetG(dst)) + (alpha * GPixel_GetG(src)));
    int b = div255((srcA * GPixel_GetB(dst)) + (alpha * GPixel_GetB(src)));
    return GPixel_PackARGB(a, r, g, b);
}
static GPixel xorPorterDuffSRC(GPixel src, GPixel dst)
{
    int srcA = GPixel_GetA(src);
    if (srcA == 0)
    {
        return dst;
    };
    int dstA = GPixel_GetA(dst);
    int alphaDST = 255 - dstA;
    int alphaSRC = 255 - srcA;
    int a = div255((alphaDST * GPixel_GetA(src)) + (alphaSRC * GPixel_GetA(dst)));
    int r = div255((alphaDST * GPixel_GetR(src)) + (alphaSRC * GPixel_GetR(dst)));
    int g = div255((alphaDST * GPixel_GetG(src)) + (alphaSRC * GPixel_GetG(dst)));
    int b = div255((alphaDST * GPixel_GetB(src)) + (alphaSRC * GPixel_GetB(dst)));
    return GPixel_PackARGB(a, r, g, b);
}

typedef GPixel (*PorterDuffShaderFunc)(GPixel, GPixel);
static PorterDuffShaderFunc porterDuffShaderFunc[] = {
    clearPorterDuffSRC,
    srcPorterDuffSRC,
    dstPorterDuffSRC,
    srcOverPorterDuffSRC,
    dstOverPorterDuffSRC,
    srcInPorterDuffSRC,
    dstInPorterDuffSRC,
    srcOutPorterDuffSRC,
    dstOutPorterDuffSRC,
    srcATopPorterDuffSRC,
    dstATopPorterDuffSRC,
    xorPorterDuffSRC,
};