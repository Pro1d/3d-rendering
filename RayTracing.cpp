#include <list>
#include <vector>

#include "RayTracing.h"
#include "color.h"
#include "Scene.h"
#include "tools.h"
#include "Physic3D.h"
#include "Engine3D.h"
#include "KDTree.h"

using std::list;
using std::vector;

RayTracing::RayTracing()
{
    //ctor
}

RayTracing::~RayTracing()
{
}
/** TODO
    retourner distance
    calculer point de contact dans l'espace
    calculer point de contact sur le triangle (a*A+b*B+c*C = P ->a+b+c=1)
        interpoler couleur (texture ?)
        interpoler normal (normal texture ?) (linéaire ou rotation)
            calculer lumière diffuse (collision entre point/lumière avec objet)
            calculer lumière spécular (collision entre point/lumière avec objet ?) ?
    calculer rayon réfléchi + coefficient rfl
    calculer rayon refracté + coefficient rfr (rfl + rfr = 1)
*/

bool RayTracing::updateMinFaceCollision(list<int> const& faces, int &faceColl, float &distMin, FaceCollParams &params) {
    bool updated = false;
    for(list<int>::const_iterator it = faces.begin(); it != faces.end(); ++it)
    if(params.lastFaceColl != *it) {
        Drawable_Face &f = params.sceneBuffer.getFace(*it);
        Transf_Vertex &v1 = params.sceneBuffer.getVertex(f.vertex_indices[0]);
        Transf_Vertex &v2 = params.sceneBuffer.getVertex(f.vertex_indices[1]);
        Transf_Vertex &v3 = params.sceneBuffer.getVertex(f.vertex_indices[2]);
        float dot = f.n[0]*params.ray.dir[0] + f.n[1]*params.ray.dir[1] + f.n[2]*params.ray.dir[2];
        if(dot < 0 || ((const Object3D*)v1.objParent)->isVolumeTranslucent)
        if(Physic3D::collisionFaceStraightLine(v1.p, v2.p, v3.p, params.ray.dir, params.ray.orig)) /// TODO collision face orientée
        {
            float d = getDist(v1, v2, v3, params.ray);
            if(d > 0 && (faceColl == -1 || d < distMin))
            {
                distMin = d;
                faceColl = *it;
                params.out_reverseSide = (dot >= 0);
                updated = true;
            }
        }
    }
    return updated;
}
/// ////////////////////////////////////////////////////////////////// ///
/// TODO moins de paramètres pour plus de performance ? Pas récursif ? ///
/// ////////////////////////////////////////////////////////////////// ///

