#include "BVH.h"

#include <memory>
#include <iostream>


BVH::BVH(const Drawable& mesh)
{
    build(mesh);
}

void BVH::build(const Drawable& mesh)
{
    tree_.clear();
    triangles_.clear();

    // Find all triangles
    std::vector<BVHTriangle> triangles;
    {
        int triIndex = 0;
        for (size_t i = 0; i < mesh.indices_.size(); i+=3)
        {
            unsigned i0 = mesh.indices_[i];
            unsigned i1 = mesh.indices_[i+1];
            unsigned i2 = mesh.indices_[i+2];

            triangles.emplace_back(mesh.positions_[i0],
                                   mesh.positions_[i1],
                                   mesh.positions_[i2]);
            triangles.back().triIndex_ = triIndex;

            if (mesh.textureCoords_.size() > 0)
            {
                triangles.back().uv0_ = mesh.textureCoords_[i0];
                triangles.back().uv1_ = mesh.textureCoords_[i1];
                triangles.back().uv2_ = mesh.textureCoords_[i2];

                triangles.back().t_ = calculateTangent(
                    triangles.back().p0_,
                    triangles.back().p1_,
                    triangles.back().p2_,
                    triangles.back().uv0_,
                    triangles.back().uv1_,
                    triangles.back().uv2_
                );
            }

            if (mesh.normals_.size() > 0)
            {
                triangles.back().n0_ = mesh.normals_[i0];
                triangles.back().n1_ = mesh.normals_[i1];
                triangles.back().n2_ = mesh.normals_[i2];
            }

            triIndex++;
        }
        triangles.shrink_to_fit();
    }

    // Create root and childern
    tree_.emplace_back(AABB(triangles));
    buildChildern(triangles);
    tree_.shrink_to_fit();
    triangles_.shrink_to_fit();
}

// create childern in depth first order
void BVH::buildChildern(std::vector<BVHTriangle>& triangles, int parentIndex, int axis)
{
    if (triangles.size() == 1)
    {
        triangles_.push_back(triangles[0]);
        tree_[parentIndex].triangleIndex_ = static_cast<int>(triangles_.size() - 1);
    }
    else if (triangles.size() > 1)
    {
        Pair pair = split(triangles, axis);
        axis = (axis + 1) % 3;

        // Insert left
        tree_.emplace_back(AABB(pair.first_));
        buildChildern(pair.first_, static_cast<int>(tree_.size() - 1), axis);

        // Insert right
        tree_.emplace_back(AABB(pair.second_));
        tree_[parentIndex].rightChildOffset_ = static_cast<int>(tree_.size() - 1);
        buildChildern(pair.second_, static_cast<int>(tree_.size() - 1), axis);
    }
}

BVH::Pair BVH::split(const std::vector<BVHTriangle> & triangles, int axis) const
{
    Pair pair;
    if (triangles.size() == 0)
        return pair;

    // TODO: find better splitting metric
    float divider = 0.f;

    for (auto& t : triangles)
        divider += barycentre(t)[axis];

    divider /= static_cast<float>(triangles.size());

    for (auto& t : triangles)
    {
        if (barycentre(t)[axis] <= divider)
            pair.first_.push_back(t);
        else
            pair.second_.push_back(t);
    }

    if (pair.first_.size() == 0 && pair.second_.size() > 1)
    {
        pair.first_.push_back(pair.second_.back());
        pair.second_.erase(pair.second_.end() - 1);
    }
    else if (pair.second_.size() == 0 && pair.first_.size() > 1)
    {
        pair.second_.push_back(pair.first_.back());
        pair.first_.erase(pair.first_.end() - 1);
    }

    return pair;
}

bool BVH::raycast(Ray& ray) const
{
    return search(ray);
}

bool BVH::search(Ray& ray, int parentIndex) const
{
    Ray parentRay = ray;
    const Node& node = tree_[parentIndex];
    if (node.isLeaf())
    {
        if (triangles_[node.triangleIndex_].raycast(parentRay))
        {
            ray = parentRay;
            return true;
        }
        return false;
    }
    else if (node.boundingBox_.raycast(parentRay))
    {
        Ray leftRay = ray, rightRay = ray;
        bool hitLeft = false, hitRight = false;

        bool hasLeft = !node.isLeaf() && node.rightChildOffset_ != parentIndex + 1;
        if (hasLeft)
            hitLeft = search(leftRay, parentIndex + 1); // search left
        if (node.rightChildOffset_ != -1) // has right child
            hitRight = search(rightRay, node.rightChildOffset_); // search right

        if (hitLeft && hitRight)
        {
            if (leftRay.t_ < rightRay.t_)
                ray = leftRay;
            else
                ray = rightRay;
            return true;
        }
        else if (hitLeft)
        {
            ray = leftRay;
            return true;
        }
        else if (hitRight)
        {
            ray = rightRay;
            return true;
        }
        else
            return false;
    }
    return false;
}
