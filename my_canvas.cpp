/*
 *  Copyright 2023 <me>
 */
#include <iostream>
#include <stack>
#include <cmath>
#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GBitmap.h"
#include "include/GPixel.h"
#include "Blends.h"
#include "BitmapShader.h"
#include "My_GPath.h"

class MyCanvas : public GCanvas {
public:
    MyCanvas(const GBitmap& device) : fDevice(device) {
        GMatrix i = GMatrix();
        ctmStack.push(i);
    }

    struct Edge {
        float m;
        float currX;
        int top;
        int bottom;
        int winding;

        bool lastY(int y) { 
            assert (y >= top && y < bottom);
            return y == bottom - 1; 
        }
        bool activeY(int y) { 
            assert (y < bottom);
            return (y >= top && y < bottom);
        }
    };

    struct Blitter{
        GBitmap fDevice;
        GPixel src;
        PorterDuffFunc blendMode;
        PorterDuffDrawProc drawProc;
        PorterDuffShaderFunc shaderFunc;
        void blit(int x, int y, int N){
            GIRect rect = {x, y, x+N, y+1};
            drawProc(src, rect, fDevice);
        }
        void shaderBlit(int x, int y, GPixel src){
            GPixel* pixel = fDevice.getAddr(x, y);
            *pixel = shaderFunc(src, *pixel);
        }
        void drawPixel(int x, int y, GPixel src){
            GPixel* pixel = fDevice.getAddr(x, y);
            *pixel = blendMode(src, *pixel);
        }
    };

    
    struct compareEdges{
        inline bool operator() (const Edge& e1, const Edge& e2){
            if(e1.top != e2.top){
                return e1.top < e2.top;
            }
            if(e1.currX != e2.currX){
                return e1.currX < e2.currX;
            }
            return e1.m < e2.m;
        }
    };

    struct compareEdges2{
        inline bool operator() (const Edge& e1, const Edge& e2){
            if(e1.currX != e2.currX){
                return e1.currX < e2.currX;
            }
            return e1.m < e2.m;
        }
    };
    /**
     *  Fill the entire canvas with the specified color, using the specified blendmode.
     */
    void drawPaint(const GPaint& color) override {
        if(color.getShader() != nullptr){
            drawRect(GRect::WH(fDevice.width(), fDevice.height()), color);
            return;
        }

        GBlendMode mode = color.getBlendMode();
        GColor c = color.getColor();
        GPixel src = colorToPixel(c);
        PorterDuffFunc blendMode = porterDuffFuncs[static_cast<int>(mode)];
        for(int y = 0; y < fDevice.height(); y++) {
            for (int x = 0; x < fDevice.width(); x++) {
                GPixel* pixel = fDevice.getAddr(x, y);
                *pixel = blendMode(src, *pixel);
            }
        }
    }
    /**
     *  Fill the rectangle with the color, using the specified blendmode.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     */
    void drawRect(const GRect& rect, const GPaint& color) override {
        if(ctmStack.top() == GMatrix() && color.getShader() == nullptr){
            GIRect adjustRect = clipRect(rect);
            GBlendMode mode = color.getBlendMode();
            GColor c = color.getColor();
            GPixel src = colorToPixel(c);
            PorterDuffDrawProc blendMode = porterDuffDrawProcs[static_cast<int>(mode)];
            blendMode(src, adjustRect, fDevice);
            return;
        }

        GPoint points[4] = {{rect.x(),rect.y()}, {rect.x() + rect.width(),rect.y()}, {rect.x() + rect.width(), rect.y() + rect.height()},{rect.x(), rect.y() + rect.height()}};
        GPoint matrixPts[4];
        GMatrix top = ctmStack.top();
        top.mapPoints(matrixPts, points, 4);
        
        //check to see if the top and bottom slopes are the same, same for left and right, if so, draw a rectangle
        if(matrixPts[0].fY == matrixPts[1].fY && matrixPts[2].fY == matrixPts[3].fY && matrixPts[0].fX == matrixPts[3].fX && matrixPts[1].fX == matrixPts[2].fX && color.getShader() == nullptr){
            //draw a rectangle
            GIRect adjustRect = clipRect(GRect::LTRB(matrixPts[0].fX, matrixPts[0].fY, matrixPts[2].fX, matrixPts[2].fY));
            GBlendMode mode = color.getBlendMode();
            GColor c = color.getColor();
            GPixel src = colorToPixel(c);
            PorterDuffDrawProc blendMode = porterDuffDrawProcs[static_cast<int>(mode)];
            blendMode(src, adjustRect, fDevice);
            return;
        }

        //else default to drawConvexPolygon
        drawConvexPolygon(points, 4, color);

    }  
    GIRect clipRect(const GRect& rect) {
        GIRect adjustRect = rect.round();
        adjustRect.fLeft > 0 ? adjustRect.fLeft : adjustRect.fLeft = 0;
        adjustRect.fTop > 0 ? adjustRect.fTop : adjustRect.fTop = 0;
        adjustRect.fRight < fDevice.width() ? adjustRect.fRight : adjustRect.fRight = fDevice.width();
        adjustRect.fBottom < fDevice.height() ? adjustRect.fBottom : adjustRect.fBottom = fDevice.height();
        return adjustRect;
    }

