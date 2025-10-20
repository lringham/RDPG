#pragma once
#include "Vec3.h"
#include "Mat4.h"
#include "Mat3.h"


class Quaternion
{
public:
    explicit Quaternion();
    explicit Quaternion(float x, float y, float z, float w);
    explicit Quaternion(const Vec3& v, float w);

    void identity();
    void add(const Quaternion& q);
    void sub(const Quaternion& q);
    Quaternion mul(const Quaternion& quat); // Not commutative
    Quaternion mul(const Vec3& vec);
    Quaternion normal();
    Quaternion conjugate();
    Mat4 toMat4();
    Mat3 toMat3();
    float magnitude();

    // TODO: need to be friends?
    // operators
    friend std::ostream& operator<<(std::ostream& os, const Quaternion& q);
    friend Quaternion operator+(const Quaternion& q1, const Quaternion& q2);
    friend Quaternion operator-(const Quaternion& q1, const Quaternion& q2);
    friend Quaternion operator*(const Quaternion& q1, const Quaternion& q2);

    // variables
    float x, y, z, w;
};

Mat3 RotationMat3(float angle, const Vec3& axis);