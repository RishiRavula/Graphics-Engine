
#include "include/GBitmap.h"
#include "include/GShader.h"
#include "include/GMatrix.h"



class BitmapShader : public GShader {
public:
    BitmapShader(const GBitmap& bitmap, const GMatrix& localInv, GShader::TileMode mode) : fBitmap(bitmap), fLocalMatrix(localInv), tMode(mode) {}
    
    bool isOpaque() override {
        return fBitmap.isOpaque();
    }

    bool setContext(const GMatrix& ctm) override {
        if (!ctm.invert(&fInverse)) {
            return false;
        }
        fInverse = fLocalMatrix * fInverse;
        return true;
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
        if(tMode == GShader::TileMode::kRepeat){
            GPoint pt = {x + 0.5f, y + 0.5f};
            GMatrix normalize = GMatrix(
                1.0f / fBitmap.width(), 0, 0,
                0, 1.0f / fBitmap.height(), 0
            );
            GMatrix newMatrix = normalize * fInverse;
            newMatrix.mapPoints(&pt, 1);
            //TODO: OPTIMIZE
            for (int i = 0; i < count; ++i) {
                float rX = pt.x();
                float rY = pt.y();
                // calculate iX and iY

                int iX = (int)((rX - floor(rX)) * fBitmap.width());
                int iY = (int)((rY - floor(rY)) * fBitmap.height());
                
                GPixel* pixels = fBitmap.getAddr(iX, iY);
                row[i] = *pixels;
                pt.fX +=  newMatrix[0];
                pt.fY +=  newMatrix[3];
            }
  
        }

        else if (tMode == GShader::TileMode::kMirror){
            GPoint pt = {x + 0.5f, y + 0.5f};
            GMatrix normalize = GMatrix(
                1.0f / fBitmap.width(), 0, 0,
                0, 1.0f / fBitmap.height(), 0
            );
            GMatrix newMatrix = normalize * fInverse;
            newMatrix.mapPoints(&pt, 1);
            for (int i = 0; i < count; ++i) {
                float rX = pt.x() * 0.5;
                float rY = pt.y() * 0.5;
                rX = rX - floor(rX);
                rY = rY - floor(rY);
                int iX = rX > 0.5 ? (int) (((1 - rX) * 2) * (fBitmap.width())) : (int)(rX*2 * fBitmap.width());
                int iY = rY > 0.5 ? (int) (((1 - rY) * 2) * (fBitmap.height())) : (int)(rY*2 * fBitmap.height());
                iX = std::max(0, std::min(iX, fBitmap.width() - 1));
                iY = std::max(0, std::min(iY, fBitmap.height() - 1));
                GPixel* pixels = fBitmap.getAddr(iX, iY);
                row[i] = *pixels;
                pt.fX +=  newMatrix[0];
                pt.fY +=  newMatrix[3]; 
            }
        }

        else{
            GPoint pt = {x + 0.5f, y + 0.5f};
            fInverse.mapPoints(&pt, 1);
            for (int i = 0; i < count; ++i) {
                float rX = pt.x();
                float rY = pt.y();
                int iX = (int)(std::max(0.0f, std::min(rX, (float)fBitmap.width() - 1.0f)));
                int iY = (int)(std::max(0.0f, std::min(rY, (float)fBitmap.height() - 1.0f)));
                GPixel* pixels = fBitmap.getAddr(iX, iY);
                row[i] = *pixels;
                pt.fX +=  fInverse[0];
                pt.fY +=  fInverse[3];
            }
        }


        return;

    }

private:
    GBitmap fBitmap;
    GMatrix fInverse;
    GMatrix fLocalMatrix;
    TileMode tMode;
};



std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localInv, GShader::TileMode tileMode) {
    if (!bitmap.pixels() || bitmap.width() <= 0 || bitmap.height() <= 0) {
        return nullptr;
    }
    //if there is a tile mode, return a tiled shader
    
    //is it fine to assume a tileMode of clamp is just the original bitmap shader we've been using?
    return std::unique_ptr<GShader>(new BitmapShader(bitmap, localInv, tileMode));
}


class GLinearGradient : public GShader {
public:
    GLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode) {
        fColors = new GColor[count+2]; // store the colors + 2 extra colors
        fColors[0] = colors[0];
        fColors[1] = colors[0];
        for (int i = 2; i <= count; ++i) {
            fColors[i] = colors[i-1];
        }
        fColors[count+1] = colors[count-1];
        fColorCount = count;
        float dx = p1.fX - p0.fX;
        float dy = p1.fY - p0.fY;
        GMatrix fBasis = GMatrix(
            dx, -dy, p0.fX,
            dy, dx, p0.fY
        );
        fBasis.invert(&fBasisInverse);
        tMode = mode;
        
    }

    bool isOpaque() override {
        for (int i = 0; i < fColorCount; ++i) {
            if (fColors[i].a != 1.0f) {
                return false;
            }
        }
        return true;
    }
    bool setContext(const GMatrix& ctm) override {
        if (!ctm.invert(&fLocalMatrix)) {
            return false;
        }
        fLocalMatrix = fBasisInverse * fLocalMatrix;
        delta = fLocalMatrix[0] * (fColorCount - 1);
        return true;
    }
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        
        GPoint pt = {x + 0.5f, y + 0.5f};
        GPoint p = fLocalMatrix * pt;
        float mapping = 1 + (p.fX * (fColorCount - 1));
        if (tMode == GShader::TileMode::kRepeat) {
           //TODO: NEED TO FIX, DOESNT TEST THIS???
            for(int i = 0; i < count; i++){ 
            int idx = (int)mapping;
            GColor currentColor = (mapping - idx)*(fColors[idx+1] - fColors[idx]) + fColors[idx];
            row[i] = colorToPixel(currentColor);
            mapping += delta;
            }
        }
        // else if (tMode == GShader::TileMode::kMirror) {
        //     //Implement for kMirror TileMode
            

        // }
        else {
            for(int i = 0; i < count; i++){ 
            int idx = (int)mapping;
            GColor currentColor = (mapping - idx)*(fColors[idx+1] - fColors[idx]) + fColors[idx];
            row[i] = colorToPixel(currentColor);
            mapping += delta;
            }
        }
    }

