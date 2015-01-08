#include "Scene.h"
#include <cstdio>
#include <string>

using namespace std;

class count_c {
    public:
        count_c(int v=0, int f=0, int l=0) : v(v),f(f),l(l) {}
        count_c& operator+=(count_c const& a) { v+=a.v; f+=a.f; l+=a.l; return *this; }
        int v,f,l;
};
count_c getNbVertexFace(Model3D const& model) {
    count_c sum;

    if(model.isLight())
        sum.l++;
    else {
        for(int i = 0; i < model.getSubModelCount(); i++)
            sum += getNbVertexFace(model.getSubModel(i));
         sum += count_c(model.getObject3D().vertexCount, model.getObject3D().facesCount);
    }
    return sum;
}

Scene::Scene(Object3D* skyBox) : skyBox(skyBox), vertexBufferSize(0), facesBufferSize(0), lightsBufferSize(0)
{
}
void Scene::addWorldModel(Model3D const& m)
{
    world.push_back(m);
    updateBufferSize(m);
}
void Scene::addBodyModel(Model3D const& m)
{
    body.push_back(m);
    updateBufferSize(m);
}
void Scene::updateBufferSize() {
    vertexBufferSize = facesBufferSize = lightsBufferSize = 0;
    for(int i = world.size(); --i >= 0;)
        updateBufferSize(world[i]);
    for(int i = body.size(); --i >= 0;)
        updateBufferSize(body[i]);

    #if defined(DEBUG) | defined(_DEBUG)
        printf("Buffer's size has changed : Scene buffer now has %d vertex, %d face(s), %d light(s)\n", vertexBufferSize, facesBufferSize, lightsBufferSize);
    #endif
}
void Scene::updateBufferSize(Model3D const& m) {
    count_c c = getNbVertexFace(m);
    vertexBufferSize += c.v;
    facesBufferSize += c.f;
    lightsBufferSize += c.l;
    vertexBuffer.resize(vertexBufferSize);
    facesBuffer.resize(facesBufferSize);
    lightsBuffer.resize(lightsBufferSize);
}

