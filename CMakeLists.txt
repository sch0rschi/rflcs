cmake_minimum_required(VERSION 3.22.1)
project(rflcs)
set(CMAKE_CXX_STANDARD 23)

#add_compile_options(-O3)
#add_compile_options(-march=native)
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-Wno-unknown-pragmas)
add_compile_options(-Wno-c99-extensions)
add_compile_options(-Wno-flexible-array-extensions)

add_subdirectory(external/boost)
add_subdirectory(external/abseil)

if (DEFINED ENV{GUROBI_HOME} AND DEFINED ENV{GUROBI_LIB})
    include_directories($ENV{GUROBI_HOME}/include)
else ()
    message(FATAL_ERROR "GUROBI_HOME or GUROBI_LIB environment variable is not set")
endif ()

set(SOURCE_FILES
        src/formatting_utils.cpp
        src/globals.cpp
        src/graph/graph_creation.cpp
        src/graph/graph_writer.cpp
        src/graph/match_deactivation.cpp
        src/graph/match_metrics.cpp
        src/graph/reduce_graph.cpp
        src/refinement_graph/refinement_graph_reduction.cpp
        src/refinement_graph/refinement_graph_initialisation.cpp
        src/graph/rf_subset_lcs_relaxation.cpp
        src/graph/simple_upper_bounds.cpp
        src/heuristic.cpp
        src/ilp_solver/gurobi_graph_ilp.cpp
        src/ilp_solver/gurobi_mdd_ilp.cpp
        src/main.cpp
        src/mdd/character_bound_utils.cpp
        src/mdd/character_selection.cpp
        src/mdd/domination_utils.cpp
        src/mdd/edge_utils.cpp
        src/mdd/flat_mdd.cpp
        src/mdd/initial_mdd.cpp
        src/mdd/mdd_dot_writer.cpp
        src/mdd/mdd_filter.cpp
        src/mdd/mdd_reduction.cpp
        src/mdd/mdd_refinement.cpp
        src/reduction_orchestration.cpp
        src/result_writer.cpp
        src/solver/sequence_enumeration_solver.cpp
        src/utils.cpp
        src/refinement_graph/refinement_graph_refinement.cpp
        src/refinement_graph/header/refinement_graph_refinement.hpp
        src/refinement_graph/refinement_graph_filter.cpp
        src/refinement_graph/header/refinement_graph_filter.hpp
        src/refinement_graph/refinement_graph_dot_writer.cpp
        src/refinement_graph/header/refinement_graph_dot_writer.hpp
)

add_executable(rflcs ${SOURCE_FILES})

target_link_libraries(rflcs
        absl::flat_hash_set
        absl::flat_hash_map
        Boost::dynamic_bitset
        Boost::timer
        Boost::graph
        $ENV{GUROBI_LIB}
        $ENV{GUROBI_HOME}/lib/libgurobi_c++.a
)
