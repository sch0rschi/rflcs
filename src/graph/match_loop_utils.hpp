#pragma once

#include "graph.hpp"

#include <ranges>

static constexpr auto active_match_pointer_filter = std::ranges::views::filter([](const rflcs_graph::match *maybe) {
    return maybe->is_active;
});
static constexpr auto active_match_filter = std::ranges::views::filter([](rflcs_graph::match const &maybe) {
    return maybe.is_active;
});
