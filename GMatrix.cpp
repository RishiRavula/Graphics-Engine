/*
 *  Copyright 2023 <me>
 */
#include <cmath>
#include "include/GMatrix.h"
#include "include/GPoint.h"


//initializes to identity

GMatrix::GMatrix() {
    fMat[0] = 1;
    fMat[1] = 0;
    fMat[2] = 0;

    fMat[3] = 0;
    fMat[4] = 1;
    fMat[5] = 0;

}

GMatrix GMatrix::Translate(float tx, float ty) {
    //x = ax + by + c
    //y = dx + ey + f
    return GMatrix(1, 0, tx,
                   0, 1, ty);
    
}

GMatrix GMatrix::Scale(float sx, float sy) {
    return GMatrix(sx, 0, 0,
                   0, sy, 0);
}

GMatrix GMatrix::Rotate(float radians) {
    float cos = std::cos(radians);
    float sin = std::sin(radians);
    return GMatrix(cos, -sin, 0,
                   sin, cos, 0);
}

GMatrix GMatrix::Concat(const GMatrix& a, const GMatrix& b) {
    GMatrix matrix;
    matrix.fMat[0] = b.fMat[0] * a.fMat[0] + b.fMat[3] * a.fMat[1];
    matrix.fMat[1] = b.fMat[1] * a.fMat[0] + b.fMat[4] * a.fMat[1];
    matrix.fMat[2] = b.fMat[2] * a.fMat[0] + b.fMat[5] * a.fMat[1] + a.fMat[2];

    matrix.fMat[3] = b.fMat[0] * a.fMat[3] + b.fMat[3] * a.fMat[4];
    matrix.fMat[4] = b.fMat[1] * a.fMat[3] + b.fMat[4] * a.fMat[4];
    matrix.fMat[5] = b.fMat[2] * a.fMat[3] + b.fMat[5] * a.fMat[4] + a.fMat[5];
    return matrix;
}

bool GMatrix::invert(GMatrix* inverse) const {
    float i = fMat[0];
    float j = fMat[1];
    float k = fMat[2];
    float l = fMat[3];
    float m = fMat[4];
    float n = fMat[5];
    /*
    [i j k]
    [l m n]
    [0 0 1]
    */
    
    float det = i * m - j * l;
    if(det == 0){
        return false;
    } 
    else {
        float div = 1 / det;        
        float a = m * div;
        float c = (j*n - k*m) * div;
        float e = i * div;
        float f = (k*l - i*n) * div;
        float b = j * -1 * div; 
        float d = l * -1 * div;

        inverse->fMat[0] = a;
        inverse->fMat[1] = b; 
        inverse->fMat[2] = c;
        inverse->fMat[3] = d;
        inverse->fMat[4] = e;
        inverse->fMat[5] = f;
        
        return true;
    }
}

void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const {
    for (int i = 0; i < count; i++) {
        GPoint point = src[i];
        dst[i].fX = fMat[0] * point.fX + fMat[1] * point.fY + fMat[2];
        dst[i].fY = fMat[3] * point.fX + fMat[4] * point.fY + fMat[5];
    }
}


