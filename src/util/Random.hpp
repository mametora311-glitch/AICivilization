#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace aic {

// Deterministic PRNG wrapper (Mersenne Twister) so a given seed reproduces a
// run exactly (spec: settings/scenario seed controls the simulation).
class Random {
public:
    void seed(unsigned int s);
    unsigned int seedValue() const { return seed_; }

    float nextFloat(float a = 0.0f, float b = 1.0f);
    int   nextInt(int a, int b);          // inclusive range [a, b]
    bool  chance(float p);                // true with probability p in [0,1]
    float gaussian(float mean, float stddev);

    template <typename T>
    const T& pick(const std::vector<T>& v) {
        return v[static_cast<size_t>(nextInt(0, static_cast<int>(v.size()) - 1))];
    }

private:
    std::mt19937 rng_{ std::random_device{}() };
    unsigned int seed_ = 0;
};

} // namespace aic
