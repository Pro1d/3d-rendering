#ifndef MODEL3D_H
#define MODEL3D_H

#include "Object3D.h"
#include "Light.h"
#include "Transformation.h"
#include <vector>
#include <cstring>

#define DEFAULT_KEY "#defaultkey"

class Model3D
{
    public:
        Model3D(Light light, Transformation const& mat);
        Model3D(Light light);
        Model3D(Object3D* obj3d);
        Model3D(Object3D* obj3d, Transformation const& mat);
        ~Model3D();

        Transformation const& getTransformation() const { return mat; }
        Transformation & getTransformation() { return mat; }

        Object3D const& getObject3D() const { return *obj3d; }
        Object3D & getObject3D() { return *obj3d; }

        bool isLight() const { return isLight_; }
        Light const& getLight() const { return light; }
        Light & getLight() { return light; }

        Model3D const& getSubModel(int i) const { return subModel[i]; }
        Model3D& getSubModel(int i) { return subModel[i]; }

        int getSubModelCount() const { return subModel.size(); }
        void addSubModel(Object3D* obj3d, Transformation const& mat);
        void addSubModel(Object3D* obj3d, float x=0, float y=0, float z=0);

        void addSubLight(Light light, Transformation const& mat);
        void addSubLight(Light light, float x=0, float y=0, float z=0);

        void loadFromFile(FILE* f, Object3DBuffer & objBuffer);
        void setKey(const char *s) { strcpy(key, s); }
        static Model3D* getModel(const char* key, std::vector<Model3D> &list) {
            for(std::vector<Model3D>::iterator it = list.begin(); it != list.end(); ++it)
            {
                if(strcmp(it->key, key) == 0)
                    return &(*it);
                Model3D *ptr = Model3D::getModel(key, it->subModel);
                if(ptr != NULL)
                    return ptr;
            }
            return NULL;
        }
        char key[50];
    protected:
    private:
        Object3D* obj3d;
        Transformation mat;
        std::vector<Model3D> subModel;
        bool isLight_;
        Light light;
};

#endif // MODEL3D_H
