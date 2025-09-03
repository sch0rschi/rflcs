#pragma once

#include <vector>
#include "../../constants.hpp"

struct character_counters_source {
private:
    std::vector<std::vector<int> *>* cache = new std::vector<std::vector<int> *>();

public:
    void return_counter(std::vector<int>* counters) const {
        cache->push_back(counters);
    }

    [[nodiscard]] std::vector<int>* get_counter() const {
        std::vector<int>* fresh_counters;
        if (cache->empty()) {
            fresh_counters = new std::vector<int>(constants::alphabet_size);
        } else {
            fresh_counters = cache->back();
            cache->pop_back();
        }
        std::ranges::fill(*fresh_counters, 0);
        return fresh_counters;
    }

    ~character_counters_source() {
        for (const auto *counter: *cache) {
            delete counter;
        }
        delete cache;
    }
};