private:
    GColor* fColors;
    GMatrix fLocalMatrix;
    GMatrix fBasisInverse;
    float delta;
    int fColorCount;
    TileMode tMode;
};



class GLinearGradient1 : public GShader {
public:
    GLinearGradient1(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode) {
        fColors = new GColor[count];
        for (int i = 0; i < count; ++i) {
            fColors[i] = colors[i];
        }
        fColorCount = count;
        float dx = p1.fX - p0.fX;
        float dy = p1.fY - p0.fY;
        GMatrix fBasis = GMatrix(
            dx, -dy, p0.fX,
            dy, dx, p0.fY
        );
        fBasis.invert(&fBasisInverse);
        tMode = mode;
    }

    bool isOpaque() override {
        for (int i = 0; i < fColorCount; ++i) {
            if (fColors[i].a != 1.0f) {
                return false;
            }
        }
        return true;
    }
    bool setContext(const GMatrix& ctm) override {
        if (!ctm.invert(&fLocalMatrix)) {
            return false;
        }
        fLocalMatrix = fBasisInverse * fLocalMatrix;
        return true;
    }
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        //IMPLEMENT TILEMODES HERE
        GPixel color = colorToPixel(fColors[0]);
        for(int i = 0; i < count; i++){
            row[i] = color;
        }
    }

private:
    GColor* fColors;
    GMatrix fLocalMatrix;
    GMatrix fBasisInverse;
    int fColorCount;
    TileMode tMode;
};

class GLinearGradient2 : public GShader {
public:
    GLinearGradient2(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode) {
        fColors = new GColor[count];
        for (int i = 0; i < count; ++i) {
            fColors[i] = colors[i];
        }
        fColorCount = count;
        float dx = p1.fX - p0.fX;
        float dy = p1.fY - p0.fY;
        GMatrix fBasis = GMatrix(
            dx, -dy, p0.fX,
            dy, dx, p0.fY
        );
        fBasis.invert(&fBasisInverse);
        tMode = mode;
    }

    bool isOpaque() override {
        for (int i = 0; i < fColorCount; ++i) {
            if (fColors[i].a != 1.0f) {
                return false;
            }
        }
        return true;
    }
    bool setContext(const GMatrix& ctm) override {
        if (!ctm.invert(&fLocalMatrix)) {
            return false;
        }
        fLocalMatrix = fBasisInverse * fLocalMatrix;
        changeInColor = fLocalMatrix[0] * (fColors[1] - fColors[0]);
        return true;
    }
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        if (tMode == GShader::TileMode::kRepeat) {
            //TODO: OPTIMIZE FOR REPEAT
            GPoint pt = {x + 0.5f, y + 0.5f};
            GPoint p = fLocalMatrix * pt;
            for(int i = 0; i < count; i++){
                float localX = p.fX - floorf(p.fX);
                GColor currColor = localX *(fColors[1] - fColors[0]) + fColors[0];
                row[i] = colorToPixel(currColor);
                p.fX += fLocalMatrix[0];
            }
        }
        else{
            GPoint pt = {x + 0.5f, y + 0.5f};
            GPoint p = fLocalMatrix * pt;
            GColor currColor = p.fX *(fColors[1] - fColors[0]) + fColors[0];
            for(int i = 0; i < count; i++){
                row[i] = colorToPixel(currColor);
                currColor += changeInColor;
            }
        }
       
    }

private:
    GColor* fColors;
    GMatrix fLocalMatrix;
    GMatrix fBasisInverse;
    int fColorCount;
    GColor changeInColor;
    TileMode tMode;
};

std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode mode) {
    if (mode == GShader::TileMode::kRepeat || mode == GShader::TileMode::kClamp){
        if (count < 1) {
        return nullptr;
        }
        if (count == 1) {
            return std::unique_ptr<GShader>(new GLinearGradient1(p0, p1, colors, count, mode));
        }
        if (count == 2) {
            return std::unique_ptr<GShader>(new GLinearGradient2(p0, p1, colors, count, mode));
        }
    }
    else if (mode == GShader::TileMode::kMirror) {
        //make a new array of colors i.e. r,g,b,g,r

        GColor* newColors = new GColor[count * 2 - 1];
        for (int i = 0; i < count; ++i) {
            newColors[i] = colors[i];
        }
        for (int i = 0; i < count - 1; ++i) {
            newColors[count + i] = colors[count - 2 - i];
        }
        //create new p1 to compensate for the new array
        GPoint newP1 = p0 + (p1 - p0) * 2;
        
        return std::unique_ptr<GShader>(new GLinearGradient(p0, newP1, newColors, count * 2 - 1, mode));
    }
    
    return std::unique_ptr<GShader>(new GLinearGradient(p0, p1, colors, count, mode));
}


class ProxyShader : public GShader {
    GShader* fRealShader;
    GMatrix  fExtraTransform;
public:
    ProxyShader(GShader* shader, const GMatrix& extraTransform)
        : fRealShader(shader), fExtraTransform(extraTransform) {}

    bool isOpaque() override { return fRealShader->isOpaque(); }

    bool setContext(const GMatrix& ctm) override {
        return fRealShader->setContext(ctm * fExtraTransform);
    }
    
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        fRealShader->shadeRow(x, y, count, row);
    }
};
