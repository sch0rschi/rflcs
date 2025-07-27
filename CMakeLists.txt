cmake_minimum_required(VERSION 3.22.1)
project(rflcs)
set(CMAKE_CXX_STANDARD 23)

add_compile_options(-O3)
add_compile_options(-march=native)
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
        src/graph/rf_subset_lcs_relaxation.cpp
        src/graph/graph_creation.cpp
        src/graph/graph_writer.cpp
        src/graph/match_deactivation.cpp
        src/graph/match_metrics.cpp
        src/graph/reduce_graph.cpp
        src/graph/simple_upper_bounds.cpp
        src/heuristic.cpp
        src/ilp_solver/gurobi_graph_ilp.cpp
        src/ilp_solver/gurobi_mdd_ilp.cpp
        src/main.cpp
        src/mdd/character_bound_utils.cpp
        src/mdd/character_selection.cpp
        src/mdd/domination_utils.cpp
        src/mdd/flat_mdd.cpp
        src/mdd/initial_mdd.cpp
        src/mdd/mdd_dot_writer.cpp
        src/mdd/mdd_filter.cpp
        src/mdd/mdd_reduction.cpp
        src/mdd/mdd_refinement.cpp
        src/mdd/edge_utils.cpp
        src/reduction_orchestration.cpp
        src/result_writer.cpp
        src/solver/sequence_enumeration_solver.cpp
        src/utils.cpp
)

if(CHARACTER_SET_SIZE)
    set(CHARACTER_SET_SIZES ${CHARACTER_SET_SIZE})
else()
    set(CHARACTER_SET_SIZES 4 8 16 32 64 128 256 512 1024 2048 4096)
endif()

foreach(CHARACTER_SET_SIZE IN LISTS CHARACTER_SET_SIZES)
    add_executable("rflcs_${CHARACTER_SET_SIZE}" ${SOURCE_FILES})
    target_compile_definitions("rflcs_${CHARACTER_SET_SIZE}" PRIVATE CHARACTER_SET_SIZE=${CHARACTER_SET_SIZE})

    target_link_libraries("rflcs_${CHARACTER_SET_SIZE}"
            absl::flat_hash_set
            absl::flat_hash_map
            Boost::dynamic_bitset
            Boost::timer
            $ENV{GUROBI_LIB}
            $ENV{GUROBI_HOME}/lib/libgurobi_c++.a
    )
endforeach()
