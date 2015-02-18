#include <cmath>
#include <algorithm>
#include <climits>
#include <ctime>
#include <queue>
#include <cfloat>
#include <SDL.h>

#include "Engine3D.h"
#include "Object3D.h"
#include "Transformation.h"
#include "Light.h"
#include "tools.h"
#include "MultiThread.h"


using namespace std;

Engine3D::Engine3D(SDL_Surface* s) : mTh(THREAD_COUNT)
{
    src = s;
    width = src->w;
    height = src->h;
    pixelCount = width * height;

    screenWidth = width;
    screenHeight = height;
    screenRatio = screenHeight / screenWidth;

    fov = 70.0f; // 70°
    updateZNearFar();
    zFar = zNear + (screenHeight + screenWidth)*4.0f*2;

    enableFog = false;
    fogNear = zFar - (zFar-zNear) * 0.5f;
    fogFar = zFar;
    fogColor.r = 0.04;
    fogColor.g = 0.04;
    fogColor.b = 0.04;

    xMin = 0.0f;
    xMax = screenWidth;
    yMin = 0.0f;
    yMax = screenHeight;

    enableDepthOfField = false;
    zDoFFocus = screenHeight*0.25 + zNear;
    maxRadiusDoF = 7.5f;
    gradientRadiusDoF = 0.75f;
    speedChangeFocusDoF = 0.65;
    pixelFocusIndex = width/2 + width*(height/2);

    enableGlow = false;
    glowRadius = 20.0f;
    minLuminosityGlow = 0.4f;

    enableBlurMotion = false;
    speedBlurMotion = 100;

    depthBuf = new float[s->w * s->h];
    colorBuf = new rgb_f[s->w * s->h];
    colorBuf_f = new rgb_f[s->w * s->h];
    colorBuf_g = new rgb_f[s->w * s->h];

    volumeDepthBuf = new priority_queue<PixelVolume, vector<PixelVolume>, PixelVolumeComparison>[s->w * s->h];
    enableVolumeRendering = false;
    volumeColor.r = 0.4;
    volumeColor.g = 0.8;
    volumeColor.b = 0.95;
    /// coéficient d'opacité > 0 (0=opaque) -> opacité = exp(-épaisseur * coéficient d'opacité)
    volumeColor.s = 0.5f / 180.0f;

    enableDrawingFilter = false;

    cam.translate(0,-3,-3, ABS);

    mRT.setParams(width, height, zNear);
    mRT.setColorBuf(colorBuf);
    mRT.setDepthBuf(depthBuf);
    enableRayTracing = false;
}

Engine3D::~Engine3D()
{
    delete[] depthBuf;
    delete[] colorBuf;
    delete[] colorBuf_f;
    delete[] colorBuf_g;
    delete[] volumeDepthBuf;
}

void Engine3D::drawObject(Object3D const& obj, bool enablePerspective) // private
{
    #ifdef DEBUG
        printf("drawing \"%s\" : %d points %d triangles\n", obj.name, obj.vertexCount, obj.facesCount);
    #endif
    if(obj.isVolumeTranslucent)
        enableVolumeRendering = true;

    // TODO vector -> reserve/capacity
    float p[obj.vertexCount][4];
    bool v[obj.vertexCount][6]; /// {0<x, x<w, y<0, y<h, z<near, z>far}
    float normal[obj.vertexCount][4];

    /// Traitement des vertex
    for(int i = 0; i < obj.vertexCount; i++) {
        /// Transformation de chaque vertex
        transformation.applyTo(obj.vertex[i].p, p[i]);
        if(enablePerspective)
            Transformation::applyPerspectiveTo(p[i], screenWidth, screenHeight, zNear, p[i]);
        else
            p[i][1] = screenHeight - p[i][1];

        /// Position des vertex par rapport à l'espace visible
        v[i][0] = p[i][0] < 0; // gauche
        v[i][1] = p[i][0] >= screenWidth; // droite
        v[i][2] = p[i][1] < 0; // haut
        v[i][3] = p[i][1] >= screenHeight; // bas
        v[i][4] = -p[i][2] < zNear; // devant
        v[i][5] = -p[i][2] > zFar; // derrière

        /// Calcul des normales
        transformation.applyToUnitaryVector(obj.vertex[i].n, normal[i]);
    }

    /// Traitement des surfaces
    for(int i = 0; i < obj.facesCount; i++) {
        int i1 = obj.faces[i].vertex_indices[0], i2 = obj.faces[i].vertex_indices[1], i3 = obj.faces[i].vertex_indices[2];

        /// Surface non affichée si tous les points sont d'un même côté
        if(!(v[i1][0] + v[i2][0] + v[i3][0] != 3
           && v[i1][1] + v[i2][1] + v[i3][1] != 3
           && v[i1][2] + v[i2][2] + v[i3][2] != 3
           && v[i1][3] + v[i2][3] + v[i3][3] != 3
           && v[i1][4] + v[i2][4] + v[i3][4] != 3
           && v[i1][5] + v[i2][5] + v[i3][5] != 3))
           continue;

        /// Objet translucide
        if(obj.isVolumeTranslucent)
        {
            triangleVolume(p[i1], p[i2], p[i3],
                     obj.vertex[i3].c, isOrientZ(p[i1], p[i2], p[i3]));
        }
        /// Objet opaque; face visible en fonction de son orientation
        else if(!isOrientZ(p[i1], p[i2], p[i3]))
            triangle(colorBuf, depthBuf, p[i1], p[i2], p[i3], obj.vertex[i1].c, obj.vertex[i2].c, obj.vertex[i3].c);
    }
}

void Engine3D::draw(Model3D const& model) // private
{
    transformation.push();

    /// Apply the model's transformation
    transformation.transform(model.getTransformation().getMatrix(), PRE);

    if(model.isLight())
    {

    }
    else
    {
        /// draw main object
        drawObject(model.getObject3D());

        /// Draw sub model
        for(int i = 0; i < model.getSubModelCount(); i++)
            draw(model.getSubModel(i));
    }
    transformation.pop();
}

void Engine3D::objectToBuffer(Object3D const& obj, Scene& sceneBuffer)
{
    #if defined(DEBUG)
        printf("Buffering \"%s\" : %d vertex, %d faces\n", obj.name, obj.vertexCount, obj.facesCount);
    #endif

    /// TODO quand on regarde vers le haut -> cage traité
    //* TODO bool isNotVisible(Object3D const& obj, Transformation const& transformation)* / {
    if(!enableRayTracing) {
        float center[4], radius = transformation.getMaxGlobalScale() * obj.radiusBS;
        transformation.applyTo(obj.centerBS, center);
        float v1[4]={center[0]+radius, center[1]+radius, center[2]-radius/*z min pour grossissment max*/, center[3]};
        float v2[4]={center[0]-radius, center[1]-radius, center[2]-radius, center[3]};
        Transformation::applyPerspectiveTo(v1, screenWidth, screenHeight, zNear, v1);
        Transformation::applyPerspectiveTo(v2, screenWidth, screenHeight, zNear, v2);
        v1[2] += 2*radius;
        #if defined(DEBUG)
            printf("center %f %f %f r %f\n", center[0], center[1], center[2], radius);
            printf("v1 %f %f %f\n", v1[0], v1[1], v1[2]);
            printf("v2 %f %f %f\n", v2[0], v2[1], v2[2]);
            printf("gauche   %d\n", v1[0] < 0);
            printf("droite   %d\n", v2[0] >= screenWidth);
            printf("haut     %d\n", v2[1] < 0);
            printf("bas      %d\n", v1[1] >= screenHeight);
            printf("devant   %d\n", -v2[2] < zNear);
            printf("derrière %d\n", -v1[2] > zFar);
        #endif
        if(v1[0] < 0           // gauche
        || v2[0] >= screenWidth   // droite
        || v2[1] < 0             // haut
        || v1[1] >= screenHeight// bas
        || -v2[2] < zNear       // devant
        || -v1[2] > zFar       // derrière
        ) return;
    }
    // }*/
    if(obj.isVolumeTranslucent)
        enableVolumeRendering = true;

    int vertexOffset = sceneBuffer.getVertexCount();
    /// Traitement des vertex
    for(int i = 0; i < obj.vertexCount; i++) {
        Transf_Vertex &vertex = sceneBuffer.nextVertexBuffer();
        vertex.c = obj.vertex[i].c;
        vertex.s = obj.vertex[i].s;
        vertex.t = obj.vertex[i].t;

        /// Transformation de chaque vertex
        transformation.applyTo(obj.vertex[i].p, vertex.p);
        if(!enableRayTracing) {
            Transformation::applyPerspectiveTo(vertex.p, screenWidth, screenHeight, zNear, vertex.onScreenCoord);

            /// Position des vertex par rapport à l'espace visible
            vertex.left__ = vertex.onScreenCoord[0] < 0;                // gauche
            vertex.right_ = vertex.onScreenCoord[0] >= screenWidth;    // droite
            vertex.up____ = vertex.onScreenCoord[1] < 0;              // haut
            vertex.down__ = vertex.onScreenCoord[1] >= screenHeight; // bas
            vertex.front_ = -vertex.onScreenCoord[2] < zNear;       // devant
            vertex.behind = -vertex.onScreenCoord[2] > zFar;       // derrière

            vertex.isUsed = false; // par défaut un vertex n'est pas utilisé
        }
        /// Calcul des normales
        transformation.applyToUnitaryVector(obj.vertex[i].n, vertex.n);

        /// Calcul de la couleur
        vertex.objParent = (void*) &obj;
    }

    /// Traitement des surfaces
    for(int i = 0; i < obj.facesCount; i++) {
        /// Index des vertex de la face avec l'offset de l'objet actuel
        int i1 = obj.faces[i].vertex_indices[0] + vertexOffset,
            i2 = obj.faces[i].vertex_indices[1] + vertexOffset,
            i3 = obj.faces[i].vertex_indices[2] + vertexOffset;

        Transf_Vertex & v1 = sceneBuffer.getVertex(i1);
        Transf_Vertex & v2 = sceneBuffer.getVertex(i2);
        Transf_Vertex & v3 = sceneBuffer.getVertex(i3);

        if(!enableRayTracing)
        /// Surface non affichée si tous les points sont d'un même côté
        if(v1.left__ + v2.left__ + v3.left__ == 3
        || v1.right_ + v2.right_ + v3.right_ == 3
        || v1.up____ + v2.up____ + v3.up____ == 3
        || v1.down__ + v2.down__ + v3.down__ == 3
        || v1.front_ + v2.front_ + v3.front_ == 3
        || v1.behind + v2.behind + v3.behind == 3)
       /* if(v1.left__ && v2.left__ && v3.left__
        || v1.right_ && v2.right_ && v3.right_
        || v1.up____ && v2.up____ && v3.up____
        || v1.down__ && v2.down__ && v3.down__
        || v1.front_ && v2.front_ && v3.front_
        || v1.behind && v2.behind && v3.behind)*/
           continue;

        /// Orientation inversée car Y inversé
        bool orientZ = !isOrientZ(v1.onScreenCoord, v2.onScreenCoord, v3.onScreenCoord);

        if(!enableRayTracing)
        /// Surface non affichée si Objet non translucide et face mal orientée
        if(!obj.isVolumeTranslucent && !orientZ)
            continue;

        /// Ajout de la face au buffer
        Drawable_Face &face = sceneBuffer.nextFaceBuffer();
        face.isTranslucent = obj.isVolumeTranslucent;
        face.isOrientZ = orientZ;
        face.vertex_indices[0] = i1;
        face.vertex_indices[1] = i2;
        face.vertex_indices[2] = i3;
        v1.isUsed = true;
        v2.isUsed = true;
        v3.isUsed = true;
        if(enableRayTracing) {
            getFaceNormal(v1.p, v2.p, v3.p, face.n);
            if(obj.hasNormalTexture)
                face.setTBNMatrix(v1, v2, v3);
        }
    }
}
void Engine3D::modelToBuffer(Model3D const& model, Scene& sceneBuffer) // private
{
    transformation.push();

    /// Apply the model's transformation
    transformation.transform(model.getTransformation().getMatrix(), PRE);

    /// Si c'est une lumière, on applique les transformations si nécessaire et on bufferise
    if(model.isLight())
    {
        Light &light = sceneBuffer.nextLightBuffer();
        light.copyLight(model.getLight());

        light.attenuation /= transformation.getGlobalScale_2();

        if(light.hasPosition)
        {
            transformation.applyTo(model.getLight().position, light.position);
        }

        if(light.hasDirection)
        {
            transformation.applyToUnitaryVector(model.getLight().direction, light.direction);
        }
    }
    /// Si c'est un modèle, on applique les transformations et on bufferise, puis on fait un appel réccursif sur chaque sous modèle
    else
    {
        /// draw main object
        objectToBuffer(model.getObject3D(), sceneBuffer);

        /// Draw sub model
        for(int i = 0; i < model.getSubModelCount(); i++)
            modelToBuffer(model.getSubModel(i), sceneBuffer);
    }

    transformation.pop();
}