#define eq(s1,s2)   strcmp(s1,s2)==0
Scene::Scene(char *fileName, Object3DBuffer & objBuffer) : skyBox(NULL)
{
    #if defined(DEBUG) | defined(_DEBUG)
        printf("Chargement de la scene \"%s\"\n", fileName);;
    #endif
    FILE* f = fopen(fileName, "r");
    char buff[256];

    fscanf(f, "%s", buff);
    objBuffer.loadFromFile(buff);

    while(fscanf(f, "%s", buff) != EOF)
    {
        if(buff[0] == '%') {
            int c;
            while((c = fgetc(f)) != '\n' && c != EOF);
        }
        else if(eq(buff, "skybox")) {
            fscanf(f, "%s", buff);
            skyBox = &objBuffer[buff];
        }
        else if(eq(buff, "world") || eq(buff, "body")) {
            vector<Model3D> &tab = eq(buff, "body") ? body : world;

            char buffer[64];
            while(fscanf(f, "%s", buffer) != EOF) {
                if(buffer[0] == '%') {
                    int c;
                    while((c = fgetc(f)) != '\n' && c != EOF);
                }
                else if(eq(buffer, "endworld") || eq(buffer, "endbody")) {
                    break;
                }
                /// OBJET
                else if(eq(buffer, "obj")) {
                    fscanf(f, "%s", buffer);
                    tab.push_back(Model3D(&objBuffer[buffer]));
                }
                else if(eq(buffer, "t")) {
                    float x,y,z; fscanf(f, "%f%f%f", &x,&y,&z);
                    tab.back().getTransformation().translate(x,y,z);
                }
                else if(eq(buffer, "tx")) {
                    float x; fscanf(f, "%f", &x);
                    tab.back().getTransformation().translateX(x);
                }
                else if(eq(buffer, "ty")) {
                    float y; fscanf(f, "%f", &y);
                    tab.back().getTransformation().translateY(y);
                }
                else if(eq(buffer, "tz")) {
                    float z; fscanf(f, "%f", &z);
                    tab.back().getTransformation().translateZ(z);
                }
                else if(eq(buffer, "r")) {
                    float a,axis[3]; fscanf(f, "%f%f%f%f", &a,axis,axis+1,axis+2);
                    tab.back().getTransformation().rotate(a * M_PI / 180, axis);
                }
                else if(eq(buffer, "rx")) {
                    float a; fscanf(f, "%f", &a);
                    tab.back().getTransformation().rotateX(a * M_PI / 180);
                }
                else if(eq(buffer, "ry")) {
                    float a; fscanf(f, "%f", &a);
                    tab.back().getTransformation().rotateY(a * M_PI / 180);
                }
                else if(eq(buffer, "rz")) {
                    float a; fscanf(f, "%f", &a);
                    tab.back().getTransformation().rotateZ(a * M_PI / 180);
                }
                else if(eq(buffer, "s")) {
                    float s; fscanf(f, "%f", &s);
                    tab.back().getTransformation().scale(s);
                }
                else if(eq(buffer, "matrix")) {
                    Matrix matrix; float* m = matrix.mat; fscanf(f, "%f%f%f%f%f%f%f%f%f%f%f%f", m, m+1, m+2, m+3, m+4, m+5, m+6, m+7, m+8, m+9, m+10, m+11);
                    tab.back().getTransformation().transform(matrix);
                }
                else if(eq(buffer, "translucent")) {
                    tab.back().getObject3D().isVolumeTranslucent = true;
                }
                else if(eq(buffer, "shininess")) {
                    fscanf(f, "%f", &tab.back().getObject3D().shininess);
                }
                else if(eq(buffer, "emissive")) {
                    fscanf(f, "%f%f%f", &tab.back().getObject3D().emissive.r, &tab.back().getObject3D().emissive.g, &tab.back().getObject3D().emissive.b);
                }
                else if(eq(buffer, "reflect")) {
                    fscanf(f, "%f", &tab.back().getObject3D().reflect);
                }
                else if(eq(buffer, "indice")) {
                    fscanf(f, "%f", &tab.back().getObject3D().indice);
                }

                /// LIGHT
                else if(eq(buffer, "light")) {
                    fscanf(f, "%s", buffer);
                    if(eq(buffer, "spot"))
                        tab.push_back(Model3D(Light(SPOT_LIGHT)));
                    else if(eq(buffer, "sun"))
                        tab.push_back(Model3D(Light(SUN_LIGHT)));
                    else if(eq(buffer, "point"))
                        tab.push_back(Model3D(Light(POINT_LIGHT)));
                }
                else if(eq(buffer, "attenuation")) {
                    fscanf(f, "%f", &tab.back().getLight().attenuation);
                }
                else if(eq(buffer, "foggle")) {
                    float a;
                    fscanf(f, "%f", &a);
                    tab.back().getLight().foggle = a * M_PI / 180;
                    tab.back().getLight().updateFoggle();
                }
                else if(eq(buffer, "pos")) {
                    float* p = tab.back().getLight().position;
                    fscanf(f, "%f%f%f", p,p+1,p+2);
                    tab.back().getLight().hasPosition = true;
                }
                else if(eq(buffer, "dir")) {
                    float* d = tab.back().getLight().direction;
                    fscanf(f, "%f%f%f", d,d+1,d+2);
                    normalizeVector(tab.back().getLight().direction);
                    tab.back().getLight().hasDirection = true;
                }

                /// COMMUN
                else if(eq(buffer, "color")) {
                    Model3D &m = tab.back();
                    rgb_f &a = m.isLight() ? m.getLight().ambient : m.getObject3D().ambient,
                        &d = m.isLight() ? m.getLight().diffuse : m.getObject3D().diffuse,
                        &s = m.isLight() ? m.getLight().specular : m.getObject3D().specular;
                    fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
                    d = s = a;
                }
                else if(eq(buffer, "ambient")) {
                    Model3D &m = tab.back();
                    rgb_f &a = m.isLight() ? m.getLight().ambient : m.getObject3D().ambient;
                    fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
                }
                else if(eq(buffer, "diffuse")) {
                    Model3D &m = tab.back();
                    rgb_f &a = m.isLight() ? m.getLight().diffuse : m.getObject3D().diffuse;
                    fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
                }
                else if(eq(buffer, "specular")) {
                    Model3D &m = tab.back();
                    rgb_f &a = m.isLight() ? m.getLight().specular : m.getObject3D().specular;
                    fscanf(f, "%f%f%f", &a.r, &a.g, &a.b);
                }

                /// OTHER
                else if(eq(buffer, "in")) {
                    tab.back().loadFromFile(f, objBuffer);
                }
                else if(eq(buffer, "key")) {
                    fscanf(f, "%s", buffer);
                    tab.back().setKey(buffer);
                }
            }
        }
        else {
            #if defined(DEBUG) | defined(_DEBUG)
                printf("Error in scene parser : unknown word \"%s\"\n", buff);
            #endif
        }
    }

    fclose(f);
    updateBufferSize();
}

Scene::~Scene()
{
}

Model3D* Scene::getModel(const char *key) {
    Model3D *ptr = Model3D::getModel(key, world);
    if(ptr != NULL)
        return ptr;
    ptr = Model3D::getModel(key, body);
    return ptr;
}

void Scene::bufferizeSkybox(Transformation const& t)
{
    skyboxVertexBuffer.clear();
    skyboxFaceBuffer.clear();

    for(int i = 0; i < skyBox->vertex.size(); i++)
    {
        skyboxVertexBuffer.push_back(Transf_Vertex());
        Transf_Vertex &v = skyboxVertexBuffer.back();

        v.c = skyBox->vertex[i].c;
        v.s = skyBox->vertex[i].s;
        v.t = skyBox->vertex[i].t;
        t.applyTo(skyBox->vertex[i].p, v.p);
        t.applyToUnitaryVector(skyBox->vertex[i].n, v.n);
        v.objParent = (void*) skyBox;
    }

    for(int i = 0; i < skyBox->faces.size(); i++)
    {
        skyboxFaceBuffer.push_back(Drawable_Face());
        Drawable_Face &f = skyboxFaceBuffer.back();

        f.vertex_indices[0] = skyBox->faces[i].vertex_indices[0];
        f.vertex_indices[1] = skyBox->faces[i].vertex_indices[1];
        f.vertex_indices[2] = skyBox->faces[i].vertex_indices[2];
        getFaceNormal(skyboxVertexBuffer[f.vertex_indices[0]].p, skyboxVertexBuffer[f.vertex_indices[1]].p, skyboxVertexBuffer[f.vertex_indices[2]].p, f.n);
    }
}