    /**
     *  Fill the convex polygon with the color and blendmode,
     *  following the same "containment" rule as rectangles.
     */
    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) override {
        if (count < 3 ) {
            return;
        }
        
        //DO MATRIX LOGIC HERE
        GPoint matrixPts[count];
        GMatrix top = ctmStack.top();
        top.mapPoints(matrixPts, points, count);

        //cont as normal
        GIRect bounds = GRect::WH(fDevice.width(), fDevice.height()).round();
        std::vector <Edge> edges;
        edges.reserve(count);
        
        for(int i = 0; i < count-1; i++){
            GPoint p0 = matrixPts[i];
            GPoint p1 = matrixPts[i+1];
            clip(p0, p1, bounds, edges);
        }

        //close the polygon
        GPoint p0 = matrixPts[count-1];
        GPoint p1 = matrixPts[0];
        clip(p0, p1, bounds, edges);
        if(edges.size() < 2){
            return;
        }
        std::sort(edges.begin(), edges.end(), compareEdges());
        GBlendMode blendMode = paint.getBlendMode();
        //begin drawing the pixels that lie between the edges, scanline algorithm

        //CHECK FOR SHADERS HERE
        if (paint.getShader() != nullptr) {
            //draw with shader
            GShader* shader = paint.getShader();
            if(!shader->setContext(top)) {
                return;
            }
            auto row = std::vector<GPixel>(fDevice.width());
            Blitter blitz = {fDevice, 0, porterDuffFuncs[static_cast<int>(blendMode)], porterDuffDrawProcs[static_cast<int>(blendMode)], porterDuffShaderFunc[static_cast<int>(blendMode)]};
           
            if(shader->isOpaque()){
                //TODO: Implement if is opaque
                GBlendMode newBlendMode = changeBlendMode(blendMode);
                blitz.blendMode = porterDuffFuncs[static_cast<int>(newBlendMode)];
                blitz.drawProc = porterDuffDrawProcs[static_cast<int>(newBlendMode)];
                blitz.shaderFunc = porterDuffShaderFunc[static_cast<int>(newBlendMode)];
            }
            //blendmode is never changing, if its opaque, maybe we can only do a single check.
            for(int y = edges[0].top; y < edges.back().bottom; y++){
                if(edges.size() == 0){
                    break;
                }
                //find the first edge that has a top that is less than or equal to y
                Edge e0 = edges[0];
                Edge e1 = edges[1];
                float x0 = e0.currX;
                float x1 = e1.currX;
                int left = GRoundToInt(x0);
                int right = GRoundToInt(x1);
                if (left > right){
                    std::swap(left, right);
                }
                //blit the pixels
                //find the row of pixels to blit
                shader->shadeRow(left, y, right-left, row.data());
                //for each pixel in the row, check the blend mode and print it
            
                for(int i = 0; i < right-left; i++){
                    blitz.src = row[i];
                    if(blitz.src == *fDevice.getAddr(left+i, y)){
                        continue;
                    }
                    blitz.shaderBlit(left+i, y, blitz.src);
                }
                //update the currX
                edges[0].currX += edges[0].m;
                edges[1].currX += edges[1].m;
                if (e1.lastY(y)){
                    edges.erase(edges.begin()+1);
                }
                if (e0.lastY(y)){
                    edges.erase(edges.begin());
                }
            }
            
        } else {
            Blitter blitz = {fDevice, colorToPixel(paint.getColor()), porterDuffFuncs[static_cast<int>(paint.getBlendMode())], porterDuffDrawProcs[static_cast<int>(paint.getBlendMode())]};
            for(int y = edges[0].top; y < edges.back().bottom; y++){
                if(edges.size() == 0){
                    break;
                }
                //find the first edge that has a top that is less than or equal to y
                Edge e0 = edges[0];
                Edge e1 = edges[1];
                float x0 = e0.currX;
                float x1 = e1.currX;
                int left = GRoundToInt(x0);
                int right = GRoundToInt(x1);
                if (left > right){
                    std::swap(left, right);
                }
                //blit the pixels
                blitz.blit(left, y, right-left);
                //update the currX
                edges[0].currX += edges[0].m;
                edges[1].currX += edges[1].m;
                if (e1.lastY(y)){
                    //maybe try storing edges in reverse
                    edges.erase(edges.begin()+1);
                }
                if (e0.lastY(y)){
                    edges.erase(edges.begin());
                }
            }
        }
    } 


    GBlendMode changeBlendMode (GBlendMode mode) {
        //Given that the src alpha = 1, we can maybe simplify the blend mode logic a bit
        //if the blend mode is opaque, we can just do a single check
        switch (mode){
            case GBlendMode::kSrcOver:
                return GBlendMode::kSrc;
            case GBlendMode::kDstIn:
                return GBlendMode::kDst;
            case GBlendMode::kDstOut:
                return GBlendMode::kClear;
            case GBlendMode::kSrcATop:
                return GBlendMode::kSrcIn;
            case GBlendMode::kDstATop:
                return GBlendMode::kDstOver;
            case GBlendMode::kXor:
                return GBlendMode::kSrcOut;
            default:
                return mode;
        }
        //afaik, these are the only alpha = 1 optimizations we can do
    }

   

    /**
     *  Clipping for a single edge
     */
    void clip(GPoint p0, GPoint p1, GIRect canvas, std::vector<Edge>& edges){
        int winding = -1;
        if(p0.fY == p1.fY){ 
            //edge is horizontal, no need to draw anything
            return;
        }
        if(p0.fY > p1.fY){
            //swap the points so that p0 is always above p1
            std::swap(p0, p1);
            winding = 1;
        }

        if(p1.fY < canvas.fTop || p0.fY > canvas.fBottom){ 
            //edge is completely outside the canvas, no need to draw anything
            return;
        }

        if(p0.fY < canvas.fTop){ 
            //clip the top of the edge, find the new p0
            float m = (canvas.fTop - p0.fY) / (p1.fY - p0.fY);
            float delX = (p1.fX - p0.fX) * m;
            p0.fX = p0.fX + delX; 
            p0.fY = canvas.fTop;
        }

        if(p1.fY > canvas.fBottom){ 
            //clip the bottom of the edge, find the new p1
            float m = (p1.fY - canvas.fBottom) / (p1.fY - p0.fY);
            float delX = (p0.fX - p1.fX) * m;
            p1.fX = p1.fX + delX;
            p1.fY = canvas.fBottom;
        }

        if(p0.fX > p1.fX){ 
            //swap the points so that p0 is always to the left of p1
            std::swap(p0, p1);
        }
        if(p1.fX < canvas.fLeft){ 
            //edge is outside canvas, DONT RETURN, project the edge to the left side of the canvas
            p0.fX = canvas.fLeft;
            p1.fX = canvas.fLeft;
            make_edge(p0, p1, edges, winding);
            return;
        }
        if(p0.fX > canvas.fRight){ 
            //edge is outside canvas, DONT RETURN, project the edge to the right side of the canvas
            p0.fX = canvas.fRight;
            p1.fX = canvas.fRight;
            make_edge(p0, p1, edges, winding);
            return;
        }
        if(p0.fX < canvas.fLeft){ 
            //only 1 point is outside the canvas, clip the left side of the edge, find the new p0
            GPoint temp = {canvas.fLeft, p0.fY};
            float m = (p0.fX - canvas.fLeft) / (p0.fX - p1.fX);
            float delY = (p1.fY - p0.fY) * m;
            p0.fY = p0.fY + delY;
            p0.fX = canvas.fLeft; 
            make_edge(temp, p0, edges, winding);
        }

        if(p1.fX > canvas.fRight){ 
            //only 1 point is outside the canvas, clip the right side of the edge, find the new p1
            GPoint temp = {canvas.fRight, p1.fY};
            float m = (p1.fX - canvas.fRight) / (p1.fX - p0.fX);
            float delY = (p0.fY - p1.fY) * m;
            p1.fY = p1.fY + delY;
            p1.fX = canvas.fRight;
            make_edge(temp, p1, edges ,winding);
        }
        make_edge(p0, p1, edges, winding);
    }

    void make_edge(GPoint p0, GPoint p1, std::vector<Edge>& edges, int winding){
        if(p0.fY > p1.fY){
            std::swap(p0, p1);
        }
         //inverse slope
        int top = round(p0.fY);
        int bottom = round(p1.fY);
        if (top == bottom){
            return;
        }
        // float m = (p1.fX - p0.fX) / (p1.fY - p0.fY);
        // float currX = p0.fX + (m/2);
        float m = (p1.fX - p0.fX) / (p1.fY - p0.fY);
        float currX = p0.fX + m * (top - p0.fY + 0.5f);
        edges.push_back(Edge{m,currX,top,bottom,winding});
    }


    void save() override{
        //push a copy of the top matrix onto the stack
        ctmStack.push(ctmStack.top());
    }
    void restore() override{
        //pop the top matrix off the stack, if the stack is empty, do nothing
        if (!ctmStack.empty()){
            ctmStack.pop();
        }
    }
    void concat(const GMatrix& matrix) override{
        ctmStack.top() = ctmStack.top() * matrix;
    }

    // int approxCurve(GPoint points[])

    void drawPath(const GPath& path, const GPaint& paint) override {
        int pathCount = path.countPoints();
        if (pathCount < 3){
            return;
        }
        
        GMatrix top = ctmStack.top();
        GPath pathCopy = path;
        pathCopy.transform(top);

        //cont as normal
        GIRect bounds = GRect::WH(fDevice.width(), fDevice.height()).round();
        std::vector <Edge> edges;
        edges.reserve(3*pathCount);

        GPoint pts[4];
        GPath::Edger edger(pathCopy);
        GPath::Verb v;
        while ((v = edger.next(pts)) != GPath::kDone) {
            if (v == GPath::kMove || v == GPath::kLine) {
                clip(pts[0], pts[1], bounds, edges); 
            }
            else if (v == GPath::kQuad) {
                //HANDLE HERE
                GPoint A = pts[0];
                GPoint B = pts[1];
                GPoint C = pts[2];
                GPoint E = (A - (2 * B) + C) * 0.25;
                float magnitudeE = std::sqrt(E.fX * E.fX + E.fY * E.fY);
                int num_segs = (int)std::ceil(std::sqrt(magnitudeE * 4));
                //Make Points
                GPoint* newPoints = new GPoint[num_segs + 1];
                newPoints[0] = A;
                float t = 0 ;
                float dt = 1.0 / num_segs;
                for (int i = 1; i < num_segs; i++){
                    t += dt;
                    newPoints[i] = (1 - t) * (1 - t) * A + 2 * (1 - t) * t * B + t * t * C;
                }
                newPoints[num_segs] = C;
                //Clip Points
                for (int i = 0; i < num_segs; i++){
                    clip(newPoints[i], newPoints[i + 1], bounds, edges);
                }
            }
            else if (v == GPath::kCubic){
                //HANDLE HERE
                GPoint A = pts[0];
                GPoint B = pts[1];
                GPoint C = pts[2];
                GPoint D = pts[3];

                GPoint E0 = A + 2*B + C;
                GPoint E1 = B + 2*C + D;
                GPoint E = {std::max(abs(E0.fX), abs(E1.fX)), std::max(abs(E0.fY), abs(E1.fY))};
                float magnitudeE = std::sqrt(E.fX * E.fX + E.fY * E.fY);

                int num_segs = (int)std::ceil(std::sqrt(magnitudeE * 3));
                //Make Points
                GPoint* newPoints = new GPoint[num_segs + 1];
                newPoints[0] = A;
                float t = 0 ;
                float dt = 1.0 / num_segs;
                for (int i = 1; i < num_segs; i++){
                    t += dt;
                    newPoints[i] = (1 - t) * (1 - t) * (1 - t) * A + 3 * (1 - t) * (1 - t) * t * B + 3 * (1 - t) * t * t * C + t * t * t * D;
                }
                newPoints[num_segs] = D;
                //Clip Points
                for (int i = 0; i < num_segs; i++){
                    clip(newPoints[i], newPoints[i + 1], bounds, edges);
                }
            }
            else {
                printf("ERROR");
            }
    
                    
        }

        if(edges.size() < 2){
            return;
        }
        std::sort(edges.begin(), edges.end(), compareEdges());
        GBlendMode blendMode = paint.getBlendMode();
        //begin drawing the pixels that lie between the edges, scanline algorithm

        //CHECK FOR SHADERS HERE
        if (paint.getShader() != nullptr) {

            //draw with shader
            GShader* shader = paint.getShader();
            if(!shader->setContext(top)) {
                return;
            }
            auto row = std::vector<GPixel>(fDevice.width());
            Blitter blitz = {fDevice, 0, porterDuffFuncs[static_cast<int>(blendMode)], porterDuffDrawProcs[static_cast<int>(blendMode)], porterDuffShaderFunc[static_cast<int>(blendMode)]};
           
            if(shader->isOpaque()){
                GBlendMode newBlendMode = changeBlendMode(blendMode);
                blitz.blendMode = porterDuffFuncs[static_cast<int>(newBlendMode)];
                blitz.drawProc = porterDuffDrawProcs[static_cast<int>(newBlendMode)];
                blitz.shaderFunc = porterDuffShaderFunc[static_cast<int>(newBlendMode)];
            }
            //blendmode is never changing, if its opaque, maybe we can only do a single check.
            int y = edges[0].top;
            while (y < bounds.bottom()){
                int blitOrNah = 0;
                int idx = 0;
                int left = 0;
                int right = 0;
                while (idx < edges.size() && edges[idx].activeY(y)){
                    if(edges.size() == 0){
                        return;
                    }

                    float currX = edges[idx].currX;
                    if(blitOrNah == 0){
                        left = GRoundToInt(currX);
                    }
                    blitOrNah += edges[idx].winding;
                    if(blitOrNah == 0){
                        right = GRoundToInt(currX);
                        shader->shadeRow(left, y, right-left, row.data());
                        for(int i = 0; i < right-left; i++){
                            blitz.src = row[i];
                            if(blitz.src == *fDevice.getAddr(left+i, y)){
                                continue;
                            }
                            blitz.shaderBlit(left+i, y, blitz.src);
                        }
                    }
                    edges[idx].currX += edges[idx].m;
                    if (edges[idx].lastY(y)){
                        edges.erase(edges.begin()+idx);
                    } else {
                        idx++;
                    }
                    if(idx == edges.size()){
                        break;
                    }

                }
                //Pick up the next active edge
                assert(blitOrNah == 0);
                y++;
                while(idx < edges.size() && edges[idx].activeY(y)){
                    idx++;
                }
                std::sort(edges.begin(), edges.begin()+idx, compareEdges2());
            }
            return;
        } else {
            Blitter blitz = {fDevice, colorToPixel(paint.getColor()), porterDuffFuncs[static_cast<int>(paint.getBlendMode())], porterDuffDrawProcs[static_cast<int>(paint.getBlendMode())]};
            int y = edges[0].top;
            while (y < bounds.bottom()){
                int blitOrNah = 0;
                int idx = 0;
                int left = 0;
                int right = 0;
                while (idx < edges.size() && edges[idx].activeY(y)){
                    if(edges.size() == 0){
                        return;
                    }

                    float currX = edges[idx].currX;
                    if(blitOrNah == 0){
                        left = GRoundToInt(currX);
                    }
                    blitOrNah += edges[idx].winding;
                    if(blitOrNah == 0){
                        right = GRoundToInt(currX);
                        blitz.blit(left, y, right-left);
                    }
                    edges[idx].currX += edges[idx].m;
                    if (edges[idx].lastY(y)){
                        edges.erase(edges.begin()+idx);
                    } else {
                        idx++;
                    }
                    if(idx == edges.size()){
                        break;
                    }

                }
                //Pick up the next active edge
                assert(blitOrNah == 0);
                y++;
                while(idx < edges.size() && edges[idx].activeY(y)){
                    idx++;
                }
                std::sort(edges.begin(), edges.begin()+idx, compareEdges2());
            }
        }
    }

    void drawTriangleTriColor(const GPoint points[3], int count, const GColor colors[3], const GPaint& paint) {
        if(count != 3){
            return;
        }
        
        //cont as normal
        GIRect bounds = GRect::WH(fDevice.width(), fDevice.height()).round();
        std::vector <Edge> edges;
        edges.reserve(count);
        
        for(int i = 0; i < count-1; i++){
            GPoint p0 = points[i];
            GPoint p1 = points[i+1];
            clip(p0, p1, bounds, edges);
        }

        //close the polygon
        GPoint p0 = points[count-1];
        GPoint p1 = points[0];
        clip(p0, p1, bounds, edges);
        if(edges.size() < 2){
            return;
        }
        std::sort(edges.begin(), edges.end(), compareEdges());
        Blitter blitz = {fDevice, colorToPixel(paint.getColor()), porterDuffFuncs[static_cast<int>(paint.getBlendMode())], porterDuffDrawProcs[static_cast<int>(paint.getBlendMode())]};
        //make color matrix
        GMatrix invMatrixColor = GMatrix(
            points[2].fX - points[0].fX, points[1].fX - points[0].fX, points[0].fX, 
            points[2].fY - points[0].fY, points[1].fY - points[0].fY, points[0].fY
        );
        invMatrixColor.invert(&invMatrixColor);
        for(int y = edges[0].top; y < edges.back().bottom; y++){
            if(edges.size() == 0){
                break;
            }
            //find the first edge that has a top that is less than or equal to y
            Edge e0 = edges[0];
            Edge e1 = edges[1];
            float x0 = e0.currX;
            float x1 = e1.currX;
            int left = GRoundToInt(x0);
            int right = GRoundToInt(x1);
            if (left > right){
                std::swap(left, right);
            }

            //blit the pixels


            //TO OPTIMIZE WITH DERIVS?

            for(int x = left; x < right; x++){
                GColor currColor;
                GPoint currPoint = invMatrixColor * GPoint{x + 0.5, y + 0.5};
                currColor = colors[2] * currPoint.fX + colors[1] * currPoint.fY + colors[0] * (1 - currPoint.fX - currPoint.fY);
                blitz.src = colorToPixel(currColor);
                blitz.drawPixel(x, y, blitz.src);
            }
            //update the currX
            edges[0].currX += edges[0].m;
            edges[1].currX += edges[1].m;
            if (e1.lastY(y)){
                //maybe try storing edges in reverse
                edges.erase(edges.begin()+1);
            }
            if (e0.lastY(y)){
                edges.erase(edges.begin());
            }
        }
        return;
    }


    void drawTriangleTriTexture(const GPoint originalPoints[3], const GPoint points[3], int count, const GPoint texs[], const GPaint& paint) {
        if(count != 3){
            return;
        }
        
        //cont as normal
        GIRect bounds = GRect::WH(fDevice.width(), fDevice.height()).round();
        std::vector <Edge> edges;
        edges.reserve(count);
        
        for(int i = 0; i < count-1; i++){
            GPoint p0 = points[i];
            GPoint p1 = points[i+1];
            clip(p0, p1, bounds, edges);
        }

        //close the polygon
        GPoint p0 = points[count-1];
        GPoint p1 = points[0];
        clip(p0, p1, bounds, edges);
        if(edges.size() < 2){
            return;
        }
        std::sort(edges.begin(), edges.end(), compareEdges());


        Blitter blitz = {fDevice, colorToPixel(paint.getColor()), porterDuffFuncs[static_cast<int>(paint.getBlendMode())], porterDuffDrawProcs[static_cast<int>(paint.getBlendMode())]};
        //make color matrix
        GMatrix invMatrixColor = GMatrix(
            originalPoints[2].fX - originalPoints[0].fX, originalPoints[1].fX - originalPoints[0].fX, originalPoints[0].fX, 
            originalPoints[2].fY - originalPoints[0].fY, originalPoints[1].fY - originalPoints[0].fY, originalPoints[0].fY
        );
        


        //TO OPTIMIZE WITH DERIVS?
        GMatrix texMatrix = GMatrix(
                texs[2].fX - texs[0].fX, texs[1].fX - texs[0].fX, texs[0].fX, 
                texs[2].fY - texs[0].fY, texs[1].fY - texs[0].fY, texs[0].fY
            );
        texMatrix.invert(&texMatrix);
        
        GMatrix superMatrix = invMatrixColor * texMatrix;
        ProxyShader proxyShader(paint.getShader(), superMatrix);
        GPaint p(&proxyShader);
        p.getShader()->setContext(ctmStack.top());
        for(int y = edges[0].top; y < edges.back().bottom; y++){
            if(edges.size() == 0){
                break;
            }
            //find the first edge that has a top that is less than or equal to y
            Edge e0 = edges[0];
            Edge e1 = edges[1];
            float x0 = e0.currX;
            float x1 = e1.currX;
            int left = GRoundToInt(x0);
            int right = GRoundToInt(x1);
            if (left > right){
                std::swap(left, right);
            }
            GPixel* row = new GPixel[right-left];
            p.getShader()->shadeRow(left, y, right-left, row);
            for(int x = left; x < right; x++){
                blitz.drawPixel(x, y, row[x-left]);
            }
            //update the currX
            edges[0].currX += edges[0].m;
            edges[1].currX += edges[1].m;
            if (e1.lastY(y)){
                //maybe try storing edges in reverse
                edges.erase(edges.begin()+1);
            }
            if (e0.lastY(y)){
                edges.erase(edges.begin());
            }
        }
        return;
    }

    void drawTriangleTriBoth(const GPoint originalPoints[3], const GPoint points[3], int count, const GPoint texs[], const GColor colors[], const GPaint& paint) {
        if(count != 3){
            return;
        }
        
        //cont as normal
        GIRect bounds = GRect::WH(fDevice.width(), fDevice.height()).round();
        std::vector <Edge> edges;
        edges.reserve(count);
        
        for(int i = 0; i < count-1; i++){
            GPoint p0 = points[i];
            GPoint p1 = points[i+1];
            clip(p0, p1, bounds, edges);
        }

        //close the polygon
        GPoint p0 = points[count-1];
        GPoint p1 = points[0];
        clip(p0, p1, bounds, edges);
        if(edges.size() < 2){
            return;
        }
        std::sort(edges.begin(), edges.end(), compareEdges());
        Blitter blitz = {fDevice, colorToPixel(paint.getColor()), porterDuffFuncs[static_cast<int>(paint.getBlendMode())], porterDuffDrawProcs[static_cast<int>(paint.getBlendMode())]};


        //SETTING TEXTURE LOGIC
        GMatrix invMatrixColorTextures = GMatrix(
            originalPoints[2].fX - originalPoints[0].fX, originalPoints[1].fX - originalPoints[0].fX, originalPoints[0].fX, 
            originalPoints[2].fY - originalPoints[0].fY, originalPoints[1].fY - originalPoints[0].fY, originalPoints[0].fY
        );
        


        
        GMatrix texMatrix = GMatrix(
                texs[2].fX - texs[0].fX, texs[1].fX - texs[0].fX, texs[0].fX, 
                texs[2].fY - texs[0].fY, texs[1].fY - texs[0].fY, texs[0].fY
            );
        texMatrix.invert(&texMatrix);
        
        GMatrix superMatrix = invMatrixColorTextures * texMatrix;
        ProxyShader proxyShader(paint.getShader(), superMatrix);
        GPaint p(&proxyShader);
        p.getShader()->setContext(ctmStack.top());

        //SETTING COLOR LOGIC
        GMatrix invMatrixColorColors = GMatrix(
            points[2].fX - points[0].fX, points[1].fX - points[0].fX, points[0].fX, 
            points[2].fY - points[0].fY, points[1].fY - points[0].fY, points[0].fY
        );
        invMatrixColorColors.invert(&invMatrixColorColors);

        //DRAW LOGIC
        
        for(int y = edges[0].top; y < edges.back().bottom; y++){
            if(edges.size() == 0){
                break;
            }
            Edge e0 = edges[0];
            Edge e1 = edges[1];
            float x0 = e0.currX;
            float x1 = e1.currX;
            int left = GRoundToInt(x0);
            int right = GRoundToInt(x1);
            if (left > right){
                std::swap(left, right);
            }
            GPixel* row = new GPixel[right-left];
            p.getShader()->shadeRow(left, y, right-left, row);
            for(int x = left; x < right; x++){
                
                GPixel texturePixel = row[x-left];

                //Color math
                GColor currColor;
                GPoint currPoint = invMatrixColorColors * GPoint{x + 0.5, y + 0.5};
                currColor = colors[2] * currPoint.fX + colors[1] * currPoint.fY + colors[0] * (1 - currPoint.fX - currPoint.fY);
                GPixel colorPixel = colorToPixel(currColor);

                int alpha = div255(GPixel_GetA(texturePixel) * GPixel_GetA(colorPixel));
                int red = div255(GPixel_GetR(texturePixel) * GPixel_GetR(colorPixel));
                int green = div255(GPixel_GetG(texturePixel) * GPixel_GetG(colorPixel));
                int blue = div255(GPixel_GetB(texturePixel) * GPixel_GetB(colorPixel));

                GPixel finalPixel = GPixel_PackARGB(alpha, red, green, blue);
                

                
                blitz.drawPixel(x, y, finalPixel);
            }
            //update the currX
            edges[0].currX += edges[0].m;
            edges[1].currX += edges[1].m;
            if (e1.lastY(y)){
                //maybe try storing edges in reverse
                edges.erase(edges.begin()+1);
            }
            if (e0.lastY(y)){
                edges.erase(edges.begin());
            }
        }
        return; 
    }
    



    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[], int count, const int indices[], const GPaint& paint) override {
        //Run the points thru the ctm
        GPoint matrixPts[count*3];
        GMatrix top = ctmStack.top();
        top.mapPoints(matrixPts, verts, count*3);

        if(texs != nullptr && colors != nullptr){           
            for(int i = 0; i < count*3; i+=3){
                GColor newColors [3] = {colors[indices[i]], colors[indices[i+1]], colors[indices[i+2]]};
                GPoint newMatrixPoints[3] = {matrixPts[indices[i]], matrixPts[indices[i+1]], matrixPts[indices[i+2]]};
                GPoint newVerts[3] = {verts[indices[i]], verts[indices[i+1]], verts[indices[i+2]]};
                GPoint newTexs[3] = {texs[indices[i]], texs[indices[i+1]], texs[indices[i+2]]};

                drawTriangleTriBoth(newVerts, newMatrixPoints, 3, newTexs, newColors, paint);
            }

        }
        

        else if(texs != nullptr){
            drawTriangleTriTexture(verts, matrixPts, 3, texs, paint);
            return;
        }
        else if(colors != nullptr){
            for(int i = 0; i < count*3; i+=3){
                GColor newColors [3] = {colors[indices[i]], colors[indices[i+1]], colors[indices[i+2]]};
                GPoint newMatrixPoints[3] = {matrixPts[indices[i]], matrixPts[indices[i+1]], matrixPts[indices[i+2]]};
                drawTriangleTriColor(newMatrixPoints, 3, newColors, paint);
            }
            
            return;
        }
        else{

            return;
        }
        return;
    };
    
    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4], int level, const GPaint& paint) override {
        GPoint transformVerts[4];
        GMatrix top = ctmStack.top();
        top.mapPoints(transformVerts, verts, 4);

        
        int n  = level + 2;
        float levelScale = 1.0f / (n-1);
        GPoint A = transformVerts[0];
        GPoint B = transformVerts[1];
        GPoint C = transformVerts[2];
        GPoint D = transformVerts[3];

        

       GPoint matrixPts[n][n];
       GColor matrixColors[n][n];
       GPoint matrixTexs[n][n];
       if(colors != nullptr){
            GColor colorA = colors[0];
            GColor colorB = colors[1];
            GColor colorC = colors[2];
            GColor colorD = colors[3];
            for(int i = 0; i < n; i++){
                for(int j = 0; j < n; j++){
                    GPoint currPoint = {A + (B-A) * (j*levelScale)};
                    GColor currColor = {colorA + (colorB-colorA) * (j*levelScale)};
                    matrixPts[i][j] = currPoint;
                    matrixColors[i][j] = currColor;
                }
                A = A + (transformVerts[3] - transformVerts[0]) * levelScale;
                B = B + (transformVerts[2] - transformVerts[1]) * levelScale;
                colorA = colorA + (colors[3] - colors[0]) * levelScale;
                colorB = colorB + (colors[2] - colors[1]) * levelScale;
            }
            for(int i = 0; i < n-1; i++){
                for(int j = 0; j < n-1; j++){
                    //Points
                    GPoint lt = matrixPts[i][j];
                    GPoint rt = matrixPts[i+1][j];
                    GPoint lb = matrixPts[i][j+1];
                    GPoint rb = matrixPts[i+1][j+1];

                    GPoint upsideDown[3] = {lt, rt, lb};
                    GPoint rightSideUp[3] = {rt, rb, lb};

                    //Colors
                    GColor ltC = matrixColors[i][j];
                    GColor rtC = matrixColors[i+1][j];
                    GColor lbC = matrixColors[i][j+1];
                    GColor rbC = matrixColors[i+1][j+1];

                    GColor upsideDownC[3] = {ltC, rtC, lbC};
                    GColor rightSideUpC[3] = {rtC, rbC, lbC};

                    drawTriangleTriColor(upsideDown, 3, upsideDownC, paint);
                    drawTriangleTriColor(rightSideUp, 3, rightSideUpC, paint);  

                }
            }

       } else{
            //Working with Texture
            //scale and translate the texture
            
            GPoint texA = texs[0];
            GPoint texB = texs[1];
            GPoint texC = texs[2];
            GPoint texD = texs[3];
            
            GPoint originalPoints[n][n];
            GPoint oA = verts[0];
            GPoint oB = verts[1];
            GPoint oC = verts[2];
            GPoint oD = verts[3];

            for(int i = 0; i < n; i++){
                for(int j = 0; j < n; j++){
                    
                    GPoint currTex = {texA + (texB-texA) * (j*levelScale)};
                    GPoint currOriginal = {oA + (oB-oA) * (j*levelScale)};

                    originalPoints[i][j] = currOriginal;
                    
                    matrixTexs[i][j] = currTex;
                }
               
                oA = oA + (verts[3] - verts[0]) * levelScale;
                oB = oB + (verts[2] - verts[1]) * levelScale;
                texA = texA + (texs[3] - texs[0]) * levelScale;
                texB = texB + (texs[2] - texs[1]) * levelScale;
            }

            for(int i = 0; i < n-1; i++){
                for(int j = 0; j < n-1; j++){
                    //Points
                    GPoint lt = originalPoints[i][j];
                    GPoint rt = originalPoints[i+1][j];
                    GPoint lb = originalPoints[i][j+1];
                    GPoint rb = originalPoints[i+1][j+1];

                    GPoint upsideDown[3] = {lt, rt, lb};
                    GPoint rightSideUp[3] = {rt, rb, lb};

                    //Texs
                    GPoint ltT = matrixTexs[i][j];
                    GPoint rtT = matrixTexs[i+1][j];
                    GPoint lbT = matrixTexs[i][j+1];
                    GPoint rbT = matrixTexs[i+1][j+1];




                    GPoint upsideDownT[3] = {ltT, rtT, lbT};
                    GPoint rightSideUpT[3] = {rtT, rbT, lbT};

                    drawMesh(upsideDown, nullptr, upsideDownT, 3, nullptr, paint);
                    drawMesh(rightSideUp, nullptr, rightSideUpT, 3, nullptr, paint); 
                }
            }
            
            
       }
       

       return;


    };
    

private:
    // Note: we store a copy of the bitmap
    const GBitmap fDevice;
    std::stack<GMatrix> ctmStack;
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new MyCanvas(device));
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {

    //initialize a shader importing grad.png


    for(int i=0; i < 20; i++){
        //Make random RGBA values from 0 to 1 with alpha = 1
        GPaint paint( GColor::RGBA( (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, 1.0f ) );
        //make a random path with 8 points with random x and y values between 0 and 100
        GPath path;
        GPoint pts[8] = {
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
            {(float)rand() / (float)RAND_MAX * 256, (float)rand() / (float)RAND_MAX * 256},
        };
        path.addPolygon(pts, 8);
        //draw the path
        canvas->save();
        canvas->drawPath(path, paint);
        
        }
    return "random shapes";

    }
