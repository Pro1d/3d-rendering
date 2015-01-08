#ifndef OBJECT3D_H
#define OBJECT3D_H

#include <string>
#include <map>
#include <cstring>
#include <vector>
#include "vertex.h"
#include "face.h"

class Object3D
{
    void initColor();
    public:
        Object3D(bool translucent=false);
        Object3D(const char* filename, bool translucent=false);
        ~Object3D() { if(texture != NULL) delete texture; }
        bool loadFromFile(const char* fileName);
        void loadTexture(const char* fileName);

        rgb_f const& getTexturePoint(float s, float t) const;
        //void updateFaceNormal();
        int vertexCount, facesCount;
        std::vector<Vertex> vertex;
        std::vector<Face> faces;
        rgb_f ambient, diffuse, specular, emissive;
        float shininess;
        float indice; /// indice de refraction
        float reflect; /// coeficient de réflexion
        bool isVolumeTranslucent;
        char name[100];
        float AABB[6];
        float centerBS[4], radiusBS; // Bounding Sphere
        bool hasTexture;
    private:
        bool hasColor;
        rgb_f *texture;
        int textWidth, textHeight;
};

class Object3DBuffer {
    public:
        Object3DBuffer();
        void loadFromFile(const char* fileName);
        ~Object3DBuffer();
        Object3D& get(std::string name);
        Object3D& operator[](std::string name) {return get(name); };
        void loadObject(std::string name, const char* fileName);
    private:
        std::map<std::string, Object3D*> hashmap;
};

#endif // OBJECT3D_H
