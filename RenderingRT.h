#ifndef RENDERINGRT_H
#define RENDERINGRT_H

#include "RayTracing.h"
#include <list>

class Ray
{
public:
    RayData data;
    std::list<Ray*> previous;
};

class RenderingRT
{
    public:
        RenderingRT();
        virtual ~RenderingRT();
    protected:
    private:
        Ray* rays;/// rayons
};
#endif // RENDERINGRT_H
