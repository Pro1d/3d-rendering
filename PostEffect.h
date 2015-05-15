#ifndef POSTEFFECT_H
#define POSTEFFECT_H

#include "color.h"
#include "Engine3D.h"
#include "Noise2D.h"

class PostEffect
{
    public:
        PostEffect(rgb_f *color, float *depth, rgb_f* destination, int width, int height);
        virtual ~PostEffect();
        bool isEnabled() { return enabled; }
        void toggleEnabled() { enabled = !enabled; }
        void setEnabled(bool e) { enabled = e; }
        void setColorSrc(rgb_f* c) { color = c; }
        void setColorDst(rgb_f* c) { dst = c; }
        void setColorTmp(rgb_f* c) { tmp = c; }
        //virtual void apply();
    protected:
        rgb_f *color;
        float* depth;
        int width, height;

        rgb_f *dst; /// destination de l'image obtenue
        rgb_f *tmp; /// buffer intermédiaire
    private:
        bool enabled;
};

class DistanceFog : public PostEffect
{
    public:
        DistanceFog(rgb_f *color, float *depth, rgb_f* destination, int width, int height, rgb_f const& fogColor, float density);
        void setFogDensity(float d);
        float getFogDensity();
        void setNoiseDensity(float d) { noiseIntensity = d; }
        float getNoiseDensity() { return noiseIntensity; }
        void setFogColor(rgb_f const& color);
        void apply();
    private:
        rgb_f fogColor;
        float density, noiseIntensity;
        Noise2D noise;
};

class DepthOfField : public PostEffect
{
    public:
        DepthOfField(rgb_f *color, float *depth, rgb_f* destination, int witdh, int height, float depthFocus, float depthMax, float radiusMax = ANTI_ALIASING ? 14.0f : 7.5f);
        void setDepthFocus(float focus);
        void setDepthMax(float max);
        void setRadiusMax(float radius);
        float getRadiusMax() { return radiusMax; }
        void apply();
    private:
        float depthFocus, depthMax, radiusMax;
};

class FastGlow : public PostEffect
{
    public:
        FastGlow(rgb_f *color, float *depth, rgb_f* destination, int witdh, int height, float radius = ANTI_ALIASING ? 40 : 20);
        void setRadius(float r) { radius = r; }
        float getRadius() { return radius; }
        void apply();
    private:
        float radius;
};

#endif // POSTEFFECTS_H
