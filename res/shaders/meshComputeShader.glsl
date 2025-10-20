#version 460
layout(local_size_x = 12) in;
#include "defines"

//=========================
// Structs
//=========================
struct Position
{
    float x, y, z;
};

struct HalfEdge
{
    int nextIndex;
    int pairIndex;
    int ccw;
    int faceIndex;
    int origIndex;
    float angle;
};

struct PrepatternInfo
{
    int sourceIndex;
    int targetIndex;
    float exponent;
    float highVal;
    float lowVal;
    bool enabled;
    bool squared;
};

struct Vert
{
    int index;
    int edgeIndex;
    float dualArea;
};

struct Face
{
    int edgeIndex;
    float area;
    Position normal;
};

struct CellData
{
    float vals[MORPHOGEN_COUNT];
};

struct IsConstVal
{
    bool vals[MORPHOGEN_COUNT];
};

struct AnisoVecs
{
    Position vec[MORPHOGEN_COUNT];
};

struct OutVal
{
    float colors[4];
};

#include "parameters"

//=========================
// SSBOs
//=========================
#define stdlayout std430,
layout(stdlayout binding = 3) buffer NormalSSBO
{
    Position normals[];
};
layout(stdlayout binding = 4) buffer PositionSSBO
{
    Position positions[];
};
layout(stdlayout binding = 5) buffer ColorSSBO
{
    OutVal outValues[];
};
layout(stdlayout binding = 6) buffer VertexSSBO
{
    Vert vertices[];
};
layout(stdlayout binding = 7) buffer HalfEdgeSSBO
{
    HalfEdge edges[];
};
layout(stdlayout binding = 8) buffer FaceSSBO
{
    Face faces[];
};
layout(stdlayout binding = 9) buffer Cell1SSBO
{
    CellData cells1[];
};
layout(stdlayout binding = 10) buffer Cell2SSBO
{
    CellData cells2[];
};
layout(stdlayout binding = 11) buffer vCotansSSBO
{
    CellData vCotans[];
};
layout(stdlayout binding = 12) buffer eCotansSSBO
{
    CellData eCotans[];
};
layout(stdlayout binding = 13) buffer t0_t1SSBO
{
    AnisoVecs t0_t1[]; // TODO: should be vec2
};
layout(stdlayout binding = 14) buffer diffVectorsSSBO
{
    AnisoVecs diffVec[];
};
layout(stdlayout binding = 15) buffer TangentSSBO
{
    AnisoVecs tangents[];
};
layout(stdlayout binding = 17) buffer paramsSSBO
{
    Parameters params[];
};
layout(stdlayout binding = 18) buffer isConstValSSBO
{
    IsConstVal isConstVal[];
};

//=========================
// Uniforms
//=========================
uniform int vertCount;
uniform int outMorphIndex;
uniform vec3 growthRate;
uniform int growthMode;
uniform float linearGrowthFactor;
uniform int stepCount;
uniform PrepatternInfo prepatternInfo0;
uniform PrepatternInfo prepatternInfo1;

// Subroutines
subroutine void calculateSub(uint gid);		// Subroutine Signature
subroutine uniform calculateSub calculate;

//=========================
// Methods
//=========================
void lap(uint gid, inout float L[MORPHOGEN_COUNT])
{
    Vert v = vertices[gid];
    int edgeIndex = v.edgeIndex;
    int startEdgeIndex = edgeIndex;
    int j = 0;
    HalfEdge e;

    for (int i = 0; i < MORPHOGEN_COUNT; ++i)
        L[i] = 0;

    do
    {
        e = edges[edgeIndex];
        j = edges[e.nextIndex].origIndex;

        for (int i = 0; i < MORPHOGEN_COUNT; ++i)
            L[i] += eCotans[edgeIndex].vals[i] * cells1[j].vals[i];

        edgeIndex = e.ccw;
    } while (startEdgeIndex != edgeIndex);

    for (int i = 0; i < MORPHOGEN_COUNT; ++i)
    {
        L[i] += vCotans[gid].vals[i] * cells1[gid].vals[i];
        L[i] /= v.dualArea;
    }
}

