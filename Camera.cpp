#include "Camera.h"
#include <cstdio>
#include <cmath>

NaturalCamera::NaturalCamera() : aX(0), aY(0), aZ(0), scaleFactor(1)
{
    pos[0] =  pos[1] = pos[2] = pos[3] = 0;
}

Matrix NaturalCamera::getTransformation() {
    Transformation t;

    t.translateX(pos[0]);
    t.translateY(pos[1]);
    t.translateZ(pos[2]);
    t.rotateZ(aZ);
    t.rotateY(aY);
    t.rotateX(aX);
    t.scale(scaleFactor);

    return t.getMatrix();
}

void NaturalCamera::translate(float x, float y, float z, int mode) {
    if(mode == ABS) {
        pos[0] += x;
        pos[1] += y;
        pos[2] += z;
    } else {
        Transformation t;
        t.rotateX(-aX);
        t.rotateY(-aY);
        t.rotateZ(-aZ);
        float v[4] = {x,y,z,1}, w[4];
        t.applyTo(v, w);
        pos[0] += w[0] / scaleFactor;
        pos[1] += w[1] / scaleFactor;
        pos[2] += w[2] / scaleFactor;
    }
}

void NaturalCamera::rotateX(float a) {
    if((aX += a) > M_PI_2 )
        aX = M_PI_2;
    else if(aX < -M_PI_2 )
        aX = -M_PI_2;
}

void NaturalCamera::rotateY(float a) {
    aY += a;
}

void NaturalCamera::rotateZ(float a) {
    aZ += a;
}

void NaturalCamera::scale(float factor) {
    scaleFactor *= factor;
}
