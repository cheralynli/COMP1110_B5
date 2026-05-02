import csv
import statistics
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


BASE_DIR = Path(__file__).resolve().parent
OUTPUT_DIR = BASE_DIR / "output"


def load_seating_records(log_path: Path) -> List[Dict[str, Any]]:
    records: List[Dict[str, Any]] = []
    with log_path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            typed_row: Dict[str, Any] = {}
            for key, value in row.items():
                typed_row[key] = int(value) if value not in (None, "") else None
            records.append(typed_row)
    return records


def avg_wait_time(seating_records: List[Dict[str, Any]]) -> float:
    waits = [float(record["wait_time"]) for record in seating_records if record.get("wait_time") is not None]
    if not waits:
        return 0.0
    return sum(waits) / len(waits)


def max_wait_time(seating_records: List[Dict[str, Any]]) -> float:
    waits = [float(record["wait_time"]) for record in seating_records if record.get("wait_time") is not None]
    if not waits:
        return 0.0
    return max(waits)


def median_wait_time(seating_records: List[Dict[str, Any]]) -> float:
    waits = sorted(float(record["wait_time"]) for record in seating_records if record.get("wait_time") is not None)
    if not waits:
        return 0.0
    return float(statistics.median(waits))


def groups_served(seating_records: List[Dict[str, Any]]) -> int:
    return len(seating_records)


def simulation_end_time(seating_records: List[Dict[str, Any]]) -> int:
    departures = [
        int(record["departure_time"])
        for record in seating_records
        if record.get("departure_time") is not None
    ]
    if not departures:
        return 0
    return max(departures)


def table_utilization(
    seating_records: List[Dict[str, Any]],
    total_seats: int,
    end_time: Optional[int] = None,
) -> float:
    if total_seats <= 0:
        return 0.0

    if end_time is None:
        end_time = simulation_end_time(seating_records)

    if end_time <= 0:
        return 0.0

    used_seat_minutes = 0
    for record in seating_records:
        if (
            record.get("group_size") is not None
            and record.get("dining_duration") is not None
        ):
            used_seat_minutes += int(record["group_size"]) * int(record["dining_duration"])

    return (used_seat_minutes / float(total_seats * end_time)) * 100.0


def assigned_table_utilization(
    seating_records: List[Dict[str, Any]],
    total_seats: int,
    end_time: Optional[int] = None,
) -> float:
    if total_seats <= 0:
        return 0.0

    if end_time is None:
        end_time = simulation_end_time(seating_records)

    if end_time <= 0:
        return 0.0

    used_table_minutes = 0
    for record in seating_records:
        if (
            record.get("table_capacity") is not None
            and record.get("dining_duration") is not None
        ):
            used_table_minutes += int(record["table_capacity"]) * int(record["dining_duration"])

    return (used_table_minutes / float(total_seats * end_time)) * 100.0


def service_level_within(
    seating_records: List[Dict[str, Any]],
    threshold_minutes: int = 15,
) -> float:
    served = groups_served(seating_records)
    if served == 0:
        return 0.0

    within_threshold = 0
    for record in seating_records:
        wait_time = record.get("wait_time")
        if wait_time is not None and int(wait_time) <= threshold_minutes:
            within_threshold += 1

    return (within_threshold / float(served)) * 100.0


def average_waiting_time_by_group_size(seating_records: List[Dict[str, Any]]) -> Dict[int, float]:
    totals: Dict[int, float] = {}
    counts: Dict[int, int] = {}

    for record in seating_records:
        if record.get("group_size") is None or record.get("wait_time") is None:
            continue

        group_size = int(record["group_size"])
        totals[group_size] = totals.get(group_size, 0.0) + float(record["wait_time"])
        counts[group_size] = counts.get(group_size, 0) + 1

    return {
        group_size: totals[group_size] / counts[group_size]
        for group_size in totals
        if counts[group_size] > 0
    }


def fairness_gap_in_average_waiting_time(seating_records: List[Dict[str, Any]]) -> float:
    avg_by_group_size = average_waiting_time_by_group_size(seating_records)
    if not avg_by_group_size:
        return 0.0
    return max(avg_by_group_size.values()) - min(avg_by_group_size.values())


def _parse_arrival_signature(arrivals_path: Path) -> Tuple[Tuple[int, int, int], ...]:
    arrivals: List[Tuple[int, int, int]] = []
    with arrivals_path.open("r", encoding="utf-8") as handle:
        for line in handle:
            text = line.strip()
            if not text or text.startswith("#"):
                continue

            parts = [part.strip() for part in text.split(",")]
            if len(parts) < 4 or parts[0] != "ARRIVAL":
                continue

            arrivals.append((int(parts[1]), int(parts[2]), int(parts[3])))

    arrivals.sort()
    return tuple(arrivals)


