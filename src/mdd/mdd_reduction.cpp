#include "header/mdd_reduction.hpp"
#include "mdd.hpp"
#include "header/mdd_refinement.hpp"

#include <memory>
#include <vector>

#include "header/character_selection.h"
#include "header/mdd_filter.h"
#include "header/initial_mdd.hpp"
#include "header/mdd_dot_writer.hpp"
#include "../config.hpp"
#include <iostream>
#include <fstream>

void make_only_one_best_solution_remaining(
    const instance &instance,
    mdd_node_source &mdd_node_source,
    const mdd &mdd_reduction);

inline bool is_power_of_2(const int n) {
    return n > 0 && (n & n - 1) == 0;
}

bool is_perfect_square(const int n) {
    if (n <= 0) return false;
    const int sqrt_n = static_cast<int>(std::sqrt(n));
    return sqrt_n * sqrt_n == n;
}

void add_counts(const mdd &mdd_reduction, std::vector<long> &time_series_node_count, std::vector<long> &time_series_edge_count) {
    auto node_count = 0l;
    auto edge_count = 0l;
    for (const auto &level: *mdd_reduction.levels) {
        node_count += static_cast<long>(level->nodes->size());
        for (const auto node: *level->nodes) {
            edge_count += static_cast<long>(node->arcs_out.size());
        }
    }
    time_series_node_count.push_back(node_count);
    time_series_edge_count.push_back(edge_count);
}

void reduce_by_mdd(instance &instance) {
    const std::unique_ptr<mdd_node_source> mdd_node_source = std::make_unique<struct mdd_node_source>();
    const std::unique_ptr<character_counters_source> character_counters_source = std::make_unique<struct character_counters_source>();

    auto mdd_reduction = mdd::copy_mdd(*instance.mdd, *mdd_node_source);
    prune_by_flat_mdd(instance.shared_object, *mdd_reduction, *mdd_node_source);
    filter_mdd(instance, *mdd_reduction, true, *mdd_node_source);
    auto mdd_character_selection = mdd::copy_mdd(*mdd_reduction, *mdd_node_source);

    if (instance.lower_bound >= instance.upper_bound) {
        instance.shared_object->is_mdd_reduction_complete = true;
        return;
    }

    instance.shared_object->number_of_refined_characters = 0;

    auto time_series_node_count = std::vector<long>();
    auto time_series_edge_count = std::vector<long>();

    if (IS_WRITING_TIME_SERIES) {
        add_counts(*mdd_reduction, time_series_node_count, time_series_edge_count);
    }

    instance.shared_object->number_of_refined_characters = 0;
    for (int split_character: std::views::iota(0, instance.alphabet_size)) {
        refine_mdd(instance, *mdd_reduction, split_character, *mdd_node_source);
        filter_mdd(instance, *mdd_reduction, true, *mdd_node_source);
        instance.shared_object->number_of_refined_characters++;
        instance.shared_object->upper_bound =
                std::min(instance.shared_object->upper_bound, mdd_reduction->levels->back()->depth);
        instance.shared_object->upper_bound =
                std::max(instance.shared_object->upper_bound, instance.lower_bound);
        if (!mdd_reduction->levels->empty()) {
            instance.shared_object->upper_bound =
                    std::max(instance.shared_object->upper_bound,
                             mdd_reduction->levels->back()->nodes->front()->upper_bound_down);
        }
        filter_flat_mdd(instance, *mdd_reduction, true);

        if (IS_WRITING_TIME_SERIES) {
            add_counts(*mdd_reduction, time_series_node_count, time_series_edge_count);
        }

        if (instance.lower_bound >= instance.shared_object->upper_bound) {
            break;
        }
    }

    if (IS_WRITING_TIME_SERIES) {
        std::ofstream node_count_file("time_series_node_count.txt", std::ios::app);
        for (const auto& count : time_series_node_count) {
            node_count_file << count << ", ";
        }
        node_count_file << "\n";
        node_count_file.close();

        std::ofstream edge_count_file("time_series_edge_count.txt", std::ios::app);
        for (const auto& count : time_series_edge_count) {
            edge_count_file << count << ", ";
        }
        edge_count_file << "\n";
        edge_count_file.close();
    }

    instance.shared_object->is_mdd_reduction_complete = true;
    make_only_one_best_solution_remaining(instance, *mdd_node_source, *mdd_reduction);
    serialize_initial_mdd(*mdd_reduction, instance.shared_object);
    if (IS_WRITING_DOT_FILE) {
        write_mdd_dot(*mdd_reduction, "refined.dot");
    }
}

void make_only_one_best_solution_remaining(
    const instance &instance,
    mdd_node_source &mdd_node_source,
    const mdd &mdd_reduction) {
    for (auto const &level: *mdd_reduction.levels) {
        for (const auto &node: *level->nodes) {
            node->is_active = false;
        }
    }
    auto solution_node = mdd_reduction.levels->front()->nodes->front();
    while (!solution_node->arcs_out.empty()) {
        solution_node->is_active = true;
        int max_upper_bound_down = -1;
        for (const auto &succ_node: solution_node->arcs_out) {
            if (succ_node->upper_bound_down > max_upper_bound_down) {
                solution_node = succ_node;
                max_upper_bound_down = succ_node->upper_bound_down;
            }
        }
    }
    solution_node->is_active = true;

    for (auto const &level: *mdd_reduction.levels) {
        for (const auto node: *level->nodes) {
            if (!node->is_active) {
                mdd_node_source.clear_node(node);
            }
        }
        std::erase_if(*level->nodes, [](auto *node) { return !node->is_active; });
    }
    filter_flat_mdd(instance, mdd_reduction, true);
}
