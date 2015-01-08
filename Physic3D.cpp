#include <cstdio>
#include "Physic3D.h"

Physic3D::Physic3D()
{
    //ctor
}

Physic3D::~Physic3D()
{
    //dtor
}

bool Physic3D::collisionFaceVector(float const *A, float const *B, float const *C, float const *normal, float const *vectDir, float const *vectOrig)
{
    /// normale dot AP et normale dot AO de signe contraire <= collision
    float dotO = normal[0]*(vectOrig[0]-A[0]) + normal[1]*(vectOrig[1]-A[1]) + normal[2]*(vectOrig[2]-A[2]);
    float dotP = normal[0]*(vectOrig[0]-A[0]+vectDir[0]) + normal[1]*(vectOrig[1]-A[1]+vectDir[1]) + normal[2]*(vectOrig[2]-A[2]+vectDir[2]);

    if(dotO * dotP < 0)
        return collisionFaceStraightLine(A, B, C, vectDir, vectOrig);

    return false;
}
bool Physic3D::collisionFaceVectorDirect(float const *A, float const *B, float const *C, float const *normal, float const *vectDir, float const *vectOrig)
{
    /// normale dot AP et normale dot AO de signe contraire <= collision
    float dotO = normal[0]*(vectOrig[0]-A[0]) + normal[1]*(vectOrig[1]-A[1]) + normal[2]*(vectOrig[2]-A[2]);
    float dotP = normal[0]*(vectOrig[0]-A[0]+vectDir[0]) + normal[1]*(vectOrig[1]-A[1]+vectDir[1]) + normal[2]*(vectOrig[2]-A[2]+vectDir[2]);

    if(dotO * dotP < 0)
        return collisionFaceStraightLineDirect(A, B, C, normal, vectDir, vectOrig);

    return false;
}
bool Physic3D::collisionFaceStraightLineDirect(float const *A, float const *B, float const *C, float const *normal, float const *vectDir, float const *vectOrig)
{
    /// normal dot vectDir
    float direction = normal[0]*vectDir[0] + normal[1]*vectDir[1] + normal[2]*vectDir[2];

    if(direction >= 0)
        return false;

    return collisionFaceStraightLine(A, B, C, vectDir, vectOrig);
}
bool Physic3D::collisionFaceStraightLine(float const *A, float const *B, float const *C, float const *vectDir, float const *vectOrig)
{
    /// O : origine vecteur, P : extrémité vecteur, N : normale de la face
    /// OI' = OP vect OI
    /// Prod_i = OI' vect OJ'
    /// Prod_i dot OP même signe -> coll
    float OA[3] = { A[0]-vectOrig[0], A[1]-vectOrig[1], A[2]-vectOrig[2] };
    float OB[3] = { B[0]-vectOrig[0], B[1]-vectOrig[1], B[2]-vectOrig[2] };
    float OC[3] = { C[0]-vectOrig[0], C[1]-vectOrig[1], C[2]-vectOrig[2] };
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
        OC_[1]*OA_[2] - OC_[2]*OA_[1],
        OC_[2]*OA_[0] - OC_[0]*OA_[2],
        OC_[0]*OA_[1] - OC_[1]*OA_[0]
    };
    char positive1 = (pv1[0] * vectDir[0] + pv1[1] * vectDir[1] + pv1[2] * vectDir[2]) > 0;
    char positive2 = (pv2[0] * vectDir[0] + pv2[1] * vectDir[1] + pv2[2] * vectDir[2]) > 0;
    char positive3 = (pv3[0] * vectDir[0] + pv3[1] * vectDir[1] + pv3[2] * vectDir[2]) > 0;
    char sum = positive1 + positive2 + positive3;

    return sum == 0 || sum == 3;
}
/** /!\ Collision Face/Droite requise **/
float Physic3D::collisionFaceVectorCoef(float const *A, float const *normal, float const *vectDir, float const *vectOrig)
{
	float OA[3] = {A[0]-vectOrig[0], A[1]-vectOrig[1], A[2]-vectOrig[2]};
	float nDotOA = OA[0]*normal[0] + OA[1]*normal[1] + OA[2]*normal[2];
	float nDotDir = normal[0]*vectDir[0] + normal[1]*vectDir[1] + normal[2]*vectDir[2];

	float k = nDotOA / nDotDir;
	// ->OP' = k*->OP
	return k;
}
/** calcul le vecteur de rebond de vectDir par rapport à la normale, avec conservation de la norme **/
void Physic3D::collisionFaceVectorBounce(float const *normal, float const *vectDir, float *bounceDir)
{
	float nDotDir = normal[0]*vectDir[0] + normal[1]*vectDir[1] + normal[2]*vectDir[2];
	bounceDir[0] = vectDir[0] - 2*nDotDir*normal[0];
	bounceDir[1] = vectDir[1] - 2*nDotDir*normal[1];
	bounceDir[2] = vectDir[2] - 2*nDotDir*normal[2];
}


void PointPhysic::moveInside_PreAlpha(Object3D & container, float kremain, int lastFaceCollIndex)
{
    /// Recherche de la face en collision la plus proche
    float kmin = kremain;
    float depl[3] = {v[0]*kremain, v[1]*kremain, v[2]*kremain};
    int nearestFaceIndex = -1;
    for(int i = 0; i < container.facesCount; i++)
    if(i != lastFaceCollIndex) {
        /// collision
        Vertex & v1 = container.vertex[container.faces[i].vertex_indices[0]];
        Vertex & v2 = container.vertex[container.faces[i].vertex_indices[1]];
        Vertex & v3 = container.vertex[container.faces[i].vertex_indices[2]];
        #ifdef DEBUG
            if(Physic3D::collisionFaceStraightLine(v1.p, v2.p, v3.p, depl, p))
                v1.c = v2.c = v3.c = rgb_f(1,1,1);
            /*else
                v1.c = v2.c = v3.c = rgb_f(0,0,1);*/
        #endif
        if(Physic3D::collisionFaceVectorDirect(v1.p, v2.p, v3.p, v1.n, depl, p))
        {
            float k = Physic3D::collisionFaceVectorCoef(v1.p, v1.n, v, p);
            if(k < kmin && k > 0.0f) {
                kmin = k;
                nearestFaceIndex = i;
            }
        }
    }

    /// Pas de collision
    if(nearestFaceIndex == -1) {
        p[0] += depl[0];
        p[1] += depl[1];
        p[2] += depl[2];
    }
    /// Calcul du rebond et du vecteur de déplacement restant
    else {
        /// position sur la face touchée
        p[0] += kmin * v[0];
        p[1] += kmin * v[1];
        p[2] += kmin * v[2];
        Vertex const& v1 = container.vertex[container.faces[nearestFaceIndex].vertex_indices[0]];
        Physic3D::collisionFaceVectorBounce(v1.n, v, v);/// changement de direction
        float remain = (kremain - kmin) * bounceFactor;
        /// Appel récursif
        moveInside_PreAlpha(container, remain, nearestFaceIndex);
        v[0] *= bounceFactor;
        v[1] *= bounceFactor;
        v[2] *= bounceFactor;
    }

}

