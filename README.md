# 🧬 RFLCS — Repetition-Free Longest Common Subsequence

This project implements exact and heuristic algorithms for solving the **Repetition-Free Longest Common Subsequence (RFLCS)** problem.
It uses Gurobi as the ILP backend.

---

## ✅ Requirements

- **C++ compiler** supporting **C++23**
- **CMake ≥ 3.22.1**
- **Gurobi Optimizer** (for an installation guide please see: https://support.gurobi.com/hc/en-us/articles/4534161999889-How-do-I-install-Gurobi-Optimizer)
- **Python v3**

---

## 🖥️ Platform Compatibility

**Tested on:**

- Linux (Ubuntu 24.04.2 LTS (GNU/Linux 6.8.0-59-generic x86_64))
- macOS (Apple Silicon M3 macOS 15.5:24F74)

---

## 🌱 Environment Variables

Before building, make sure the following environment variable is set (included in the installation guide) and cmake can access them:

| Variable      | Description                 | Examples                                                            | 
|---------------|-----------------------------|---------------------------------------------------------------------|
| `GUROBI_HOME` | Path to Gurobi installation | `/opt/gurobi1200/linux64` or `/Library/gurobi1201/macos_universal2` |

---

## 🛠️ Build Instructions

### Get the Project

```bash
git clone -b masters_thesis https://github.com/sch0rschi/rflcs 
cd rflcs
```

### Build the Project

#### Classic build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/rflcs ./rflcs
```

#### Fast build with Ninja
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
cp build/rflcs ./rflcs
```

## 🚀 Run instructions

```console
./rflcs -i path/to/instance
```

e.g. one instance ⚡

```bash
./rflcs -i ./RFLCS_instances/type0/4096_n-div-4.0
```

or all instances (takes 90+ hours)️
```bash
find ./RFLCS_instances/**/* -type f -exec ./rflcs -i {} \;
```

or all solvable instances (recommended)
```bash
find ./RFLCS_instances/type0/{32,64,128,256,512}* -type f -exec ./rflcs -i {} \;
find ./RFLCS_instances/type0/{1024,2048,4096}_* \
  -type f ! -name '*n-div-8.*' \
  -exec ./rflcs -i {} \;
find ./RFLCS_instances/type1/* -type f -exec ./rflcs -i {} \;
```

---

## Experiment Evaluation (for full run)

```bash
python3 -m venv venv
source venv/bin/activate
python3 -m pip install pandas
python3 results/latex_table.py
python3 results/box_plots.py
deactivate
```
---

## 📦 Dependencies

This project includes third-party libraries (under external/) under the following licenses:

- [Boost](https://www.boost.org/LICENSE_1_0.txt) — Boost Software License 1.0
- [Abseil](https://github.com/abseil/abseil-cpp/blob/master/LICENSE) — Apache License 2.0

These are compatible with the GPLv3 license under which this project is released.