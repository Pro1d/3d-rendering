#ifndef KDTREE_H
#define KDTREE_H

#include "Scene.h"
#include <list>

/**
 ncol=3+1 tests de collision avec rayon
 -> x < (1+k)/2 * x + ncol*2,
 avec k ~= proba que le rayon ne touche qu'un seul subTree
 k = 0 : x = ncol*4 = 16
 */
#define NB_FACE_INSIDE_MIN  48

class ListCell {
    public:
        ListCell(std::list<int> const& f) : faces(f), next(NULL) {}
        std::list<int> const& faces;
        ListCell *next;
};

enum {axisX, axisY, axisZ};
class KDTree
{
    public:
        KDTree(Scene const &scene);
        KDTree(KDTree& parent, int subIndex, Scene const &scene, int height=1);
        virtual ~KDTree();
        ListCell* getFaceInBoxCollWithRay_M_kay_IfYouSeeWhatIMean(float const* rayOrig, float const* rayDir, ListCell *prevCell=NULL);

        KDTree* subTree[2];
        int cutAxis;
        float xmin, xmax, ymin, ymax, zmin, zmax;
        std::list<int> faceInside;
    protected:
    private:
};

#endif // KDTREE_H
