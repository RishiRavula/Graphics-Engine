/*
 *  Copyright 2023 <me>
 */
#include <cmath>
#include "include/GPath.h"
#include "include/GMatrix.h"


void GPath::addRect(const GRect& r, Direction dir) {
    GPoint lt = {r.left(), r.top()};
    GPoint rt = {r.right(), r.top()};
    GPoint rb = {r.right(), r.bottom()};
    GPoint lb = {r.left(), r.bottom()};
    if (dir == Direction::kCCW_Direction) {
        this->moveTo(lt);
        this->lineTo(lb);
        this->lineTo(rb);
        this->lineTo(rt);
    } else {
        this->moveTo(lt);
        this->lineTo(rt);
        this->lineTo(rb);
        this->lineTo(lb);  
    }
}


void GPath::addPolygon(const GPoint pts[], int count) {
    this->moveTo(pts[0]);
    for(int i = 1; i < count; i++) {
        this->lineTo(pts[i]);
    }
}

void GPath::addCircle(GPoint center, float radius, Direction dir) {
    float sqrtElem = 0.70710678118; //sqrt(2)/2
    float tanElem = 0.41421356237; //tan(pi/8)
    GPoint unit_Circle[17] = { {1, 0}, {1, tanElem}, {sqrtElem, sqrtElem}, {tanElem, 1}, {0, 1}, {-tanElem, 1}, {-sqrtElem, sqrtElem}, {-1, tanElem}, {-1, 0}, {-1, -tanElem}, {-sqrtElem, -sqrtElem}, {-tanElem, -1}, {0, -1}, {tanElem, -1}, {sqrtElem, -sqrtElem}, {1, -tanElem}, {1, 0} };
    GMatrix transform = GMatrix::Translate(center.x(), center.y()) * GMatrix::Scale(radius, radius);
    transform.mapPoints(unit_Circle, 17);
    this->moveTo(unit_Circle[0]);
    //use quadTo to draw the circle
    if(dir == Direction::kCW_Direction) {
        for(int i = 1; i < 17; i+=2) {
            this->quadTo(unit_Circle[i], unit_Circle[i+1]);
        }
    } else {
        //counter clockwise
        for(int i = 15 ; i > 0; i-=2) {
            this->quadTo(unit_Circle[i], unit_Circle[i-1]); 
        }
        
    }    
}




GPoint linearInterp(float t, GPoint a, GPoint b) {
    return ((1 - t) * a + t * b);
}
void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t){
    //first point
    dst[0] = src[0];
    dst[1] = linearInterp(t, src[0], src[1]);
    dst[5] = linearInterp(t, src[2], src[3]);
    GPoint mid1 = linearInterp(t, src[1], src[2]);
    //midpoint interp(s) between 1&5
    dst[2] = linearInterp(t, dst[1], mid1);
    dst[4] = linearInterp(t, mid1, dst[5]);
    //midpoint interp between 2&4
    dst[3] = linearInterp(t, dst[2], dst[4]);
    //last point
    dst[6] = src[3];

    

}
void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t){
    //first point
    dst[0] = src[0];
    dst[1] = linearInterp(t, src[0], src[1]);
    dst[3] = linearInterp(t, src[1], src[2]);
    dst[2] = linearInterp(t, dst[1], dst[3]);
    //last point
    dst[4] = src[2];
}


GRect GPath::bounds() const{
    
    if (fPts.size() == 0) {
        return GRect::LTRB(0, 0, 0, 0);
        //NO RECT EXISTS
    }
    float left = fPts[0].fX;
    float top = fPts[0].fY;
    float right = fPts[0].fX;
    float bottom = fPts[0].fY;

    for (int i = 1; i < fPts.size(); ++i) {
        left = std::min(left, fPts[i].fX);
        top = std::min(top, fPts[i].fY);
        right = std::max(right, fPts[i].fX);
        bottom = std::max(bottom, fPts[i].fY);
    }

    return GRect::LTRB(left, top, right, bottom);
} 




void GPath::transform(const GMatrix& matrix) {
    matrix.mapPoints(fPts.data(), fPts.data(), fPts.size());
}