void Engine3D::drawSkyBox(Object3D const& sky)
{
    transformation.push();

    Matrix m(cam.getTransformation());
    m.mat[3] = m.mat[7] = m.mat[11] = 0;
    transformation.transform(m);
    if(!enableRayTracing)
        transformation.scale(screenWidth/2.0f);

    /// BLENDER COMPATIBILITE ORIENTATION
    transformation.rotateX(-M_PI_2, PRE);
    float zf = zFar, zn = zNear;
    zFar = FLT_MAX;
    //zNear = 0;
    /// Clear Z-buffer with the new zFar
    clearZBuffer();
    drawObject(sky, true);
    zFar = zf;
    zNear = zn;
    /// Clear Z-buffer
    clearZBuffer();

    transformation.pop();
}

void Engine3D::drawScene(Scene& scene)
{
    /// Clear buffers
    clearBuf();
    scene.clearBuffer();

    /// Draw the skybox
    if(scene.hasSkyBox())
    {
        if(enableRayTracing) {
            transformation.push();
            Matrix m(cam.getTransformation());
            m.mat[3] = m.mat[7] = m.mat[11] = 0;
            transformation.transform(m);
            /// BLENDER COMPATIBILITE ORIENTATION
            transformation.rotateX(-M_PI_2, PRE);

            scene.bufferizeSkybox(transformation);

            transformation.pop();
        }
        else
            drawSkyBox(scene.getSkyBox());

    }
    transformation.push();
    /// BLENDER COMPATIBILITE ORIENTATION
    transformation.rotateX(-M_PI_2, PRE);
    {
        /// Bufferisation des objets du décors
        transformation.push();
        {
            /// Camera transformation
            transformation.transform(cam.getTransformation());
            /// Screen transformation
            if(!enableRayTracing) {
                transformation.translateX(1.0f);
                transformation.translateY(1.0f);
                transformation.scale(screenWidth/2.0f);
            }
            /// -> Bufferisation
            for(vector<Model3D>::const_iterator it = scene.getWorld().begin(); it != scene.getWorld().end(); ++it)
                modelToBuffer(*it, scene);
        }
        transformation.pop();

        /// Bufferisation des objets à position relative à la caméra
        transformation.push();
        {
            /// Screen transformation
            if(!enableRayTracing) {
                transformation.translateX(1.0f);
                transformation.translateY(1.0f);
                transformation.scale(screenWidth/2.0f);
            }
            for(vector<Model3D>::const_iterator it = scene.getBody().begin(); it != scene.getBody().end(); ++it)
                modelToBuffer(*it, scene);
        }
        transformation.pop();
    }
    transformation.pop();

    /// -> Dessin
    scene.updateLightsAmbientSum();
    if(enableRayTracing) {
        if(!MTH_ENABLED)
            mRT.render(scene);
        else
            mRT.renderMutliThread(scene, mTh);
    }
    else if(!MTH_ENABLED)
        draw(scene);
    else
        drawMultiThread(scene);

    /// Apply effect and draw to SDL surface
    postEffects();
    if(enableRayTracing)
        toggleRayTracing();

    SDL_LockSurface(src);
    toBitmap();
    SDL_UnlockSurface(src);
}

void Engine3D::setVertexColorWithLights(Transf_Vertex & vertex, Scene const& sceneBuffer)
{
    /// obj.emissive + v.color * (obj.ambient * Sum(light.ambient) + obj.diffuse * Sum(dotNL * light.diffuse * rayIntesity)) + obj.specular * Sum(sped * light.specular * rayIntensity)
    #if DRAW_NORMAL_MAP == 1
        vertex.c.r = vertex.n[0];
        vertex.c.g = vertex.n[1];
        vertex.c.b = vertex.n[2];
        return;
    #endif // DRAW_NORMAL_MAP

    Object3D const&obj = *((Object3D*)vertex.objParent);
    const int lightsCount = sceneBuffer.getLightsCount();

    if(lightsCount == 0)
        return;

    /// Ambient
    rgb_f lightAmbient(sceneBuffer.getLightsAmbientSum());
    lightAmbient *= obj.ambient;


    /// Diffuse & coef
    rgb_f lightDiffuse;
    float SDcoefBuffer[lightsCount*2];
    for(int i = lightsCount; --i >= 0;) {
        sceneBuffer.getLight(i).getSpecularAndDiffuseCoef(vertex, &SDcoefBuffer[i*2], ((Object3D*)vertex.objParent)->shininess);
        lightDiffuse += sceneBuffer.getLight(i).diffuse * SDcoefBuffer[i*2];
    }
    lightDiffuse *= obj.diffuse;

    /// Specular
    rgb_f lightSpecular;
    for(int i = lightsCount; --i >= 0;)
        lightSpecular += sceneBuffer.getLight(i).specular * SDcoefBuffer[i*2+1];
    lightSpecular *= obj.specular;


    vertex.c *= lightDiffuse + lightAmbient;
    vertex.c += lightSpecular;
    vertex.c += obj.emissive;
    vertex.c.clamp();
}


/// return true if the triangle is well oriented and must be shown; <=> the points A, B, C are in the clockwise order in the XY plan
bool Engine3D::isOrientZ(const float* A, const float* B, const float* C) {
    /// N = AB vect AC
    return (B[0]-A[0])*(C[1]-A[1]) - (C[0]-A[0])*(B[1]-A[1]) > 0.0f;
}

