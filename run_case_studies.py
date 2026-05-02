import csv
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from metrics import (
    avg_wait_time,
    groups_served,
    load_seating_records,
    max_wait_time,
    service_level_within,
    table_utilization,
)
from scenarios import Scenario, list_scenarios


BASE_DIR = Path(__file__).resolve().parent
OUTPUT_DIR = BASE_DIR / "outputs"
LOG_DIR = OUTPUT_DIR / "seating_logs"
RESULTS_PATH = BASE_DIR / "results.csv"
MARKDOWN_SUMMARY_PATH = OUTPUT_DIR / "case_study_tables.md"

FAIRNESS_WEIGHT = 1.0
LOOK_AHEAD_WINDOW = 15
DEFAULT_MAX_WAIT = 30
ALGORITHMS = ("custom", "fcfs", "size_queue")
MAX_GROUP_SIZE = 10**9
SIZE_QUEUE_RULES = (
    (1, 2),
    (3, 4),
    (5, MAX_GROUP_SIZE),
)


@dataclass
class Table:
    id: int
    capacity: int
    available_at: int = 0
    is_free: bool = True


@dataclass(frozen=True)
class QueueRule:
    min_size: int
    max_size: int


@dataclass(frozen=True)
class Group:
    id: int
    arrival_time: int
    size: int
    dining_duration: int
    max_wait_tolerance: int = DEFAULT_MAX_WAIT


@dataclass
class SimulationOutcome:
    max_queue_length: int
    total_seats: int
    seating_log_path: Path


