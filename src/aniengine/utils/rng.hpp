#ifndef __RNG_H__
#define __RNG_H__

#include <random>
#include <vector>

class RNG {
public:
    virtual void manual_seed(uint64_t seed) = 0;
    virtual std::vector<float> randn(uint32_t n) = 0;
};

class STDDefaultRNG : public RNG {
private:
    std::mt19937_64 generator;
    std::uniform_int_distribution<uint64_t> dist;

public:
    STDDefaultRNG() : generator(std::random_device{}()) {}

    void manual_seed(uint64_t seed) override {
        generator.seed(seed);
    }

    std::vector<float> randn(uint32_t n) override {
        std::vector<float> result;
        result.reserve(n);
        std::normal_distribution<float> distribution(0.0f, 1.0f);
        for (uint32_t i = 0; i < n; ++i) {
            result.push_back(distribution(generator));
        }
        return result;
    }

    static uint64_t generate_seed() {
        static STDDefaultRNG instance;
        uint64_t raw = instance.dist(instance.generator);
        // Clear the most significant bit to guarantee non-negative int64_t
        return raw & 0x7FFFFFFFFFFFFFFFULL;
    }
};

#endif  // __RNG_H__