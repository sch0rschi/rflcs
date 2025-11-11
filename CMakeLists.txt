cmake_minimum_required(VERSION 3.22.1)
project(rflcs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

#---------------#
# Build Options #
#---------------#
option(MDD_FREQUENT_SAVE_FEATURE "Enable frequent writebacks while in MDD phase (slow)" ON)
if(MDD_FREQUENT_SAVE_FEATURE)
    add_compile_definitions(MDD_FREQUENT_SAVE_FEATURE)
endif()

option(ILP_FEATURE "Enable ILP feature" ON)

#-----------------------#
# Gurobi Detection      #
#-----------------------#
if(ILP_FEATURE)
    if(NOT DEFINED ENV{GUROBI_HOME})
        message(WARNING "GUROBI_HOME environment variable not set. ILP feature disabled.")
        set(ILP_FEATURE OFF)
    else()
        add_compile_definitions(ILP_FEATURE)
        set(GUROBI_INCLUDE_DIR "$ENV{GUROBI_HOME}/include")
        set(GUROBI_LIB_DIR "$ENV{GUROBI_HOME}/lib")

        message(STATUS "Using GUROBI_HOME = $ENV{GUROBI_HOME}")
        message(STATUS "Expecting includes in ${GUROBI_INCLUDE_DIR}")

        find_library(GUROBI_LIBRARY
                NAMES gurobi gurobi120 gurobi110 gurobi100
                HINTS "${GUROBI_LIB_DIR}"
        )

        find_library(GUROBI_CXX_LIBRARY
                NAMES gurobi_c++ gurobi_c++.a libgurobi_c++.a
                HINTS "${GUROBI_LIB_DIR}"
        )

        if(NOT GUROBI_LIBRARY OR NOT GUROBI_CXX_LIBRARY)
            message(WARNING "Could not find Gurobi libraries in ${GUROBI_LIB_DIR}. ILP feature disabled.")
            set(ILP_FEATURE OFF)
        endif()
    endif()
endif()

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

add_library(boost_timer INTERFACE)
target_include_directories(boost_timer INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/timer/include
)

file(GLOB BOOST_PROGRAM_OPTIONS_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/program_options/src/*.cpp
)
add_library(boost_program_options STATIC ${BOOST_PROGRAM_OPTIONS_SRC})
target_include_directories(boost_program_options PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/program_options/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/any/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/bind/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/config/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/container/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/container_hash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/detail/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/function/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/iterator/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/lexical_cast/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/mp11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/mpl/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/move/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/throw_exception/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/type_index/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/smart_ptr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/static_assert/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/tokenizer/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/boost/type_traits/include
)

add_subdirectory(external/abseil)

#--------------#
# Source Files #
#--------------#
set(SOURCE_FILES
        src/constants.cpp
        src/heuristic.cpp
        src/temporaries.cpp
        src/main.cpp
        src/reduction_orchestration.cpp
        src/result_writer.cpp
        src/graph/graph_creation.cpp
        src/graph/match_deactivation.cpp
        src/graph/match_metrics.cpp
        src/graph/reduce_graph.cpp
        src/graph/rf_subset_lcs_relaxation.cpp
        src/graph/simple_upper_bounds.cpp
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
        src/input_processing.cpp
        src/input_processing.hpp
)

if(ILP_FEATURE)
    list(APPEND SOURCE_FILES
            src/ilp_solver/gurobi_mis_ilp.cpp
            src/ilp_solver/gurobi_mdd_ilp.cpp
            src/ilp_solver/gurobi_graph_ilp.cpp
    )
endif()

#-------------------------------#
# Target Configuration Function #
#-------------------------------#
function(configure_rflcs_target target_name)
    target_link_libraries(${target_name} PRIVATE
            absl::flat_hash_map
            absl::flat_hash_set
            boost_dynamic_bitset
            boost_timer
            boost_program_options
    )

    if(ILP_FEATURE AND GUROBI_INCLUDE_DIR)
        target_include_directories(${target_name} PRIVATE ${GUROBI_INCLUDE_DIR})
        target_link_directories(${target_name} PRIVATE ${GUROBI_LIB_DIR})
        target_link_libraries(${target_name} PRIVATE ${GUROBI_CXX_LIBRARY} ${GUROBI_LIBRARY})
    endif()

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

        # Attach Gurobi includes to IDE/compiler
        if(ILP_FEATURE AND GUROBI_INCLUDE_DIR)
            target_include_directories(${target_name} PRIVATE ${GUROBI_INCLUDE_DIR})
        endif()

        target_compile_definitions(${target_name} PRIVATE CHARACTER_SET_SIZE=${CHARACTER_SET_SIZE})
        configure_rflcs_target(${target_name})
    endforeach()
else()
    add_executable(rflcs ${SOURCE_FILES})

    # Attach Gurobi includes to IDE/compiler
    if(ILP_FEATURE AND GUROBI_INCLUDE_DIR)
        target_include_directories(rflcs PRIVATE ${GUROBI_INCLUDE_DIR})
    endif()

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
message(STATUS "ILP feature:           ${ILP_FEATURE}")
if (ILP_FEATURE)
    message(STATUS "Gurobi include dir:    $ENV{GUROBI_HOME}/include")
endif ()
message(STATUS "===================================================")
