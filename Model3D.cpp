#include <cstdio>
#include "Model3D.h"
#include "Transformation.h"


Model3D::Model3D(Light light) : isLight_(true), light(light)
{
    setKey(DEFAULT_KEY);
}
Model3D::Model3D(Light light, Transformation const& mat) : mat(mat), isLight_(true), light(light)
{
    setKey(DEFAULT_KEY);
}
Model3D::Model3D(Object3D* obj3d) : obj3d(obj3d), isLight_(false)
{
    setKey(DEFAULT_KEY);
}
Model3D::Model3D(Object3D* obj3d, Transformation const& mat) : obj3d(obj3d), mat(mat), isLight_(false)
{
    setKey(DEFAULT_KEY);
}

Model3D::~Model3D()
{
    //dtor
}

#define eq(s1,s2)   strcmp(s1,s2)==0
void Model3D::loadFromFile(FILE* f, Object3DBuffer & objBuffer) {
    // {obj/light name} dans étape précédante
    // {in}
    char buffer[64];
    while(fscanf(f, "%s", buffer) != EOF) {
        /// OBJET
        if(buffer[0] == '%') {
            int c;
            while((c = fgetc(f)) != '\n' && c != EOF);
        }
        else if(eq(buffer, "obj")) {
            fscanf(f, "%s", buffer);
            addSubModel(&objBuffer[buffer]);
        }
        else if(eq(buffer, "t")) {
            float x,y,z; fscanf(f, "%f%f%f", &x,&y,&z);
            subModel.back().mat.translate(x,y,z);
        }
        else if(eq(buffer, "tx")) {
            float x; fscanf(f, "%f", &x);
            subModel.back().mat.translateX(x);
        }
        else if(eq(buffer, "ty")) {
            float y; fscanf(f, "%f", &y);
            subModel.back().mat.translateY(y);
        }
        else if(eq(buffer, "tz")) {
            float z; fscanf(f, "%f", &z);
            subModel.back().mat.translateZ(z);
        }
        else if(eq(buffer, "r")) {
            float a,axis[3]; fscanf(f, "%f%f%f%f", &a,axis,axis+1,axis+2);
            subModel.back().mat.rotate(a * M_PI / 180, axis);
        }
        else if(eq(buffer, "rx")) {
            float a; fscanf(f, "%f", &a);
            subModel.back().mat.rotateX(a * M_PI / 180);
        }
        else if(eq(buffer, "ry")) {
            float a; fscanf(f, "%f", &a);
            subModel.back().mat.rotateY(a * M_PI / 180);
        }
        else if(eq(buffer, "rz")) {
            float a; fscanf(f, "%f", &a);
            subModel.back().mat.rotateZ(a * M_PI / 180);
        }
        else if(eq(buffer, "s")) {
            float s; fscanf(f, "%f", &s);
            subModel.back().mat.scale(s);
        }
        else if(eq(buffer, "matrix")) {
            Matrix matrix; float* m = matrix.mat; fscanf(f, "%f%f%f%f%f%f%f%f%f%f%f%f", m, m+1, m+2, m+3, m+4, m+5, m+6, m+7, m+8, m+9, m+10, m+11);
            subModel.back().mat.transform(matrix);
        }
        else if(eq(buffer, "translucent")) {
            subModel.back().obj3d->isVolumeTranslucent = true;
        }
        else if(eq(buffer, "shininess")) {
            fscanf(f, "%f", &subModel.back().obj3d->shininess);
        }
        else if(eq(buffer, "emissive")) {
            fscanf(f, "%f%f%f", &subModel.back().obj3d->emissive.r, &subModel.back().obj3d->emissive.g, &subModel.back().obj3d->emissive.b);
        }
        else if(eq(buffer, "reflect")) {
            fscanf(f, "%f", &subModel.back().obj3d->reflect);
        }
        else if(eq(buffer, "indice")) {
            fscanf(f, "%f", &subModel.back().obj3d->indice);
        }

        /// LIGHT
        else if(eq(buffer, "light")) {
            fscanf(f, "%s", buffer);
            if(eq(buffer, "spot"))
                addSubLight(Light(SPOT_LIGHT));
            else if(eq(buffer, "sun"))
                addSubLight(Light(SUN_LIGHT));
            else if(eq(buffer, "point"))
                addSubLight(Light(POINT_LIGHT));
        }
        else if(eq(buffer, "attenuation")) {
            fscanf(f, "%f", &subModel.back().light.attenuation);
        }
        else if(eq(buffer, "foggle")) {
            float a;
            fscanf(f, "%f", &a);
            subModel.back().light.foggle = a * M_PI / 180;
            subModel.back().light.updateFoggle();
        }
        else if(eq(buffer, "pos")) {
            float* p = subModel.back().light.position;
            fscanf(f, "%f%f%f", p,p+1,p+2);
            subModel.back().light.hasPosition = true;
        }
        else if(eq(buffer, "dir")) {
            float* d = subModel.back().light.direction;
            fscanf(f, "%f%f%f", d,d+1,d+2);
            subModel.back().light.hasDirection = true;
        }

        /// COMMUN
        else if(eq(buffer, "color")) {
            Model3D &m = subModel.back();
            rgb_f &a = m.isLight() ? m.light.ambient : m.obj3d->ambient,
                &d = m.isLight() ? m.light.diffuse : m.obj3d->diffuse,
                &s = m.isLight() ? m.light.specular : m.obj3d->specular;
            fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
            d = s = a;
        }
        else if(eq(buffer, "ambient")) {
            Model3D &m = subModel.back();
            rgb_f &a = m.isLight() ? m.light.ambient : m.obj3d->ambient;
            fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
        }
        else if(eq(buffer, "diffuse")) {
            Model3D &m = subModel.back();
            rgb_f &a = m.isLight() ? m.light.diffuse : m.obj3d->diffuse;
            fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
        }
        else if(eq(buffer, "specular")) {
            Model3D &m = subModel.back();
            rgb_f &a = m.isLight() ? m.light.specular : m.obj3d->specular;
            fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
        }

        /// OTHER
        else if(eq(buffer, "in")) {
            subModel.back().loadFromFile(f, objBuffer);
        } else if(eq(buffer, "out")) {
            return;
        }
        else if(eq(buffer, "key")) {
            fscanf(f, "%s", buffer);
            subModel.back().setKey(buffer);
        }
    }
    //      obj/light params
    //      ..
    //      in <- recc
    // out
}

void Model3D::addSubModel(Object3D* obj3d, Transformation const& mat) {
    subModel.push_back(Model3D(obj3d, mat));
}
void Model3D::addSubModel(Object3D* obj3d, float x, float y, float z) {
    addSubModel(obj3d, Transformation(1,0,0,x, 0,1,0,y, 0,0,1,z, 0,0,0,1));
}
void Model3D::addSubLight(Light light, Transformation const& mat) {
    subModel.push_back(Model3D(light, mat));
}
void Model3D::addSubLight(Light light, float x, float y, float z) {
    addSubLight(light, Transformation(1,0,0,x, 0,1,0,y, 0,0,1,z, 0,0,0,1));
}