/// COLOR & RASTERIZER
typedef struct ColorVertexLight_params_t {
    Scene &sceneBuffer;
    Engine3D &engine3D;
} ColorVertexLight_params_t;
void funcColorVertexLight(thread_params_t* thparams) {
    Scene &sceneBuffer = ((ColorVertexLight_params_t*)thparams->params)->sceneBuffer;
    Engine3D &e3d = ((ColorVertexLight_params_t*)thparams->params)->engine3D;
    for(int i = sceneBuffer.getVertexCount()-1-thparams->threadNum; i >= 0; i -= thparams->threadCount)
        if(sceneBuffer.getVertex(i).isUsed) {
            /// Normal map
            /*sceneBuffer.getVertex(i).c.r = sceneBuffer.getVertex(i).n[0];
            sceneBuffer.getVertex(i).c.g = sceneBuffer.getVertex(i).n[1];
            sceneBuffer.getVertex(i).c.b = sceneBuffer.getVertex(i).n[2];*/
            /// Vertex color
            e3d.setVertexColorWithLights(sceneBuffer.getVertex(i), sceneBuffer);
        }
}
typedef struct Rasterizer_params_t {
    Engine3D* e;
    Scene* sceneBuffer;
    float **depth;
    rgb_f **clr;
    ThreadSync sync[THREAD_COUNT];
} Rasterizer_params_t;
void funcRasterizer(thread_params_t* thparams) {
    Rasterizer_params_t &params = *((Rasterizer_params_t*)thparams->params);

    float *d = params.depth[thparams->threadNum]; d += params.e->getPixelCount();
    while(d != params.depth[thparams->threadNum])
        *(--d) = params.e->getZFar();

    int count = params.sceneBuffer->getFacesCount();
    for(int i = thparams->threadNum; i < count; i+=THREAD_COUNT) {
        const Drawable_Face &face = params.sceneBuffer->getFace(i);
        const int i1 = face.vertex_indices[0], i2 = face.vertex_indices[1], i3 = face.vertex_indices[2];

        Transf_Vertex const& v1 = params.sceneBuffer->getVertex(i1);
        Transf_Vertex const& v2 = params.sceneBuffer->getVertex(i2);
        Transf_Vertex const& v3 = params.sceneBuffer->getVertex(i3);

        /// Objet translucide
        if(face.isTranslucent) {
            //params.e->triangleVolume(v1.onScreenCoord, v2.onScreenCoord, v3.onScreenCoord, v1.c, face.isOrientZ);
        }
        else  {
            params.e->triangle(params.clr[thparams->threadNum], params.depth[thparams->threadNum], v1.onScreenCoord, v2.onScreenCoord, v3.onScreenCoord, v1.c, v2.c, v3.c);
        }
    }

    int i = thparams->threadNum, e = 0;
    float *depth = params.depth[thparams->threadNum];
    rgb_f *clr = params.clr[thparams->threadNum];
    #ifdef DEBUG
        Uint32 ttt = SDL_GetTicks();
    #endif
    while(i%2==0 && (1<<e) < THREAD_COUNT) {
        int thnum = (i+1) << e;
        #ifdef DEBUG
            printf("waitformerge[%d] with [%d]\n", thparams->threadNum, thnum);
        #endif
        params.sync[thnum].wait();

        for(int j = params.e->getPixelCount(); --j >= 0;)
        if(params.depth[thnum][j] < depth[j]) {
            depth[j] = params.depth[thnum][j];
            memcpy(&clr[j], &params.clr[thnum][j], sizeof(rgb_f));
        }
        i /= 2;
        e++;
    }
    #ifdef DEBUG
        printf("release[%d] %dms\n", thparams->threadNum, SDL_GetTicks()-ttt);
    #endif
    if(thparams->threadNum != 0)
        params.sync[thparams->threadNum].signal();
}
rgb_f* clr[THREAD_COUNT];
float* depth[THREAD_COUNT];
void Engine3D::drawMultiThread(Scene &sceneBuffer) {
    #ifdef DEBUG
        Uint32 t = SDL_GetTicks();
    #endif
    ColorVertexLight_params_t paramsColor = {sceneBuffer, *this};
    mTh.execute(&funcColorVertexLight, (void*)&paramsColor);
    #ifdef DEBUG
        printf("Color %dms\n", SDL_GetTicks()-t);
    #endif

    if(clr[0] == NULL) {
        for(int i = 1; i < THREAD_COUNT; i++) {
            clr[i] = new rgb_f[width * height];
            depth[i] = new float[width * height];
        }
        /// le 1er thread travaille sur le depthBuf et colorBuf principals -> moins de fusion
        clr[0] = colorBuf;
        depth[0] = depthBuf;
    }

    #ifdef DEBUG
        t = SDL_GetTicks();
    #endif

    Rasterizer_params_t paramsRast = {this, &sceneBuffer, depth, clr};
    mTh.execute(&funcRasterizer, (void*)&paramsRast);
    #ifdef DEBUG
        printf("Rasterizer %dms\n", SDL_GetTicks()-t);
    #endif
}
void Engine3D::draw(Scene& sceneBuffer) {// private
    #if defined(DEBUG)
        Uint32 t = SDL_GetTicks();
    #endif
    /*for(int i = sceneBuffer.getVertexCount(); --i >= 0;)
        setVertexColorWithLights(sceneBuffer.getVertex(i), sceneBuffer);*/
    /// Dessinde toutes les surfaces
    for(int i = sceneBuffer.getFacesCount(); --i >= 0;) {
        Drawable_Face &face = sceneBuffer.getFace(i);
        int i1 = face.vertex_indices[0], i2 = face.vertex_indices[1], i3 = face.vertex_indices[2];

        /// Objet translucide
        if(face.isTranslucent)
            triangleVolume(sceneBuffer.getVertex(i1).onScreenCoord, sceneBuffer.getVertex(i2).onScreenCoord, sceneBuffer.getVertex(i3).onScreenCoord,
                     sceneBuffer.getVertex(i3).c, face.isOrientZ);
        else {
            Transf_Vertex &v1 = sceneBuffer.getVertex(i1);
            Transf_Vertex &v2 = sceneBuffer.getVertex(i2);
            Transf_Vertex &v3 = sceneBuffer.getVertex(i3);
            /// Calcul des couleurs si non fait
            if(v1.objParent != NULL) {
                setVertexColorWithLights(v1, sceneBuffer);
                v1.objParent = NULL;
            }
            if(v2.objParent != NULL) {
                setVertexColorWithLights(v2, sceneBuffer);
                v2.objParent = NULL;
            }
            if(v3.objParent != NULL) {
                setVertexColorWithLights(v3, sceneBuffer);
                v3.objParent = NULL;
            }

            triangle(colorBuf, depthBuf, v1.onScreenCoord, v2.onScreenCoord, v3.onScreenCoord, v1.c, v2.c, v3.c);
        }
    }
    #if defined(DEBUG)
        printf("total : %d\n", SDL_GetTicks()-t);
    #endif
}


/// GLOW
typedef struct Glow_params_t {
    rgb_f *colorBuf, *colorBuf_g, *colorBuf_f;
    float *k;
    float glowRadius, minLuminosityGlow;
    int w,h;
} Glow_params_t;
/// partage de la surface en bandes horizontales
void funcGlow1(thread_params_t* params) {
    Glow_params_t &p = *(Glow_params_t*)params->params;
    int ylength = (p.h + (params->threadCount-1)) / params->threadCount;
    int ystart = params->threadNum * ylength, yend = min(ystart + ylength, p.h);
    /// Masque de lumière
    for(int y = ystart; y < yend; y++)
    for(int x = 0; x < p.w; x++) {
        int i = x+y*p.w;
        float intensity = (p.colorBuf[i].r * p.colorBuf[i].g * p.colorBuf[i].b); // 0.715160 * colorBuf[i].r + 0.212671 * colorBuf[i].g + 0.072169 * colorBuf[i].b;
        //intensity = 1.0f - (1.0f - intensity)*(1.0f - intensity);
        intensity = clampIn01(intensity / (1-p.minLuminosityGlow) - p.minLuminosityGlow);
        /*float mini = min(p.colorBuf[i].r, min(p.colorBuf[i].g, p.colorBuf[i].b))*0.95;
        float maxi = max(p.colorBuf[i].r, max(p.colorBuf[i].g, p.colorBuf[i].b));*/
        /*p.colorBuf_g[i].r = (p.colorBuf[i].r - mini) / (maxi-mini)* intensity;
        p.colorBuf_g[i].g = (p.colorBuf[i].g - mini) / (maxi-mini)* intensity;
        p.colorBuf_g[i].b = (p.colorBuf[i].b - mini) / (maxi-mini)* intensity;*/
        p.colorBuf_g[i] = p.colorBuf[i] * intensity;
        memset(&p.colorBuf_f[i], 0, sizeof(rgb_f)-4);/// inutile d'effacer le '.s'
    }
    /// Flou  x
    for(int y = ystart; y < yend; y++)
    for(int x = 0; x < p.w; x++) {
        int i = x+y*p.w;
        /// on passe si il va faire tout noir ("Ta gueule !")
        if(p.colorBuf_g[i].r == 0 && p.colorBuf_g[i].g == 0 && p.colorBuf_g[i].b == 0)
            continue;

        int sw = max(0, x-(int)p.glowRadius), ew = min(p.w-1,  x+(int)p.glowRadius);
        for(int w = sw; w <= ew; w++)
        {
            p.colorBuf_f[w+y*p.w] = p.colorBuf_f[w+y*p.w] + p.colorBuf_g[i] * p.k[abs(x-w)];
        }
        memset(&p.colorBuf_g[i], 0, sizeof(rgb_f)-4);/// inutile d'effacer le '.s'
    }
}
/// partage de la surface en bandes verticales
void funcGlow2(thread_params_t* params) {
    Glow_params_t &p = *(Glow_params_t*)params->params;
    int xlength = (p.w + (params->threadCount-1)) / params->threadCount;
    int xstart = params->threadNum * xlength, xend = min(xstart + xlength, p.w);
    /// Flou  y
    for(int y = 0; y < p.h; y++)
    for(int x = xstart; x < xend; x++) {
        int i = x+y*p.w;
        /// on passe si il va faire tout noir ("Ta gueule !")
        if(p.colorBuf_f[i].r == 0 && p.colorBuf_f[i].g == 0 && p.colorBuf_f[i].b == 0)
            continue;

        int sh = max(0, y-(int)p.glowRadius), eh = min(p.h-1,  y+(int)p.glowRadius);
        for(int h = sh; h <= eh; h++)
        {
            p.colorBuf_g[x+h*p.w] = p.colorBuf_g[x+h*p.w] + p.colorBuf_f[i] * p.k[abs(y-h)];
        }
    }

    /// Surperposition du flou sur l'image
    for(int y = 0; y < p.h; y++)
    for(int x = xstart; x < xend; x++)
    {
        int i = x+y*p.w;
        p.colorBuf_g[i].clamp();
        p.colorBuf[i] = p.colorBuf[i] + p.colorBuf_g[i];
    }
}
void Engine3D::glowMultiThread() {
    float k[(int)glowRadius + 1];
    for(int i = 0; i <= glowRadius; i++)
        k[i] = (1.0f - (float) i / glowRadius) / ((glowRadius+1)/2);
    Glow_params_t params = {colorBuf, colorBuf_g, colorBuf_f, k, glowRadius, minLuminosityGlow, width, height};

    mTh.execute(funcGlow1, (void*)&params);
    mTh.execute(funcGlow2, (void*)&params);
}
void Engine3D::glow() {
    /// Masque de lumière
    for(int i = 0, y = 0; y < height; y++)
    for(int x = 0; x < width; x++, i++) {
        float intensity = (colorBuf[i].r * colorBuf[i].g * colorBuf[i].b); // 0.715160 * colorBuf[i].r + 0.212671 * colorBuf[i].g + 0.072169 * colorBuf[i].b;
        //intensity = 1.0f - (1.0f - intensity)*(1.0f - intensity);
        intensity = clampIn01(intensity / (1-minLuminosityGlow) - minLuminosityGlow);
        colorBuf_g[i] = colorBuf[i] * intensity;
        memset(&colorBuf_f[i], 0, sizeof(rgb_f)-4);/// inutile d'effacer le '.s'
    }

    float k[(int)glowRadius + 1];
    //float gaussien = /*exp(-x*x/2) **/ magic_sqrt_inv(2*M_PI)/glowRadius;
    for(int i = 0; i <= glowRadius; i++)
        k[i] = /*exp(-i*i/(2*glowRadius*glowRadius)) * gaussien;/*/(1.0f - (float) i / glowRadius) / ((glowRadius+1)/2);//*/

    /// Flou  x
    for(int i = 0, y = 0; y < height; y++)
    for(int x = 0; x < width; x++, i++) {
        /// on passe si il va faire tout noir ("Ta gueule !")
        if(colorBuf_g[i].r == 0 && colorBuf_g[i].g == 0 && colorBuf_g[i].b == 0)
            continue;

        int sw = max(0, x-(int)glowRadius), ew = min(width-1,  x+(int)glowRadius);
        for(int w = sw; w <= ew; w++)
        {
            colorBuf_f[w+y*width] = colorBuf_f[w+y*width] + colorBuf_g[i] * k[abs(x-w)];
        }
        memset(&colorBuf_g[i], 0, sizeof(rgb_f)-4);/// inutile d'effacer le '.s'
    }
    /// Flou  y
    for(int i = 0, y = 0; y < height; y++)
    for(int x = 0; x < width; x++, i++) {
        /// on passe si il va faire tout noir ("Ta gueule !")
        if(colorBuf_f[i].r == 0 && colorBuf_f[i].g == 0 && colorBuf_f[i].b == 0)
            continue;

        int sh = max(0, y-(int)glowRadius), eh = min(height-1,  y+(int)glowRadius);
        for(int h = sh; h <= eh; h++)
        {
            colorBuf_g[x+h*width] = colorBuf_g[x+h*width] + colorBuf_f[i] * k[abs(y-h)];
        }
    }

    /// Surperposition du flou sur l'image
    for(int i = 0; i < pixelCount; i++)
    {
        colorBuf_g[i].clamp();
        colorBuf[i] = colorBuf[i] + colorBuf_g[i];
    }
}

