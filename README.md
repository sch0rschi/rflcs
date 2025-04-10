# RFLCS — Repetition-Free Longest Common Subsequence

This project implements exact and heuristic algorithms for solving the
**Repetition-Free Longest Common Subsequence (RFLCS)** problem.
It uses Gurobi as the ILP backend.

---

## Requirements

- **C++ compiler** supporting **C++23**
- **CMake ≥ 3.22.1**
- **Gurobi Optimizer v20.0**
  It is free to use for academic purposes.
  An installation guide is provided
  under [How do I install Gurobi Optimizer?](https://support.gurobi.com/hc/en-us/articles/4534161999889-How-do-I-install-Gurobi-Optimizer).
  And a guide for license setup is available
  under [How do I retrieve and set up a Gurobi license?](https://support.gurobi.com/hc/en-us/articles/12872879801105-How-do-I-retrieve-and-set-up-a-Gurobi-license)
- **python3**
- **pdflatex**

---

## Platform Compatibility

**Tested on:**

- Linux (Ubuntu 24.04.2 LTS (GNU/Linux 6.8.0-59-generic x86_64))
- macOS (Apple Silicon M3 macOS 15.5:24F74)

---

## Environment Variables

Before building, make sure the following environment variable is set (included in the installation guide) and cmake can
access it:

| Variable      | Description                 | Examples                                                            | 
|---------------|-----------------------------|---------------------------------------------------------------------|
| `GUROBI_HOME` | Path to Gurobi installation | `/opt/gurobi1200/linux64` or `/Library/gurobi1201/macos_universal2` |

---

## Build Instructions

The project is built with cmake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/rflcs ./rflcs
```

## Run instructions

The executable takes one parameter `-i` pointing to the input file.
E.g. one instance

```bash
./rflcs -i ./RFLCS_instances/type0/4096_n-div-4.0
```

or all instances (takes 120+ hours)️

```bash
find ./RFLCS_instances/**/* -type f -exec ./rflcs -i {} \;
```

or all solvable instances (takes <1h)

```bash
find ./RFLCS_instances/type0/{32,64,128,256,512}* -type f -exec ./rflcs -i {} \;
find ./RFLCS_instances/type0/{1024,2048,4096}* \
  -type f ! -name '*_n-div-8.*' \
  -exec ./rflcs -i {} \;
find ./RFLCS_instances/type1/* -type f -exec ./rflcs -i {} \;
```

The `runme.sh` script is defined so that all instances are run.
If only the solvable instance categories should be run, then please follow
the instructions in the `runme.sh` Run Experiments section.

---

## Result Files

The result files from our original experiments are provided in the `experiments` folder.

### Result File Example

Each experiment generates a text file in the `results` folder with key metrics.
Below is an example, with a brief description for each field:

| Field                           | Example Value                   | Description                                                    |
|---------------------------------|---------------------------------|----------------------------------------------------------------|
| solved                          | true                            | Whether the algorithm found and proved an optimal solution     |
| solution_length                 | 8                               | Length of the solution found                                   |
| upper_bound                     | 8                               | Derived upper bound                                            |
| solution_runtime                | 8.8e-05                         | Time taken to find the solution (in seconds)                   |
| heuristic_solution_length       | 8                               | Length of the solution found by the heuristic                  |
| heuristic_solution_runtime      | 6.4e-05                         | Time taken by the heuristic (in seconds)                       |
| heuristic_runtime               | 8.3e-05                         | Total runtime of the heuristic (in seconds)                    |
| solving_forward                 | true                            | Indicates forward-solving was applied                          |
| reduction_is_complete           | true                            | Whether the MDD reduction process completed for all characters |
| reduction_quality               | 1                               | Quality of the reduction (float between 0 and 1)               |
| reduction_upper_bound           | 8                               | Upper bound after the MDD reduction                            |
| reduction_runtime               | 5e-06                           | Time taken for the MDD reduction process                       |
| mdd_memory_consumption          | 0                               | Memory used by the MDD (bytes)                                 |
| main_process_memory_consumption | 0                               | Memory used by the main process (bytes)                        |
| solver_solution_length          | 0                               | Length of the solution found by the ILP                        |
| solver_upper_bound              | 0                               | Upper bound reported by the ILP                                |
| solver_runtime                  | 0                               | Time taken by the ILP                                          |
| solution                        | [20, 12, 23, 15, 1, 10, 22, 16] | The found solution subsequence                                 |

---

## Experiment Evaluation

With the result files, the comparison result tables from the paper can be generated.

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install pandas
cd results || exit
python3 latex_table.py
python3 box_plots.py
deactivate
pdflatex -jobname=result_tables main.tex
cd ..
```

The [result_tables.pdf](results/result_tables.pdf) file contains the result tables generated from the experiments.
Any experiments that were not executed will not appear in these tables.

---

## Dependencies

This project includes third-party libraries (under external/) under the following licenses:

- [Boost](https://www.boost.org/LICENSE_1_0.txt) — Boost Software License 1.0
- [Abseil](https://github.com/abseil/abseil-cpp/blob/master/LICENSE) — Apache License 2.0

These are compatible with the GPLv3 license under which this project is released.