/*
 *  Copyright 2020 Mike Reed
 */

#ifndef GFinal_DEFINED
#define GFinal_DEFINED

#include "GCanvas.h"
#include "GPath.h"
#include "GShader.h"

/**
 * Override and implement these methods. You must implement GCreateFinal() to return your subclass.
 *
 * Each method has a default (do nothing) impl, so you only need to override the ones you intended to implement.
 */
class GFinal {
public:
    virtual ~GFinal() {}

    /**
     *  Return a radial-gradient shader.
     *
     *  This is a shader defined by a circle with center and a radius.
     *  The array of colors are evenly distributed between the center (color[0]) out to
     *  the radius (color[count-1]). Beyond the radius, it respects the TileMode.
     */
    virtual std::unique_ptr<GShader> createRadialGradient(GPoint center, float radius,
                                                          const GColor colors[], int count,
                                                          GShader::TileMode mode) {
        return nullptr;
    }

    /**
     * Return a bitmap shader that performs kClamp tiling with a bilinear filter on each sample, respecting the
     * localMatrix.
     *
     * This is in contrast to the existing GCreateBitmapShader, which performs "nearest neightbor" sampling
     * when it fetches a pixel from the src bitmap.
     */
    virtual std::unique_ptr<GShader> createBilerpShader(const GBitmap&,
                                                        const GMatrix& localMatrix) {
        return nullptr;
    }

    enum CapType {
        kButt,      // no cap on the line
        kSquare,    // square cap extending width/2
        kRound,     // round cap with radius = width/2
    };

    /**
     *  Add contour(s) to the specified path that will draw a line from p0 to p1 with the specified
     *  width and CapType. Note that "width" is the distance from one side of the stroke to the
     *  other, ala its thickness.
     */
    virtual void addLine(GPath* path, GPoint p0, GPoint p1, float width, CapType) {}

    /*
     *  Draw the corresponding mesh constructed from a quad with each side defined by a
     *  quadratic bezier, evaluating them to produce "level" interior lines (same convention
     *  as drawQuad().
     *
     *  pts[0]    pts[1]    pts[2]
     *  pts[7]              pts[3]
     *  pts[6]    pts[5]    pts[4]
     *
     *  Evaluate values within the mesh using the Coons Patch formulation:
     *
     *  value(u,v) = TB(u,v) + LR(u,v) - Corners(u,v)
     *
     *  Where
     *      TB is computed by first evaluating the Top and Bottom curves at (u), and then
     *      linearly interpolating those points by (v)
     *      LR is computed by first evaluating the Left and Right curves at (v), and then
     *      linearly interpolating those points by (u)
     *      Corners is computed by our standard "drawQuad" evaluation using the 4 corners 0,2,4,6
     */
    virtual void drawQuadraticCoons(GCanvas*, const GPoint pts[8], const GPoint tex[4],
                                    int level, const GPaint&) {}
};

/**
 *  Implement this to return ain instance of your subclass of GFinal.
 */
std::unique_ptr<GFinal> GCreateFinal();

#endif