mat3 rotMat(float halfAngleRad, vec3 axis)
{
    // Build a quaternion and convert it into a mat3
    vec3 v = sin(halfAngleRad) * axis;
    float x = v.x;
    float y = v.y;
    float z = v.z;
    float w = cos(halfAngleRad);

    float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;
    float xx = x * x2;
    float xy = x * y2;
    float xz = x * z2;
    float yy = y * y2;
    float yz = y * z2;
    float zz = z * z2;
    float wx = w * x2;
    float wy = w * y2;
    float wz = w * z2;

    mat3 res;
    res[0] = vec3(1.f - (yy + zz), xy + wz, xz - wy);
    res[1] = vec3(xy - wz, 1.f - (xx + zz), yz + wx);
    res[2] = vec3(xz + wy, yz - wx, 1.f - (xx + yy));
    return res;
}

float getFaceMorph(uint faceIndex, uint morphIndex)
{
	Face f = faces[faceIndex];

	HalfEdge e0 = edges[f.edgeIndex];
	HalfEdge e1 = edges[e0.nextIndex];
	HalfEdge e2 = edges[e1.nextIndex];

	float Aconc = cells1[e0.origIndex].vals[morphIndex];
	float Bconc = cells1[e1.origIndex].vals[morphIndex];
	float Cconc = cells1[e2.origIndex].vals[morphIndex];

    return (Aconc + Bconc + Cconc) / 3.f;
}

mat3 CalcD(int faceIndex, uint morphIndex, uint gid)
{
    float Dt0 = t0_t1[faceIndex].vec[morphIndex].x;
    float Dt1 = t0_t1[faceIndex].vec[morphIndex].y;	
	
    if(prepatternInfo0.enabled && prepatternInfo0.targetIndex == morphIndex)
    {
	    Dt0 = mix(prepatternInfo0.lowVal, prepatternInfo0.highVal, 
                  pow(getFaceMorph(faceIndex, prepatternInfo0.sourceIndex), prepatternInfo0.exponent));

        if(prepatternInfo0.squared)
            Dt0 *= Dt0;
    }

    if(prepatternInfo1.enabled && prepatternInfo1.targetIndex == morphIndex)
    {
	    Dt1 = mix(prepatternInfo1.lowVal, prepatternInfo1.highVal, 
                  pow(getFaceMorph(faceIndex, prepatternInfo1.sourceIndex), prepatternInfo1.exponent));
        if(prepatternInfo1.squared)
           Dt1 *= Dt1;
    }

    mat3 D;
    D[0] = vec3(Dt0, 0, 0);
    D[1] = vec3(0, Dt1, 0);
    D[2] = vec3(0,   0, 1);

    return D;
}

mat3 DT(vec3 norm, vec3 dir, mat3 D)
{
    float rad45 = 0.785398;
    mat3 R = rotMat(rad45, norm); // 90 degree rotation matrix
    mat3 RT = transpose(R);

    // Transform from triangle frame to standard basis for comparison against diag tensor
    mat3 Q;
    Q[0] = normalize(dir);
    Q[1] = normalize(cross(norm, Q[0]));
    Q[2] = normalize(cross(Q[0], Q[1]));
    mat3 QT = transpose(Q);

    return RT * Q * D * QT * R;
}

vec3 posToVec(Position p0, Position p1)
{
    vec3 res = vec3(p0.x - p1.x, p0.y - p1.y, p0.z - p1.z);
    return res;
}

vec3 posToVec(Position p)
{
    vec3 res = vec3(p.x, p.y, p.z);
    return res;
}

Position vecToPos(vec3 v)
{
    Position res;
    res.x = v.x;
    res.y = v.y;
    res.z = v.z;
    return res;
}

vec3 getFaceTangent(uint faceIndex, uint morphIndex)
{
    vec3 res = posToVec(tangents[faceIndex].vec[morphIndex]);
    return res;
}

//=========================
// Subroutines
//=========================
#include "model"



subroutine(calculateSub) void calculateFaceNormalsAreas(uint gid)
{
    Face f = faces[gid];

    HalfEdge e0 = edges[f.edgeIndex];
    HalfEdge e1 = edges[e0.nextIndex];
    HalfEdge e2 = edges[e1.nextIndex];
    Position p0 = positions[vertices[e0.origIndex].index];
    Position p1 = positions[vertices[e1.origIndex].index];
    Position p2 = positions[vertices[e2.origIndex].index];

    vec3 v0 = vec3(p0.x, p0.y, p0.z);
    vec3 v1 = vec3(p1.x, p1.y, p1.z);
    vec3 v2 = vec3(p2.x, p2.y, p2.z);
    vec3 e01 = v1 - v0;
    vec3 e02 = v2 - v0;

    vec3 normal = cross(e01, e02);
    faces[gid].area = length(normal) / 2.f;

    normal = normalize(normal);
    faces[gid].normal = vecToPos(normal);
}

