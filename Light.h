#ifndef LIGHT_H
#define LIGHT_H

#include <cmath>
#include "Object3D.h"
#include "color.h"
#include "vertex.h"
#define INNER_COEF	0.5f
enum {POINT_LIGHT, SPOT_LIGHT, SUN_LIGHT};
class Light
{
    public:
        Light(int type, rgb_f const& color);
        Light(int type=-1);
        virtual ~Light();

        void copyLight(Light const& src);

        void setAmbientColor(float r, float g, float b) { ambient.r = r; ambient.g = g; ambient.b = b; }
        void setDiffuseColor(float r, float g, float b) { diffuse.r = r; diffuse.g = g; diffuse.b = b; }
        void setSpecularColor(float r, float g, float b) { specular.r = r; specular.g = g; specular.b = b; }
        void setAttenuation(float a) { attenuation = a; }

        static rgb_f getColor(Light const& light, Object3D const& obj, const float *pos, const float *normal, rgb_f const& color);
        static rgb_f getColor(Light const& light, Object3D const& obj, Vertex const& v) { return getColor(light, obj, v.p, v.n, v.c); }
        virtual void getRay(const float *vertex, float &rayIntesity, float *rayDirection) const;
        void getDirectionFromPoint(const float *pos, float *dir) const;
        float getDistanceToPoint(const float* pos) const;
        void getSpecularAndDiffuseCoef(const float *p, const float *n, float* out, float shininess) const;
        void getSpecularAndDiffuseCoef(Vertex const& v, float* out, float shininess) const { getSpecularAndDiffuseCoef(v.p, v.n, out, shininess); }
        void updateFoggle() { cos_foggle = cos(foggle); cos_foggleInner = cos(foggle*INNER_COEF); }

        bool hasDirection, hasPosition;
        float direction[4]; // Sun, Spot
        float position[4]; // Point, Spot
        rgb_f ambient, diffuse, specular;
        float attenuation;
        float foggle, cos_foggle, cos_foggleInner;
    protected:
        int type;
    private:
};

class SunLight : public Light
{
    public:
        SunLight(rgb_f const& color, float dx, float dy, float dz);
        virtual void getRay(const float *vertex, float &rayIntesity, float *rayDirection) const;
        static void getRay(Light const& l, const float *vertex, float &rayIntesity, float *rayDirection);
    private:
};
class PointLight : public Light
{
    public:
        PointLight(rgb_f const& color, float x=0, float y=0, float z=0);
        virtual void getRay(const float *vertex, float &rayIntesity, float *rayDirection) const;
        static void getRay(Light const& l, const float *vertex, float &rayIntesity, float *rayDirection);
    private:
};
class SpotLight : public Light
{
    public:
        SpotLight(rgb_f const& color, float x, float y, float z, float dx, float dy, float dz, float foggle);
        virtual void getRay(const float *vertex, float &rayIntesity, float *rayDirection) const;
        static void getRay(Light const& l, const float *vertex, float &rayIntesity, float *rayDirection);
    private:
};

#endif // LIGHT_H
