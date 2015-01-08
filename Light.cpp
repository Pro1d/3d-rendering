#include "Light.h"
#include "color.h"
#include "Object3D.h"
#include "vertex.h"
#include "tools.h"
#include <cmath>
#include <cfloat>
#include <cstring>
#include <cstdio>

#define RAY_INTENSITY_MIN   0.01f

using std::max;

Light::Light(int type, rgb_f const& color) : hasDirection(false), attenuation(0), hasPosition(false), type(type)
{
    memcpy(&ambient, &color, sizeof(rgb_f));
    memcpy(&diffuse, &color, sizeof(rgb_f));
    memcpy(&specular, &color, sizeof(rgb_f));
}
Light::Light(int type) : attenuation(0), hasDirection(false), hasPosition(false), type(type)
{
    float k = 0.5f;
    ambient.r = k; ambient.g = k; ambient.b = k;
    diffuse.r = 1-k; diffuse.g = 1-k; diffuse.b = 1-k;
    specular.r = 1; specular.g = 1; specular.b = 1;
}
Light::~Light()
{

}
void Light::copyLight(Light const& src)
{
    memcpy(&ambient, &src.ambient, sizeof(rgb_f));
    memcpy(&diffuse, &src.diffuse, sizeof(rgb_f));
    memcpy(&specular, &src.specular, sizeof(rgb_f));
    attenuation = src.attenuation;
    hasDirection = src.hasDirection;
    hasPosition = src.hasPosition;
    type = src.type;
    foggle = src.foggle;
    cos_foggle = src.cos_foggle;
    cos_foggleInner = src.cos_foggleInner;
}
void Light::getRay(const float *vertex, float &rayIntesity, float *rayDirection) const
{
    rayIntesity = 0;
    rayDirection[0] = rayDirection[1] = rayDirection[2] = 0;
}

