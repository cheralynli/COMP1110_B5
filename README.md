# Restaurant Queue Simulation Case Study

This repo is a COMP1110 restaurant queue simulation project. It compares three seating algorithms across paired restaurant case studies, then generates metric summaries and charts from the seating logs.

## What This Project Does

The simulator models groups arriving at a restaurant, waiting for a suitable table, dining for a fixed duration, and leaving. Each run writes a seating log to `output/`, and the Python analysis scripts turn those logs into CSV summaries and chart images.

Algorithms:

- `custom` / WokThisWaySim: scores possible table-group pairings using waiting time, group size, dining duration, table fit, and future-demand regret.
- `fcfs`: first-come, first-served. Seats the earliest waiting group that fits an available table.
- `size_queue`: splits waiting groups into fixed queues: `1-2`, `3-4`, and `5+` people. Larger tables prefer larger-group queues first.

## Repo Layout

- `main.cpp`: main interactive UI for choosing algorithms, scenarios, comparisons, and metric/chart generation.
- `simulation.cpp`: WokThisWaySim custom algorithm.
- `fcfs_simulation.cpp`: FCFS baseline.
- `size_queue_simulation.cpp`: size-based queue baseline.
- `restaurant_parser.cpp`: reads table configs and arrivals.
- `input/default/`: active A/B case-study inputs.
- `metrics.py`: generates `output/*metrics_summary.csv`.
- `visualize.py`: generates chart images from metric summaries.
- `run_case_studies.py`: optional Python-only batch runner.
- `requirements.txt`: Python dependencies for charts.

## Prerequisites

You need:

- Python 3
- A C++17 compiler, usually `g++`
- Optional: `make`

On Windows, the easiest C++ options are:

- WSL with Ubuntu and `g++`
- MSYS2/MinGW with `g++`
- Any terminal where `g++ --version` works

## Setup

Clone the repo and enter the project folder:

```bash
git clone https://github.com/cheralynli/COMP1110_B5.git
cd COMP1110_B5
```

### PowerShell

Create and activate a Python virtual environment:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

If PowerShell blocks activation, run this once in the same terminal:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\.venv\Scripts\Activate.ps1
```

Compile the main UI:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp simulation.cpp fcfs_simulation.cpp size_queue_simulation.cpp restaurant_parser.cpp -o restaurant_sim.exe
```

Run it:

```powershell
.\restaurant_sim.exe
```

### Bash, Git Bash, macOS, Linux, Or WSL

Create and activate a Python virtual environment:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

If `make` is available:

```bash
make
./restaurant_sim
```

If `make` is not available, compile directly:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp simulation.cpp fcfs_simulation.cpp size_queue_simulation.cpp restaurant_parser.cpp -o restaurant_sim
./restaurant_sim
```

## Using The Main UI

Run the compiled program from the repo root.

Start screen:

- `s`: start the case-study UI
- `g`: generate metrics and charts by running `metrics.py` and `visualize.py`
- `q`: quit

Inside the case-study UI, you can:

- choose `Our Custom Algorithm`
- choose `FCFS`
- choose `Size-Based Queue`
- choose `Run Everything` to run every A/B case study on every algorithm
- choose a restaurant case study
- run variation `A`
- run variation `B`
- compare `A vs B` side by side
- enter custom config/arrival file paths

Typical workflow:

1. Run `restaurant_sim`.
2. Press `s`.
3. Choose an algorithm, or choose `Run Everything`.
4. Run the desired case studies.
5. Return to the start screen.
6. Press `g` to regenerate metrics and charts.

## Outputs

Interactive C++ runs write logs to `output/`:

- `output/seating_log_<scenario>.csv`: WokThisWaySim custom runs
- `output/fcfs_seating_log_<scenario>.csv`: FCFS runs
- `output/size_seating_log_<scenario>.csv`: size-based queue runs

Metric summaries:

- `output/metrics_summary.csv`
- `output/fcfs_metrics_summary.csv`
- `output/size_metrics_summary.csv`

Chart folders:

- `output/ourAlgo_charts/`
- `output/fcfs_charts/`
- `output/size_charts/`
- `output/charts/`
- `output/scenario_comparison_charts/`

## Manual Python Commands

The UI can generate metrics and charts for you, but you can still run the scripts manually.

PowerShell:

```powershell
python metrics.py
python visualize.py
```

Bash or WSL:

```bash
python3 metrics.py
python3 visualize.py
```

Optional Python-only batch runner:

```bash
python3 run_case_studies.py
```

On PowerShell:

```powershell
python run_case_studies.py
```

## Input File Format

Config files:

```text
TABLE,capacity,id
```

The size-based queue algorithm does not read queue definitions from config files. It always uses fixed bands `1-2`, `3-4`, and `5+`.

Arrival files:

```text
ARRIVAL,time,group_size,dining_duration
```

Table ids must be numeric.

## Active Case Studies

The active paired scenarios live under `input/default/`:

- `family_dinner_A` vs `family_dinner_B`
- `cafe_lunch_A` vs `cafe_lunch_B`
- `fastfood_rush_A` vs `fastfood_rush_B`
- `sushi_belt_A` vs `sushi_belt_B`
- `kbbq_hotpot_A` vs `kbbq_hotpot_B`

Each A/B pair keeps the arrival pattern the same and changes one restaurant setup factor.

To inspect the scenario registry:

```bash
python3 scenarios.py
```

PowerShell:

```powershell
python scenarios.py
```
