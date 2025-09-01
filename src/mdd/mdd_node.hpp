#pragma once

#include "../graph/graph.hpp"

#include <memory>
#include <sstream>

#include "../temporaries.hpp"
#include "boost/dynamic_bitset/dynamic_bitset.hpp"

struct node;

// TODO: benchmark other containers
typedef std::vector<node *> edges_type;

struct node {
    rflcs_graph::match *match = nullptr;
    Character_set characters_on_paths_to_root; // including match character
    Character_set characters_on_all_paths_to_root; // including match character
    Character_set characters_on_paths_to_some_sink; // not including match character
    Character_set characters_on_all_paths_to_lower_bound_levels; // not including match character
    std::vector<int> *sequences_character_counter; // not counting match character
    edges_type edges_out = edges_type();
    edges_type edges_in = edges_type();
    node *copy_helper;
    GRBVar gurobi_variable;
    int upper_bound_down = INT_MAX / 4; // not including match character
    bool needs_update_from_pred = true;
    bool needs_update_from_succ = true;
    bool is_active = true;

    void clear();

    void link_pred_to_succ(node *succ);

    void unlink_pred_from_succ(node *succ);

    bool update_from_preds(int depth);

    bool update_from_succs(int depth, int lower_bound);

    void notify_relatives_of_update() const;

    void notify_succs_of_update() const;

    void notify_preds_of_update() const;

    void deactivate();

    [[maybe_unused]] [[nodiscard]] std::string to_string() const;
};

inline void node::clear() {
    this->is_active = false;
    this->match = nullptr;
    for (auto *pred: edges_in) {
        std::erase(pred->edges_out, this);
        pred->needs_update_from_succ = true;
    }
    this->edges_in.clear();
    for (auto *succ: edges_out) {
        std::erase(succ->edges_in, this);
        succ->needs_update_from_pred = true;
    }
    this->edges_out.clear();
    this->characters_on_paths_to_root.set();
    this->characters_on_all_paths_to_root.reset();
    this->characters_on_paths_to_some_sink.set();
    this->characters_on_all_paths_to_lower_bound_levels.reset();
    this->needs_update_from_pred = false;
    this->needs_update_from_succ = false;
}

inline void node::link_pred_to_succ(node *succ) {
    this->edges_out.push_back(succ);
    succ->edges_in.push_back(this);
}

inline void node::unlink_pred_from_succ(node *succ) {
    std::erase(this->edges_out, succ);
    std::erase(succ->edges_in, this);
    this->needs_update_from_succ = true;
    succ->needs_update_from_pred = true;
}

inline bool node::update_from_preds(const int depth) {
    this->needs_update_from_pred = false;

    temporaries::old_characters_on_paths_to_root = this->characters_on_paths_to_root;
    temporaries::old_characters_on_all_paths_to_root = this->characters_on_all_paths_to_root;
    temporaries::old_characters_on_paths_to_some_sink = this->characters_on_paths_to_some_sink;

    temporaries::temp_character_set_1.reset();
    this->characters_on_all_paths_to_root.set();
    for (const auto pred: this->edges_in) {
        temporaries::temp_character_set_1 |= pred->characters_on_paths_to_root;
        this->characters_on_all_paths_to_root &= pred->characters_on_all_paths_to_root;
    }
    this->characters_on_paths_to_root &= temporaries::temp_character_set_1;
    this->characters_on_paths_to_root.set(this->match->character);
    this->characters_on_all_paths_to_root.set(this->match->character);
    if (depth == static_cast<int>(this->characters_on_paths_to_root.count())) {
        this->characters_on_all_paths_to_root = this->characters_on_paths_to_root;
    }
    this->characters_on_paths_to_some_sink &= ~ this->characters_on_all_paths_to_root;

    temporaries::old_characters_on_paths_to_root &= ~ this->characters_on_paths_to_root;
    temporaries::old_characters_on_all_paths_to_root ^= this->characters_on_all_paths_to_root;
    temporaries::old_characters_on_paths_to_some_sink &= ~ this->characters_on_paths_to_some_sink;
    const bool notify_preds = temporaries::old_characters_on_paths_to_some_sink.any();
    const bool notify_succs = temporaries::old_characters_on_paths_to_root.any()
                              || temporaries::old_characters_on_all_paths_to_root.any();
    if (notify_preds && notify_succs) {
      this->notify_relatives_of_update();
    } else {
        if (notify_preds) {
            this->notify_preds_of_update();
        }
        if (notify_succs) {
            this->notify_succs_of_update();
        }
    }

    return notify_succs || notify_preds;
}

