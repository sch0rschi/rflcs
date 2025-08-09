# 🧬 RFLCS — Repetition-Free Longest Common Subsequence

This project implements exact and heuristic algorithms for solving the **Repetition-Free Longest Common Subsequence (RFLCS)** problem.
It uses Gurobi as the ILP backend.

---

## ✅ Requirements

- **C++ compiler** supporting **C++23**
- **CMake ≥ 3.22.1**
- **Gurobi Optimizer**

---

## 🖥️ Platform Compatibility

**Tested on:**

- Linux (Ubuntu 24.04.2 LTS (GNU/Linux 6.8.0-59-generic x86_64))
- macOS (Apple Silicon M3 macOS 15.5:24F74)

---

## 🌱 Environment Variables

Before building, make sure the following environment variables are set and cmake can access them:

| Variable      | Description                                   | Example                                                                                                        |
|---------------|-----------------------------------------------|----------------------------------------------------------------------------------------------------------------|
| `GUROBI_HOME` | Root path to Gurobi installation              | `/opt/gurobi1200/linux64` or `/Library/gurobi1201/macos_universal2`                                            |
| `GUROBI_LIB`  | Root path to Gurobi lib                       | `/opt/gurobi1200/linux64/lib/libgurobi120.so` or `/Library/gurobi1201/macos_universal2/lib/libgurobi120.dylib` |
---

## 🛠️ Build Instructions

### Get the Project

```bash
git clone --recurse-submodules -b masters_thesis https://github.com/sch0rschi/rflcs 
cd rflcs
```

### Build the Project

#### A standard build produces 11 executables for maximum alphabet sizes 4, 8, 16, ... 4096 
```bash
cmake -B build -G Ninja
ninja -C build
cp build/rflcs_* .
```

#### The maximum alphabet size can be overridden with 
```bash
cmake -B build -G Ninja -DCHARACTER_SET_SIZE=1024
ninja -C build
cp build/rflcs_* .
```

## 🚀 Run instructions

```console
./rflcs -i path/to/instance
```

e.g. one instance with an alphabet size of 1024 ⚡

```bash
./rflcs_1024 -i ./RFLCS_instances/type0/4096_n-div-4.0
```

---

## 📦 Dependencies

This project includes third-party libraries under the following licenses:

- [Boost](https://www.boost.org/LICENSE_1_0.txt) — Boost Software License 1.0
- [Abseil](https://github.com/abseil/abseil-cpp/blob/master/LICENSE) — Apache License 2.0

These are compatible with the GPLv3 license under which this project is released.