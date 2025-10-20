#include "Mesh.h"


Mesh::Mesh(int xRes, int yRes, float width, float height)
{
    if (xRes <= 0)
        xRes = 1;
    if (yRes <= 0)
        yRes = 1;

    float xStep = width / float(xRes);
    float yStep = (height / float(yRes));
    float xOffset = -width / 2.f;
    float yOffset = -height / 2.f;
    int vertsInRow = (xRes + 1);

    for (int i = 0; i <= yRes; ++i)
    {
        for (int j = 0; j <= xRes; ++j)
        {
            // make triangle
            if (i < yRes && j < xRes)
            {
                int i0 = j + i * vertsInRow;
                int i1 = j + (i + 1) * vertsInRow;

                if ((i % 2))
                {
                    indices_.emplace_back(i0);
                    indices_.emplace_back(i1 + 1);
                    indices_.emplace_back(i1);

                    indices_.emplace_back(i0);
                    indices_.emplace_back(i0 + 1);
                    indices_.emplace_back(i1 + 1);
                }
                else
                {
                    indices_.emplace_back(i0);
                    indices_.emplace_back(i0 + 1);
                    indices_.emplace_back(i1);

                    indices_.emplace_back(i1 + 1);
                    indices_.emplace_back(i1);
                    indices_.emplace_back(i0 + 1);
                }
            }

            // make first vertex
            positions_.emplace_back(Vec3(
                float(j) * xStep + xOffset + (i % 2) * xStep * .5f,
                float(i) * yStep + yOffset,
                0.f));

            textureCoords_.push_back(Vec2(float(j) / xRes, float(i) / yRes));
            normals_.push_back(Vec3(0.f, 0.f, 1.f));
            colors_.push_back(Vec4(1.f, 1.f, 1.f));
        }
    }

    textureCoords_.shrink_to_fit();
    colors_.shrink_to_fit();
    normals_.shrink_to_fit();
    positions_.shrink_to_fit();
    indices_.shrink_to_fit();

    initVBOs();
}

void Mesh::subdivide(unsigned iterations)
{
    auto edgeKey = [](uint32_t a, uint32_t b) -> unsigned long long
    {
        return ((uint64_t)a << 32) | b;
    };

    for (; iterations > 0; --iterations)
    {
        std::unordered_map<uint64_t, unsigned> midpoints;
        std::vector<unsigned> newIndices;
        for (size_t i = 0; i < indices_.size(); i += 3)
        {
            unsigned i0 = indices_[i];
            unsigned i1 = indices_[i + 1];
            unsigned i2 = indices_[i + 2];
            unsigned i3 = static_cast<unsigned>(positions_.size());
            unsigned i4 = i3 + 1;
            unsigned i5 = i3 + 2;
            unsigned long long key = 0;

            // e01
            key = edgeKey(i0, i1);
            if (midpoints.count(key) == 0)
                key = edgeKey(i1, i0);
            if (midpoints.count(key) == 1)
                i3 = midpoints[key];
            else
            {
                i3 = static_cast<unsigned>(positions_.size());
                positions_.emplace_back((positions_[i0] + positions_[i1]) * .5f);
                midpoints[key] = i3;
            }

            // e12
            key = edgeKey(i1, i2);
            if (midpoints.count(key) == 0)
                key = edgeKey(i2, i1);
            if (midpoints.count(key) == 1)
                i4 = midpoints[key];
            else
            {
                i4 = static_cast<unsigned>(positions_.size());
                positions_.emplace_back((positions_[i1] + positions_[i2]) * .5f);
                midpoints[key] = i4;
            }

            // e20 
            key = edgeKey(i2, i0);
            if (midpoints.count(key) == 0)
                key = edgeKey(i0, i2);
            if (midpoints.count(key) == 1)
                i5 = midpoints[key];
            else
            {
                i5 = static_cast<unsigned>(positions_.size());
                positions_.emplace_back((positions_[i2] + positions_[i0]) * .5f);
                midpoints[key] = i5;
            }


            newIndices.push_back(i0);
            newIndices.push_back(i3);
            newIndices.push_back(i5);

            newIndices.push_back(i3);
            newIndices.push_back(i1);
            newIndices.push_back(i4);

            newIndices.push_back(i5);
            newIndices.push_back(i4);
            newIndices.push_back(i2);

            newIndices.push_back(i5);
            newIndices.push_back(i3);
            newIndices.push_back(i4);
        }
        indices_ = newIndices;
    }

    normals_.resize(positions_.size(), Vec3(0, 0, 1));
    colors_.resize(positions_.size(), Vec4(1, 1, 1));
    calculateNormals();
    destroyVBOs();
    initVBOs();
}

void Mesh::calculateNormals(bool useAngleWeights)
{
    for (size_t i = 0; i < normals_.size(); ++i)
        normals_[i].zero();

    for (size_t i = 0; i < indices_.size(); i += 3)
    {
        unsigned i0 = indices_[i];
        unsigned i1 = indices_[i + 1];
        unsigned i2 = indices_[i + 2];
        Vec3 n = normalize(cross(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]));

        if (useAngleWeights)
        {
            float angle0 = acuteAngleBetween(positions_[i1] - positions_[i0], positions_[i2] - positions_[i0]);
            float angle1 = acuteAngleBetween(positions_[i2] - positions_[i1], positions_[i0] - positions_[i1]);
            float angle2 = acuteAngleBetween(positions_[i0] - positions_[i2], positions_[i1] - positions_[i2]);

            normals_[i0] = normals_[i0] + n * angle0;
            normals_[i1] = normals_[i1] + n * angle1;
            normals_[i2] = normals_[i2] + n * angle2;
        }
        else
        {
            normals_[i0] = normals_[i0] + n;
            normals_[i1] = normals_[i1] + n;
            normals_[i2] = normals_[i2] + n;
        }
    }

    for (size_t i = 0; i < normals_.size(); ++i)
        normals_[i].normalize();

    updateNormalVBO();
}