/// VOLUME RENDERING
typedef struct VolumeRendering_params_t {
    HeapVolumeDepth *volumeDepthBuf;
    rgb_f volumeColor;
    int pixelCount;
    float *depthBuf;
    rgb_f *colorBuf;
    float zNear;
} VolumeRendering_params_t;
void funcVolumeRendering(thread_params_t* params) {
    VolumeRendering_params_t &p = *((VolumeRendering_params_t*)params->params);
    HeapVolumeDepth *volumeDepthBuf = p.volumeDepthBuf;
    rgb_f const &volumeColor = p.volumeColor;
    float *depthBuf = p.depthBuf;
    const float zNear = p.zNear;
    rgb_f* colorBuf = p.colorBuf;

    for(int i = params->threadNum; i < p.pixelCount; i += params->threadCount)
    if(!volumeDepthBuf[i].empty())
    {
        const float zBuf = depthBuf[i];
        float zMinVolume = zBuf;
        float totalThickness = 0.0f;
        int densityFactor = 0;
        float lastDepth = zNear;

        /// Traitement de chaque profondeur de changement de milieu (ou de densité) par ordre croissant
        while(!volumeDepthBuf[i].empty())
        {
            PixelVolume const& pv = volumeDepthBuf[i].top();

            /// Surface opaque dépassée -> plus de volume
            /// on ne vide pas le tas, mais on prend en compte toutes les frontières fermantes suivante
            /*if(pv.depth > zBuf) {

                while(!volumeDepthBuf[i].empty()) {
                    volumeDepthBuf[i].pop();
                }
                break;
            }*/

            float depth = max(min(pv.depth, zBuf), zNear);
            /// Entrée dans le volume / augmentation de la densité
            if(!pv.front) /* '!' -> WtF ? */
            {
                if(densityFactor > 0)
                {
                    totalThickness += (depth - lastDepth) * densityFactor;
                }
                densityFactor++;
                /// profondeur min ?
                if(depth < zMinVolume)
                    zMinVolume = depth;
            }
            /// Sortie dans le volume / réduction de la densité
            else
            {
                /// fin sans début, le début se trouve probablement avant le zNear
                if(densityFactor == 0) {
                    totalThickness += (depth - zNear);

                    /// profondeur min = zNear
                    zMinVolume = zNear;
                }
                /// réduction de la densité
                else {
                    totalThickness += (depth - lastDepth) * densityFactor;
                    densityFactor--;
                }
            }

            volumeDepthBuf[i].pop();
            lastDepth = depth;
        }

        /*/// Complétion si surface opaque atteinte et volume non fermé
        if(densityFactor > 0) {
            totalThickness += (zBuf - lastDepth) / densityFactor;
        }*/


        /// Calcul de l'opacité
        /// "densityFactor == 0" permet de limiter les pixels foireux (principalement sur les contours des objets -> /!\ problème non résolu si plusieurs objets)
        if(totalThickness > 0.0f && densityFactor == 0)
        {
            float alpha = (1.0f-exp(-totalThickness*volumeColor.s));
            colorBuf[i] = volumeColor * alpha + colorBuf[i] * (1.0f-alpha);
            colorBuf[i].clamp();

            /// Mise à jour du depth-buffer
            depthBuf[i] = zMinVolume;
        }
    }
}
void Engine3D::volumeRenderingMultiThread() {
    VolumeRendering_params_t params = {volumeDepthBuf, volumeColor, pixelCount, depthBuf, colorBuf, zNear};

    mTh.execute(funcVolumeRendering, (void*)&params);
}
void Engine3D::volumeRendering() {
    // #define VOLUME_RENDERING_INSTRUCTIONS  \ *
    for(int i = 0; i < pixelCount; i++)
    if(!volumeDepthBuf[i].empty())
    {
        const float zBuf = depthBuf[i];
        float zMinVolume = zBuf;
        float totalThickness = 0.0f;
        int densityFactor = 0;
        float lastDepth = zNear;

        /// Traitement de chaque profondeur de changement de milieu (ou de densité) par ordre croissant
        while(!volumeDepthBuf[i].empty())
        {
            PixelVolume const& pv = volumeDepthBuf[i].top();

            /// Surface opaque dépassée -> plus de volume
            /// on ne vide pas le tas, mais on prend en compte toutes les frontières fermantes suivante
            /*if(pv.depth > zBuf) {

                while(!volumeDepthBuf[i].empty()) {
                    volumeDepthBuf[i].pop();
                }
                break;
            }*/

            float depth = max(min(pv.depth, zBuf), zNear);
            /// Entrée dans le volume / augmentation de la densité
            if(!pv.front) /* '!' -> WtF ? */
            {
                if(densityFactor > 0)
                {
                    totalThickness += (depth - lastDepth) * densityFactor;
                }
                densityFactor++;
                /// profondeur min ?
                if(depth < zMinVolume)
                    zMinVolume = depth;
            }
            /// Sortie dans le volume / réduction de la densité
            else
            {
                /// fin sans début, le début se trouve probablement avant le zNear
                if(densityFactor == 0) {
                    totalThickness += (depth - zNear);

                    /// profondeur min = zNear
                    zMinVolume = zNear;
                }
                /// réduction de la densité
                else {
                    totalThickness += (depth - lastDepth) * densityFactor;
                    densityFactor--;
                }
            }

            volumeDepthBuf[i].pop();
            lastDepth = depth;
        }

        /*/// Complétion si surface opaque atteinte et volume non fermé
        if(densityFactor > 0) {
            totalThickness += (zBuf - lastDepth) / densityFactor;
        }*/


        /// Calcul de l'opacité
        /// "densityFactor == 0" permet de limiter les pixels foireux (principalement sur les contours des objets -> /!\ problème non résolu si plusieurs objets)
        if(totalThickness > 0.0f && densityFactor == 0)
        {
            float alpha = (1.0f-exp(-totalThickness*volumeColor.s));
            colorBuf[i] = volumeColor * alpha + colorBuf[i] * (1.0f-alpha);
            colorBuf[i].clamp();

            /// Mise à jour du depth-buffer
            depthBuf[i] = zMinVolume;
        }
    }
}

/// DISTANCE FOG
typedef struct DistanceFog_params_t {
    float fogDepth, fogFar;
    rgb_f fogColor;
    int pixelCount;
    rgb_f *colorBuf;
    float *depthBuf;
} DistanceFog_params_t;
void funcDistanceFog(thread_params_t* params) {
    DistanceFog_params_t &p = *((DistanceFog_params_t*)params->params);
    float fogDepth = p.fogDepth;
    float fogFar = p.fogFar;
    rgb_f &fogColor = p.fogColor;
    int pixelCount = p.pixelCount;
    rgb_f *colorBuf = p.colorBuf;
    float *depthBuf = p.depthBuf;

    for(int i = params->threadNum; i < pixelCount; i += params->threadCount)
    {
        float k = clampIn01((fogFar - depthBuf[i]) / fogDepth);
        //float k = clampIn01(exp(-(-depthBuf[i] + fogFar) / (fogDepth/5)));
        if(k >= 1) continue;
        if(k <= 0)
            colorBuf[i] = fogColor;
        else
            colorBuf[i] = colorBuf[i] * k + fogColor * (1.0f-k);
    }
}
void Engine3D::distanceFogMultiThread() {
    DistanceFog_params_t params = {(fogFar - fogNear), fogFar, fogColor, pixelCount, colorBuf, depthBuf};

    mTh.execute(funcDistanceFog, (void*)&params);
}
void Engine3D::distanceFog() {
    float fogDepth = (fogFar - fogNear);
    for(int i = 0; i < pixelCount; ++i)
    {
        float k = clampIn01((fogFar - depthBuf[i]) / fogDepth);
        //float k = clampIn01(exp(-(-depthBuf[i] + fogFar) / (fogDepth/5)));
        if(k >= 1) continue;
        if(k <= 0)
            colorBuf[i] = fogColor;
        else
            colorBuf[i] = colorBuf[i] * k + fogColor * (1.0f-k);
    }
}

