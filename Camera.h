#ifndef CAMERA_H
#define CAMERA_H

#include "Transformation.h"

enum{ABS, REL};

class NaturalCamera
{
    public:
        NaturalCamera();

        Matrix getTransformation();
        void translate(float x, float y, float z, int mode=REL);
        void rotateX(float a);
        void rotateY(float a);
        void rotateZ(float a);
        void scale(float factor);
    protected:
    private:
        float pos[4];
        float aX, aY, aZ;
        float scaleFactor;
        Matrix mat;
};

#endif // CAMERA_H