def parse_config(config_path: Path) -> Tuple[List[Table], List[QueueRule]]:
    tables: List[Table] = []

    with config_path.open(encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            fields = [field.strip() for field in line.split(",")]
            if fields[0] == "TABLE":
                tables.append(Table(id=int(fields[2]), capacity=int(fields[1])))

    queue_rules = [
        QueueRule(min_size=min_size, max_size=max_size)
        for min_size, max_size in SIZE_QUEUE_RULES
    ]
    return tables, queue_rules


def parse_arrivals(arrivals_path: Path) -> List[Group]:
    arrivals: List[Group] = []
    next_group_id = 1

    with arrivals_path.open(encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            fields = [field.strip() for field in line.split(",")]
            if fields[0] != "ARRIVAL":
                continue

            arrivals.append(
                Group(
                    id=next_group_id,
                    arrival_time=int(fields[1]),
                    size=int(fields[2]),
                    dining_duration=int(fields[3]),
                )
            )
            next_group_id += 1

    arrivals.sort(key=lambda item: (item.arrival_time, item.id))
    return arrivals


def precompute_hourly_rates(arrivals: List[Group]) -> Dict[int, Dict[int, float]]:
    hourly_counts: Dict[int, Dict[int, int]] = {}
    for group in arrivals:
        hour = group.arrival_time // 60
        hourly_counts.setdefault(hour, {})
        hourly_counts[hour][group.size] = hourly_counts[hour].get(group.size, 0) + 1

    hourly_rates: Dict[int, Dict[int, float]] = {}
    for hour, counts in hourly_counts.items():
        hourly_rates[hour] = {}
        for group_size, count in counts.items():
            hourly_rates[hour][group_size] = count / 60.0

    return hourly_rates


def calculate_opportunity_cost(
    table_capacity: int,
    current_time: int,
    hourly_rates: Dict[int, Dict[int, float]],
) -> float:
    if not hourly_rates:
        return 0.0

    current_hour = current_time // 60
    if current_hour not in hourly_rates:
        current_hour = max(hourly_rates)

    expected_value = 0.0
    current_rates = hourly_rates[current_hour]
    for size in range(1, table_capacity + 1):
        if size not in current_rates:
            continue

        lam = current_rates[size]
        probability = 1.0 - math.exp(-lam * LOOK_AHEAD_WINDOW)
        expected_value += probability * (size * 45.0)

    return expected_value


def has_table_that_fits(tables: List[Table], group_size: int) -> bool:
    return any(table.capacity >= group_size for table in tables)


def next_departure_time(tables: List[Table]) -> Optional[int]:
    departures = [table.available_at for table in tables if not table.is_free]
    if not departures:
        return None
    return min(departures)


def next_custom_deadline(waiting: List[Group], tables: List[Table], current_time: int) -> Optional[int]:
    deadlines: List[int] = []
    for group in waiting:
        can_use_free_table = any(table.is_free and table.capacity >= group.size for table in tables)
        if not can_use_free_table:
            continue

        deadline = group.arrival_time + group.max_wait_tolerance
        if deadline > current_time:
            deadlines.append(deadline)

    if not deadlines:
        return None
    return min(deadlines)


def release_tables(tables: List[Table], current_time: int) -> None:
    for table in tables:
        if not table.is_free and table.available_at <= current_time:
            table.is_free = True


def append_record(records: List[Dict[str, int]], group: Group, table: Table, seating_time: int) -> None:
    records.append(
        {
            "group_id": group.id,
            "group_size": group.size,
            "arrival_time": group.arrival_time,
            "seating_time": seating_time,
            "wait_time": seating_time - group.arrival_time,
            "table_id": table.id,
            "table_capacity": table.capacity,
            "dining_duration": group.dining_duration,
            "departure_time": seating_time + group.dining_duration,
        }
    )


def select_fcfs_index(waiting: List[Group], table: Table) -> Optional[int]:
    best_index: Optional[int] = None
    for index, group in enumerate(waiting):
        if group.size > table.capacity:
            continue

        if best_index is None:
            best_index = index
            continue

        candidate = waiting[best_index]
        if (group.arrival_time, group.id) < (candidate.arrival_time, candidate.id):
            best_index = index

    return best_index


def select_custom_index(
    waiting: List[Group],
    table: Table,
    current_time: int,
    hourly_rates: Dict[int, Dict[int, float]],
) -> Optional[int]:
    best_index: Optional[int] = None
    best_utility = -1.0

    for index, group in enumerate(waiting):
        if group.size > table.capacity:
            continue

        current_wait = current_time - group.arrival_time
        utility = (group.size * group.dining_duration) + (current_wait * FAIRNESS_WEIGHT)
        if utility > best_utility:
            best_utility = utility
            best_index = index

    if best_index is None:
        return None

    best_group = waiting[best_index]
    current_wait = current_time - best_group.arrival_time
    expected_wait_value = calculate_opportunity_cost(table.capacity, current_time, hourly_rates)

    if best_utility <= expected_wait_value and current_wait < best_group.max_wait_tolerance:
        return None

    return best_index


def select_size_queue_index(waiting: List[Group], table: Table, queue_rules: List[QueueRule]) -> Optional[int]:
    candidate_rules = [
        rule for rule in queue_rules
        if rule.min_size <= table.capacity
    ]
    candidate_rules.sort(key=lambda rule: (rule.max_size, rule.min_size), reverse=True)

    for rule in candidate_rules:
        best_index: Optional[int] = None
        for index, group in enumerate(waiting):
            if not (rule.min_size <= group.size <= rule.max_size):
                continue
            if group.size > table.capacity:
                continue

            if best_index is None:
                best_index = index
                continue

            candidate = waiting[best_index]
            if (group.arrival_time, group.id) < (candidate.arrival_time, candidate.id):
                best_index = index

        if best_index is not None:
            return best_index

    return None


def seat_waiting_groups(
    algorithm: str,
    current_time: int,
    tables: List[Table],
    waiting: List[Group],
    queue_rules: List[QueueRule],
    hourly_rates: Dict[int, Dict[int, float]],
    records: List[Dict[str, int]],
) -> None:
    seated_someone = True
    while seated_someone:
        seated_someone = False

        for table in tables:
            if not table.is_free:
                continue

            if algorithm == "custom":
                selected_index = select_custom_index(waiting, table, current_time, hourly_rates)
            elif algorithm == "fcfs":
                selected_index = select_fcfs_index(waiting, table)
            else:
                selected_index = select_size_queue_index(waiting, table, queue_rules)

            if selected_index is None:
                continue

            group = waiting.pop(selected_index)
            table.is_free = False
            table.available_at = current_time + group.dining_duration
            append_record(records, group, table, current_time)
            seated_someone = True
            break


def write_seating_log(log_path: Path, records: List[Dict[str, int]]) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w", newline="", encoding="utf-8") as handle:
        fieldnames = [
            "group_id",
            "group_size",
            "arrival_time",
            "seating_time",
            "wait_time",
            "table_id",
            "table_capacity",
            "dining_duration",
            "departure_time",
        ]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(records)


def simulate(
    scenario: Scenario,
    algorithm: str,
) -> SimulationOutcome:
    tables, queue_rules = parse_config(scenario.config_path)
    arrivals = parse_arrivals(scenario.arrivals_path)
    waiting: List[Group] = []
    records: List[Dict[str, int]] = []
    hourly_rates = precompute_hourly_rates(arrivals)
    total_seats = sum(table.capacity for table in tables)

    next_arrival_index = 0
    current_time = 0
    max_queue_length = 0

    while (
        next_arrival_index < len(arrivals)
        or waiting
        or any(not table.is_free for table in tables)
    ):
        next_times: List[int] = []
        if next_arrival_index < len(arrivals):
            next_times.append(arrivals[next_arrival_index].arrival_time)

        departure_time = next_departure_time(tables)
        if departure_time is not None:
            next_times.append(departure_time)

        if algorithm == "custom":
            deadline = next_custom_deadline(waiting, tables, current_time)
            if deadline is not None:
                next_times.append(deadline)

        if not next_times:
            break

        current_time = min(next_times)
        release_tables(tables, current_time)

        while (
            next_arrival_index < len(arrivals)
            and arrivals[next_arrival_index].arrival_time <= current_time
        ):
            waiting.append(arrivals[next_arrival_index])
            next_arrival_index += 1

        max_queue_length = max(max_queue_length, len(waiting))
        seat_waiting_groups(
            algorithm=algorithm,
            current_time=current_time,
            tables=tables,
            waiting=waiting,
            queue_rules=queue_rules,
            hourly_rates=hourly_rates,
            records=records,
        )

        if not next_times and waiting:
            break

        impossible_waiting = [
            group for group in waiting
            if not has_table_that_fits(tables, group.size)
        ]
        if impossible_waiting and next_arrival_index >= len(arrivals) and not any(not table.is_free for table in tables):
            break

    log_path = LOG_DIR / f"{scenario.name}_{algorithm}.csv"
    write_seating_log(log_path, records)
    return SimulationOutcome(
        max_queue_length=max_queue_length,
        total_seats=total_seats,
        seating_log_path=log_path,
    )


def build_results_row(
    scenario: Scenario,
    algorithm: str,
    outcome: SimulationOutcome,
) -> Dict[str, object]:
    records = load_seating_records(outcome.seating_log_path)
    return {
        "restaurant": scenario.restaurant,
        "case_study_title": scenario.case_study_title,
        "pair_id": scenario.pair_id,
        "variation": scenario.variation,
        "factor_changed": scenario.factor_changed,
        "variation_summary": scenario.variation_summary,
        "algorithm": algorithm,
        "scenario_name": scenario.name,
        "avg_wait": round(avg_wait_time(records), 2),
        "max_wait": round(max_wait_time(records), 2),
        "groups_served": groups_served(records),
        "table_utilization": round(table_utilization(records, outcome.total_seats), 2),
        "service_level_15": round(service_level_within(records, 15), 2),
        "max_queue_length_if_available": outcome.max_queue_length,
        "config_path": str(scenario.config_path),
        "arrivals_path": str(scenario.arrivals_path),
    }


def write_results(rows: List[Dict[str, object]]) -> None:
    with RESULTS_PATH.open("w", newline="", encoding="utf-8") as handle:
        fieldnames = [
            "restaurant",
            "case_study_title",
            "pair_id",
            "variation",
            "factor_changed",
            "variation_summary",
            "algorithm",
            "scenario_name",
            "avg_wait",
            "max_wait",
            "groups_served",
            "table_utilization",
            "service_level_15",
            "max_queue_length_if_available",
            "config_path",
            "arrivals_path",
        ]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def write_markdown_summary(rows: List[Dict[str, object]]) -> None:
    lines: List[str] = [
        "# Case Study Tables",
        "",
        "These tables are generated from `results.csv` and are meant to be report-friendly summaries.",
        "",
    ]

    custom_rows = [row for row in rows if row["algorithm"] == "custom"]
    grouped: Dict[str, List[Dict[str, object]]] = {}
    for row in custom_rows:
        grouped.setdefault(str(row["pair_id"]), []).append(row)

    ordered_pairs = list_scenarios()
    seen_pairs: List[str] = []
    for scenario in ordered_pairs:
        if scenario.pair_id not in seen_pairs:
            seen_pairs.append(scenario.pair_id)

    for pair_id in seen_pairs:
        pair_rows = sorted(grouped.get(pair_id, []), key=lambda item: str(item["variation"]))
        if len(pair_rows) != 2:
            continue

        first = pair_rows[0]
        lines.extend(
            [
                f"## {first['restaurant']}: {first['case_study_title']}",
                "",
                f"- Case ID: `{first['pair_id']}`",
                f"- Factor changed: {first['factor_changed']}",
                f"- Same in A and B: arrivals, group sizes, and dining-duration pattern",
                "",
                "| Variation | Setup summary | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length |",
                "|---|---|---:|---:|---:|---:|---:|---:|",
            ]
        )

        for row in pair_rows:
            lines.append(
                f"| {row['variation']} | {row['variation_summary']} | "
                f"{row['avg_wait']} | {row['max_wait']} | {row['groups_served']} | "
                f"{row['table_utilization']}% | {row['service_level_15']}% | "
                f"{row['max_queue_length_if_available']} |"
            )

        lines.extend(
            [
                "",
                "Suggested interpretation:",
                f"- Variation A: {pair_rows[0]['variation_summary']}",
                f"- Variation B: {pair_rows[1]['variation_summary']}",
                "- Compare which side lowers wait and queue length without causing poor utilization.",
                "- Because the arrivals are the same, differences come from the restaurant setup rather than demand.",
                "",
            ]
        )

    lines.extend(
        [
            "## Overall Summary",
            "",
            "| Restaurant | Case study | Better variation | Main trade-off to discuss |",
            "|---|---|---|---|",
        ]
    )

    for pair_id in seen_pairs:
        pair_rows = sorted(grouped.get(pair_id, []), key=lambda item: str(item["variation"]))
        if len(pair_rows) != 2:
            continue

        better = "Tie"
        if float(pair_rows[0]["avg_wait"]) != float(pair_rows[1]["avg_wait"]):
            better = "A" if float(pair_rows[0]["avg_wait"]) < float(pair_rows[1]["avg_wait"]) else "B"

        lines.append(
            f"| {pair_rows[0]['restaurant']} | {pair_rows[0]['case_study_title']} | {better} | "
            "Explain the wait/utilization trade-off seen in the table above. |"
        )

    MARKDOWN_SUMMARY_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    OUTPUT_DIR.mkdir(exist_ok=True)
    LOG_DIR.mkdir(parents=True, exist_ok=True)

    rows: List[Dict[str, object]] = []
    for scenario in list_scenarios():
        for algorithm in ALGORITHMS:
            outcome = simulate(scenario, algorithm)
            rows.append(build_results_row(scenario, algorithm, outcome))

    write_results(rows)
    write_markdown_summary(rows)
    print(f"Wrote {RESULTS_PATH}")
    print(f"Saved seating logs in {LOG_DIR}")
    print(f"Wrote {MARKDOWN_SUMMARY_PATH}")


if __name__ == "__main__":
    main()
