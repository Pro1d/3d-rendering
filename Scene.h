#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include "Model3D.h"
#include "Object3D.h"
#include "Light.h"

class Scene
{
    public:
        Scene(Object3D* skyBox = NULL);
        Scene(const char *fileName, Object3DBuffer & objBuffer);

        ~Scene();
        bool hasSkyBox() const { return skyBox != NULL; }
        Object3D const& getSkyBox() const { return *skyBox; }

        std::vector<Model3D> & getWorld() { return world; }
        std::vector<Model3D> & getBody() { return body; }
        Model3D* getModel(const char *key);
        void addWorldModel(Model3D const& m);
        void addBodyModel(Model3D const& m);
        void updateBufferSize(Model3D const& m);
        void updateBufferSize();

        void clearBuffer() { vertexCount = facesCount = lightsCount = 0; }

        Transf_Vertex& nextVertexBuffer() { return vertexBuffer[vertexCount++]; }
        Drawable_Face& nextFaceBuffer() { return facesBuffer[facesCount++]; }
        Light& nextLightBuffer() { return lightsBuffer[lightsCount++]; }

        inline Transf_Vertex& getVertex(int i) { return vertexBuffer[i]; }
        inline Transf_Vertex const& getVertex(int i) const { return vertexBuffer[i]; }
        std::vector<Transf_Vertex> const& getVertexArray() const { return vertexBuffer; }
        inline int getVertexCount() const { return vertexCount; }
        inline Drawable_Face& getFace(int i) { return facesBuffer[i]; }
        inline Drawable_Face const& getFace(int i) const { return facesBuffer[i]; }
        inline int getFacesCount() const { return facesCount; }
        inline Light const& getLight(int i) const { return lightsBuffer[i]; }
        inline int getLightsCount() const { return lightsCount; }
        void updateLightsAmbientSum() {
            lightAmbient_Sum=rgb_f();
            for(int i = lightsBufferSize; --i >= 0;)
                lightAmbient_Sum += lightsBuffer[i].ambient;
        }
        inline rgb_f const& getLightsAmbientSum() const { return lightAmbient_Sum; }

        void bufferizeSkybox(Transformation const& t);

        std::vector<Transf_Vertex> const& getSkyboxVertexBuffer() const { return skyboxVertexBuffer; }
        std::vector<Drawable_Face> const& getSkyboxFaceBuffer() const { return skyboxFaceBuffer; }
    protected:
    private:
        /// Models 3D
        std::vector<Model3D> world;
        std::vector<Model3D> body;

        /// Skybox
        Object3D* skyBox;
        /// Lights array
        int lightsBufferSize;
        std::vector<Light> lightsBuffer;
        rgb_f lightAmbient_Sum;
        /// Vertex and faces buffer
        int vertexBufferSize, facesBufferSize;
        std::vector<Transf_Vertex> vertexBuffer;
        std::vector<Drawable_Face> facesBuffer;

        std::vector<Transf_Vertex> skyboxVertexBuffer;
        std::vector<Drawable_Face> skyboxFaceBuffer;

        int vertexCount, facesCount, lightsCount;

};

#endif // SCENE_H
