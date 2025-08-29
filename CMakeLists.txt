cmake_minimum_required(VERSION 3.22.1)
project(rflcs)
set(CMAKE_CXX_STANDARD 23)

add_compile_options(-O3)
add_compile_options(-march=native)
add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options(-Wno-unknown-pragmas)
add_compile_options(-Wno-c99-extensions)
add_compile_options(-Wno-flexible-array-extensions)

add_library(boost_dynamic_bitset INTERFACE)
target_include_directories(boost_dynamic_bitset INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/dynamic_bitset/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/config/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/static_assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/integer/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/type_traits/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/functional/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/move/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/throw_exception/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/container_hash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/describe/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/mp11/include
)

add_library(boost_timer INTERFACE)
target_include_directories(boost_timer INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/timer/include
)

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
)

if(DEFINED ALPHABET_SIZES)
    foreach(CHARACTER_SET_SIZE IN LISTS ALPHABET_SIZES)
        add_executable("rflcs_${CHARACTER_SET_SIZE}" ${SOURCE_FILES})
        target_compile_definitions("rflcs_${CHARACTER_SET_SIZE}" PRIVATE CHARACTER_SET_SIZE=${CHARACTER_SET_SIZE})

        target_link_libraries("rflcs_${CHARACTER_SET_SIZE}"
                absl::flat_hash_set
                absl::flat_hash_map
                boost_dynamic_bitset
                boost_timer
                $ENV{GUROBI_LIB}
                $ENV{GUROBI_HOME}/lib/libgurobi_c++.a
        )
    endforeach()
else()
    add_executable(rflcs ${SOURCE_FILES})

    target_link_libraries(rflcs
            absl::flat_hash_set
            absl::flat_hash_map
            boost_dynamic_bitset
            boost_timer
            $ENV{GUROBI_LIB}
            $ENV{GUROBI_HOME}/lib/libgurobi_c++.a
    )
endif()
