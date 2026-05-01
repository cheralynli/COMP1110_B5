# Restaurant Queue Simulation Case Study

This repository contains a student-level restaurant queue simulation project for COMP1110. The project keeps the original C++ custom seating heuristic for manual runs, and adds a Python batch runner so the case-study scenarios can be compared cleanly across multiple queueing strategies.

## What The Algorithms Do

- `custom`: the original WokThisWay heuristic. It scores waiting groups using group size, dining duration, waiting time, and a fairness weight, then compares that score against an opportunity-cost estimate before seating a group.
- `fcfs`: a first-come, first-served baseline. The earliest arriving group that fits a free table is seated first.
- `size_queue`: a size-based queue baseline used by the case-study runner. Groups are assigned to size bands from the `QUEUE,min_size,max_size` lines, and larger tables prefer the closest larger queue first so they are less likely to be consumed by very small groups.

## Important Note About `QUEUE` Lines

The project now reads both `TABLE` lines and `QUEUE` lines:

- the interactive C++ menu supports `custom`, `fcfs`, and `size_queue`
- the `size_queue` mode uses the `QUEUE,min_size,max_size` bands
- the Python batch runner uses the same queue-band idea when it evaluates `size_queue`

## Build And Run

Compile the C++ simulator with:

```bash
make
```

Run the interactive custom simulator with:

```bash
make run
```

The interactive C++ menu now lets you:

- choose `Our Custom Algorithm`, `FCFS`, or `Size-Based Queue`
- browse the 5 main restaurant case studies: `Fast Food`, `Cafe`, `Family Dining`, `Sushi Belt`, and `KBBQ / Hotpot`
- run variation `A`, variation `B`, or a side-by-side `A vs B` comparison for a case study

Clean compiled files with:

```bash
make clean
```

## Manual Scenario Run

The menu keeps the text-based interface. For a manual run you can:

1. Run `make run`
2. Choose an algorithm
3. Choose `Custom file paths` from the menu, or select one of the built-in case-study categories
4. Enter a paired case-study config and arrivals file such as:

```text
pair_6_A/config.txt
pair_6_A/arrivals.txt
```

The interactive run writes a seating log such as `seating_log_custom_run.csv` or `seating_log_<scenario>.csv`.

## Case-Study Design

The assignment requires paired scenarios where variation A and variation B keep the **same arrivals** and change exactly **one restaurant setup factor**. This repository now highlights 5 main restaurant case studies:

- `Family Dining`: balanced vs large-table-heavy layout
- `Cafe`: two-seat-heavy vs mixed cafe layout
- `Fast Food`: limited vs expanded rush capacity
- `Sushi Belt`: solo-seat priority vs more shared seating
- `KBBQ / Hotpot`: medium-group focus vs large-group focus

The older `pair_1` and `pair_2` fast-food scenarios still exist on disk, but the main scenario registry and UI are now organized around the 5 restaurant case studies above.

Each pair folder contains:

- `config.txt`
- `arrivals.txt`

Within each pair, `A` and `B` use identical arrival patterns.

## Active Scenario Registry

Inspect the active case-study set with:

```bash
python3 scenarios.py
```

This prints the restaurant, case-study title, factor changed, and the exact config and arrivals paths.

## Run All Case Studies

Use the batch runner to execute every active paired scenario for all available algorithms:

```bash
python3 run_case_studies.py
```

This produces:

- `results.csv`: one row per scenario and algorithm
- `outputs/seating_logs/`: seating logs for every scenario and algorithm
- `outputs/case_study_tables.md`: neat markdown tables generated from the current results
- `CASE_STUDY_TABLE_TEMPLATE.md`: a report-friendly template for turning results into side-by-side analysis tables

## `results.csv` Columns

The batch runner writes these columns:

```text
restaurant,
case_study_title,
pair_id,
variation,
factor_changed,
variation_summary,
algorithm,
scenario_name,
avg_wait,
max_wait,
groups_served,
table_utilization,
service_level_15,
max_queue_length_if_available,
config_path,
arrivals_path
```

Notes:

- `table_utilization` is reported as a percentage of total seat-minutes used over total seat-minutes available until the last departure.
- `service_level_15` is the percentage of seated groups that waited 15 minutes or less.
- `max_queue_length_if_available` is tracked by the Python case-study runner.

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

Table ids should be numeric because the current C++ parser expects numeric ids.

## Python Files

- `scenarios.py`: lists the active paired scenarios used for the case study
- `run_case_studies.py`: runs all active scenarios across `custom`, `fcfs`, and `size_queue`
- `metrics.py`: reusable metric helpers for seating logs
- `CASE_STUDY_TABLE_TEMPLATE.md`: suggested table layout and analysis prompts for the written case-study section

## Current Limitations

- The interactive C++ simulator does not use `QUEUE` lines.
- The batch runner compares three algorithms, but only the `custom` algorithm exists in the original C++ source.
- The older `fastfood_peak`, `fastfood_non_peak`, `cafe_peak`, `cafe_non_peak`, `family_peak`, and `family_non_peak` folders are legacy examples and are not used as the main case-study set because their peak/non-peak variants change the arrivals pattern.