def _arrival_signature_from_records(seating_records: List[Dict[str, Any]]) -> Tuple[Tuple[int, int, int], ...]:
    arrivals: List[Tuple[int, int, int]] = []
    for record in seating_records:
        if (
            record.get("arrival_time") is None
            or record.get("group_size") is None
            or record.get("dining_duration") is None
        ):
            continue

        arrivals.append(
            (
                int(record["arrival_time"]),
                int(record["group_size"]),
                int(record["dining_duration"]),
            )
        )

    arrivals.sort()
    return tuple(arrivals)


def _read_config_tables(config_path: Path) -> Dict[int, int]:
    tables: Dict[int, int] = {}
    with config_path.open("r", encoding="utf-8") as handle:
        for line in handle:
            text = line.strip()
            if not text or text.startswith("#"):
                continue

            parts = [part.strip() for part in text.split(",")]
            if len(parts) < 3 or parts[0] != "TABLE":
                continue

            tables[int(parts[2])] = int(parts[1])

    return tables


def _total_seats_from_config(config_path: Path) -> int:
    return sum(_read_config_tables(config_path).values())


def _discover_scenarios() -> List[Dict[str, Any]]:
    scenarios: List[Dict[str, Any]] = []
    for config_path in sorted(BASE_DIR.rglob("config.txt")):
        arrivals_path = config_path.with_name("arrivals.txt")
        if not arrivals_path.exists():
            continue

        scenarios.append(
            {
                "config_path": config_path,
                "arrivals_path": arrivals_path,
                "tables": _read_config_tables(config_path),
                "total_seats": _total_seats_from_config(config_path),
                "arrival_signature": _parse_arrival_signature(arrivals_path),
            }
        )

    return scenarios


def _used_tables_from_records(seating_records: List[Dict[str, Any]]) -> Dict[int, int]:
    tables: Dict[int, int] = {}
    for record in seating_records:
        if record.get("table_id") is None or record.get("table_capacity") is None:
            continue

        table_id = int(record["table_id"])
        table_capacity = int(record["table_capacity"])
        tables[table_id] = table_capacity

    return tables


def _config_matches_used_tables(config_tables: Dict[int, int], used_tables: Dict[int, int]) -> bool:
    for table_id, capacity in used_tables.items():
        if config_tables.get(table_id) != capacity:
            return False
    return True


def _score_table_match(config_tables: Dict[int, int], used_tables: Dict[int, int]) -> int:
    score = 0
    for table_id, capacity in used_tables.items():
        if config_tables.get(table_id) == capacity:
            score += 1
    return score


def _infer_total_seats(
    scenario_name: str,
    seating_records: List[Dict[str, Any]],
    scenario_files: List[Dict[str, Any]],
) -> int:
    direct_paths = [
        BASE_DIR / scenario_name / "config.txt",
        BASE_DIR / "input" / "default" / scenario_name / "config.txt",
    ]
    for config_path in direct_paths:
        if config_path.exists():
            return _total_seats_from_config(config_path)

    arrival_signature = _arrival_signature_from_records(seating_records)
    used_tables = _used_tables_from_records(seating_records)

    exact_matches = [
        scenario
        for scenario in scenario_files
        if scenario["arrival_signature"] == arrival_signature
    ]
    if len(exact_matches) == 1:
        return int(exact_matches[0]["total_seats"])

    if exact_matches:
        exact_matches = [
            scenario
            for scenario in exact_matches
            if _config_matches_used_tables(scenario["tables"], used_tables)
        ]
        if len(exact_matches) == 1:
            return int(exact_matches[0]["total_seats"])
        if exact_matches:
            exact_matches.sort(
                key=lambda scenario: (
                    -_score_table_match(scenario["tables"], used_tables),
                    str(scenario["config_path"]),
                )
            )
            return int(exact_matches[0]["total_seats"])

    if used_tables:
        return sum(used_tables.values())

    return 0


