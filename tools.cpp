#include "tools.h"
#include <SDL.h>
#include <cmath>

using std::abs;

float clamp(float min, float max, float v) {
    if(v < min)
        return min;
    if(v > max)
        return max;
    return v;
}

float clampIn01(float v) {
    if(v < 0.0f)
        return 0.0f;
    if(v > 1.0f)
        return 1.0f;
    return v;
}



float magic_sqrt_inv(float number)
{
    Sint32 i;
    float x2 = number * 0.5f, y = number;
    const float threehalfs = 1.5f;

    i  = * ( Sint32 * ) &y;                       // evil floating point bit level hacking
    i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
    y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

    return y;
}

void normalizeVector(float *v)
{
    float n = magic_sqrt_inv(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] *= n;
    v[1] *= n;
    v[2] *= n;
}

float triangleArea(float *a, float *b, float *c) {
    float abx = b[0]-a[0], aby = b[1]-a[1];
    float acx = c[0]-a[0], acy = c[1]-a[1];
    float cbx = b[0]-c[0], cby = b[1]-c[1];
    float ab = abx*abx + aby*aby;
    float ac = acx*acx + acy*acy;
    float cb = cbx*cbx + cby*cby;

    if(ab > ac) {
        if(cb > ab)
            return abs(cbx*acy-acx*cby) / 2;
        else
            return abs(abx*acy-acx*aby) / 2;

    } else {
        if(cb > ac)
            return abs(cbx*acy-acx*cby) / 2;
        else
            return abs(abx*acy-acx*aby) / 2;
    }
}

float smoothStep(float min, float max, float x)
{
    if(x <= min)
        return 0.0f;
    if(x >= max)
        return 1.0f;

    float y = (x - min) / (max - min);
    return y * y * (3.0f - 2.0f * y);
}

/// return the normal of the triangle face; z component is positive if A, B, C are in anti-clockwise;
void getFaceNormal(float* A, float* C, float* B, float *n) {
    /// N = AC vect AB
    n[0] = (B[1]-A[1])*(C[2]-A[2]) - (C[1]-A[1])*(B[2]-A[2]);
    n[1] = (B[2]-A[2])*(C[0]-A[0]) - (C[2]-A[2])*(B[0]-A[0]);
    n[2] = (B[0]-A[0])*(C[1]-A[1]) - (C[0]-A[0])*(B[1]-A[1]);
    float norm = sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
    if(norm > 0) {
        n[0] /= -norm;
        n[1] /= -norm;
        n[2] /= -norm;
    }
}

