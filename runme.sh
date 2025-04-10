#!/bin/bash

### Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/rflcs ./rflcs

### Run experiments
# Run all instances (comment out for minimal version)
find ./RFLCS_instances/**/* -type f -exec ./rflcs -i {} \;

# Run only solvable instance categories (comment next 3 lines in for minimal version)
# find ./RFLCS_instances/type0/{32,64,128,256,512}* -type f -exec ./rflcs -i {} \;
# find ./RFLCS_instances/type0/{1024,2048,4096}* -type f ! -name '*_n-div-8.*' -exec ./rflcs -i {} \;
# find ./RFLCS_instances/type1/* -type f -exec ./rflcs -i {} \;

### Create result artefacts
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install pandas
cd results || exit
python3 latex_table.py
python3 box_plots.py
deactivate
pdflatex -jobname=result_tables main.tex
cd ..
