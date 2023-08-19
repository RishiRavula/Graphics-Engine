#include "include/GFinal.h"
#include "include/GShader.h"
#include "include/GBitmap.h"




class MyFinal : public GFinal {
    static GPixel colorToPixel(GColor color){
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
    } //FOR USE IN BILERP 
    class GBilerpShader : public GShader{
    public:
        

        GBilerpShader(const GBitmap &bitmap, const GMatrix &localInverse) : fBitmap(bitmap), fLocalMatrix(localInverse){}

        bool isOpaque() override{
            return fBitmap.isOpaque();
        }

        bool setContext(const GMatrix &ctm) override{
            if (!ctm.invert(&fInverse)){
                return false;
            }
            fInverse = fLocalMatrix * fInverse;
            return true;
        }


        void shadeRow(int x, int y, int count, GPixel row[]) override {
            GPoint pt = GPoint{x + 0.5, y + 0.5};
            fInverse.mapPoints(&pt, 1);
            for (int i = 0; i < count; i++){
                
                // Clamp to bounds of bitmap, then get pixel by looking at the four corners and some "window" around the point
                //Window =  -----------------
                //          | -1,-1 | 0,-1  |
                //          |       |       |
                //          -----------------
                //          | -1, 0 |  0,0  |
                //          |       |       |
                //          -----------------
                GPixel topLeft = *fBitmap.getAddr(std::max(0, std::min(fBitmap.width() - 1, GRoundToInt(pt.fX) - 1)), std::max(0, std::min(fBitmap.height() - 1, GRoundToInt(pt.fY) - 1)));
                //DIV255 NOT WORKING :((( doing /255 instead
                GColor A = GColor::RGBA(GPixel_GetR(topLeft) / 255.f,
                                        GPixel_GetG(topLeft) / 255.f,
                                        GPixel_GetB(topLeft) / 255.f,
                                        GPixel_GetA(topLeft) / 255.f);

                GPixel topRight = *fBitmap.getAddr(std::max(0, std::min(fBitmap.width() - 1, GRoundToInt(pt.fX))), std::max(0, std::min(fBitmap.height() - 1, GRoundToInt(pt.fY) - 1)));
                GColor B = GColor::RGBA(GPixel_GetR(topRight) / 255.f,
                                        GPixel_GetG(topRight) / 255.f,
                                        GPixel_GetB(topRight) / 255.f,
                                        GPixel_GetA(topRight) / 255.f);
           
                GPixel bottomLeft = *fBitmap.getAddr(std::max(0, std::min(fBitmap.width() - 1, GRoundToInt(pt.fX) - 1)), std::max(0, std::min(fBitmap.height() - 1, GRoundToInt(pt.fY))));
                GColor C = GColor::RGBA(GPixel_GetR(bottomLeft) / 255.f,
                                        GPixel_GetG(bottomLeft) / 255.f,
                                        GPixel_GetB(bottomLeft) / 255.f,
                                        GPixel_GetA(bottomLeft) / 255.f);
              

                GPixel bottomRight = *fBitmap.getAddr(std::max(0, std::min(fBitmap.width() - 1, GRoundToInt(pt.fX))), std::max(0, std::min(fBitmap.height() - 1, GRoundToInt(pt.fY))));
                GColor D = GColor::RGBA(GPixel_GetR(bottomRight) / 255.f,
                                        GPixel_GetG(bottomRight) / 255.f,
                                        GPixel_GetB(bottomRight) / 255.f,
                                        GPixel_GetA(bottomRight) / 255.f);
                float u = pt.fX + 0.5 - floor(pt.fX + 0.5);
                float v = pt.fY + 0.5 - floor(pt.fY + 0.5);

                GColor AModified = A * (1 - u) * (1 - v);
                GColor BModified = B * (u) * (1 - v);
                GColor CModified = C * (1 - u) * (v);
                GColor DModified = D * (u) * (v);

                row[i] = colorToPixel(AModified + BModified + CModified + DModified);

                pt.fX += fInverse[0];
                pt.fY += fInverse[3];
            }
            return;
        }

    private:
        GBitmap fBitmap;
        GMatrix fLocalMatrix;
        GMatrix fInverse;
    };




    std::unique_ptr<GShader> createBilerpShader(const GBitmap &bitmap, const GMatrix &localMatrix){
        return std::unique_ptr<GShader>(new GBilerpShader(bitmap, localMatrix));
    }

    



    void addLine(GPath* path, GPoint p0, GPoint p1, float width, CapType cap) override {  
        // angle of the perpendicular line, determined by neg reciprocal of slope atan
        float angle = atan(-1 / ((p1.fY - p0.fY) / (p1.fX - p0.fX)));
        float diameter = width; // diameter of the line / square / circle
        float radius = diameter / 2; // radius of the line / square / circle
        float dxPerp = radius * cos(angle); 
        float dyPerp = radius * sin(angle);
        float startX = p0.fX + dxPerp; // start of the line, perpendicular to the original line
        float startY = p0.fY + dyPerp;
        float topLeftX = p0.fX - dxPerp;
        float topLeftY = p0.fY - dyPerp;
        float topRightX = p1.fX - dxPerp;
        float topRightY = p1.fY - dyPerp;
        float endX = p1.fX + dxPerp;
        float endY = p1.fY + dyPerp; // end of the line

        //DRAW THE FIRST END OF THE LINE
        if (cap == kButt) {
            path->moveTo(GPoint{startX, startY});
            path->lineTo(GPoint{topLeftX , topLeftY});
            path->lineTo(GPoint{topRightX, topRightY});
            path->lineTo(GPoint{endX, endY});
        }
        else if (cap == kSquare) {
            //draw a slightly larger line => halfsquare + line + halfsquare
            path->moveTo(GPoint{startX + dyPerp, startY + dxPerp});
            path->lineTo(GPoint{topLeftX + dyPerp, topLeftY - dxPerp});
            path->lineTo(GPoint{topRightX - dyPerp, topRightY - dxPerp});
            path->lineTo(GPoint{endX - dyPerp, endY - dxPerp});
        }
        else if (cap == kRound) {
            path->addCircle(p0, radius);
            
            path->moveTo(GPoint{startX, startY});
            path->lineTo(GPoint{topLeftX , topLeftY});
            path->lineTo(GPoint{topRightX, topRightY});
            path->lineTo(GPoint{endX, endY});

            path->addCircle(p1, radius);
            //REDRAW CUZ WEIRD ISSUE WITH WINDING
            path->moveTo(GPoint{startX, startY});
            path->lineTo(GPoint{topLeftX , topLeftY});
            path->lineTo(GPoint{topRightX, topRightY});
            path->lineTo(GPoint{endX, endY});
        }
        else{
            //do nothing
            printf("ERROR: INVALID CAP TYPE, check func\n");
        }
        return;
    }
};

std::unique_ptr<GFinal> GCreateFinal() {
    return std::unique_ptr<GFinal>(new MyFinal());
}