inline bool node::update_from_succs(const int depth, const int lower_bound) {
    this->needs_update_from_succ = false;
    temporaries::old_characters_on_paths_to_some_sink = this->characters_on_paths_to_some_sink;
    temporaries::old_characters_on_all_paths_to_lower_bound_levels = this->characters_on_all_paths_to_lower_bound_levels;
    const int old_upper_bound_down = this->upper_bound_down;

    if (this->edges_out.empty()) {
        if (this->characters_on_paths_to_some_sink.any()
            || this->characters_on_all_paths_to_lower_bound_levels.any()
            || this->upper_bound_down > 0) {
            this->notify_preds_of_update();
        }
        this->upper_bound_down = 0;
        this->characters_on_paths_to_some_sink.reset();
        this->characters_on_all_paths_to_lower_bound_levels.reset();
    }

    auto max_upper_bound_down_succ = 0;
    temporaries::temp_character_set_1.reset();
    this->characters_on_all_paths_to_lower_bound_levels.set();
    for (const auto succ: this->edges_out) {
        max_upper_bound_down_succ = std::max(max_upper_bound_down_succ, succ->upper_bound_down);
        temporaries::temp_character_set_1 |= succ->characters_on_paths_to_some_sink;
        temporaries::temp_character_set_1.set(succ->match->character);
        temporaries::temp_character_set_2 = succ->characters_on_all_paths_to_lower_bound_levels;
        if (depth > 0 && depth <= lower_bound) {
            temporaries::temp_character_set_2.set(succ->match->character);
        }
        this->characters_on_all_paths_to_lower_bound_levels &= temporaries::temp_character_set_2;
    }
    this->characters_on_paths_to_some_sink &= temporaries::temp_character_set_1;

    if (this->edges_out.empty()) {
        this->characters_on_all_paths_to_lower_bound_levels.reset();
    }

    this->upper_bound_down = std::min(this->upper_bound_down, max_upper_bound_down_succ + 1);
    this->upper_bound_down = std::min(this->upper_bound_down,
                                      static_cast<int>(this->characters_on_paths_to_some_sink.count()));

    const bool notify_relatives = old_upper_bound_down > this->upper_bound_down;
    temporaries::old_characters_on_paths_to_some_sink &= ~ this->characters_on_paths_to_some_sink;
    temporaries::old_characters_on_all_paths_to_lower_bound_levels ^= this->characters_on_all_paths_to_lower_bound_levels;
    const bool notify_preds = temporaries::old_characters_on_paths_to_some_sink.any() ||
                              temporaries::old_characters_on_all_paths_to_lower_bound_levels.any();

    if (notify_relatives) {
        this->notify_relatives_of_update();
    } else if (notify_preds) {
        this->notify_preds_of_update();
    }

    return notify_relatives || notify_preds;
}

inline void node::notify_relatives_of_update() const {
    for (auto *pred: this->edges_in) {
        pred->needs_update_from_succ = true;
    }
    for (auto *succ: this->edges_out) {
        succ->needs_update_from_pred = true;
    }
}

inline void node::notify_succs_of_update() const {
    for (auto *succ: this->edges_out) {
        succ->needs_update_from_pred = true;
    }
}

inline void node::notify_preds_of_update() const {
    for (auto *pred: this->edges_in) {
        pred->needs_update_from_succ = true;
    }
}

[[maybe_unused]] [[maybe_unused]] inline std::string node::to_string() const {
    std::stringstream string_stream;
    string_stream << this << "\t"
            << this->match->character << "\t"
            << this->upper_bound_down << "\t"
            << this->edges_in.size() << "\t"
            << this->edges_out.size() << "\t";
    return string_stream.str();
}

inline void node::deactivate() {
    for (const auto pred: edges_in) {
        pred->unlink_pred_from_succ(this);
    }
    for (const auto succ: edges_out) {
        this->unlink_pred_from_succ(succ);
    }
    this->is_active = false;
}
