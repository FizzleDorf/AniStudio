#ifndef __RNG_H__
#define __RNG_H__

#include <random>
#include <vector>
#include <chrono>
#include <thread>

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
    STDDefaultRNG() {
        std::random_device rd;
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto tid = std::this_thread::get_id();
        // Combine entropy sources to avoid deterministic seeds on Windows/MinGW
        uint64_t seed = static_cast<uint64_t>(rd()) ^
            static_cast<uint64_t>(now) ^
            std::hash<std::thread::id>{}(tid);
        generator.seed(seed);
    }

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