void Engine3D::postEffects() {
    if(enableFog) {
        if(MTH_ENABLED)
            distanceFogMultiThread();
        else
            distanceFog();
    }

    if(enableVolumeRendering) {
        if(MTH_ENABLED)
            ;//volumeRenderingMultiThread();
        else
            volumeRendering();
    }

    if(enableDepthOfField && !enableRayTracing) {
        int i = 0;

        /// Compute the Z distance in focus with depth of field
        float zFocus = depthBuf[pixelFocusIndex];
        zDoFFocus = enableRayTracing ? zFocus : zDoFFocus * speedChangeFocusDoF + zFocus * (1.0f-speedChangeFocusDoF);

        float *CoCarray = new float[pixelCount*3];
        for(int i = pixelCount; --i >= 0;) {
            CoCarray[i*3] = maxRadiusDoF * clampIn01((enableRayTracing ? 1.0f : gradientRadiusDoF) * abs(1.0f-(zDoFFocus)/depthBuf[i]));
            CoCarray[i*3+1] = CoCarray[i*3]*CoCarray[i*3];
            CoCarray[i*3+2] = clampIn01(1.0f/CoCarray[i*3+1]);
        }

        for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++, i++) {
            if(depthBuf[i] < zFar)
            {
                const float CoC = CoCarray[i*3], CoC2 = CoCarray[i*3+1];
                const float alpha = CoCarray[i*3+2];
                const int c = CoC+1;//0.5f;
                for(int a = max(0, x-c), aa = min(width-1, x+c); a <= aa ; a++)
                for(int b = max(0, y-c), bb = min(height-1, y+c); b <= bb ; b++) {
                    int d2 = (a-x)*(a-x)+(b-y)*(b-y);
                    //if(depthBuf[i] <= depthBuf[a+b*width]+CoC) // zi za
                    if(CoC2 >= d2)
                    {
                        float overAlpha = (depthBuf[i] > depthBuf[a+b*width]+CoC) ? clampIn01(1.0f-CoCarray[(a+b*width)*3+2]) : 1;
                        colorBuf_f[a+b*width].add(colorBuf[i], alpha * overAlpha);
                    }
                }
            }

        }
        delete[] CoCarray;

        for(i = 0; i < height*width; ++i) {
            rgb_f c(colorBuf_f[i].getRGB(colorBuf[i]));
            colorBuf[i].r = c.r;
            colorBuf[i].g = c.g;
            colorBuf[i].b = c.b;
        }
    }

    if(enableGlow) {
        if(MTH_ENABLED)
            glowMultiThread();
        else
            glow();
    }

    /// Motion blur : let's go !
    /// TODO vecteur de flou par pixel en fonction d'une vitesse angulaire et linaire
    if(enableBlurMotion) {
        memset(colorBuf_f, 0, sizeof(rgb_f)*pixelCount);
        int /*screenCenter*/cx = width/2, cy = height/2;
        for(int i = 0; i < pixelCount; i++) {
            float x = i%width, y = i/width;
            float dx = x-cx, dy = y-cy;
            if(dx == 0 && dy == 0) continue;
            float d = 1 / magic_sqrt_inv(dx*dx+dy*dy);
            dx /= d * (speedBlurMotion<0?-1:1);
            dy /= d * (speedBlurMotion<0?-1:1);
            /*float maxabs = max(abs(dx), abs(dy));
            dx /= maxabs;
            dy /= maxabs;*/
            int length = abs(speedBlurMotion) * d / depthBuf[i] + 1;
            float k = 1.0f, down = pow(0.1, 1.0f/length);
            while(--length >= 0)
            {
                int xi = x, yi = y;
                if(xi < 0 || xi >= width || yi < 0 || yi >= height)// || i != 100+300*width)
                    break;
                colorBuf_f[xi+yi*width].r += colorBuf[i].r * k;
                colorBuf_f[xi+yi*width].g += colorBuf[i].g * k;
                colorBuf_f[xi+yi*width].b += colorBuf[i].b * k;
                colorBuf_f[xi+yi*width].s += k;

                x += dx;
                y += dy;
                k *= down;
            }
        }
        for(int i = 0; i < pixelCount; i++) {
            colorBuf[i].r = colorBuf_f[i].r / colorBuf_f[i].s;
            colorBuf[i].g = colorBuf_f[i].g / colorBuf_f[i].s;
            colorBuf[i].b = colorBuf_f[i].b / colorBuf_f[i].s;
        }
    }

    filters2D();
}

/// Effet sur image plane (sans depth buffer)
void Engine3D::filters2D() {
    /// en fonction de l'image précédente
    /// palette de couleurs
    /// filtres photo

    /// Filtre contour
    /* METHODE : VARIATION DE PROFONDEUR
    float diffZMax[pixelCount];
    float diffAverage = 0.0f;
    for(int y = 0; y < height; y++)
    for(int x = 0; x < width; x++) {
        float z = depthBuf[x+y*width];
        diffZMax[x+y*width] = 0;
        int p[8*2] = {0,1, 1,1, 1,0, 1,-1, 0,-1, -1,-1, 0,-1, -1,1};
        for(int j = 0; j < 8; j+=2)
            if(0 <= x+p[j] && x+p[j] < width && 0 <= y+p[j+1] && y+p[j+1] < height)
                diffZMax[x+y*width] = max(diffZMax[x+y*width], abs(z-depthBuf[x+p[j]+(y+p[j+1])*width]));
        diffAverage += diffZMax[x+y*width];
    }
    diffAverage /= pixelCount;
    diffAverage = sqrt(diffAverage);
    for(int i = 0; i < pixelCount; i++)
        if(diffZMax[i] > diffAverage)
            colorBuf[i] = rgb_f(1, 1, 1);
        else
            colorBuf[i] = rgb_f(0,0,0);//*/
    if(enableDrawingFilter) {
        //* METHODE : VARIATION DE COULEUR (NORMAL MAP)
        float *diffCMax = new float[pixelCount];
        float diffCAverage = 0.0f;

        for(int y = 0; y < height; y++)
        for(int x = 0; x < width; x++) {
            rgb_f c = colorBuf[x+y*width];
            diffCMax[x+y*width] = 0;
            int p[8*2] = {0,1, 1,1, 1,0, 1,-1, 0,-1, -1,-1, 0,-1, -1,1};
            for(int j = 0; j < 8; j+=2)
            if(0 <= x+p[j] && x+p[j] < width && 0 <= y+p[j+1] && y+p[j+1] < height) {
                rgb_f const& v = colorBuf[x+p[j]+(y+p[j+1])*width];
                diffCMax[x+y*width] = max(diffCMax[x+y*width], (abs(v.r-c.r)+abs(v.g-c.g)+abs(v.b-c.b)) / ((j/2) %2 ? 1.41f:1.0f) );
            }
            diffCAverage += diffCMax[x+y*width];
        }

        diffCAverage /= pixelCount;
        diffCAverage = sqrt(diffCAverage);
        for(int i = 0; i < pixelCount; i++)
            if(diffCMax[i] < diffCAverage) {
                float k = 1;//-(diffCMax[i]/diffCAverage)*(diffCMax[i]/diffCAverage);
                colorBuf[i] = rgb_f(k, k, k);

                /* Petit dégradé
                int x = i%width, y = i/width;
                int p[8*2] = {0,1, 1,1, 1,0, 1,-1, 0,-1, -1,-1, 0,-1, -1,1};
                for(int j = 0; j < 8; j+=2)
                if(0 <= x+p[j] && x+p[j] < width && 0 <= y+p[j+1] && y+p[j+1] < height)
                if(diffCMax[x+p[j]+(y+p[j+1])*width] > diffCAverage) {
                    float k = (diffCAverage / diffCMax[x+p[j]+(y+p[j+1])*width]) * (diffCAverage / diffCMax[x+p[j]+(y+p[j+1])*width]) / 5;
                    k = k*0.5f+0.5f;
                    colorBuf[i] = rgb_f(k,k,k);
                }//*/
            }
            else {
                float k = (diffCAverage / diffCMax[i]) * (diffCAverage / diffCMax[i]);
                k = k*0.8f+0.2f;
                colorBuf[i] = rgb_f(k,k,k);//*/
            }
        delete[] diffCMax;
    }

    /// Anti-aliasing -> SDL_Bitmap 2* plus grand
    //*
    #if ANTI_ALIASING == 1
        for(int y = 0; y < height; y+=2)
        for(int x = 0; x < width; x+=2) {
            colorBuf[x/2+y/2*width] = colorBuf[x + y*width]*0.25f
                + colorBuf[x+1 + y*width]*0.25f
                + colorBuf[x + (y+1)*width]*0.25f
                + colorBuf[x+1 + (y+1)*width]*0.25f;
        }//*/
    #endif
    /// déformations
    /// Oculus -> image carré
/*    for(int c = 2; c < width; c++)
    for(int o = (width - c)/2; o <= (width + c)/2; o++)
    for(int o = (width - c)/2; o <= (width + c)/2; o++)
    for(int oy = 0; oy < height / 2; y++)
*/
}

void Engine3D::toBitmap() {
    Uint8 bpp = src->format->BytesPerPixel;
    for(int y = 0; y < height; ++y)
    for(int x = 0; x < width; ++x) {
        Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * bpp;
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = clampIn01(colorBuf[x+y*width].r) * 255;
            p[1] = clampIn01(colorBuf[x+y*width].g) * 255;
            p[2] = clampIn01(colorBuf[x+y*width].b) * 255;
        } else {
            p[0] = clampIn01(colorBuf[x+y*width].b) * 255;
            p[1] = clampIn01(colorBuf[x+y*width].g) * 255;
            p[2] = clampIn01(colorBuf[x+y*width].r) * 255;
        }
    }
}
void Engine3D::toBitmap(SDL_Surface *bmp, rgb_f* color, int width, int height) {
    Uint8 bpp = bmp->format->BytesPerPixel;
    for(int y = 0; y < height; ++y)
    for(int x = 0; x < width; ++x) {
        Uint8 *p = (Uint8 *)bmp->pixels + y * bmp->pitch + x * bpp;
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = clampIn01(color[x+y*width].r) * 255;
            p[1] = clampIn01(color[x+y*width].g) * 255;
            p[2] = clampIn01(color[x+y*width].b) * 255;
        } else {
            p[0] = clampIn01(color[x+y*width].b) * 255;
            p[1] = clampIn01(color[x+y*width].g) * 255;
            p[2] = clampIn01(color[x+y*width].r) * 255;
        }
    }
}

void Engine3D::putPointBuf(rgb_f *dst, float* depth, int x, int y, float z, rgb_f const& c) {
    int i = x + y * width;
    if(depth[i] > -z && -z > zNear) {
        depth[i] = -z;
        dst[i] = c;
    }
}
void Engine3D::putPointBufSecure(int x, int y, float z, rgb_f const& c) {
    if(x < 0 || x >= src-> w || y < 0 || y >= height)
        return;

    int i = x + y * width;
    if(depthBuf[i] > -z && -z > zNear) {
        depthBuf[i] = -z;
        colorBuf[i] = c;
    }
}

void Engine3D::putPointBufVolume(int x, int y, float z, rgb_f const&  c, bool front) {
    int i = x + y * width;
    volumeDepthBuf[i].push(PixelVolume(-z, front));
}
void Engine3D::putPointBufSecureVolume(int x, int y, float z, rgb_f const&  c, bool front) {
    if(x < 0 || x >= src-> w || y < 0 || y >= height)
        return;

    int i = x + y * width;
    volumeDepthBuf[i].push(PixelVolume(-z, front));
}

void Engine3D::clearBuf() {
    enableVolumeRendering = false;
    memset(colorBuf, 0, sizeof(rgb_f)*pixelCount);

    for(int i = pixelCount - 1; i >= 0; i--)
    {
        depthBuf[i] = zFar;
        memcpy(&colorBuf[i], &fogColor, sizeof(rgb_f));
    }
    if(enableDepthOfField)
   //     memcpy(colorBuf_f, colorBuf, sizeof(rgb_f)*width * height);
        memset(colorBuf_f, 0, sizeof(rgb_f)*pixelCount);
}
void Engine3D::clearZBuffer() {
    for(int i = pixelCount; --i >= 0;)
        depthBuf[i] = zFar;
}

