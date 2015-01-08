#ifndef VERTEX_H
#define VERTEX_H

#include "color.h"
//#include "Object3D.h"


class Vertex
{
    public:
        Vertex() {}
        Vertex(float x, float y, float z, float nx, float ny, float nz, rgb_f const& c) : c(c) {
            p[0] = x; p[1] = y; p[2] = z; p[3] = 1;
            n[0] = nx; n[1] = ny; n[2] = nz; n[3] = 1;
        }
        Vertex(float *pos, float *norm, rgb_f const& c) : c(c) { memcpy(p, pos, sizeof(float)*4); memcpy(n, norm, sizeof(float)*4); }
        void setNormal(float *_n) { memcpy(n, _n, sizeof(float)*4); }
        void setPos(float *_p) { memcpy(p, _p, sizeof(float)*4); }
        void setColor(int *_c) { c.r = (float) _c[0]/255.0f; c.g = (float) _c[1]/255.0f; c.b = (float) _c[2]/255.0f;}

        float p[4];
        float n[4];
        rgb_f c;
        float s,t; /// texture coord
};
class Transf_Vertex : public Vertex
{
    public:
        bool left__, right_, up____, down__, behind, front_, isUsed;
        const void* objParent;
        float onScreenCoord[4];
};

#endif

