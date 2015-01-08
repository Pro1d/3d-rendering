#include "Interpolator.h"
#include <cmath>
#include <cstdlib>
#include "color.h"

#define min(X,Y)   X<Y?X:Y
#define max(X,Y)   X>Y?X:Y

Interpolator::Interpolator()
{
    //ctor
}

Interpolator::~Interpolator()
{
    //dtor
}

float toHue(float k) {
    k = fmod(k, 1);
    k *= 6;
    if(k < 2)
        return 0.0;
    else if(k < 3)
        return k - 2.0;
    else if(k < 5)
        return 1.0;
    else
        return -k + 6.0;
}

Uint32 mapRGBDouble(const SDL_PixelFormat * f, float r, float g, float b) {
    return SDL_MapRGB(f, r*255, g*255, b*255);
}

rgb_f Interpolator::getColor(int type, float x, float k) {
    float r,g,b;
    switch(type) {
        case HUE:
            r = toHue(k+2.0/6);
            g = toHue(k+4.0/6);
            b = toHue(k);
            break;
        case RtoYtoG:
            r = min(2*k, 1.0);
            g = min(2*(1.0 - k), 1.0);
            b = 0.0;
            break;
        case RINGS:
            r = k*toHue(x+2.0/6);
            g = k*toHue(x+4.0/6);
            b = k*toHue(x);
            break;
        case WOOD:
            r = 0.5*k + 0.2;
            g = 0.3*k + 0.1;
            b = 0;
            break;
        case MOUNTAIN: {
                const int n = 7;
                float h[n]={0.0,     0.25,    0.27,    0.48,     0.8,      0.85,        1.0};
                int c[n*3] ={0,0,255, 0,0,255, 0,150,0, 128,64,0, 128,64,0, 255,225,255, 255,255,255};
                getRangedColor(x, c, h, n, r, g, b);
            }
            break;
        case BandW:
        default:
            r = g = b = k;
            break;
    }
    return rgb_f(r, g, b);
}
Uint32 Interpolator::getColor(int type, float x, float k, const SDL_PixelFormat* f) {
    switch(type) {
        case HUE:
            return mapRGBDouble(f, toHue(k+2.0/6), toHue(k+4.0/6), toHue(k));
        case RtoYtoG:
            return mapRGBDouble(f, min(2*k, 1.0), min(2*(1.0 - k), 1.0), 0.0);
        case RINGS:
            return mapRGBDouble(f, k*toHue(x+2.0/6), k*toHue(x+4.0/6), k*toHue(x));
        case WOOD:
            return mapRGBDouble(f, 0.5*k + 0.2, 0.3*k + 0.1, 0);
        case MOUNTAIN: {
                const int n = 7;
                float h[n]={0.0,     0.25,    0.27,    0.48,     0.8,      0.85,        1.0};
                int c[n*3] ={0,0,255, 0,0,255, 0,150,0, 128,64,0, 128,64,0, 255,225,255, 255,255,255};
                return getRangedColor(x, c, h, n, f);
            }
        case BandW:
        default:
            return mapRGBDouble(f, k, k, k);
    }
}

Uint32 Interpolator::getRangedColor(float x, int color[], float limit[], int nbColor, const SDL_PixelFormat* f){
    int l = 0;
    while(x > limit[l+1]) l++;

    float k = (x - limit[l]) / (limit[l+1] - limit[l]);
    int r = color[l*3+0] * (1-k) + color[(l+1)*3+0] * k;
    int g = color[l*3+1] * (1-k) + color[(l+1)*3+1] * k;
    int b = color[l*3+2] * (1-k) + color[(l+1)*3+2] * k;

    return SDL_MapRGB(f, r, g, b);
}
void Interpolator::getRangedColor(float x, int color[], float limit[], int nbColor, float &r, float &g, float &b){
    int l = 0;
    while(x > limit[l+1]) l++;

    float k = (x - limit[l]) / (limit[l+1] - limit[l]);
    r = (color[l*3+0] * (1-k) + color[(l+1)*3+0] * k) / 255.0;
    g = (color[l*3+1] * (1-k) + color[(l+1)*3+1] * k) / 255.0;
    b = (color[l*3+2] * (1-k) + color[(l+1)*3+2] * k) / 255.0;
}

float interpolateInverse(float x, int niv) {
    if(niv == 0)
        return x;
    return interpolateInverse(2 / (2-x) - 1, niv-1);
}

float Interpolator::get(int type, float x, float niv, float hardness) {
    switch(type) {
        case iIntX:
            return floor(x*niv) / (niv-1);
            break;
        case iCutX:
            return fmod(x*niv, 1.0);
            break;
        case iAbsCutX:
            return fabs((0.5 - fmod(x*niv, 1.0)) * 2);
            break;
        case iBiX:
            return ((int)(x*niv)) % 2 ? 0 : 1;
            break;
        case iInvX:
            return interpolateInverse(x, niv);
            break;
        case iPosSinX:
            return (1 + sin(x*niv*M_PI)) / 2;
            break;
        case iExpSinX:
            return pow(fabs(sin(x*niv*M_PI)), hardness);
            break;
        case iAbsSinX:
            return fabs(sin(x*niv*M_PI));
            break;
        case iCutSinX:
            return fabs(sin(fmod(x*niv*M_PI, M_PI_2)));
            break;
        case iCliffX:
            x = get(iAbsCutX, x, hardness);
            if(niv*(x - (float)((int)(x*niv))/niv) < 0.8)
                return (float)((int)(x*niv))/niv;
            else {
                float y = x - (float)((int)(x*niv))/niv;
                float k = (1 - cos((niv*y - 0.8) / (1.0 - 0.8) * M_PI)) / 2;
                return (x - y) * (1 - k) + (x - y + 1.0/niv) * k;
            }
            break;
        case iMountainX:
            if(x < 0.27)
                return 0.27;
            else
                return sqrt((x-0.27)/(1-0.27)) + 0.27;
            break;
        case iX:
        default:
            return x;
            break;
    }
}