void Engine3D::putPixel(int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * src->format->BytesPerPixel;

    if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = r & 0xff;
        p[1] = g & 0xff;
        p[2] = b & 0xff;
    } else {
        p[0] = b & 0xff;
        p[1] = g & 0xff;
        p[2] = r & 0xff;
    }
}
void Engine3D::putPixel(int x, int y, rgb_f const& c)
{
    Uint8 *p = (Uint8 *)src->pixels + y * src->pitch + x * src->format->BytesPerPixel;

    if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        p[0] = c.r * 255;
        p[1] = c.g * 255;
        p[2] = c.b * 255;
    } else {
        p[0] = c.b * 255;
        p[1] = c.g * 255;
        p[2] = c.r * 255;
    }
}

void Engine3D::triangleSortedPointsVolume(const float *A, const float *B, const float *C, rgb_f const& color, bool front) {
    // cas A[1] == B[1] == C[1] => ne dessine rien
	/// côté inférieur horizontal
	if(B[1] == C[1]) {
		if(C[0] < B[0])
            triangleSupVolume(A, C, B, color, front);
        else
            triangleSupVolume(A, B, C, color, front);
	}
	/// côté supérieur horizontal
	else if(A[1] == B[1]) {
		if(A[0] < B[0])
            triangleInfVolume(C, A, B, color, front);
		else
            triangleInfVolume(C, B, A, color, front);
	}
	/// côté supérieur horizontal
	else if(A[1] == C[1]) {
		if(A[0] < C[0])
            triangleInfVolume(B, A, C, color, front);
        else
            triangleInfVolume(B, C, A, color, front);
	}
	/// pas de côté horizontal
	else {
		if(C[1] < B[1]) {
            /// On calcule I, le point de AC de même y que B <=> AI.y = k * AC.y = AB.y
            float k = (C[1]-A[1]) / (B[1]-A[1]); /// rmq : 0 < k < 1
            float I[3] = {(B[0]-A[0]) * k + A[0], C[1], (B[2]-A[2]) * k + A[2]};

            /// I est à gauche de C
            if(I[0] < C[0]) {
                /// on dessine le triangle supérieur AIC
                triangleSupVolume(A, I, C, color, front);
                /// on dessine le triangle inférieur BCI
                triangleInfVolume(B, I, C, color, front);
            }
            /// C est à gauche de A
            else {
                /// on dessine le triangle supérieur ACI
                triangleSupVolume(A, C, I, color, front);
                /// on dessine le triangle inférieur BCI
                triangleInfVolume(B, C, I, color, front);
            }
		}
		else {
            /// On calcule I, le point de AC de même y que B <=> AI.y = k * AC.y = AB.y
            float k = (B[1]-A[1]) / (C[1]-A[1]); /// rmq : 0 < k < 1
            float I[3] = {(C[0]-A[0]) * k + A[0], B[1], (C[2]-A[2]) * k + A[2]};

            /// I est à gauche de B
            if(I[0] < B[0]) {
                /// on dessine le triangle supérieur AIB
                triangleSupVolume(A, I, B, color, front);
                /// on dessine le triangle inférieur CBI
                triangleInfVolume(C, I, B, color, front);
            }
            /// B est à gauche de A
            else {
                /// on dessine le triangle supérieur ABI
                triangleSupVolume(A, B, I, color, front);
                /// on dessine le triangle inférieur CBI
                triangleInfVolume(C, B, I, color, front);
            }
		}
	}
    /*
    float value[9], *A = value, *B = value+3, *C = value+6;
    memcpy(A, a, sizeof(float)*3);
    memcpy(B, b, sizeof(float)*3);
    memcpy(C, c, sizeof(float)*3);
	// cas A[1] == B[1] == C[1] => ne rien dessiner
	/// côté inférieur horizontal
	if(B[1] == C[1]) {
		if(C[0] < B[0]) { float *t = B; B = C; C = t; }
		/// ...maintenant, B est à gauche de C
		/// on dessine le triangle supérieur ABC
		triangleSupVolume(A, B, C, color, front);
	}
	/// côté supérieur horizontal
	else if(A[1] == B[1]) {
		if(A[0] < B[0]) { float *t = B; B = A; A = t; }
		/// ...maintenant, B est à gauche de A
		/// on dessine le triangle inférieur ABC
		triangleInfVolume(C, B, A, color, front);
	}
	/// côté supérieur horizontal
	else if(A[1] == C[1]) {
		if(A[0] < C[0]) { float *t = C; C = A; A = t; }
		/// ...maintenant, C est à gauche de A
		/// on dessine le triangle inférieur ABC
		triangleInfVolume(B, C, A, color, front);
	}
	/// pas de côté horizontal
	else {
		if(C[1] < B[1]) { float *t = B; B = C; C = t; }
		/// ...maintenant, B est au-dessus de C

		/// On calcule I, le point de AC de même y que B <=> AI.y = k * AC.y = AB.y
		float k = (B[1]-A[1]) / (C[1]-A[1]); /// rmq : 0 < k < 1
		float I[3] = {(C[0]-A[0]) * k + A[0], B[1], (C[2]-A[2]) * k + A[2]};

		/// I est à gauche de B
		if(I[0] < B[0]) {
			/// on dessine le triangle supérieur AIB
			triangleSupVolume(A, I, B, color, front);
			/// on dessine le triangle inférieur CBI
			triangleInfVolume(C, I, B, color, front);
		}
		/// B est à gauche de A
		else {
			/// on dessine le triangle supérieur ABI
			triangleSupVolume(A, B, I, color, front);
			/// on dessine le triangle inférieur CBI
			triangleInfVolume(C, B, I, color, front);
		}
	}*/
}
/** A : UP, B : LEFT, C : RIGHT, By=Cy
 *	remplit les pixels dont le centre est contenu dans le triangle
 **/
void Engine3D::triangleSupVolume(const float *A, const float *B, const float *C, rgb_f const& color, bool front) {
	/// suffix -> s : start (côté gauche/haut), e : end (côté droit/bas)
	/// preffix -> v : vecteur, d : delta (variation), l : ligne (concerne la ligne actuelle)

	/// ys in[i-0.5; i-0.5[ -> ys' = i -> ys'=ys'+0.5
	/// ye in[i-0.5; i-0.5[ -> ye' = i -> ye'=ye'-0.5
	/// => ys in triangle ABC, ye in triangle ABC

	/// coord y de bord sup du pixel dont le centre est strictement après A
	//float ysmin = round(A[1]);
	/// coord y du centre (du pixel) [strictement après/avant] A/B (=> [ys-1 <=] A[1] < ys) (si A est au centre d'un pixel, le pixel n'est pas dans le triangle)
	float ys = max((float)round(A[1]), 0.0f) + .5f, ye = min((float)round(B[1]), screenHeight) - .5f;
	/// coord y du vecteur A->B (=A->C)
	float vy = B[1]-A[1];
	/// A et BC sont sur la même ligne horizontale -> le triangle est plat, il ne contient aucun point
	if(vy <= 0) return;

	/// var en x du bord gauche et droite par rapport à y
	float dxs = (B[0]-A[0]) / vy, dxe = (C[0]-A[0]) / vy;
	/// var en z du bord gauche et droite par rapport à y
	float dzs = (B[2]-A[2]) / vy, dze = (C[2]-A[2]) / vy;

	/// coord en x du centre du premier pixel de la première ligne
	float xs = A[0] + dxs*(ys-A[1]), xe = A[0] + dxe*(ys-A[1]);
	/// coord en z du centre du premier pixel de la première  ligne
	float zs = A[2] + dzs*(ys-A[1]), ze = A[2] + dze*(ys-A[1]);


	for(float y = ys; y <= ye; y += 1.0f)
	{
		/// coord en x du centre du pixel [strictement après/avant] le côté gauche/droit
		float lxs = max((float)round(xs), 0.0f) + .5f, lxe = min((float)round(xe), screenWidth) - .5f, lvx = xe - xs;
		/// var en z par rapport à x
		float ldz = (ze - zs) / lvx;
		/// coord z du point associé au centre du premier pixel de la première  ligne
		float lz = zs + ldz*(lxs-xs);
		for(float lx = lxs; lx <= lxe; lx += 1.0f) {
			putPointBufVolume(lx, y, lz, color, front);
			lz += ldz;
		}
		xs += dxs;
		xe += dxe;
		zs += dzs;
		ze += dze;
	}
}
/** A : DOWN, B : LEFT, C : RIGHT, By=Cy
 *	remplit les pixels dont le centre^+ est contenu dans le triangle
 **/
