#define USE_NRAN_METHOD 0 // 0 fastest, 1 slower, 2 slowest... -1 fastest wrapped in a class... todo: needs to be cleaned-up!!!
#if USE_NRAN_METHOD==-1
#include "Nran.h"

#include <cmath>


NormalDist::NormalDist(unsigned int rng_seed)
	: rng_state(rng_seed), hasSpare(false), spare(0.f)
{}

// psuedorandom number generator via a hash function:
// Mark Jarzynski and Marc Olano, Hash Functions for GPU Rendering, Journal of Computer Graphics Techniques (JCGT), vol. 9, no. 3, 21-38, 2020
// Available online http://jcgt.org/published/0009/03/02/
// Also, see Nathan Reed's blog post from May 21, 2021: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
unsigned int NormalDist::rand_pcg()
{
	unsigned int state = rng_state;
	rng_state = rng_state * 747796405u + 2891336453u;
	unsigned int word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

// https://en.wikipedia.org/wiki/Marsaglia_polar_method
float NormalDist::random()
{
	if (hasSpare) {
		hasSpare = false;
		return spare;
	}
	else {
		float u, v, s;
		unsigned int n1, n2;
		do {
			n1 = rand_pcg();
			n2 = rand_pcg();
			u = (((float)n1) / ((float)UINT_MAX + 1)) * 2.f - 1.f;
			v = (((float)n2) / ((float)UINT_MAX + 1)) * 2.f - 1.f;
			s = u * u + v * v;
		} while (s >= 1.f || s == 0.f);
		s = std::sqrt(-2.f * std::log(s) / s);
		spare = v * s;
		hasSpare = true;
		return u * s;
	}
}

#elif USE_NRAN_METHOD==0
#include "Nran.h"

#include <cstdlib>
#include <cmath>


// psuedorandom number generator via a hash function:
// Mark Jarzynski and Marc Olano, Hash Functions for GPU Rendering, Journal of Computer Graphics Techniques (JCGT), vol. 9, no. 3, 21-38, 2020
// Available online http://jcgt.org/published/0009/03/02/
// Also, see Nathan Reed's blog post from May 21, 2021: https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
static thread_local unsigned int rng_state;
void seed_nran(unsigned int rng_seed)
{
	rng_state = rng_seed;
}
unsigned int rand_pcg()
{
	unsigned int state = rng_state;
	rng_state = rng_state * 747796405u + 2891336453u;
	unsigned int word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

// https://en.wikipedia.org/wiki/Marsaglia_polar_method
float nran() {
	static thread_local float spare;
	static thread_local bool hasSpare = false;

	if (hasSpare) {
		hasSpare = false;
		return spare;
	}
	else {
		float u, v, s;
		unsigned int n1, n2;
		do {
			n1 = rand_pcg();
			n2 = rand_pcg();
			u = (((float)n1) / ((float)UINT_MAX + 1)) * 2.f - 1.f;
			v = (((float)n2) / ((float)UINT_MAX + 1)) * 2.f - 1.f;
			s = u * u + v * v;
		} while (s >= 1.f || s == 0.f);
		s = std::sqrt(-2.f * std::log(s) / s);
		spare = v * s;
		hasSpare = true;
		return u * s;
	}
}

#elif USE_NRAN_METHOD==1
#define _CRT_RAND_S
#include "Nran.h"

#include <cstdlib>
#include <cmath>


// TODO: is this really thread safe?
// maybe it would be better to save state of random number generator and use a different seed for each thread
// https://en.wikipedia.org/wiki/Marsaglia_polar_method
float nran() {
	static thread_local float spare;
	static thread_local bool hasSpare = false;

	if (hasSpare) {
		hasSpare = false;
		return spare;
	}
	else {
		float u, v, s;
		unsigned int n1, n2;
		do {
			rand_s(&n1);
			rand_s(&n2);
			u = (((float)n1) / ((float)UINT_MAX + 1)) * 2.f - 1.f;
			v = (((float)n2) / ((float)UINT_MAX + 1)) * 2.f - 1.f;
			s = u * u + v * v;
		} while (s >= 1.f || s == 0.f);
		s = std::sqrt(-2.f * std::log(s) / s);
		spare = v * s;
		hasSpare = true;
		return u * s;
	}
}

#else

#include <random>
#include "Nran.h"

float nran(void)
{
	static thread_local std::random_device __randomDevice;
	static thread_local std::mt19937 __randomGen(__randomDevice());
	static std::normal_distribution<float> __nran(0., 1.);
	return (__nran(__randomGen));
}

#endif