def queue_length_metrics(seating_records: List[Dict[str, Any]]) -> Tuple[int, float]:
    arrivals_by_time: Dict[int, int] = {}
    seatings_by_time: Dict[int, int] = {}
    event_times = set()

    for record in seating_records:
        if record.get("arrival_time") is None or record.get("seating_time") is None:
            continue

        arrival_time = int(record["arrival_time"])
        seating_time = int(record["seating_time"])
        arrivals_by_time[arrival_time] = arrivals_by_time.get(arrival_time, 0) + 1
        seatings_by_time[seating_time] = seatings_by_time.get(seating_time, 0) + 1
        event_times.add(arrival_time)
        event_times.add(seating_time)

    if not event_times:
        return 0, 0.0

    ordered_times = sorted(event_times)
    current_queue = 0
    max_queue = 0
    queue_area = 0.0

    for index, current_time in enumerate(ordered_times):
        current_queue += arrivals_by_time.get(current_time, 0)
        if current_queue > max_queue:
            max_queue = current_queue

        current_queue -= seatings_by_time.get(current_time, 0)

        if index + 1 < len(ordered_times):
            next_time = ordered_times[index + 1]
            queue_area += current_queue * (next_time - current_time)

    end_time = simulation_end_time(seating_records)
    if end_time <= 0:
        return max_queue, 0.0

    return max_queue, queue_area / float(end_time)


def _classify_log_file(log_path: Path) -> Optional[Tuple[str, str, str]]:
    name = log_path.name

    if name.startswith("fcfs_seating_log_"):
        scenario_name = name[len("fcfs_seating_log_"):-4]
        return "fcfs", scenario_name, f"fcfs_seating_log_{scenario_name}.csv"

    if name.startswith("size_seating_log_"):
        scenario_name = name[len("size_seating_log_"):-4]
        return "size", scenario_name, f"size_seating_log_{scenario_name}.csv"

    if not name.startswith("seating_log_"):
        return None

    scenario_name = name[len("seating_log_"):-4]
    if (
        scenario_name.endswith("_custom")
        or scenario_name.endswith("_fcfs")
        or scenario_name.endswith("_size_queue")
    ):
        return None

    return "custom", scenario_name, f"seating_log_{scenario_name}.csv"


def _build_summary_row(
    log_path: Path,
    scenario_name: str,
    source_file: str,
    scenario_files: List[Dict[str, Any]],
) -> Dict[str, Any]:
    records = load_seating_records(log_path)
    total_seats = _infer_total_seats(scenario_name, records, scenario_files)
    end_time = simulation_end_time(records)
    max_queue_length, avg_queue_length = queue_length_metrics(records)

    return {
        "source_file": source_file,
        "scenario_name": scenario_name,
        "actual_log_file": log_path.name,
        "avg_wait": round(avg_wait_time(records), 2),
        "max_wait": round(max_wait_time(records), 2),
        "median_wait": round(median_wait_time(records), 2),
        "groups_served": groups_served(records),
        "seat_util": round(assigned_table_utilization(records, total_seats, end_time), 2),
        "true_seat_util": round(table_utilization(records, total_seats, end_time), 2),
        "service_level_15": round(service_level_within(records, 15), 2),
        "max_queue_length": max_queue_length,
        "avg_queue_length": round(avg_queue_length, 2),
        "fairness_gap": round(fairness_gap_in_average_waiting_time(records), 2),
        "total_seats": total_seats,
        "end_time": end_time,
    }


def _write_summary(summary_path: Path, rows: List[Dict[str, Any]]) -> None:
    fieldnames = [
        "source_file",
        "scenario_name",
        "actual_log_file",
        "avg_wait",
        "max_wait",
        "median_wait",
        "groups_served",
        "seat_util",
        "true_seat_util",
        "service_level_15",
        "max_queue_length",
        "avg_queue_length",
        "fairness_gap",
        "total_seats",
        "end_time",
    ]

    with summary_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    OUTPUT_DIR.mkdir(exist_ok=True)
    scenario_files = _discover_scenarios()

    summary_rows: Dict[str, List[Dict[str, Any]]] = {
        "custom": [],
        "fcfs": [],
        "size": [],
    }

    for log_path in sorted(OUTPUT_DIR.glob("*seating_log_*.csv")):
        info = _classify_log_file(log_path)
        if info is None:
            continue

        kind, scenario_name, source_file = info
        summary_rows[kind].append(
            _build_summary_row(log_path, scenario_name, source_file, scenario_files)
        )

    summary_rows["custom"].sort(key=lambda row: str(row["source_file"]))
    summary_rows["fcfs"].sort(key=lambda row: str(row["source_file"]))
    summary_rows["size"].sort(key=lambda row: str(row["source_file"]))

    _write_summary(OUTPUT_DIR / "metrics_summary.csv", summary_rows["custom"])
    _write_summary(OUTPUT_DIR / "fcfs_metrics_summary.csv", summary_rows["fcfs"])
    _write_summary(OUTPUT_DIR / "size_metrics_summary.csv", summary_rows["size"])


if __name__ == "__main__":
    main()