void Engine3D::triangleInfVolume(const float *A, const float *B, const float *C, rgb_f const& color, bool front) {
	/// suffix -> s : start (côté gauche/haut), e : end (côté droit/bas)
	/// preffix -> v : vecteur, d : delta (variation), l : ligne (concerne la ligne actuelle)

	/// ys in[i-0.5; i-0.5[ -> ys' = i -> ys'=ys'+0.5
	/// ye in[i-0.5; i-0.5[ -> ye' = i -> ye'=ye'-0.5
	/// => ys in triangle ABC, ye in triangle ABC

	/// coord y de bord sup du pixel dont le centre est strictement après A
	/// TODO float ysmin = clamp(0.0f, screenHeight, round(A[1]));
	/// coord y du centre (du pixel) [strictement après/avant] A/B (=> [ys-1 <=] A[1] < ys) (si A est au centre d'un pixel, le pixel n'est pas dans le triangle)
	float ys = max((float)round(B[1]), 0.0f) + .5f, ye = min((float)round(A[1]), screenHeight) - .5f;
	/// coord y du vecteur B->A.y (=C->Ay)
	float vy = A[1]-B[1];
	/// A et BC sont sur la même ligne horizontale -> le triangle est plat, il ne contient aucun point
	if(vy <= 0) return;

	/// var en x du bord gauche et droite par rapport à y
	float dxs = (A[0]-B[0]) / vy, dxe = (A[0]-C[0]) / vy;
	/// var en z du bord gauche et droite par rapport à y
	float dzs = (A[2]-B[2]) / vy, dze = (A[2]-C[2]) / vy;

	/// coord en x du centre du premier pixel de la première ligne
	float xs = B[0] + dxs*(ys-B[1]), xe = C[0] + dxe*(ys-C[1]);
	/// coord en z du centre du premier pixel de la première  ligne
	float zs = B[2] + dzs*(ys-B[1]), ze = C[2] + dze*(ys-C[1]);


	for(float y = ys; y <= ye; y += 1.0f)
	{
		/// coord en x du centre du pixel [strictement après/avant] le côté gauche/droit
		float lxs = max((float)round(xs), 0.0f) + .5f, lxe = min((float)round(xe), screenWidth) - .5f, lvx = xe - xs;
		/// var en z par rapport à x
		float ldz = (ze - zs) / lvx;
		/// coord z du point associé au centre du premier pixel de la première  ligne
		float lz = zs + ldz*(lxs-xs);
		for(float lx = lxs; lx <= lxe; lx += 1.0f) {
			putPointBufVolume(lx, y, lz, color, front);
			lz += ldz;
		}
		xs += dxs;
		xe += dxe;
		zs += dzs;
		ze += dze;
	}
}
/**
the coordinates of vertices are (A.x,A.y), (B.x,B.y), (C.x,C.y) we assume that A.y<=B.y<=C.y (you should sort them first)
vertex A has color (A.r,A.g,A.b), B (B.r,B.g,B.b), C (C.r,C.g,C.b), where X.r is color's red component, X.g is color's green component and X.b is color's blue component
dx1,dx2,dx3 are deltas used in interpolation of x-coordinate
dr1,dr2,dr3, dg1,dg2,dg3, db1,db2,db3 are deltas used in interpolation of color's components
putpixel(P) plots a pixel with coordinates (P.x,P.y) and color (P.r,P.g,P.b)
S=A means that S.x=A.x; S.y=A.y; S.r=A.r; S.g=A.g; S.b=A.b;*/
void Engine3D::triangleVolume(const float* A, const float* B, const float* C, rgb_f const& c, bool front) {
    if(A[1] <= B[1]) {
        if(B[1] <= C[1]) {
                //printf("A, B, C\n");
            triangleSortedPointsVolume(A, B, C, c, front);
        } else if(C[1] <= A[1]) {
                //printf("C, A, B\n");
            triangleSortedPointsVolume(C, A, B, c, front);
        } else {
                //printf("A, C, B\n");
            triangleSortedPointsVolume(A, C, B, c, front);
        }
    } else {
        if(A[1] <= C[1]) {
                //printf("B, A, C\n");
            triangleSortedPointsVolume(B, A, C, c, front);
        } else if(C[1] <= B[1]) {
                //printf("B, B, A\n");
            triangleSortedPointsVolume(C, B, A, c, front);
        } else {
               // printf("B, C, A\n");
            triangleSortedPointsVolume(B, C, A, c, front);
        }
    }
}


void Engine3D::triangleSortedPoints(rgb_f *dst, float* depth, const float *A, const float *B, const float *C, rgb_f const& colorA, rgb_f const& colorB, rgb_f const& colorC) {
	// cas A[1] == B[1] == C[1] => ne dessine rien
	/// côté inférieur horizontal
	if(B[1] == C[1]) {
		if(C[0] < B[0])
            triangleSup(dst, depth, A, C, B, colorA, colorC, colorB);
        else
            triangleSup(dst, depth, A, B, C, colorA, colorB, colorC);
	}
	/// côté supérieur horizontal
	else if(A[1] == B[1]) {
		if(A[0] < B[0])
            triangleInf(dst, depth, C, A, B, colorC, colorA, colorB);
		else
            triangleInf(dst, depth, C, B, A, colorC, colorB, colorA);
	}
	/// côté supérieur horizontal
	else if(A[1] == C[1]) {
		if(A[0] < C[0])
            triangleInf(dst, depth, B, A, C, colorB, colorA, colorC);
        else
            triangleInf(dst, depth, B, C, A, colorB, colorC, colorA);
	}
	/// pas de côté horizontal
	else {
		if(C[1] < B[1]) {
            /// On calcule I, le point de AC de même y que B <=> AI.y = k * AC.y = AB.y
            float k = (C[1]-A[1]) / (B[1]-A[1]); /// rmq : 0 < k < 1
            float I[3] = {(B[0]-A[0]) * k + A[0], C[1], (B[2]-A[2]) * k + A[2]};
            rgb_f colorI((colorB.r-colorA.r) * k + colorA.r, (colorB.g-colorA.g) * k + colorA.g, (colorB.b-colorA.b) * k + colorA.b);

            /// I est à gauche de C
            if(I[0] < C[0]) {
                /// on dessine le triangle supérieur AIC
                triangleSup(dst, depth, A, I, C, colorA, colorI, colorC);
                /// on dessine le triangle inférieur BCI
                triangleInf(dst, depth, B, I, C, colorB, colorI, colorC);
            }
            /// C est à gauche de A
            else {
                /// on dessine le triangle supérieur ACI
                triangleSup(dst, depth, A, C, I, colorA, colorC, colorI);
                /// on dessine le triangle inférieur BCI
                triangleInf(dst, depth, B, C, I, colorB, colorC, colorI);
            }
		}
		else {
            /// On calcule I, le point de AC de même y que B <=> AI.y = k * AC.y = AB.y
            float k = (B[1]-A[1]) / (C[1]-A[1]); /// rmq : 0 < k < 1
            float I[3] = {(C[0]-A[0]) * k + A[0], B[1], (C[2]-A[2]) * k + A[2]};
            rgb_f colorI((colorC.r-colorA.r) * k + colorA.r, (colorC.g-colorA.g) * k + colorA.g, (colorC.b-colorA.b) * k + colorA.b);

            /// I est à gauche de B
            if(I[0] < B[0]) {
                /// on dessine le triangle supérieur AIB
                triangleSup(dst, depth, A, I, B, colorA, colorI, colorB);
                /// on dessine le triangle inférieur CBI
                triangleInf(dst, depth, C, I, B, colorC, colorI, colorB);
            }
            /// B est à gauche de A
            else {
                /// on dessine le triangle supérieur ABI
                triangleSup(dst, depth, A, B, I, colorA, colorB, colorI);
                /// on dessine le triangle inférieur CBI
                triangleInf(dst, depth, C, B, I, colorC, colorB, colorI);
            }
		}
	}
}
/** A : UP, B : LEFT, C : RIGHT, By=Cy
 *	remplit les pixels dont le centre est contenu dans le triangle
 **/
void Engine3D::triangleSup(rgb_f *dst, float* depth, const float *A, const float *B, const float *C, rgb_f const& colorA, rgb_f const& colorB, rgb_f const& colorC) {
	/// suffix -> s : start (côté gauche/haut), e : end (côté droit/bas)
	/// preffix -> v : vecteur, d : delta (variation), l : ligne (concerne la ligne actuelle)

	/// ys in[i-0.5; i-0.5[ -> ys' = i -> ys'=ys'+0.5
	/// ye in[i-0.5; i-0.5[ -> ye' = i -> ye'=ye'-0.5
	/// => ys in triangle ABC, ye in triangle ABC

	/// coord y de bord sup du pixel dont le centre est strictement après A
	//float ysmin = round(A[1]);
	/// coord y du centre (du pixel) [strictement après/avant] A/B (=> [ys-1 <=] A[1] < ys) (si A est au centre d'un pixel, le pixel n'est pas dans le triangle)
	float ys = max((float)round(A[1]), 0.0f) + .5f, ye = min((float)round(B[1]), screenHeight) - .5f;
	/// coord y du vecteur A->B (=A->C)
	float vy = B[1]-A[1];
	/// A et BC sont sur la même ligne horizontale -> le triangle est plat, il ne contient aucun point
	if(vy <= 0) return;

	/// var en x du bord gauche et droite par rapport à y
	float dxs = (B[0]-A[0]) / vy, dxe = (C[0]-A[0]) / vy;
	/// var en z du bord gauche et droite par rapport à y
	float dzs = (B[2]-A[2]) / vy, dze = (C[2]-A[2]) / vy;
	/// var de la couleur par rapport à y
	float drs = (colorB.r-colorA.r) / vy, dre = (colorC.r-colorA.r) / vy;
	float dgs = (colorB.g-colorA.g) / vy, dge = (colorC.g-colorA.g) / vy;
	float dbs = (colorB.b-colorA.b) / vy, dbe = (colorC.b-colorA.b) / vy;

    /// différence de y entre le sommet du triangle et le premier pixel de la première ligne
    float vy1 = (ys-A[1]);
	/// coord en x du centre du premier pixel de la première ligne
	float xs = A[0] + dxs*vy1, xe = A[0] + dxe*vy1;
	/// coord en z du centre du premier pixel de la première  ligne
	float zs = A[2] + dzs*vy1, ze = A[2] + dze*vy1;
	/// couleur au centre du premier pixel de la première  ligne
	float rs = colorA.r + drs*vy1, re = colorA.r + dre*vy1;
	float gs = colorA.g + dgs*vy1, ge = colorA.g + dge*vy1;
	float bs = colorA.b + dbs*vy1, be = colorA.b + dbe*vy1;


	for(float y = ys; y <= ye; y += 1.0f)
	{
		/// coord en x du centre du pixel [strictement après/avant] le côté gauche/droit
		float lxs = max((float)round(xs), 0.0f) + .5f, lxe = min((float)round(xe), screenWidth) - .5f, lvx = xe - xs;
		/// var en z par rapport à x
		float ldz = (ze - zs) / lvx;
		/// var de la couleur par rapport à x
		float ldr = (re - rs) / lvx;
		float ldg = (ge - gs) / lvx;
		float ldb = (be - bs) / lvx;

		/// Différence entre le bord de gauche et le premier pixel de la ligne
		float vx1 = (lxs-xs);
		/// coord z du point associé au centre du premier pixel de la première  ligne
		float lz = zs + ldz*vx1;
		/// couleur du point associé au centre du premier pixel de la première  ligne
		rgb_f lc(rs + ldr*vx1,
                gs + ldg*vx1,
                bs + ldb*vx1);

		for(float lx = lxs; lx <= lxe; lx += 1.0f) {
			putPointBuf(dst, depth, lx, y, lz, lc);
			lz += ldz;
			lc.r += ldr;
			lc.g += ldg;
			lc.b += ldb;
		}
		xs += dxs; xe += dxe;
		zs += dzs; ze += dze;
		rs += drs; re += dre;
		gs += dgs; ge += dge;
		bs += dbs; be += dbe;
	}
}
/** A : DOWN, B : LEFT, C : RIGHT, By=Cy
 *	remplit les pixels dont le centre^+ est contenu dans le triangle
 **/