int RayTracing::getFaceCollision(RayData const& ray, Scene &sceneBuffer, float &out_dist, bool &out_reverseSide, int lastFaceColl) {
    FaceCollParams params(ray, sceneBuffer, out_dist, out_reverseSide, lastFaceColl);
    return getFaceCollision(params, currentTree);
}
int RayTracing::getFaceCollision(FaceCollParams &params, KDTree *currentTree)
{
    int first, second;
    currentTree->getSubTreeColl(params.ray.orig, params.ray.dir, first, second);

    params.out_dist = 0;
    int faceColl = -1;

    if(first != -1) {
        /// recherche dans le premier sous bloc
        faceColl = getFaceCollision(params, currentTree->subTree[first]);

        /// si toujours rien trouvé...
        if(faceColl == -1) {
            /*if(second == -2)
                second = currentTree->getSecondSubTreeColl(ray.orig, ray.dir, first);*/

            /// recherche dans le deuxième sous bloc
            if(second != -1)
                faceColl = getFaceCollision(params, currentTree->subTree[second]);
        }
    }

    /// recherche parmi les faces de cette branche
    updateMinFaceCollision(currentTree->faceInside, faceColl, params.out_dist, params);

    return faceColl;
}
/*bool RayTracing::isFaceCollision(const float *rayDir, const float *rayOrig, Scene const &sceneBuffer, float distMax, int lastFaceColl) {
    FaceCollParams params(ray, sceneBuffer, out_dist, out_reverseSide, lastFaceColl);
    return
}*/
bool RayTracing::isFaceCollision(const float *rayDir, const float *rayOrig, Scene const &sceneBuffer, float distMax, int lastFaceColl, KDTree *currentTree)
{
    if(currentTree == NULL)
        currentTree = this->currentTree;

    if(currentTree->isSubTreeCollWithRay(rayOrig, rayDir, 0)
            && isFaceCollision(rayDir, rayOrig, sceneBuffer, distMax, lastFaceColl, currentTree->subTree[0]))
        return true;
    if(currentTree->isSubTreeCollWithRay(rayOrig, rayDir, 1)
            && isFaceCollision(rayDir, rayOrig, sceneBuffer, distMax, lastFaceColl, currentTree->subTree[1]))
        return true;
    for(list<int>::const_iterator it = currentTree->faceInside.begin(); it != currentTree->faceInside.end(); ++it)
    if(lastFaceColl != *it) {
        Drawable_Face const&f = sceneBuffer.getFace(*it);
        Transf_Vertex const&v1 = sceneBuffer.getVertex(f.vertex_indices[0]);
        Transf_Vertex const&v2 = sceneBuffer.getVertex(f.vertex_indices[1]);
        Transf_Vertex const&v3 = sceneBuffer.getVertex(f.vertex_indices[2]);
        if(Physic3D::collisionFaceStraightLineDirect(v1.p, v2.p, v3.p, f.n, rayDir, rayOrig)) { /// TODO collision face orientée
            float dist = Physic3D::collisionFaceVectorCoef(v1.p, v1.n, rayDir, rayOrig);
            if(dist > 0 && dist < distMax)
                return true;
        }
    }

    return false;
}/*
int RayTracing::getFaceCollision(RayData const& ray, Scene &sceneBuffer, float &distOut, bool &reverseSideOut, int lastFaceColl)
{
    ListCell *cell = currentTree->getFaceInBoxCollWithRay_M_kay_IfYouSeeWhatIMean(ray.orig, ray.dir);

    float distMin = 0;
    int faceColl = -1;

    while(cell != NULL) {
        for(list<int>::const_iterator it = cell->faces.begin(); it != cell->faces.end(); ++it)
        if(lastFaceColl != *it) {
            Drawable_Face &f = sceneBuffer.getFace(*it);
            Transf_Vertex &v1 = sceneBuffer.getVertex(f.vertex_indices[0]);
            Transf_Vertex &v2 = sceneBuffer.getVertex(f.vertex_indices[1]);
            Transf_Vertex &v3 = sceneBuffer.getVertex(f.vertex_indices[2]);
            float dot = f.n[0]*ray.dir[0] + f.n[1]*ray.dir[1] + f.n[2]*ray.dir[2];
            if(dot < 0 || ((const Object3D*)v1.objParent)->isVolumeTranslucent)
            if(Physic3D::collisionFaceStraightLine(v1.p, v2.p, v3.p, ray.dir, ray.orig)) /// TODO collision face orientée
            {
                float d = getDist(v1, v2, v3, ray);
                if(d > 0 && (faceColl == -1 || d < distMin))
                {
                    distMin = d;
                    faceColl = *it;
                    reverseSideOut = dot >= 0;
                }
            }
        }
        ListCell *c = cell->next;
        delete cell;
        cell = c;
    }

    distOut = distMin;
    return faceColl;
}
bool RayTracing::isFaceCollision(const float *rayDir, const float *rayOrig, Scene const &sceneBuffer, float distMax, int lastFaceColl)
{
    ListCell *cell = currentTree->getFaceInBoxCollWithRay_M_kay_IfYouSeeWhatIMean(rayOrig, rayDir);

    while(cell != NULL) {
        for(list<int>::const_iterator it = cell->faces.begin(); it != cell->faces.end(); ++it)
        if(lastFaceColl != *it) {
            Drawable_Face const&f = sceneBuffer.getFace(*it);
            Transf_Vertex const&v1 = sceneBuffer.getVertex(f.vertex_indices[0]);
            Transf_Vertex const&v2 = sceneBuffer.getVertex(f.vertex_indices[1]);
            Transf_Vertex const&v3 = sceneBuffer.getVertex(f.vertex_indices[2]);
            if(Physic3D::collisionFaceStraightLineDirect(v1.p, v2.p, v3.p, f.n, rayDir, rayOrig)) { /// TODO collision face orientée
                float dist = Physic3D::collisionFaceVectorCoef(v1.p, v1.n, rayDir, rayOrig);
                if(dist > 0 && dist < distMax)
                    return true;
            }
        }

        ListCell *c = cell->next;
        delete cell;
        cell = c;
    }

    return false;
}*/

