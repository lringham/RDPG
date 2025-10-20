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
layout(stdlayout binding = 11) buffer eNormalRandsSSBO
{
	AnisoVecs eNormalRands[];
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
uniform int veinMorphIndex;
uniform float veinDiffusionScale;
uniform float petalDiffusionScale;
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

void lap(uint gid, inout float L[MORPHOGEN_COUNT])
{
	Vert v = vertices[gid];
	int edgeIndex = v.edgeIndex;
	int startEdgeIndex = edgeIndex;
	vec3 c;
	HalfEdge e, ePair;

	for (int i = 0; i < MORPHOGEN_COUNT; ++i)
		L[i] = 0;

	do
	{
		e = edges[edgeIndex];
		ePair = edges[e.pairIndex];

		for (int i = 0; i < MORPHOGEN_COUNT; ++i) {

			c.x = cells1[edges[edges[e.nextIndex].nextIndex].origIndex].vals[i];
			c.y = cells1[e.origIndex].vals[i];
			c.z = cells1[edges[e.nextIndex].origIndex].vals[i];
			L[i] -= dot(c, posToVec(diffVec[edgeIndex].vec[i]));

			c.x = cells1[edges[edges[ePair.nextIndex].nextIndex].origIndex].vals[i];
			c.y = cells1[ePair.origIndex].vals[i];
			c.z = cells1[edges[ePair.nextIndex].origIndex].vals[i];
			L[i] += dot(c, posToVec(diffVec[e.pairIndex].vec[i]));

		}

		edgeIndex = e.ccw;
	} while (startEdgeIndex != edgeIndex);

	for (int i = 0; i < MORPHOGEN_COUNT; ++i)
	{
		L[i] /= v.dualArea;
	}
}

void lap_noise(uint gid, inout float L[MORPHOGEN_COUNT], inout float LNoise[MORPHOGEN_COUNT])
{
	Vert v = vertices[gid];
	int edgeIndex = v.edgeIndex;
	int startEdgeIndex = edgeIndex;
	vec3 c, sqrtc, d;
	HalfEdge e, ePair;

	for (int i = 0; i < MORPHOGEN_COUNT; ++i) {
		L[i] = 0;
		LNoise[i] = 0;
	}

	do
	{
		e = edges[edgeIndex];
		ePair = edges[e.pairIndex];

		for (int i = 0; i < MORPHOGEN_COUNT; ++i) {

			c.x = cells1[edges[edges[e.nextIndex].nextIndex].origIndex].vals[i];
			c.y = cells1[e.origIndex].vals[i];
			c.z = cells1[edges[e.nextIndex].origIndex].vals[i];
			d = posToVec(diffVec[edgeIndex].vec[i]);
			L[i] -= dot(c, d);
			
			sqrtc = sqrt(c * abs(d));
			LNoise[i] -= dot(sqrtc,posToVec(eNormalRands[edgeIndex].vec[i]));

			c.x = cells1[edges[edges[ePair.nextIndex].nextIndex].origIndex].vals[i];
			c.y = cells1[ePair.origIndex].vals[i];
			c.z = cells1[edges[ePair.nextIndex].origIndex].vals[i];
			d = posToVec(diffVec[e.pairIndex].vec[i]);
			L[i] += dot(c, d);
			
			sqrtc = sqrt(c * abs(d));
			LNoise[i] += dot(sqrtc,posToVec(eNormalRands[e.pairIndex].vec[i]));
		}

		edgeIndex = e.ccw;
	} while (startEdgeIndex != edgeIndex);

	for (int i = 0; i < MORPHOGEN_COUNT; ++i)
	{
		L[i] /= v.dualArea;
		LNoise[i] /= v.dualArea;
	}
}


void lap_noise_veins(uint gid, inout float L[MORPHOGEN_COUNT], inout float LNoise[MORPHOGEN_COUNT])
{
	Vert v = vertices[gid];
	int edgeIndex = v.edgeIndex;
	int startEdgeIndex = edgeIndex;
	vec3 c, sqrtc, d, vein;
	HalfEdge e, ePair;

	for (int i = 0; i < MORPHOGEN_COUNT; ++i) {
		L[i] = 0;
		LNoise[i] = 0;
	}

	do
	{
		e = edges[edgeIndex];
		ePair = edges[e.pairIndex];

		for (int i = 0; i < MORPHOGEN_COUNT; ++i) {
		
			float scale;
			vein.x = cells1[edges[edges[e.nextIndex].nextIndex].origIndex].vals[veinMorphIndex];
			vein.y = cells1[e.origIndex].vals[veinMorphIndex];
			vein.z = cells1[edges[e.nextIndex].origIndex].vals[veinMorphIndex];
			if (vein.y > 0.f && vein.z > 0.f)
				scale = veinDiffusionScale;
			else
				scale = petalDiffusionScale;
		
			c.x = cells1[edges[edges[e.nextIndex].nextIndex].origIndex].vals[i];
			c.y = cells1[e.origIndex].vals[i];
			c.z = cells1[edges[e.nextIndex].origIndex].vals[i];
			d = posToVec(diffVec[edgeIndex].vec[i]);
			L[i] -= scale * dot(c, d);
			
			sqrtc = sqrt(c * abs(d));
			LNoise[i] -= scale * dot(sqrtc,posToVec(eNormalRands[edgeIndex].vec[i]));

			vein.x = cells1[edges[edges[ePair.nextIndex].nextIndex].origIndex].vals[veinMorphIndex];
			vein.y = cells1[ePair.origIndex].vals[veinMorphIndex];
			vein.z = cells1[edges[ePair.nextIndex].origIndex].vals[veinMorphIndex];
			if (vein.y > 0.f && vein.z > 0.f)
				scale = veinDiffusionScale;
			else
				scale = petalDiffusionScale;
				
			c.x = cells1[edges[edges[ePair.nextIndex].nextIndex].origIndex].vals[i];
			c.y = cells1[ePair.origIndex].vals[i];
			c.z = cells1[edges[ePair.nextIndex].origIndex].vals[i];
			d = posToVec(diffVec[e.pairIndex].vec[i]);
			L[i] += scale * dot(c, d);
			
			sqrtc = sqrt(c * abs(d));
			LNoise[i] += scale * dot(sqrtc,posToVec(eNormalRands[e.pairIndex].vec[i]));
			
		}

		edgeIndex = e.ccw;
	} while (startEdgeIndex != edgeIndex);

	for (int i = 0; i < MORPHOGEN_COUNT; ++i)
	{
		L[i] /= v.dualArea;
		LNoise[i] /= v.dualArea;
	}
}


vec3 getGradientFace(uint faceIndex, uint morphIndex)
{
	Face f = faces[faceIndex];

	HalfEdge e0 = edges[f.edgeIndex];
	HalfEdge e1 = edges[e0.nextIndex];
	HalfEdge e2 = edges[e1.nextIndex];

	float Aconc = cells1[e0.origIndex].vals[morphIndex];
	float Bconc = cells1[e1.origIndex].vals[morphIndex];
	float Cconc = cells1[e2.origIndex].vals[morphIndex];

	Position pA = positions[e0.origIndex];
	Position pB = positions[e1.origIndex];
	Position pC = positions[e2.origIndex];
	
	vec3 AB = posToVec(pB, pA);//B - A;
	vec3 BC = posToVec(pC, pB);//C - B;
	vec3 CA = posToVec(pA, pC);//A - C;

	vec3 N = cross(AB, BC);
	vec3 Nn = normalize(N);

	vec3 gradient = cross(Aconc * BC + Bconc * CA + Cconc * AB, -N) / dot(N, N);

	return gradient;
}

// psuedorandom number generator via a hash function:
// Mark Jarzynski and Marc Olano, Hash Functions for GPU Rendering, Journal of Computer Graphics Techniques (JCGT), vol. 9, no. 3, 21-38, 2020
// Available online http://jcgt.org/published/0009/03/02/
// Also, see Nathan Reed's blog post from May 21, 2021: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
uint rng_state; // this is seeded in main() for each compute-shader thread and each simulation step
uint rand_pcg()
{
    uint state = rng_state;
    rng_state = rng_state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// TODO: generate two normal random variables but return one and store the other
// nran() should return one number to be compatible with the CPU implementation of the custom RD model
vec2 nran() // returns two normal random variables with mean=0 and std=1
{
	// https://en.wikipedia.org/wiki/Marsaglia_polar_method
	// same method that is used by GNU GCC libstdc++...
	
	float u, v, s;
	do {
	
		u = float(rand_pcg()) * 2. * (1. / 4294967296.0) - 1.;
		v = float(rand_pcg()) * 2. * (1. / 4294967296.0) - 1.;
		s = u * u + v * v;
	
	} while (s >= 1.0 || s == 0);
	
	s = sqrt(-2.0 * log(s) / s);
	
	return vec2(u * s, v * s);
}

//=========================
// Subroutines
//=========================
#include "model"



subroutine(calculateSub) void calculateFlux_pwp(uint gid) // TODO: rename to generateRandomNumbers(uint gid)
{
	Vert v = vertices[gid];
	int edgeIndex = v.edgeIndex;
	int startEdgeIndex = edgeIndex;
	HalfEdge e;

	do
	{
		e = edges[edgeIndex];
		for (uint morphIndex = 0; morphIndex < MORPHOGEN_COUNT; ++morphIndex)
		{
			vec2 n1 = nran();
			vec2 n2 = nran();
			eNormalRands[edgeIndex].vec[morphIndex].x = n1.x;
			eNormalRands[edgeIndex].vec[morphIndex].y = n1.y;
			eNormalRands[edgeIndex].vec[morphIndex].z = n2.x; // TODO: here n2.y is wasted...
		}
		edgeIndex = e.ccw;
	} while (startEdgeIndex != edgeIndex);
}

subroutine(calculateSub) void calculateCotangents_pwp(uint gid)
{
	Vert v = vertices[gid];
	int edgeIndex = v.edgeIndex;
	int startEdgeIndex = edgeIndex;
	HalfEdge e;
	HalfEdge ePair;

	uint j, k;
	float cotan = 0.f;
	do
	{
		cotan = 0.f;
		e = edges[edgeIndex];
		ePair = edges[e.pairIndex];
		j = vertices[edges[e.nextIndex].origIndex].index;

		if (e.faceIndex != -1)
		{
			k = vertices[edges[edges[e.nextIndex].nextIndex].origIndex].index;
			vec3 e_i = posToVec(positions[k], positions[gid]);
			vec3 e_j = posToVec(positions[k], positions[j]);
			cotan = dot(e_i, e_j) / length(cross(e_i, e_j));
		}
		for (uint morphIndex = 0; morphIndex < MORPHOGEN_COUNT; ++morphIndex)
			eCotans[edgeIndex].vals[morphIndex] = cotan;
		edgeIndex = e.ccw;
	} while (startEdgeIndex != edgeIndex);
}

subroutine(calculateSub) void calculateDiffVectors(uint gid)
{	
	HalfEdge e1 = edges[gid];
	HalfEdge e2 = edges[e1.nextIndex];
	HalfEdge e0 = edges[edges[e1.nextIndex].nextIndex];

	Position pA = positions[e0.origIndex];
	Position pB = positions[e1.origIndex];
	Position pC = positions[e2.origIndex];
	
	vec3 AB = posToVec(pB, pA);//B - A;
	vec3 BC = posToVec(pC, pB);//C - B;
	vec3 CA = posToVec(pA, pC);//A - C;

	vec3 N = cross(AB, BC);

	for (uint morphIndex = 0; morphIndex < MORPHOGEN_COUNT; ++morphIndex)
		diffVec[gid].vec[morphIndex] = Position(0,0,0);

	float a, b, c;
	uint fid = e1.faceIndex;
	if (fid != -1) 
    {
		for (uint morphIndex = 0; morphIndex < MORPHOGEN_COUNT; ++morphIndex) 
        {
			// TODO: store veclambdas--they don't need to be recomputed
			vec3 gradient = getGradientFace(fid, morphIndex);
		
			float t0 = t0_t1[fid].vec[morphIndex].x;
			float t1 = t0_t1[fid].vec[morphIndex].y;
	
            if(prepatternInfo0.enabled && prepatternInfo0.targetIndex == morphIndex)
            {
	            t0 = mix(prepatternInfo0.lowVal, prepatternInfo0.highVal, 
                          pow(getFaceMorph(fid, prepatternInfo0.sourceIndex), prepatternInfo0.exponent));
                if(prepatternInfo0.squared)
                    t0 *= t0;
            }

            if(prepatternInfo1.enabled && prepatternInfo1.targetIndex == morphIndex)
            {
	            t1 = mix(prepatternInfo1.lowVal, prepatternInfo1.highVal, 
                          pow(getFaceMorph(fid, prepatternInfo1.sourceIndex), prepatternInfo1.exponent));
                if(prepatternInfo1.squared)
                   t1 *= t1;
            }

			vec3 tangent = getFaceTangent(fid,morphIndex);
			vec3 veclambda1 = t0 * tangent;
			vec3 veclambda2 = t1 * cross(tangent,posToVec(faces[fid].normal));
			
			vec3 vec1n = normalize(veclambda1);
			vec3 vec2n = normalize(veclambda2);
			// Is it safer to avoid division by zero? e.g.,
			//float len1 = length(veclambda1) + 1e-7;
			//float len2 = length(veclambda2) + 1e-7;
			//vec3 vec1n = veclambda1 / len1;
			//vec3 vec2n = veclambda2 / len2;
			
			a = dot(-0.5f * dot(cross(BC, -N) / dot(N, N), vec1n) * veclambda1, BC * eCotans[gid].vals[morphIndex]);
			a += dot(-0.5f * dot(cross(BC, -N) / dot(N, N), vec2n) * veclambda2, BC * eCotans[gid].vals[morphIndex]);

			b = dot(-0.5f * dot(cross(CA, -N) / dot(N, N), vec1n) * veclambda1, BC * eCotans[gid].vals[morphIndex]);
			b += dot(-0.5f * dot(cross(CA, -N) / dot(N, N), vec2n) * veclambda2, BC * eCotans[gid].vals[morphIndex]);

			c = dot(-0.5f * dot(cross(AB, -N) / dot(N, N), vec1n) * veclambda1, BC * eCotans[gid].vals[morphIndex]);
			c += dot(-0.5f * dot(cross(AB, -N) / dot(N, N), vec2n) * veclambda2, BC * eCotans[gid].vals[morphIndex]);

			diffVec[gid].vec[morphIndex] = Position(a, b, c);
		}
	}
}

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
	
	rng_state = gid * stepCount; // seed the random number generator

    if (gid < vertCount)
        calculate(gid);
}