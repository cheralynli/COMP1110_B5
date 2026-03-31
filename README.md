# Restaurant Simulation Project

This repository contains template files for a C++ restaurant seating simulation and Python analysis scripts.

## C++ Components

- `main.cpp`: main entry point, menu flow, and result display.
- `simulation.h`: declarations for simulation data structures and class interfaces.
- `simulation.cpp`: implementation of queueing, seating, timing, and metrics logic.

## How To Compile And Run

Compilation will eventually follow a command similar to:

```bash
g++ -std=c++17 main.cpp simulation.cpp -o restaurant_sim
```

Execution will eventually follow a command similar to:

```bash
./restaurant_sim
```

## Input File Formats

Restaurant configuration format:

```text
TABLE,capacity,id
QUEUE,min_size,max_size
```

Customer arrivals format:

```text
ARRIVAL,time,group_size,dining_duration
```

## Python Analysis

- `analyze.py`: will load CSV outputs and compute summary statistics.
- `visualize.py`: will build charts comparing restaurant scenarios.
- `scenarios.py`: will manage batch runs and consolidated exports.

Python execution will eventually follow commands similar to:

```bash
python3 scenarios.py
python3 analyze.py
python3 visualize.py
```

## Scenario Descriptions

- `scenario_1_fastfood`: mostly 2-seat tables and short dining times for small groups.
- `scenario_2_cafe`: a mix of 2-seat and 4-seat tables with longer visits.
- `scenario_3_family_diner`: includes 2, 4, and 6-seat tables for family groups.
- `scenario_4_sushi_belt`: emphasizes bar seating and small fast-turnover parties.
- `scenario_5_dimsum_kbbq`: large tables and long dining times for big groups.

## Output File

`results.csv` is intended to store summary metrics with the columns:

```text
scenario,avg_wait,max_wait,table_util,service_level_15,max_queue_length
```

