#ifndef BINTREE_H
#define BINTREE_H

#include "Scene.h"
#include <list>

#define NUM_SUB_QUAD    2
#define MIN_FACE
enum {axisX, axisY, axisZ};
class BinTree
{
    public:
        BinTree();
        ~BinTree();
        void build(Scene& s);
    BinTree* subQuad[NUM_SUB_QUAD];
    int cutAxis;
    float xmin, xmax, ymin, ymax, zmin, zmax;
    std::list<int> faceInside;
};

#endif // BINTREE_H
