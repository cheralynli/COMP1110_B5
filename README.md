# Restaurant Simulation Project

This repository contains template files for a C++ restaurant seating simulation and Python analysis scripts.

## C++ Components

- `main.cpp`: main entry point, menu flow, and result display.
- `simulation.h`: declarations for simulation data structures and class interfaces.
- `simulation.cpp`: implementation of queueing, seating, timing, and metrics logic.

## How To Compile And Run

Build the simulator with:

```bash
make
```

Run the interactive simulator with:

```bash
make run
```

Clean the compiled files with:

```bash
make clean
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

- `metrics.py`: computes wait-time, utilization, and fairness-style summary metrics from seating records.
- `visualize.py`: placeholder for future charting and scenario comparisons.
- `scenarios.py`: defines the active scenario set and points to the corresponding config and arrival files.

You can inspect the active scenario set with:

```bash
python3 scenarios.py
```

## Scenario Descriptions

The current project is organized around 3 restaurant types, each with peak and non-peak demand cases:

- `fastfood_non_peak`: lighter fast food traffic with quick turnover and mostly very small groups.
- `fastfood_peak`: busy fast food rush with dense arrivals and short dining times.
- `cafe_non_peak`: lighter cafe traffic, mostly solo diners and pairs.
- `cafe_peak`: cafe rush with many tightly packed small-party arrivals.
- `family_non_peak`: moderate family dining traffic with larger groups and longer stays.
- `family_peak`: family dining rush with many medium and large groups.

Each scenario folder contains:

- `config.txt`: table capacities and queue size ranges.
- `arrivals.txt`: arrival times, group sizes, and dining durations.

Note: the current C++ parser expects numeric table IDs, so these scenario files use numeric IDs intentionally.

## Output File

`results.csv` is intended to store summary metrics with the columns:

```text
scenario,avg_wait,max_wait,table_util,service_level_15,max_queue_length
```
