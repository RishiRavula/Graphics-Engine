/**
 *  Copyright 2018 Mike Reed
 */

#include "image.h"

#include "../include/GFinal.h"
#include "../include/GCanvas.h"
#include "../include/GBitmap.h"
#include "../include/GColor.h"
#include "../include/GMatrix.h"
#include "../include/GPath.h"
#include "../include/GPoint.h"
#include "../include/GRandom.h"
#include "../include/GRect.h"
#include <string>

static GPoint center(const GRect& r) {
    return { (r.fLeft + r.fRight)*0.5f, (r.fTop + r.fBottom)*0.5f };
}

static void draw_rect_radial(GFinal* fin, GCanvas* canvas, const GRect& r, float radius,
                             const GColor colors[], int count, GShader::TileMode mode) {
    auto sh = fin->createRadialGradient(center(r), radius, colors, count, mode);
    if (sh) {
        canvas->drawRect(r, GPaint(sh.get()));
    }
}

static void final_radial(GCanvas* canvas) {
    auto fin = GCreateFinal();

    const GColor c0[] = { {1,0,0,1}, {0,1,0,1}, {0,0,1,1} };
    const GColor c1[] = { {0,0,0,1}, {1,1,1,1} };

    struct {
        GPoint              offset;
        float               radius;
        float               scale;
        GShader::TileMode   mode;
        const GColor*       colors;
        int                 count;
    } recs[] = {
        { {-256, -256},  90, 4, GShader::kClamp,  c1, GARRAY_COUNT(c1) },
        { {   0,    0}, 120, 1, GShader::kClamp,  c0, GARRAY_COUNT(c0) },
        { { 256,    0},  60, 1, GShader::kRepeat, c0, GARRAY_COUNT(c0) },
        { {   0,  256},  60, 1, GShader::kMirror, c0, GARRAY_COUNT(c0) },
    };
    for (auto& r : recs) {
        canvas->save();
        canvas->translate(r.offset.fX, r.offset.fY);
        canvas->scale(r.scale, r.scale);
        draw_rect_radial(fin.get(), canvas, GRect::LTRB(0, 0, 256, 256), r.radius,
                         r.colors, r.count, r.mode);
        canvas->restore();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

static void draw_stroke(GFinal* fin, GCanvas* canvas, GPoint p0, GPoint p1, float width,
                        GFinal::CapType cap, const GPaint& paint) {
    GPath path;
    fin->addLine(&path, p0, p1, width, cap);
    canvas->drawPath(path, paint);
}

static void pinwheel(GCanvas* canvas, GFinal::CapType cap) {
    auto fin = GCreateFinal();

    const GColor colors[] = {
        {1, 0, 0, 1},
        {1, 1, 0, 1},
        {0, 1, 0, 1},
        {0, 1, 1, 1},
        {0, 0, 1, 1},
        {1, 0, 1, 1},
        {0, 0, 0, 1},
    };
    constexpr int N = GARRAY_COUNT(colors);

    const GPoint pts[] = { {-90, 0}, {90, 0} };

    float width = 60;
    for (int i = 0; i < N; ++i) {
        float angle = float(i * 2 * M_PI / N);
        canvas->save();
        canvas->rotate(angle);
        draw_stroke(fin.get(), canvas, pts[0], pts[1], width, cap, GPaint(colors[i]));
        canvas->restore();
        width -= 60.0f / (N+1);
    }
}

static void final_stroke(GCanvas* canvas) {
    constexpr float delta = 140;

    const struct {
        GPoint offset;
        GFinal::CapType cap;
    } recs[] = {
        { {delta, delta}, GFinal::CapType::kButt },
        { {512-delta, 512/2}, GFinal::CapType::kSquare },
        { {delta, 512-delta}, GFinal::CapType::kRound },
    };
    for (auto& r : recs) {
        canvas->save();
        canvas->translate(r.offset.fX, r.offset.fY);
        pinwheel(canvas, r.cap);
        canvas->restore();
    }
}

//////////////////////////////

static void final_bilerp(GCanvas* canvas) {
    GRandom rand;
    GPixel pixels[16];
    
    for (auto& p : pixels) {
        p = rand.nextU() | 0xFF000000;
    }
    
    GBitmap bm(4, 4, 4*sizeof(GPixel), pixels, true);
    auto lm = GMatrix();

    GRect r = GRect::LTRB(0, 0, 4, 4);

    canvas->save();
    canvas->translate(90, 90);
    canvas->scale(100, 100);
    auto sh = GCreateFinal()->createBilerpShader(bm, lm);
    if (!sh) {
        return;
    }
    canvas->drawRect(r, GPaint(sh.get()));
    canvas->restore();

    // thumbnail
    canvas->save();
    canvas->translate(16, 16);
    canvas->scale(16, 16);
    sh = GCreateBitmapShader(bm, lm);
    canvas->drawRect(r, GPaint(sh.get()));
    canvas->restore();
}

////////

static void final_coons(GCanvas* canvas) {
    GBitmap bm;
    bm.readFromFile("apps/spock.png");
    assert(bm.width());
    assert(bm.height());

    GPoint pts[] = {
        {0, 0}, {0.25f, 0.5}, {1, 0},
                              {1.25, 0.5f},
                              {0.75, 1},
                {0.5f, 0.75},
        {0, 1.125},
        {0.25, 0.5f},
    };
    auto mx = GMatrix::Translate(30, 30) * GMatrix::Scale(400, 400);
    mx.mapPoints(pts, pts, 8);

    GPoint tex[] = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1},
    };
    GMatrix::Scale(bm.width(), bm.height()).mapPoints(tex, tex, 4);

    auto sh = GCreateBitmapShader(bm, GMatrix());
    GPaint paint(sh.get());
    assert(sh.get());

    const int N = 8;
    GCreateFinal()->drawQuadraticCoons(canvas, pts, tex, N, paint);
}
