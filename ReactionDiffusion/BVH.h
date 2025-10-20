#pragma once
#include "Ray.h"
#include "AABB.h"
#include "Drawable.h"

#include <vector>


class BVH
{
public:
    BVH() = default;
    BVH(const Drawable& obj);

    void build(const Drawable& obj);
    bool raycast(Ray& ray) const;

private:
    struct Pair
    {
        std::vector<BVHTriangle> first_;
        std::vector<BVHTriangle> second_;
    };

    struct Node
    {
        AABB boundingBox_;
        int triangleIndex_ = -1;
        int rightChildOffset_ = -1;

        Node() = default;

        Node(AABB boundingBox, int triangleIndex = -1, int rightChildOffset = -1) :
            boundingBox_(boundingBox), triangleIndex_(triangleIndex), rightChildOffset_(rightChildOffset)
        {}

        inline bool isLeaf() const
        {
            return triangleIndex_ != -1;
        }
    };

    void buildChildern(std::vector<BVHTriangle>& triangles, int parentIndex = 0, int axis = 0);
    BVH::Pair split(const std::vector<BVHTriangle>& triangles, int axis) const;
    bool search(Ray& ray, int parentIndex = 0) const;

    std::vector<Node> tree_;
    std::vector<BVHTriangle> triangles_;
};
