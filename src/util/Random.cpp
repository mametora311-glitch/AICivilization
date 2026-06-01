#include "util/Random.hpp"

#include <algorithm>

namespace aic {

void Random::seed(unsigned int s) {
    seed_ = s;
    rng_.seed(s);
}

float Random::nextFloat(float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng_);
}

int Random::nextInt(int a, int b) {
    if (b < a) std::swap(a, b);
    std::uniform_int_distribution<int> d(a, b);
    return d(rng_);
}

bool Random::chance(float p) {
    if (p <= 0.0f) return false;
    if (p >= 1.0f) return true;
    std::bernoulli_distribution d(p);
    return d(rng_);
}

float Random::gaussian(float mean, float stddev) {
    std::normal_distribution<float> d(mean, stddev);
    return d(rng_);
}

} // namespace aic