subroutine(calculateSub) void calculateVertAnglesNormalsAreas(uint gid)
{
    vertices[gid].dualArea = 0.f;
    Vert v = vertices[gid];
    int edgeIndex = v.edgeIndex;
    int startEdgeIndex = edgeIndex;
    HalfEdge e;
    vec3 e01, e02, v1, v2;
    vec3 v0 = vec3(positions[v.index].x, positions[v.index].y, positions[v.index].z);
    vec3 normal = vec3(0, 0, 0);

    do
    {
        // Get the current half edge
        e = edges[edgeIndex];

        if (e.faceIndex != -1)
        {
            // Get vectors from vertex at gid to neighbouring vertices in face
            Position p1 = positions[vertices[edges[e.nextIndex].origIndex].index];
            Position p2 = positions[vertices[edges[edges[e.nextIndex].nextIndex].origIndex].index];
            v1 = vec3(p1.x, p1.y, p1.z);
            v2 = vec3(p2.x, p2.y, p2.z);
            e01 = v1 - v0;
            e02 = v2 - v0;

            // Calculate and store angle and face area
            float value = dot(normalize(e01), normalize(e02));
            clamp(value, -1.f, 1.f);
            edges[edgeIndex].angle = acos(value);
            vertices[gid].dualArea += faces[e.faceIndex].area / 3.f;

            // Accumulate face normals
            normal += posToVec(faces[e.faceIndex].normal);
        }
        else // if edge has no face it's angle is 0
            edges[edgeIndex].angle = -1.f;

        edgeIndex = e.ccw;
    } while (startEdgeIndex != edgeIndex);

    // Calculate vertex normal
    normal = normalize(normal);
    normals[v.index].x = normal.x;
    normals[v.index].y = normal.y;
    normals[v.index].z = normal.z;
}