void Light::getDirectionFromPoint(const float *pos, float *dir) const {
    if(type == POINT_LIGHT || type == SPOT_LIGHT)
    {
        dir[0] = position[0] - pos[0];
        dir[1] = position[1] - pos[1];
        dir[2] = position[2] - pos[2];
        normalizeVector(dir);
    }
    else if(type == SUN_LIGHT)
    {
        dir[0] = -direction[0];
        dir[1] = -direction[1];
        dir[2] = -direction[2];
    }
}
float Light::getDistanceToPoint(const float* pos) const {
    if(type == POINT_LIGHT || type == SPOT_LIGHT)
    {
        float d[3] = {
            pos[0]-position[0],
            pos[1]-position[1],
            pos[2]-position[2]
        };
        return 1.0f / magic_sqrt_inv(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
    }
    else
        return FLT_MAX;
}

/// Static method
rgb_f Light::getColor(Light const& light, Object3D const& obj, const float *pos, const float *normal, rgb_f const& color)
{
    float rayDir[3];
    float rayIntensity;
    light.getRay(pos, rayIntensity, rayDir);

    /// Produit scalaire : normal . rayDirection
    float dotNL = max(normal[0]*(-rayDir[0]) + normal[1]*(-rayDir[1]) + normal[2]*(-rayDir[2]), 0.0f);

    /// Intensité lumière spéculaire
    float spec = 0.0f;
    if(dotNL > 0.0f) {
        /// Vecteur médiant des deux vecteurs normal et rayDirection
        float halfway[3] = {
            (-rayDir[0]) + normal[0],
            (-rayDir[1]) + normal[1],
            (-rayDir[2]) + normal[2]
        };
        float normHW = magic_sqrt_inv(halfway[0]*halfway[0] + halfway[1]*halfway[1] + halfway[2]*halfway[2]);

        float dotNH = max(halfway[0]*normal[0] + halfway[1]*normal[1] + halfway[2]*normal[2], 0.0f);
        spec = pow(dotNH * normHW, obj.shininess);
    }
    /// c = VertexColor * (ObjAmb * LightAmb + dotNL * ObjDif * LightDif * rayIntensity) + ObjSpec * LightSpec * spec * rayIntensity + ObjEmi
    return rgb_f(
        clampIn01(color.r * (obj.ambient.r * light.ambient.r + dotNL * obj.diffuse.r * light.diffuse.r * rayIntensity + obj.emissive.r) + (spec * obj.specular.r * light.specular.r * rayIntensity)),
        clampIn01(color.g * (obj.ambient.g * light.ambient.g + dotNL * obj.diffuse.g * light.diffuse.g * rayIntensity + obj.emissive.g) + (spec * obj.specular.g * light.specular.g * rayIntensity)),
        clampIn01(color.b * (obj.ambient.b * light.ambient.b + dotNL * obj.diffuse.b * light.diffuse.b * rayIntensity + obj.emissive.b) + (spec * obj.specular.b * light.specular.b * rayIntensity))
    );
}

void Light::getSpecularAndDiffuseCoef(const float *p, const float *n, float* out, float shininess) const
{
    float rayDir[3];
    float rayIntensity;
    if(type == POINT_LIGHT)
        PointLight::getRay(*this, p, rayIntensity, rayDir);
    else if(type == SPOT_LIGHT)
        SpotLight::getRay(*this, p, rayIntensity, rayDir);
    else if(type == SUN_LIGHT)
        SunLight::getRay(*this, p, rayIntensity, rayDir);

    /// Optimisation / simplification
    if(rayIntensity < RAY_INTENSITY_MIN) {
        out[0] = 0;
        out[1] = 0;
        return;
    }

    /// Produit scalaire : normal . rayDirection
    float dotNL = max(n[0]*(-rayDir[0]) + n[1]*(-rayDir[1]) + n[2]*(-rayDir[2]), 0.0f);

     /// Intensité lumière spéculaire
    float spec = 0.0f;
    if(dotNL > 0.0f) {
        /// Vecteur du rayon réfléchi
        float reflect[3] = {
            rayDir[0] + 2 * dotNL * n[0],
            rayDir[1] + 2 * dotNL * n[1],
            rayDir[2] + 2 * dotNL * n[2]
        };
        float normReflect = magic_sqrt_inv(reflect[0]*reflect[0] + reflect[1]*reflect[1] + reflect[2]*reflect[2]);
        float normEye = magic_sqrt_inv(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
        float dotReflectEye = reflect[0]*p[0] + reflect[1]*p[1] + reflect[2]*p[2];
        spec = pow(dotReflectEye * normEye * normReflect, shininess);
    }

    out[0] = rayIntensity * dotNL;
    out[1] = rayIntensity * spec;
}

/// SUN LIGHT
SunLight::SunLight(rgb_f const& color, float dx, float dy, float dz) : Light(SUN_LIGHT, color)
{
    hasDirection = true;

    direction[0] = dx;
    direction[1] = dy;
    direction[2] = dz;
    direction[3] = 1;

    normalizeVector(direction);
}
void SunLight::getRay(const float *vertex, float &rayIntensity, float *rayDirection) const
{
    memcpy(rayDirection, direction, sizeof(float)*3);
    rayIntensity = 1.0f;
}
void SunLight::getRay(Light const& l, const float *vertex, float &rayIntensity, float *rayDirection)
{
    memcpy(rayDirection, l.direction, sizeof(float)*3);
    rayIntensity = 1.0f;
}

/// POINT LIGHT
PointLight::PointLight(rgb_f const& color, float x, float y, float z) : Light(POINT_LIGHT, color)
{
    hasPosition = true;

    position[0] = x;
    position[1] = y;
    position[2] = z;
    position[3] = 1;
}
void PointLight::getRay(const float *vertex, float &rayIntensity, float *rayDirection) const
{
    rayDirection[0] = vertex[0]-position[0];
    rayDirection[1] = vertex[1]-position[1];
    rayDirection[2] = vertex[2]-position[2];
    float dist_2 = rayDirection[0]*rayDirection[0] + rayDirection[1]*rayDirection[1] + rayDirection[2]*rayDirection[2];

    rayIntensity = 1 / (1 + dist_2 * attenuation);

    /// Optimisation / Simplification
    if(rayIntensity < RAY_INTENSITY_MIN)
        return;

    float n = (dist_2 == 0) ? 1 : magic_sqrt_inv(dist_2);

    rayDirection[0] *= n;
    rayDirection[1] *= n;
    rayDirection[2] *= n;
}
void PointLight::getRay(Light const& l, const float *vertex, float &rayIntensity, float *rayDirection)
{
    rayDirection[0] = vertex[0]-l.position[0];
    rayDirection[1] = vertex[1]-l.position[1];
    rayDirection[2] = vertex[2]-l.position[2];
    float dist_2 = rayDirection[0]*rayDirection[0] + rayDirection[1]*rayDirection[1] + rayDirection[2]*rayDirection[2];

    rayIntensity = 1 / (1 + dist_2 * l.attenuation);

    /// Optimisation / Simplification
    if(rayIntensity < RAY_INTENSITY_MIN)
        return;

    float n = (dist_2 == 0) ? 1 : magic_sqrt_inv(dist_2);

    rayDirection[0] *= n;
    rayDirection[1] *= n;
    rayDirection[2] *= n;
}

/// SPOT LIGHT
SpotLight::SpotLight(rgb_f const& color, float x, float y, float z, float dx, float dy, float dz, float _foggle) : Light(SPOT_LIGHT, color)
{
    hasDirection = true;
    hasPosition = true;
    position[0] = x;
    position[1] = y;
    position[2] = z;
    position[3] = 1;

    direction[0] = dx;
    direction[1] = dy;
    direction[2] = dz;
    direction[3] = 1;

    foggle = _foggle;
    updateFoggle();

    normalizeVector(direction);
}
void SpotLight::getRay(const float *vertex, float &rayIntensity, float *rayDirection) const
{
    rayDirection[0] = vertex[0]-position[0];
    rayDirection[1] = vertex[1]-position[1];
    rayDirection[2] = vertex[2]-position[2];
    float dist_2 = rayDirection[0]*rayDirection[0] + rayDirection[1]*rayDirection[1] + rayDirection[2]*rayDirection[2];

    rayIntensity = 1 / (1 + dist_2 * attenuation);

    /// Optimisation / Simplification
    if(rayIntensity < RAY_INTENSITY_MIN)
        return;

    float n = (dist_2 == 0) ? 1 : magic_sqrt_inv(dist_2);

    rayDirection[0] *= n;
    rayDirection[1] *= n;
    rayDirection[2] *= n;

    float RayDotDir = clampIn01(rayDirection[0]*direction[0] + rayDirection[1]*direction[1] + rayDirection[2]*direction[2]);
    if(RayDotDir <= cos_foggle) { rayIntensity = 0; return; }
    float intensity = (RayDotDir >= cos_foggleInner) ? 1 : smoothStep(-foggle, -foggle*INNER_COEF, -acos(RayDotDir));

    rayIntensity *= intensity;
}
void SpotLight::getRay(Light const& l, const float *vertex, float &rayIntensity, float *rayDirection)
{
    rayDirection[0] = vertex[0]-l.position[0];
    rayDirection[1] = vertex[1]-l.position[1];
    rayDirection[2] = vertex[2]-l.position[2];
    float dist_2 = rayDirection[0]*rayDirection[0] + rayDirection[1]*rayDirection[1] + rayDirection[2]*rayDirection[2];

    rayIntensity = 1 / (1 + dist_2 * l.attenuation);

    /// Optimisation / Simplification
    if(rayIntensity < RAY_INTENSITY_MIN)
        return;

    float n = (dist_2 == 0) ? 1 : magic_sqrt_inv(dist_2);

    rayDirection[0] *= n;
    rayDirection[1] *= n;
    rayDirection[2] *= n;

    float RayDotDir = clampIn01(rayDirection[0]*l.direction[0] + rayDirection[1]*l.direction[1] + rayDirection[2]*l.direction[2]);
    if(RayDotDir <= l.cos_foggle) { rayIntensity = 0; return; }
    float intensity = (RayDotDir >= l.cos_foggleInner) ? 1 : smoothStep(-l.foggle, -l.foggle*INNER_COEF, -acos(RayDotDir));

    rayIntensity *= intensity;
}

