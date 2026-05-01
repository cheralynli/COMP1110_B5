import csv
from pathlib import Path
from typing import Any, Dict, List, Optional


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
