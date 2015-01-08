#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include <SDL.h>
#include "color.h"

enum{BandW, RtoYtoG, HUE, WOOD, MOUNTAIN, RINGS, NB_COLOR_TYPE};
enum{iX, iIntX, iCutX, iAbsCutX, iBiX, iInvX, iPosSinX, iExpSinX, iAbsSinX, iCutSinX, iCliffX, iMountainX, NB_RELIEF_TYPE};

class Interpolator
{
    public:
        Interpolator();
        virtual ~Interpolator();

        static Uint32 getColor(int type, float x, float k, const SDL_PixelFormat* f);
        static Uint32 getRangedColor(float x, int color[], float limit[], int nbColor, const SDL_PixelFormat* f);
        static void getRangedColor(float x, int color[], float limit[], int nbColor, float &r, float &g, float &b);
        static rgb_f getColor(int type, float x, float k);
        static float get(int type, float x, float level, float hardness = 1.0);
    protected:
    private:
};

#endif // INTERPOLATOR_H
