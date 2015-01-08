#ifndef ENGINE2D_H
#define ENGINE2D_H

#include <SDL.h>

#include "Noise2D.h"

class Engine2D
{
    public:
        Engine2D(SDL_Surface *s, int ctype, int iParams[]);
        virtual ~Engine2D();

        void draw(Noise2D &noise);

        void putPixel(int x, int y, Uint32 pixel);

        void setColorType(int t) {colorType = t;}
        void setInterParams(int iParams[]) {
            interpolatorType = iParams[0];
            interpolatorNiv = iParams[1];
            interpolatorHard = iParams[2];
        }
    protected:
    private:
        SDL_Surface * src;
        int colorType;
        int interpolatorType;
        int interpolatorNiv;
        int interpolatorHard;
};

#endif // ENGINE2D_H