/*
int RayTracing::getFaceCollision(const float *rayDir, const float *rayOrig, Scene &sceneBuffer, float &distOut, int lastFaceColl) {
    float distMin = 0;
    int faceColl = -1;
    for(int i = 0; i < sceneBuffer.getFacesCount(); i++)
    if(lastFaceColl != i) {
        Drawable_Face &f = sceneBuffer.getFace(i);
        Transf_Vertex &v1 = sceneBuffer.getVertex(f.vertex_indices[0]);
        Transf_Vertex &v2 = sceneBuffer.getVertex(f.vertex_indices[1]);
        Transf_Vertex &v3 = sceneBuffer.getVertex(f.vertex_indices[2]);
        if(Physic3D::collisionFaceStraightLine(v1.p, v2.p, v3.p, rayDir, rayOrig))
        {
            float d = Physic3D::collisionFaceVectorCoef(v1.p, v1.n, rayDir, rayOrig);
            if(d > 0 && (faceColl == -1 || d < distMin))
            {
                distMin = d;
                faceColl = i;
            }
        }
    }
    distOut = distMin;
    return faceColl;
}
*/
void RayTracing::getCoef(Transf_Vertex const& A, Transf_Vertex const& B, Transf_Vertex const& C, RayData const& ray, float *coef)
{
    float a,b,c;
    float OA[3] = { A.p[0]-ray.orig[0], A.p[1]-ray.orig[1], A.p[2]-ray.orig[2] };
    float OB[3] = { B.p[0]-ray.orig[0], B.p[1]-ray.orig[1], B.p[2]-ray.orig[2] };
    float OC[3] = { C.p[0]-ray.orig[0], C.p[1]-ray.orig[1], C.p[2]-ray.orig[2] };
    float OA_[3] = {
        ray.dir[1]*OA[2] - OA[1]*ray.dir[2],
        ray.dir[2]*OA[0] - OA[2]*ray.dir[0],
        ray.dir[0]*OA[1] - OA[0]*ray.dir[1]
    };
    float OB_[3] = {
        ray.dir[1]*OB[2] - OB[1]*ray.dir[2],
        ray.dir[2]*OB[0] - OB[2]*ray.dir[0],
        ray.dir[0]*OB[1] - OB[0]*ray.dir[1]
    };
    float OC_[3] = {
        ray.dir[1]*OC[2] - OC[1]*ray.dir[2],
        ray.dir[2]*OC[0] - OC[2]*ray.dir[0],
        ray.dir[0]*OC[1] - OC[0]*ray.dir[1]
    };
    float AB_[3] = {
        -OA_[0] + OB_[0],
        -OA_[1] + OB_[1],
        -OA_[2] + OB_[2]
    }, norm2AB_ = AB_[0]*AB_[0]+AB_[1]*AB_[1]+AB_[2]*AB_[2];
    float AC_[3] = {
        -OA_[0] + OC_[0],
        -OA_[1] + OC_[1],
        -OA_[2] + OC_[2]
    }, norm2AC_ = AC_[0]*AC_[0]+AC_[1]*AC_[1]+AC_[2]*AC_[2];
    /// A'B' . A'O = -A'B' . OA'
    float AB_dotOA_ = -(AB_[0]*OA_[0] + AB_[1]*OA_[1] + AB_[2]*OA_[2]) / norm2AB_;
    /// A'C' . A'O = -A'C' . OA'
    float AC_dotOA_ = -(AC_[0]*OA_[0] + AC_[1]*OA_[1] + AC_[2]*OA_[2]) / norm2AC_;
    /// A'B' . A'C'
    float AB_dotAC_ = (AB_[0]*AC_[0] + AB_[1]*AC_[1] + AB_[2]*AC_[2]);
    b = (AB_dotOA_ - AC_dotOA_*AB_dotAC_/norm2AB_) / (1 - AB_dotAC_*AB_dotAC_/(norm2AB_*norm2AC_));
    b = clampIn01(b);
    c = AC_dotOA_ - b*AB_dotAC_/norm2AC_;//(AC_dotOA_ - AB_dotOA_*AB_dotAC_/norm2AC_) / (1 - AB_dotAC_*AB_dotAC_/(norm2AB_*norm2AC_));
    c = clamp(0, 1-b, c);
    a = 1-b-c;
    coef[0] = a;
    coef[1] = b;
    coef[2] = c;
}
float RayTracing::getDist(Transf_Vertex const& A, Transf_Vertex const& B, Transf_Vertex const& C, RayData const& ray)
{
    float coef[3];
    getCoef(A, B, C, ray, coef);

    float p[3] = {
        coef[0]*A.p[0] + coef[1]*B.p[0] + coef[2]*C.p[0] - ray.orig[0],
        coef[0]*A.p[1] + coef[1]*B.p[1] + coef[2]*C.p[1] - ray.orig[1],
        coef[0]*A.p[2] + coef[1]*B.p[2] + coef[2]*C.p[2] - ray.orig[2]
    };
    /// direction rayon dot OP' (P' point d'intersection rayon/face)
    float dirdotp = ray.dir[0]*p[0] + ray.dir[1]*p[1] + ray.dir[2]*p[2];

    return (dirdotp < 0 ? -1.0f : 1.0f) / magic_sqrt_inv(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
}

/**
Onde transverse élèctrique, polarisé perpendiculairement à un truc
    R = (n1 * cos a1 - n2 * cos a2) / (n1 * cos a1 + n2 * cos a2)
    T = (2 * n1 * cos a1) / (n1 * cos a1 + n2 * cos a2)
    -> R² + T² * (n2 * cos a2 / n1 * cos a1) = 1
*/

/// retourne vrai si il ya refraction, faux si refection totale); n1 milieu actuel , n2 milieu coté source
float RayTracing::getRefractRay(RayData &rayTransmit, const float *orig, const float *normal, float n1, float n2)
{
    float e = n1/n2;
    float dotI = -(rayTransmit.dir[0]*normal[0] + rayTransmit.dir[1]*normal[1] + rayTransmit.dir[2]*normal[2]);
    float dotT2 = 1.0f - e*e*(1.0f - dotI*dotI);
    if(dotT2 <= 0)
        return 0.0f;

    float dotT = 1.0f / magic_sqrt_inv(dotT2);

    rayTransmit.dir[0] = e*rayTransmit.dir[0] + (e*dotI - dotT)*normal[0];
    rayTransmit.dir[1] = e*rayTransmit.dir[1] + (e*dotI - dotT)*normal[1];
    rayTransmit.dir[2] = e*rayTransmit.dir[2] + (e*dotI - dotT)*normal[2];

    float cos1 = -(rayTransmit.dir[0]*normal[0] + rayTransmit.dir[1]*normal[1] + rayTransmit.dir[2]*normal[2]);
    float cos2 = dotI;
    n1 *= cos1;
    n2 *= cos2;
    /// onde transverse élèctrique perpediculaire ! lol.
    float coef = clampIn01(4*n1*n2 / ((n1+n2)*(n1+n2)));
    rayTransmit.intensity *= coef;
    memcpy(rayTransmit.orig, orig, sizeof(float)*3);

    return coef;
}
float RayTracing::getReflectRay(RayData &rayReflect, const float *orig, const float* normal, float n1, float n2, bool translucent)
{
    float e = n1/n2;
    float dotI = -(rayReflect.dir[0]*normal[0] + rayReflect.dir[1]*normal[1] + rayReflect.dir[2]*normal[2]);
    float dotT2 = 1.0f - e*e*(1.0f - dotI*dotI);

    float coef = 1.0f;
    // Reflection total
    if(dotT2 > 0 && translucent) {
        float dotT = 1.0f / magic_sqrt_inv(dotT2);
        float transmitDir[3] = {
            e*rayReflect.dir[0] + (e*dotI - dotT)*normal[0],
            e*rayReflect.dir[1] + (e*dotI - dotT)*normal[1],
            e*rayReflect.dir[2] + (e*dotI - dotT)*normal[2]
        };

        float cos1 = dotI;
        float cos2 = -(transmitDir[0]*normal[0] + transmitDir[1]*normal[1] + transmitDir[2]*normal[2]);
        n1 *= cos1;
        n2 *= cos2;

        coef = clampIn01(((n1-n2)*(n1-n2)) / ((n1+n2)*(n1+n2)));
    }
    rayReflect.intensity *= coef;

    Physic3D::collisionFaceVectorBounce(normal, rayReflect.dir, rayReflect.dir);
    memcpy(rayReflect.orig, orig, sizeof(float)*3);

    return coef;
}

rgb_f RayTracing::getColorLight(Scene const& sceneBuffer, rgb_f const& color, Object3D const& obj, float *normal, float *position, int faceColl, float specularCoef)
{
    const int lightsCount = sceneBuffer.getLightsCount();

    if(lightsCount == 0)
        return color;

    bool isOn[lightsCount];
    for(int i = lightsCount; --i >= 0;)
    {
        Light const& l = sceneBuffer.getLight(i);
        float dir[3]; l.getDirectionFromPoint(position, dir);
        float dist = l.getDistanceToPoint(position);
        isOn[i] = !isFaceCollision(dir, position, sceneBuffer, dist, faceColl);
        //printf("%d %f %f %f ", isOn[i], sceneBuffer.getLight(i).position[0], sceneBuffer.getLight(i).position[1], sceneBuffer.getLight(i).position[2]);
    }//printf("\n");

    /// Ambient
    rgb_f lightAmbient(sceneBuffer.getLightsAmbientSum());
    lightAmbient *= obj.ambient;

    /// Diffuse & coef
    rgb_f lightDiffuse;
    float SDcoefBuffer[lightsCount*2];
    for(int i = lightsCount; --i >= 0;) if(isOn[i]) {
        sceneBuffer.getLight(i).getSpecularAndDiffuseCoef(position, normal, &SDcoefBuffer[i*2], obj.shininess);
        lightDiffuse += sceneBuffer.getLight(i).diffuse * SDcoefBuffer[i*2];
    }
    lightDiffuse *= obj.diffuse;

    /// Specular
    rgb_f lightSpecular;
    for(int i = lightsCount; --i >= 0;) if(isOn[i])
        lightSpecular += sceneBuffer.getLight(i).specular * SDcoefBuffer[i*2+1];
    lightSpecular *= obj.specular;

    rgb_f c(color);
    c *= lightDiffuse + lightAmbient;
    c += lightSpecular * specularCoef;
    c += obj.emissive;
    //c.clamp();
    return c;
}
int reccMax = 0;
void RayTracing::getColorRay(RayData &ray, Scene &sceneBuffer, float currentIndice, int lastFaceColl, int recc) {
    #ifdef DEBUG
        if(++recc > reccMax)
            printf("r:%d i:%f\n", reccMax = recc, ray.intensity);
    #endif
    /// Intensité trop faible, invisible
    if(ray.intensity < 0.05f) {
        ray.intensity = 0;
        #ifdef DEBUG
            recc--;
        #endif
        return;
    }

    float dist;
    bool reverseSide = false;
    int faceColl = getFaceCollision(ray, sceneBuffer, dist, reverseSide, lastFaceColl);

    #ifdef DEBUG
        //printf("|< %d\n", faceColl);
    #endif // DEBUG

    /// Pas de collision
    if(faceColl == -1) {
        ray.color = getColorSky(ray.dir, sceneBuffer);
        ray.length = 1e10;
        #ifdef DEBUG
            recc--;
        #endif
        return;
    }


    ray.length = dist;

    Drawable_Face const& face = sceneBuffer.getFace(faceColl);
    /// Point de contact aA+bB+cC=P, a+b+c=1
    Transf_Vertex const& A = sceneBuffer.getVertex(face.vertex_indices[0]);
    Transf_Vertex const& B = sceneBuffer.getVertex(face.vertex_indices[1]);
    Transf_Vertex const& C = sceneBuffer.getVertex(face.vertex_indices[2]);
    Object3D const& objParent = * (const Object3D*) A.objParent;

    float coef[3];
    getCoef(A, B, C, ray, coef);
    float text[2] = {
        A.s*coef[0]+B.s*coef[1]+C.s*coef[2],
        A.t*coef[0]+B.t*coef[1]+C.t*coef[2]
    };
    float n[3] = {
        coef[0]*A.n[0] + coef[1]*B.n[0] + coef[2]*C.n[0],
        coef[0]*A.n[1] + coef[1]*B.n[1] + coef[2]*C.n[1],
        coef[0]*A.n[2] + coef[1]*B.n[2] + coef[2]*C.n[2]
    };
    normalizeVector(n);
    if(objParent.hasNormalTexture) {
        face.applyTBN(objParent.getNormalPoint(text[0], text[1]), n);
    }
    if(reverseSide) {
        n[0] = -n[0];
        n[1] = -n[1];
        n[2] = -n[2];
    }

    if(DRAW_NORMAL_MAP) {
        rgb_f xyz(*((rgb_f*) n));
        ray.color = (xyz + 1.0f) * 0.5f;

        return;
    }

    float p[3] = {
        coef[0]*A.p[0] + coef[1]*B.p[0] + coef[2]*C.p[0],
        coef[0]*A.p[1] + coef[1]*B.p[1] + coef[2]*C.p[1],
        coef[0]*A.p[2] + coef[1]*B.p[2] + coef[2]*C.p[2]
    };
    rgb_f clr(
        coef[0]*A.c.r + coef[1]*B.c.r + coef[2]*C.c.r,
        coef[0]*A.c.g + coef[1]*B.c.g + coef[2]*C.c.g,
        coef[0]*A.c.b + coef[1]*B.c.b + coef[2]*C.c.b
    );

    /// Texture
    if(objParent.hasTexture)
        clr *= objParent.getTexturePoint(text[0], text[1]);

    /// Light diffuse/ambient and specular
    float specCoef = 1.0f;
    if(objParent.hasSpecularTexture)
        specCoef = objParent.getSpecularPoint(text[0], text[1]);

    ray.color = getColorLight(sceneBuffer, clr, objParent, n, p, faceColl, specCoef);

    if(objParent.isVolumeTranslucent)
        ray.color *= 1.0f - objParent.reflect;

    /// Transmit
    float indice = reverseSide ? 1.0f : objParent.indice;
    if(objParent.isVolumeTranslucent) {
        RayData rayTransmit(ray);
        float coefTransmit = getRefractRay(rayTransmit, p, n, currentIndice, indice);

        if(coefTransmit > 0)
        {
            getColorRay(rayTransmit, sceneBuffer, indice, faceColl, recc);
            ray.color += rayTransmit.color*rayTransmit.intensity;
        }
    }

    /// Reflect
    RayData rayReflect(ray);
    float coefReflect = getReflectRay(rayReflect, p, n, currentIndice, indice, objParent.isVolumeTranslucent);
    bool enableAntiTotalReflect = coefReflect > 0.85f && objParent.isVolumeTranslucent;
    rayReflect.intensity *= specCoef * objParent.reflect * (enableAntiTotalReflect?0.75f:1.0f);
    getColorRay(rayReflect, sceneBuffer, currentIndice, faceColl, recc);

    if(rayReflect.intensity > 0)
        ray.color += rayReflect.color*(rayReflect.intensity / (enableAntiTotalReflect?0.75f:1.0f));
    #ifdef DEBUG
        recc--;
    #endif
}

rgb_f RayTracing::getColorSky(float *raydir, Scene &sceneBuffer) {
    if(!sceneBuffer.hasSkyBox())
        return rgb_f();

    RayData rd;
    rd.dir[0] = raydir[0]; rd.dir[1] = raydir[1]; rd.dir[2] = raydir[2];
    rd.orig[0] = rd.orig[1] = rd.orig[2] = 0;

    vector<Drawable_Face> const& faces = sceneBuffer.getSkyboxFaceBuffer();
    vector<Transf_Vertex> const& vertex = sceneBuffer.getSkyboxVertexBuffer();

    for(int i = 0; i < (int)faces.size(); i++) {
        Drawable_Face const& f = faces[i];
        Transf_Vertex const& v1 = vertex[f.vertex_indices[0]];
        Transf_Vertex const& v2 = vertex[f.vertex_indices[1]];
        Transf_Vertex const& v3 = vertex[f.vertex_indices[2]];
        if(Physic3D::collisionFaceStraightLineDirect(v1.p, v2.p, v3.p, f.n, rd.dir, rd.orig))
        {
            float c[3];
            getCoef(v1, v2, v3, rd, c);
            rgb_f clr(
                c[0]*v1.c.r + c[1]*v2.c.r + c[2]*v3.c.r,
                c[0]*v1.c.g + c[1]*v2.c.g + c[2]*v3.c.g,
                c[0]*v1.c.b + c[1]*v2.c.b + c[2]*v3.c.b
            );
            if(((const Object3D*)v1.objParent)->hasTexture)
                clr *= ((const Object3D*)v1.objParent)->getTexturePoint(v1.s*c[0]+v2.s*c[1]+v3.s*c[2],
                                                                        v1.t*c[0]+v2.t*c[1]+v3.t*c[2]);

            return clr;
        }
    }



    return rgb_f();
}

void RayTracing::render(Scene &sceneBuffer) {
    currentTree = new KDTree(sceneBuffer);

    //printf("render : %d %d %f->%f\n", width, height, zNear, -zNear/(width/2));
    float vx, vy = -1.0f * ((float)height / width), vz = -zNear/(width/2);
    float rayOrig[3] = {0,0,0};
    for(int y = 0; y < height; y++)
    {
        vx = -1.0f;
        for(int x = 0; x < width; x++)
        {
            #ifdef DEBUG
//                printf("%f %f %f\n", rayBack[0], rayBack[1], rayBack[2]);
            #endif
            float rayBack[3] = {vx, vy, vz};
            normalizeVector(rayBack);
            RayData ray;
            memcpy(ray.dir, rayBack, sizeof(float)*3);
            memcpy(ray.orig, rayOrig, sizeof(float)*3);
            ray.intensity = 1.0f;
            getColorRay(ray, sceneBuffer);
            colorBuf[x+width * (height - y - 1)] = ray.color*ray.intensity;
            depthBuf[x+width * (height - y - 1)] = ray.length;
            vx += 2.0f/width;
        }
        vy += 2.0f/height * ((float)height / width);
    }
    delete currentTree;
}
typedef struct RayTracing_params_t {
    RayTracing &rt;
    Scene &sceneBuffer;
} RayTracing_params_t;
void funcRayTracingRender(thread_params_t* params) {
    int height = ((RayTracing_params_t*)params->params)->rt.height;
    int width = ((RayTracing_params_t*)params->params)->rt.width;
    float zNear = ((RayTracing_params_t*)params->params)->rt.zNear;
    //printf("render : %d %d %f->%f\n", width, height, zNear, -zNear/(width/2));
    float vx, vy = -1.0f * ((float)height / width), vz = -zNear/(width/2);
    float rayOrig[3] = {0,0,0};
    vy +=  params->threadNum * 2.0f/height * ((float)height / width);
    for(int y = params->threadNum; y < height; y+=params->threadCount)
    {
        vx = -1.0f;
        for(int x = 0; x < width; x++)
        {
            #ifdef DEBUG
//                printf("%f %f %f\n", rayBack[0], rayBack[1], rayBack[2]);
            #endif
            float rayBack[3] = {vx, vy, vz};
            normalizeVector(rayBack);
            RayData ray;
            memcpy(ray.dir, rayBack, sizeof(float)*3);
            memcpy(ray.orig, rayOrig, sizeof(float)*3);
            ray.intensity = 1.0f;
            ((RayTracing_params_t*)params->params)->rt.getColorRay(ray, ((RayTracing_params_t*)params->params)->sceneBuffer);
            ((RayTracing_params_t*)params->params)->rt.colorBuf[x+width * (height - y - 1)] = ray.color*ray.intensity;
            ((RayTracing_params_t*)params->params)->rt.depthBuf[x+width * (height - y - 1)] = ray.length;
            vx += 2.0f/width;
        }
        //printf("%d : %f%%\n", params->threadNum, (float)y/height*100);
        vy += params->threadCount * 2.0f/height * ((float)height / width);
    }
}
void RayTracing::renderMutliThread(Scene &sceneBuffer, MultiThread &mt) {
    currentTree = new KDTree(sceneBuffer);

    RayTracing_params_t params = {*this, sceneBuffer};
    mt.execute(funcRayTracingRender, (void*)&params);

    delete currentTree;
}
