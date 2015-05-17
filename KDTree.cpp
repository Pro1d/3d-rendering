#include <vector>
#include <list>
#include "KDTree.h"
#include "Scene.h"
#include "vertex.h"

using namespace std;

bool isInside(float const *pt, KDTree const& box) {
    return pt[0] >= box.xmin && pt[0] <= box.xmax
        && pt[1] >= box.ymin && pt[1] <= box.ymax
        && pt[2] >= box.zmin && pt[2] <= box.zmax;
}

KDTree::KDTree(KDTree& parent, int subIndex, Scene const &scene, int height) :
        xmin(parent.xmin), xmax(parent.xmax), ymin(parent.ymin), ymax(parent.ymax), zmin(parent.zmin), zmax(parent.zmax)
{
    subTree[0] = subTree[1] = NULL;
    switch(parent.cutAxis) {
    case axisX:
        (subIndex? xmin : xmax) = (xmin + xmax) / 2;
        break;
    case axisY:
        (subIndex? ymin : ymax) = (ymin + ymax) / 2;
        break;
    case axisZ:
        (subIndex? zmin : zmax) = (zmin + zmax) / 2;
        break;
    }
    if(xmax-xmin > ymax-ymin) {
        if(zmax-zmin > xmax-xmin)
            cutAxis = axisZ;
        else
            cutAxis = axisX;
    }
    else if(zmax-zmin > ymax-ymin)
        cutAxis = axisZ;
    else
        cutAxis = axisY;

    list<int>::iterator it = parent.faceInside.begin();
    while(it != parent.faceInside.end())
    {
        Drawable_Face const& f(scene.getFace(*it));
        /// TODO : optimiser -> juste vérifier qu'il est du bon côté du plan de coupe
        if(isInside(scene.getVertex(f.vertex_indices[0]).p, *this)
           && isInside(scene.getVertex(f.vertex_indices[1]).p, *this)
           && isInside(scene.getVertex(f.vertex_indices[2]).p, *this))
        {
            faceInside.push_back(*it);
            it = parent.faceInside.erase(it);
        }
        else
            ++it;
    }
   // printf("$< %d : %d\n", height, faceInside.size());

    if(faceInside.size() > NB_FACE_INSIDE_MIN)
    {
        subTree[0] = new KDTree(*this, 0, scene, height+1);
        if(subTree[0]->faceInside.size() == 0
            && subTree[0]->subTree[0] == NULL
            && subTree[0]->subTree[1] == NULL)
                { delete subTree[0]; subTree[0] = NULL; }
        subTree[1] = new KDTree(*this, 1, scene, height+1);
        if(subTree[1]->faceInside.size() == 0
            && subTree[1]->subTree[0] == NULL
            && subTree[1]->subTree[1] == NULL)
                { delete subTree[1]; subTree[1] = NULL; }
    }
   // printf("$ %d : %d\n", height, faceInside.size());
}

KDTree::KDTree(Scene const &scene) {
    subTree[0] = subTree[1] = NULL;
    if(scene.getVertexCount() <= 0) return;
    vector<Transf_Vertex> const& arrayVertex(scene.getVertexArray());
    xmin = xmax = arrayVertex[0].p[0];
    ymin = ymax = arrayVertex[0].p[1];
    zmin = zmax = arrayVertex[0].p[2];
    for(int i = 1; i < scene.getVertexCount(); i++) {
        const float *p = arrayVertex[i].p;
        if(p[0] < xmin)
            xmin = p[0];
        else if(p[0] > xmax)
            xmax = p[0];
        if(p[1] < ymin)
            ymin = p[1];
        else if(p[1] > ymax)
            ymax = p[1];
        if(p[2] < zmin)
            zmin = p[2];
        else if(p[2] > zmax)
            zmax = p[2];
    }
    if(xmax-xmin > ymax-ymin) {
        if(zmax-zmin > xmax-xmin)
            cutAxis = axisZ;
        else
            cutAxis = axisX;
    }
    else if(zmax-zmin > ymax-ymin)
        cutAxis = axisZ;
    else
        cutAxis = axisY;

    for(int i = scene.getFacesCount(); --i >= 0;)
        faceInside.push_back(i);

    if(faceInside.size() > NB_FACE_INSIDE_MIN)
    {
        subTree[0] = new KDTree(*this, 0, scene);
        if(subTree[0]->faceInside.size() == 0
            && subTree[0]->subTree[0] == NULL
            && subTree[0]->subTree[1] == NULL)
                { delete subTree[0]; subTree[0] = NULL; }
        subTree[1] = new KDTree(*this, 1, scene);
        if(subTree[1]->faceInside.size() == 0
            && subTree[1]->subTree[0] == NULL
            && subTree[1]->subTree[1] == NULL)
                { delete subTree[1]; subTree[1] = NULL; }
    }
}