subroutine(calculateSub) void calculateCotangents(uint gid)
{
    for (uint morphIndex = 0; morphIndex < MORPHOGEN_COUNT; ++morphIndex)
    {
        Vert v = vertices[gid];
        int edgeIndex = v.edgeIndex;
        int startEdgeIndex = edgeIndex;
        HalfEdge e;
        HalfEdge ePair;

        float alpha, beta, theta;
        float Gamma, gamma, Xi, xi;
        vec3 norm, dir;

        // precompute innerproduct of neighbouring bases
        uint j, k;
        float cotan = 0.f;
        do // For each edge, calculate innerproduct 
        {
            cotan = 0.f;
            e = edges[edgeIndex];
            ePair = edges[e.pairIndex];
            j = vertices[edges[e.nextIndex].origIndex].index;

            if (e.faceIndex != -1)
            {
                k = vertices[edges[edges[e.nextIndex].nextIndex].origIndex].index;
                alpha = edges[edges[e.nextIndex].nextIndex].angle;
                beta = edges[edges[ePair.nextIndex].nextIndex].angle;

                norm = posToVec(faces[e.faceIndex].normal);
                dir = getFaceTangent(e.faceIndex, morphIndex);
                dir = normalize(cross(cross(norm, dir), norm));

                vec3 e_i = posToVec(positions[gid], positions[k]);
                vec3 e_j = posToVec(positions[k], positions[j]);

                mat3 D = CalcD(e.faceIndex, morphIndex, gid);
                vec3 Dtei = DT(norm, dir, D) * e_i;
                Gamma = length(Dtei) / length(e_i);
                float value = dot(Dtei, -1.f*e_j) / (length(Dtei)*length(e_j));
                value = clamp(value, -1.f, 1.f);
                gamma = acos(value);

                cotan = .5f * Gamma * (cos(gamma) / sin(alpha));

                // Handle adjacent face
                if (ePair.faceIndex != -1)
                {
                    k = vertices[edges[edges[ePair.nextIndex].nextIndex].origIndex].index;

                    norm = posToVec(faces[ePair.faceIndex].normal);
                    dir = getFaceTangent(ePair.faceIndex, morphIndex);
                    dir = normalize(cross(cross(norm, dir), norm));

                    vec3 e_l = posToVec(positions[gid], positions[k]);
                    vec3 e_m = posToVec(positions[k], positions[j]);

                    mat3 D = CalcD(ePair.faceIndex, morphIndex, gid);
                    vec3 Dtel = DT(norm, dir, D) * e_l;
                    Xi = length(Dtel) / length(e_l);
                    float value = dot(Dtel, -1.f * e_m) / (length(Dtel)*length(e_m));
                    value = clamp(value, -1.f, 1.f);
                    xi = acos(value);

                    cotan += .5f * Xi * (cos(xi) / sin(beta));
                }
            }
            else
            {
                beta = edges[edges[ePair.nextIndex].nextIndex].angle;
                k = vertices[edges[edges[ePair.nextIndex].nextIndex].origIndex].index;

                norm = posToVec(faces[ePair.faceIndex].normal);
                dir = getFaceTangent(ePair.faceIndex, morphIndex);
                dir = normalize(cross(cross(norm, dir), norm));

                vec3 e_l = posToVec(positions[gid], positions[k]);
                vec3 e_m = posToVec(positions[k], positions[j]);

                mat3 D = CalcD(ePair.faceIndex, morphIndex, gid);
                vec3 Dtel = DT(norm, dir, D) * e_l;
                Xi = length(Dtel) / length(e_l);
                float value = dot(Dtel, -1.f * e_m) / (length(Dtel)*length(e_m));
                value = clamp(value, -1.f, 1.f);
                xi = acos(value);

                cotan = .5f * Xi * (cos(xi) / sin(beta));
            }
            eCotans[edgeIndex].vals[morphIndex] = cotan;
            edgeIndex = e.ccw;
        } while (startEdgeIndex != edgeIndex);

        // precompute innerproduct of basis with it's self
        edgeIndex = v.edgeIndex;
        cotan = 0.f;
        do // For each face, calculate cotangent
        {
            e = edges[edgeIndex];
            j = vertices[edges[e.nextIndex].origIndex].index;
            k = vertices[edges[edges[e.nextIndex].nextIndex].origIndex].index;
            if (e.faceIndex != -1)
            {
                alpha = edges[edges[e.nextIndex].nextIndex].angle;
                theta = edges[e.nextIndex].angle;

                norm = posToVec(faces[e.faceIndex].normal);
                dir = getFaceTangent(e.faceIndex, morphIndex);
                dir = normalize(cross(cross(norm, dir), norm));

                vec3 e_i = posToVec(positions[gid], positions[k]);
                vec3 e_j = posToVec(positions[k], positions[j]);

                mat3 D = CalcD(e.faceIndex, morphIndex, gid);
                cotan += dot(DT(norm, dir, D)*e_j / length(e_j), e_j / length(e_j)) * ((1.f / tan(alpha) + 1.f / tan(theta))); // calculate I
            }
            edgeIndex = e.ccw;
        } while (startEdgeIndex != edgeIndex);
        cotan *= -.5f;
        //QQQ vCotans[gid].vals[morphIndex] = cotan;
    }
}

subroutine(calculateSub) void grow(uint gid)
{
    if (growthMode == 0)
    {
        Vert v = vertices[gid];
        Position p = positions[v.index];
        p.x = (1.f + growthRate.x) * p.x;
        p.y = (1.f + growthRate.y) * p.y;
        p.z = (1.f + growthRate.z) * p.z;
        positions[gid] = p;
    }
    else if (growthMode == 1)
    {
        Vert v = vertices[gid];
        Position p = positions[v.index];
        p.x = linearGrowthFactor * p.x;
        p.y = linearGrowthFactor * p.y;
        p.z = linearGrowthFactor * p.z;

        positions[gid] = p;
    }
    else if (growthMode == 2)
    {
        Vert v = vertices[gid];
        Position p = positions[v.index];
		Position n = normals[v.index];

		//float growthPos = cells1[gid].vals[0] * growthRate.x;
		//float growthNeg = cells1[gid].vals[1] * growthRate.y; // vals[1] causes compile error
		float growthPos = cells1[gid].vals[0] * growthRate.x;
		float growthNeg = cells1[gid].vals[0] * growthRate.y;
		float growth = growthPos - growthNeg;
		
        p.x += n.x * growth;
        p.y += n.y * growth;
        p.z += n.z * growth;

        positions[gid] = p;
    }
}
//=========================
// Main
//=========================
void main()
{
    uint gid = gl_GlobalInvocationID.x;

    if (gid < vertCount)
        calculate(gid);
}