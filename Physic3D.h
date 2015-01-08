#ifndef PHYSIC3D_H
#define PHYSIC3D_H

#include "Object3D.h"

class Physic3D
{
    public:
        Physic3D();
        virtual ~Physic3D();
        static bool collisionFaceVector(float const *A, float const *B, float const *C, float const *normal, float const *vectDir, float const *vectOrig);
        static bool collisionFaceVectorDirect(float const *A, float const *B, float const *C, float const *normal, float const *vectDir, float const *vectOrig);
        static bool collisionFaceStraightLineDirect(float const *A, float const *B, float const *C, float const *normal, float const *vectDir, float const *vectOrig);
        static bool collisionFaceStraightLine(float const *A, float const *B, float const *C, float const *vectDir, float const *vectOrig);
        static float collisionFaceVectorCoef(float const *A, /*float const *B, float const *C,*/ float const *normal, float const *vectDir, float const *vectOrig);
        static void collisionFaceVectorBounce(float const *normal, float const *vectDir, float *bounceDir);
    protected:
    private:

};

class PointPhysic
{
    public:
        PointPhysic(float bounceFactor) : bounceFactor(bounceFactor) {}
        void moveInside_PreAlpha(Object3D /*const*/& container, float kremain=1.0f, int lastFaceCollIndex=-1);
        float p[3];
        float v[3];
        float bounceFactor;
};

#endif // PHYSIC3D_H