KDTree::~KDTree()
{
    for(int i = 0; i < 2; i++)
        if(subTree[i] != NULL)
            delete subTree[i];
}


bool collisionFaceQuadStraightLine(float const *A, float const *B, float const *C, float const *D, float const *vectDir, float const *vectOrig)
{
    /// O : origine vecteur, P : extrémité vecteur, N : normale de la face
    /// OI' = OP vect OI
    /// Prod_i = OI' vect OJ'
    /// Prod_i dot OP même signe -> coll
    float OA[3] = { A[0]-vectOrig[0], A[1]-vectOrig[1], A[2]-vectOrig[2] };
    float OB[3] = { B[0]-vectOrig[0], B[1]-vectOrig[1], B[2]-vectOrig[2] };
    float OC[3] = { C[0]-vectOrig[0], C[1]-vectOrig[1], C[2]-vectOrig[2] };
    float OD[3] = { D[0]-vectOrig[0], D[1]-vectOrig[1], D[2]-vectOrig[2] };
    float OA_[3] = {
        vectDir[1]*OA[2] - OA[1]*vectDir[2],
        vectDir[2]*OA[0] - OA[2]*vectDir[0],
        vectDir[0]*OA[1] - OA[0]*vectDir[1]
    };
    float OB_[3] = {
        vectDir[1]*OB[2] - OB[1]*vectDir[2],
        vectDir[2]*OB[0] - OB[2]*vectDir[0],
        vectDir[0]*OB[1] - OB[0]*vectDir[1]
    };
    float OC_[3] = {
        vectDir[1]*OC[2] - OC[1]*vectDir[2],
        vectDir[2]*OC[0] - OC[2]*vectDir[0],
        vectDir[0]*OC[1] - OC[0]*vectDir[1]
    };
    float OD_[3] = {
        vectDir[1]*OD[2] - OD[1]*vectDir[2],
        vectDir[2]*OD[0] - OD[2]*vectDir[0],
        vectDir[0]*OD[1] - OD[0]*vectDir[1]
    };
    float pv1[3] = {
        OA_[1]*OB_[2] - OA_[2]*OB_[1],
        OA_[2]*OB_[0] - OA_[0]*OB_[2],
        OA_[0]*OB_[1] - OA_[1]*OB_[0]
    };
    float pv2[3] = {
        OB_[1]*OC_[2] - OB_[2]*OC_[1],
        OB_[2]*OC_[0] - OB_[0]*OC_[2],
        OB_[0]*OC_[1] - OB_[1]*OC_[0]
    };
    float pv3[3] = {
        OC_[1]*OD_[2] - OC_[2]*OD_[1],
        OC_[2]*OD_[0] - OC_[0]*OD_[2],
        OC_[0]*OD_[1] - OC_[1]*OD_[0]
    };
    float pv4[3] = {
        OD_[1]*OA_[2] - OD_[2]*OA_[1],
        OD_[2]*OA_[0] - OD_[0]*OA_[2],
        OD_[0]*OA_[1] - OD_[1]*OA_[0]
    };
    char positive1 = (pv1[0] * vectDir[0] + pv1[1] * vectDir[1] + pv1[2] * vectDir[2]) > 0;
    char positive2 = (pv2[0] * vectDir[0] + pv2[1] * vectDir[1] + pv2[2] * vectDir[2]) > 0;
    char positive3 = (pv3[0] * vectDir[0] + pv3[1] * vectDir[1] + pv3[2] * vectDir[2]) > 0;
    char positive4 = (pv4[0] * vectDir[0] + pv4[1] * vectDir[1] + pv4[2] * vectDir[2]) > 0;
    char sum = positive1 + positive2 + positive3 + positive4;

    return sum == 0 || sum == 4;
}
bool collisionRayBox(float const* rayOrig, float const* rayDir, KDTree const& box) {
    float vertex[8][3] = {
        {box.xmin, box.ymin, box.zmin},
        {box.xmin, box.ymin, box.zmax},
        {box.xmin, box.ymax, box.zmin},
        {box.xmin, box.ymax, box.zmax},
        {box.xmax, box.ymin, box.zmin},
        {box.xmax, box.ymin, box.zmax},
        {box.xmax, box.ymax, box.zmin},
        {box.xmax, box.ymax, box.zmax}
    };/// fix, ii ia aa ai
    return (rayDir[0] >= 0 ?
            rayOrig[0] <= box.xmin && collisionFaceQuadStraightLine(vertex[0], vertex[2], vertex[3], vertex[1], rayDir, rayOrig) // normal : -x
            : rayOrig[0] >= box.xmax && collisionFaceQuadStraightLine(vertex[4], vertex[6], vertex[7], vertex[5], rayDir, rayOrig)) // normal : +x
        || (rayDir[1] >= 0 ?
            rayOrig[1] <= box.ymin && collisionFaceQuadStraightLine(vertex[0], vertex[1], vertex[5], vertex[4], rayDir, rayOrig) // normal : -y
            : rayOrig[1] >= box.ymax && collisionFaceQuadStraightLine(vertex[2], vertex[3], vertex[7], vertex[6], rayDir, rayOrig)) // normal : +y
        || (rayDir[2] >= 0 ?
            rayOrig[2] <= box.zmin && collisionFaceQuadStraightLine(vertex[0], vertex[2], vertex[6], vertex[4], rayDir, rayOrig) // normal : -z
            : rayOrig[2] >= box.zmax && collisionFaceQuadStraightLine(vertex[1], vertex[3], vertex[7], vertex[5], rayDir, rayOrig));// normal : +z
}

