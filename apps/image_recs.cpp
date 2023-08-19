/**
 *  Copyright 2020 Mike Reed
 */

#include "image_final.cpp"

const GDrawRec gDrawRecs[] = {
    { final_bilerp, 512, 512, "final_bilerp", 0 },
    { final_radial, 512, 512, "final_radial", 0 },
    { final_stroke, 512, 512, "final_stroke", 0 },
    { final_coons, 512, 512, "final_coons", 0 },

    { nullptr, 0, 0, nullptr },
};
