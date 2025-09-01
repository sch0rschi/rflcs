#pragma once

#include "mdd_node.hpp"

typedef std::vector<node *> level_nodes_type;

struct level_type {
    level_nodes_type nodes = level_nodes_type();
    int depth = 0;
    bool needs_pruning;

    ~level_type() {
        nodes.clear();
    }
};

typedef std::vector<std::unique_ptr<level_type>> levels_type;