/// TODO
ListCell* KDTree::getFaceInBoxCollWithRay_M_kay_IfYouSeeWhatIMean(float const* rayOrig, float const* rayDir, ListCell *nextCell) {
    ListCell* cell = new ListCell(faceInside);
    if(nextCell != NULL)
        cell->next = nextCell;

    /// Test de collision avec les sous trucs
    if(subTree[0] != NULL && (isInside(rayOrig, *subTree[0]) || collisionRayBox(rayOrig, rayDir, *subTree[0])))
        cell = subTree[0]->getFaceInBoxCollWithRay_M_kay_IfYouSeeWhatIMean(rayOrig, rayDir, cell);
    if(subTree[1] != NULL && (isInside(rayOrig, *subTree[1]) || collisionRayBox(rayOrig, rayDir, *subTree[1])))
        cell = subTree[1]->getFaceInBoxCollWithRay_M_kay_IfYouSeeWhatIMean(rayOrig, rayDir, cell);

    return cell;
}

bool KDTree::isSubTreeCollWithRay(float const* rayOrig, float const* rayDir, int subNum) {
    if(subTree[subNum] != NULL && (isInside(rayOrig, *subTree[subNum]) || collisionRayBox(rayOrig, rayDir, *subTree[subNum])))
        return true;
    return false;
}