void Engine3D::triangleInf(rgb_f *dst, float* depth, const float *A, const float *B, const float *C, rgb_f const& colorA, rgb_f const& colorB, rgb_f const& colorC) {
	/// suffix -> s : start (côté gauche/haut), e : end (côté droit/bas)
	/// preffix -> v : vecteur, d : delta (variation), l : ligne (concerne la ligne actuelle)

	/// ys in[i-0.5; i-0.5[ -> ys' = i -> ys'=ys'+0.5
	/// ye in[i-0.5; i-0.5[ -> ye' = i -> ye'=ye'-0.5
	/// => ys in triangle ABC, ye in triangle ABC

	/// coord y de bord sup du pixel dont le centre est strictement après A
	//float ysmin = round(A[1]);
	/// coord y du centre (du pixel) [strictement après/avant] A/B (=> [ys-1 <=] A[1] < ys) (si A est au centre d'un pixel, le pixel n'est pas dans le triangle)
	float ys = max((float)round(B[1]), 0.0f) + .5f, ye = min((float)round(A[1]), screenHeight) - .5f;
	/// coord y du vecteur B->A.y (=C->Ay)
	float vy = A[1]-B[1];
	/// A et BC sont sur la même ligne horizontale -> le triangle est plat, il ne contient aucun point
	if(vy <= 0) return;

	/// var en x du bord gauche et droite par rapport à y
	float dxs = (A[0]-B[0]) / vy, dxe = (A[0]-C[0]) / vy;
	/// var en z du bord gauche et droite par rapport à y
	float dzs = (A[2]-B[2]) / vy, dze = (A[2]-C[2]) / vy;
	/// var de la couleur par rapport à y
	float drs = (colorA.r-colorB.r) / vy, dre = (colorA.r-colorC.r) / vy;
	float dgs = (colorA.g-colorB.g) / vy, dge = (colorA.g-colorC.g) / vy;
	float dbs = (colorA.b-colorB.b) / vy, dbe = (colorA.b-colorC.b) / vy;

    /// différence de y entre le haut du triangle et le premier pixel de la première ligne
    float vy1 = (ys-B[1]);
	/// coord en x du centre du premier pixel de la première ligne
	float xs = B[0] + dxs*vy1, xe = C[0] + dxe*vy1;
	/// coord en z du centre du premier pixel de la première  ligne
	float zs = B[2] + dzs*vy1, ze = C[2] + dze*vy1;
	/// couleur au centre du premier pixel de la première  ligne
	float rs = colorB.r + drs*vy1, re = colorC.r + dre*vy1;
	float gs = colorB.g + dgs*vy1, ge = colorC.g + dge*vy1;
	float bs = colorB.b + dbs*vy1, be = colorC.b + dbe*vy1;


	for(float y = ys; y <= ye; y += 1.0f)
	{
		/// coord en x du centre du pixel [strictement après/avant] le côté gauche/droit
		float lxs = max((float)round(xs), 0.0f) + .5f, lxe = min((float)round(xe), screenWidth) - .5f, lvx = xe - xs;
		/// var en z par rapport à x
		float ldz = (ze - zs) / lvx;
		/// var de la couleur par rapport à x
		float ldr = (re - rs) / lvx;
		float ldg = (ge - gs) / lvx;
		float ldb = (be - bs) / lvx;

		/// Différence entre le bord de gauche et le premier pixel de la ligne
		float vx1 = (lxs-xs);
		/// coord z du point associé au centre du premier pixel de la première  ligne
		float lz = zs + ldz*vx1;
		/// couleur du point associé au centre du premier pixel de la première  ligne
		rgb_f lc(rs + ldr*vx1,
                gs + ldg*vx1,
                bs + ldb*vx1);

		for(float lx = lxs; lx <= lxe; lx += 1.0f) {
			putPointBuf(dst, depth, lx, y, lz, lc);
			lz += ldz;
			lc.r += ldr;
			lc.g += ldg;
			lc.b += ldb;
		}
		xs += dxs; xe += dxe;
		zs += dzs; ze += dze;
		rs += drs; re += dre;
		gs += dgs; ge += dge;
		bs += dbs; be += dbe;
	}
}
/**
the coordinates of vertices are (A.x,A.y), (B.x,B.y), (C.x,C.y) we assume that A.y<=B.y<=C.y (you should sort them first)
vertex A has color (A.r,A.g,A.b), B (B.r,B.g,B.b), C (C.r,C.g,C.b), where X.r is color's red component, X.g is color's green component and X.b is color's blue component
dx1,dx2,dx3 are deltas used in interpolation of x-coordinate
dr1,dr2,dr3, dg1,dg2,dg3, db1,db2,db3 are deltas used in interpolation of color's components
putpixel(P) plots a pixel with coordinates (P.x,P.y) and color (P.r,P.g,P.b)
S=A means that S.x=A.x; S.y=A.y; S.r=A.r; S.g=A.g; S.b=A.b;*/
void Engine3D::triangle(rgb_f *dst, float* depth, const float* /*_*/A, const float* /*_*/B, const float* /*_*/C, rgb_f const& /*_*/colorA, rgb_f const& /*_*/colorB, rgb_f const& /*_*/colorC) {
    /*float A[3], B[3], C[3];
    memcpy(A, _A, sizeof(float) * 3);
    memcpy(B, _B, sizeof(float) * 3);
    memcpy(C, _C, sizeof(float) * 3);
    rgb_f colorA, colorB, colorC;
    memcpy(&colorA, &_colorA, sizeof(rgb_f));
    memcpy(&colorB, &_colorB, sizeof(rgb_f));
    memcpy(&colorC, &_colorC, sizeof(rgb_f));


    /// on ne garde que le triangle visible (zNear)
    if(A[2] > -zNear) {
        if(B[2] > -zNear) {
            float dAz = (C[2] - A[2]) == 0 ? 0 : ((-zNear - A[2]) / (C[2] - A[2]));
            A[0] += (C[0] - A[0]) * dAz;
            A[1] += (C[1] - A[1]) * dAz;
            A[2] = -zNear;
            colorA.r += (colorC.r-colorA.r) * dAz;
            colorA.g += (colorC.g-colorA.g) * dAz;
            colorA.b += (colorC.b-colorA.b) * dAz;
            float dBz = (C[2] - B[2]) == 0 ? 0 : ((-zNear - B[2]) / (C[2] - B[2]));
            B[0] += (C[0] - B[0]) * dBz;
            B[1] += (C[1] - B[1]) * dBz;
            B[2] = -zNear;
            colorB.r += (colorC.r-colorB.r) * dBz;
            colorB.g += (colorC.g-colorB.g) * dBz;
            colorB.b += (colorC.b-colorB.b) * dBz;
            //printf("A B < zNear\n");
            //printf("1 %f <- %f\n", triangleArea(A,B,C), triangleArea(_A,_B,_C));
        }
        else if(C[2] > -zNear) {
            float dAz = (B[2] - A[2]) == 0 ? 0 : ((-zNear - A[2]) / (B[2] - A[2]));
            A[0] += (B[0] - A[0]) * dAz;
            A[1] += (B[1] - A[1]) * dAz;
            A[2] = -zNear;
            colorA.r += (colorB.r-colorA.r) * dAz;
            colorA.g += (colorB.g-colorA.g) * dAz;
            colorA.b += (colorB.b-colorA.b) * dAz;
            float dCz = (B[2] - C[2]) == 0 ? 0 : ((-zNear - C[2]) / (B[2] - C[2]));
            C[0] += (B[0] - C[0]) * dCz;
            C[1] += (B[1] - C[1]) * dCz;
            C[2] = -zNear;
            colorC.r += (colorB.r-colorC.r) * dCz;
            colorC.g += (colorB.g-colorC.g) * dCz;
            colorC.b += (colorB.b-colorC.b) * dCz;
            //printf("A C < zNear\n");
            //printf("2 %f <- %f\n", triangleArea(A,B,C), triangleArea(_A,_B,_C));
        }
    }
    else if(B[2] > -zNear && C[2] > -zNear) {
        float dBz = (A[2] - B[2]) == 0 ? 0 : ((-zNear - B[2]) / (A[2] - B[2]));
        B[0] += (A[0] - B[0]) * dBz;
        B[1] += (A[1] - B[1]) * dBz;
        B[2] = -zNear;
        colorB.r += (colorA.r-colorB.r) * dBz;
        colorB.g += (colorA.g-colorB.g) * dBz;
        colorB.b += (colorA.b-colorB.b) * dBz;
        float dCz = (A[2] - C[2]) == 0 ? 0 : ((-zNear - C[2]) / (A[2] - C[2]));
        C[0] += (A[0] - C[0]) * dCz;
        C[1] += (A[1] - C[1]) * dCz;
        C[2] = -zNear;
        colorC.r += (colorA.r-colorC.r) * dCz;
        colorC.g += (colorA.g-colorC.g) * dCz;
        colorC.b += (colorA.b-colorC.b) * dCz;
            //printf("C B < zNear\n");
            //printf("3 %f <- %f\n", triangleArea(A,B,C), triangleArea(_A,_B,_C));
    }*/

    /// / !!!
    //if(triangleArea(_A,_B,_C) > width*height)
    //    return;

    if(A[1] <= B[1]) {
        if(B[1] <= C[1]) {
            //if(C[1] >= 0.0f && A[1] < screenHeight) /// Ay <= By <= Cy
                triangleSortedPoints(dst, depth, A, B, C, colorA, colorB, colorC);
        } else if(C[1] <= A[1]) {
            //if(B[1] >= 0.0f && C[1] < screenHeight) /// Cy <= Ay <= By
                triangleSortedPoints(dst, depth, C, A, B, colorC, colorA, colorB);
        } else {
            //if(B[1] >= 0.0f && A[1] < screenHeight) /// Ay < Cy < By
                triangleSortedPoints(dst, depth, A, C, B, colorA, colorC, colorB);
        }
    } else {
        if(A[1] <= C[1]) {
            //if(C[1] >= 0.0f && B[1] < screenHeight) /// By < Ay <= Cy
                triangleSortedPoints(dst, depth, B, A, C, colorB, colorA, colorC);
        } else if(C[1] <= B[1]) {
            //if(A[1] >= 0.0f && C[1] < screenHeight) /// Cy <= By < Ay
                triangleSortedPoints(dst, depth, C, B, A, colorC, colorB, colorA);
        } else {
            //if(A[1] >= 0.0f && B[1] < screenHeight) /// By < Cy < Ay
                triangleSortedPoints(dst, depth, B, C, A, colorB, colorC, colorA);
        }
    }
}


