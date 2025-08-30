cmake_minimum_required(VERSION 3.22.1)
project(rflcs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

#---------------#
# Build Options #
#---------------#
option(MDD_FREQUENT_SAVE_FEATURE "Enable frequent writebacks while in MDD phase (slow)" ON)
set(ALPHABET_SIZES "" CACHE STRING "List of alphabet sizes to build executables for(e.g. 16;32;64)")

#----------------#
# Compiler Flags #
#----------------#
add_compile_options(
        -O3
        -march=native
        -Wall -Wextra -Wpedantic
        -Wno-unknown-pragmas
        -Wno-c99-extensions
        -Wno-flexible-array-extensions
        -Wno-gcc-compat
        -Wno-nullability-extension
)

#--------------------#
# External Libraries #
#--------------------#
add_library(boost_timer INTERFACE)
add_library(boost_dynamic_bitset INTERFACE)
target_include_directories(boost_dynamic_bitset INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/config/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/container_hash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/describe/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/dynamic_bitset/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/functional/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/integer/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/move/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/mp11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/static_assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/throw_exception/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/type_traits/include
)

target_include_directories(boost_timer INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/timer/include
)
add_subdirectory(external/abseil)

if(DEFINED ENV{GUROBI_HOME} AND DEFINED ENV{GUROBI_LIB})
    include_directories($ENV{GUROBI_HOME}/include)
else()
    message(FATAL_ERROR "GUROBI_HOME or GUROBI_LIB environment variable is not set")
endif()

#--------------#
# Source Files #
#--------------#
set(SOURCE_FILES
        src/heuristic.cpp
        src/globals.cpp
        src/main.cpp
        src/reduction_orchestration.cpp
        src/result_writer.cpp
        src/graph/graph_creation.cpp
        src/graph/match_deactivation.cpp
        src/graph/match_metrics.cpp
        src/graph/reduce_graph.cpp
        src/graph/rf_subset_lcs_relaxation.cpp
        src/graph/simple_upper_bounds.cpp
        src/ilp_solver/gurobi_graph_ilp.cpp
        src/ilp_solver/gurobi_mdd_ilp.cpp
        src/mdd/character_bound_utils.cpp
        src/mdd/character_selection.cpp
        src/mdd/domination_utils.cpp
        src/mdd/edge_utils.cpp
        src/mdd/flat_mdd.cpp
        src/mdd/initial_mdd.cpp
        src/mdd/mdd_filter.cpp
        src/mdd/mdd_reduction.cpp
        src/mdd/mdd_refinement.cpp
        src/solver/sequence_enumeration_solver.cpp
)

#-------------------------------#
# Target Configuration Function #
#-------------------------------#
function(configure_rflcs_target target_name)
    target_link_libraries(${target_name}
            absl::flat_hash_map
            absl::flat_hash_set
            boost_dynamic_bitset
            boost_timer
            $ENV{GUROBI_LIB}
            $ENV{GUROBI_HOME}/lib/libgurobi_c++.a
    )

    if(MDD_FREQUENT_SAVE_FEATURE)
        target_compile_definitions(${target_name} PRIVATE MDD_FREQUENT_SAVE_FEATURE)
    endif()
endfunction()

#-------------#
# Executables #
#-------------#
if(ALPHABET_SIZES)
    foreach(CHARACTER_SET_SIZE IN LISTS ALPHABET_SIZES)
        set(target_name "rflcs_${CHARACTER_SET_SIZE}")
        add_executable(${target_name} ${SOURCE_FILES})
        target_compile_definitions(${target_name} PRIVATE CHARACTER_SET_SIZE=${CHARACTER_SET_SIZE})
        configure_rflcs_target(${target_name})
    endforeach()
else()
    add_executable(rflcs ${SOURCE_FILES})
    configure_rflcs_target(rflcs)
endif()

#----------------#
# Summary Output #
#----------------#
message(STATUS "===================================================")
message(STATUS "RFLCS Build Configuration")
message(STATUS "===================================================")
message(STATUS "C++ Standard:          ${CMAKE_CXX_STANDARD}")
message(STATUS "Memory safe feature:   ${MDD_FREQUENT_SAVE_FEATURE}")
if(ALPHABET_SIZES)
    message(STATUS "Alphabet sizes:        ${ALPHABET_SIZES}")
else()
    message(STATUS "Alphabet size:         dynamic")
endif()
message(STATUS "Gurobi include dir:    $ENV{GUROBI_HOME}/include")
message(STATUS "Gurobi lib:            $ENV{GUROBI_LIB}")
message(STATUS "===================================================")
