
#include <cmath>
#include <cstdlib>
#include <SDL.h>
#include "Engine2D.h"
#include "Interpolator.h"

Engine2D::Engine2D(SDL_Surface *s, int ctype, int iParams[3]) : src(s), colorType(ctype), interpolatorType(iParams[0]), interpolatorNiv(iParams[1]), interpolatorHard(iParams[2]) {
    //ctor
}

Engine2D::~Engine2D() {
    //dtor
}

void Engine2D::draw(Noise2D &noise) {
    SDL_LockSurface(src);

    for(int x = 0; x < src->w; ++x)
    for(int y = 0; y < src->h; ++y)
    {
        double z = noise.bruit_coherent2D((double) x, (double) y);
        putPixel(x, y, Interpolator::getColor(colorType, z, Interpolator::get(interpolatorType, z, interpolatorNiv, interpolatorHard), src->format));
    }

    SDL_UnlockSurface(src);
}


void Engine2D::putPixel(int x, int y, Uint32 pixel)
{
    Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * src->format->BytesPerPixel;

    if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
    } else {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
    }
}
