# RFLCS — Repetition-Free Longest Common Subsequence

This project implements exact and heuristic algorithms for solving the **Repetition-Free Longest Common Subsequence (RFLCS)** problem.
It uses Gurobi as the ILP backend.

---

## Requirements

- **C++ compiler** supporting **C++23**
- **CMake ≥ 3.22.1**
- **Gurobi Optimizer**

---

## Platform Compatibility

**Tested on:**

- Linux (Ubuntu 24.04.2 LTS (GNU/Linux 6.8.0-59-generic x86_64))
- macOS (Apple Silicon M3 macOS 15.5:24F74)

---

## Environment Variables

Before building, make sure the following environment variable is set and CMake can access them:

| Variable      | Description                                   | Example                                                                                                        |
|---------------|-----------------------------------------------|----------------------------------------------------------------------------------------------------------------|
| `GUROBI_HOME` | Root path to Gurobi installation              | `/opt/gurobi1200/linux64` or `/Library/gurobi1201/macos_universal2`                                            |

---

## Getting the project

Because this project has git submodules the correct way to clone is by considering the submodules.

```bash
git clone --recurse-submodules https://github.com/sch0rschi/rflcs
cd rflcs
```

## Build Instructions

### Build Configuration Options

ALPHABET_SIZES: List of alphabet sizes to build executables for, e.g., 16;32;64;512. If empty, a single dynamic target rflcs is built.
MDD_FREQUENT_SAVE_FEATURE: Enable (ON) or disable (OFF) frequent writebacks during the MDD phase (default ON, is slower).

Build the Project
Example: Build with alphabet size 512 and disable frequent MDD save

```bash
cmake -S . -B build -G Ninja -DALPHABET_SIZES=512 -DMDD_FREQUENT_SAVE_FEATURE=OFF
ninja -C build
cp build/rflcs_512 .
```

This produces an executable rflcs_512 with MDD_FREQUENT_SAVE_FEATURE enabled.
Standard build

```bash
cmake -S . -B build -G Ninja
ninja -C build
cp build/rflcs .
```

This produces a single executable rflcs that works with every alphabet size.
Run Instructions

```bash
./rflcs -i ./RFLCS_instances/type1/512_8reps.24
```

Or, for a specific alphabet size (example 512):

```bash
./rflcs_512 -i ./RFLCS_instances/type1/512_8reps.24
```

And all the type 1 512 characters with eight repetition:

```bash
find ./RFLCS_instances/type1/512_8reps.* -type f -exec ./rflcs_512 -i {} \;
```

---

## Dependencies

This project includes third-party libraries under the following licenses:

- Boost — Boost Software License 1.0
- Abseil — Apache License 2.0

These are compatible with the GPLv3 license under which this project is released.