void KDTree::getSubTreeColl(float const* rayOrig, float const* rayDir, int &out_first, int &out_second) {
    out_first = -1;
    out_second = -1;

    /// Si un seul ou aucun sub tree existe, c'est vite vu..
    if(subTree[0] == NULL) {
        if(subTree[1] != NULL)
            out_first = 1;
        return;
    }
    else if(subTree[1] == NULL) {
        out_first = 0;
        return;
    }

    /// on dans un des sub tree
   /* if(isInside(rayOrig, *subTree[0])) {
        out_first = 0;

        out_second = -2;
        return;
    }
    if(isInside(rayOrig, *subTree[1])) {
        out_first = 1;
        out_second = -2;
        return;
    }*/

    /// collision avec les 2 sub tree
    /// Si on coupe le plan du milieu (plan en commun), cela dépend de la direction
    switch(cutAxis) {
    case axisX: {
        float x = subTree[0]->xmax;
        float a[3] = {x, ymin, zmin};
        float b[3] = {x, ymin, zmax};
        float c[3] = {x, ymax, zmax};
        float d[3] = {x, ymax, zmin};
        bool middleColl = (rayDir[0] > 0 ? x > rayOrig[0] : x < rayOrig[0])
                && collisionFaceQuadStraightLine(a,b,c,d, rayDir, rayOrig);
        if(middleColl) {
            if(rayDir[0] > 0) {
                out_first = 0;
                out_second = 1;
            } else {
                out_first = 1;
                out_second = 0;
            }
            return;
        }
        break;
    }
    case axisY: {
        float y = subTree[0]->ymax;
        float a[3] = {xmin, y, zmin};
        float b[3] = {xmin, y, zmax};
        float c[3] = {xmax, y, zmax};
        float d[3] = {xmax, y, zmin};
        bool middleColl = (rayDir[1] > 0 ? y > rayOrig[1] : y < rayOrig[1])
                && collisionFaceQuadStraightLine(a,b,c,d, rayDir, rayOrig);
        if(middleColl) {
            if(rayDir[1] > 0) {
                out_first = 0;
                out_second = 1;
            } else {
                out_first = 1;
                out_second = 0;
            }
            return;
        }
        break;
    }
    case axisZ: {
        float z = subTree[0]->zmax;
        float a[3] = {xmin, ymin, z};
        float b[3] = {xmin, ymax, z};
        float c[3] = {xmax, ymax, z};
        float d[3] = {xmax, ymin, z};
        bool middleColl = (rayDir[2] > 0 ? z > rayOrig[2] : z < rayOrig[2])
                && collisionFaceQuadStraightLine(a,b,c,d, rayDir, rayOrig);
        if(middleColl) {
            if(rayDir[2] > 0) {
                out_first = 0;
                out_second = 1;
            } else {
                out_first = 1;
                out_second = 0;
            }
            return;
        }
        break;
    }
    }

    /// collision avec un seul des 2 sub tree
    if(isInside(rayOrig, *subTree[0]) || collisionRayBox(rayOrig, rayDir, *subTree[0]))
        out_first = 0;
    else if(isInside(rayOrig, *subTree[1]) || collisionRayBox(rayOrig, rayDir, *subTree[1]))
        out_first = 1;
}
int KDTree::getSecondSubTreeColl(float const* rayOrig, float const* rayDir, int first) {
    /// Un sub tree a déjà été selectionné, si le deuxieme est touché alors le rayon passe par le plan en commun

    int i = !first;
    /*// l'autre sub tree n'existe pas
    if(subTree[i] == NULL)
        return -1;*/

    bool middleColl = false;
    ///
    switch(cutAxis) {
    case axisX: {
        float x = subTree[0]->xmax;
        float a[3] = {x, ymin, zmin};
        float b[3] = {x, ymin, zmax};
        float c[3] = {x, ymax, zmax};
        float d[3] = {x, ymax, zmin};
        middleColl = collisionFaceQuadStraightLine(a,b,c,d, rayDir, rayOrig);
        break;
    }
    case axisY: {
        float y = subTree[0]->ymax;
        float a[3] = {xmin, y, zmin};
        float b[3] = {xmin, y, zmax};
        float c[3] = {xmax, y, zmax};
        float d[3] = {xmax, y, zmin};
        middleColl = collisionFaceQuadStraightLine(a,b,c,d, rayDir, rayOrig);
        break;
    }
    case axisZ: {
        float z = subTree[0]->zmax;
        float a[3] = {xmin, ymin, z};
        float b[3] = {xmin, ymax, z};
        float c[3] = {xmax, ymax, z};
        float d[3] = {xmax, ymin, z};
        middleColl = collisionFaceQuadStraightLine(a,b,c,d, rayDir, rayOrig);
        break;
    }
    }

    return middleColl ? i : -1;
}
