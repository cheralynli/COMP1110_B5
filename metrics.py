from typing import Any, Dict, Iterable, List
import csv
import glob
import os


def avg_wait_time(seating_records: List[Dict[str, Any]]) -> float:
    """
    THere must be a field "wait_time" in each record of seating_records, which represents the waiting time for that group.
    """
    total_wait = 0.0
    count = 0

    for record in seating_records:
        if "wait_time" in record and record["wait_time"] is not None:
            total_wait += float(record["wait_time"])
            count += 1

    if count == 0:
        return 0.0

    return total_wait / count


def max_wait_time(seating_records: List[Dict[str, Any]]) -> float:
    max_wait = 0.0
    found = False

    for record in seating_records:
        if ("wait_time" in record and record["wait_time"] is not None):
            wait_time = float(record["wait_time"])
            if (not found or wait_time > max_wait):
                max_wait = wait_time
                found = True

    if (not found):
        return 0.0

    return max_wait


def groups_served(seating_records: List[Dict[str, Any]]) -> int:
    return len(seating_records)


def seat_utilization(seating_records: List[Dict[str, Any]]) -> float:
    total_used_seat_time = 0.0
    total_available_seat_time = 0.0

    for record in seating_records:
        if ("group_size" in record and "table_capacity" in record and "dining_duration" in record and record["group_size"] is not None and record["table_capacity"] is not None and record["dining_duration"] is not None):
            group_size = float(record["group_size"])
            table_capacity = float(record["table_capacity"])
            dining_duration = float(record["dining_duration"])

            total_used_seat_time += group_size * dining_duration
            total_available_seat_time += table_capacity * dining_duration

    if total_available_seat_time == 0:
        return 0.0

    return total_used_seat_time / total_available_seat_time


def average_waiting_time_by_group_size(seating_records: List[Dict[str, Any]]) -> Dict[int, float]:
    total=  {}
    # int, float
    count = {}
    # int, int
    for record in seating_records:
        if ("group_size" in record and "wait_time" in record and record["group_size"] is not None and record["wait_time"] is not None):
            group_size = int(record["group_size"])
            wait_time = float(record["wait_time"])

            if (group_size not in total):
                total[group_size] = 0.0
                count[group_size] = 0

            total[group_size] += wait_time
            count[group_size] += 1

    result = {}
    # int, float 

    for group_size in total:
        if count[group_size] > 0:
            result[group_size] = total[group_size] / count[group_size]

    # A dictionary mapping group size to average waiting time.
    return result 


def fairness_gap_in_average_waiting_time(seating_records: List[Dict[str, Any]]) -> float:
    """
    Formula: max(avg waiting time by group size) -min(avg waiting time by group size)
    """
    avg_by_group_size = average_waiting_time_by_group_size(seating_records)

    if len(avg_by_group_size) == 0:
        return 0.0

    min_avg = None
    max_avg = None

    for group_size in avg_by_group_size:
        avg_wait = avg_by_group_size[group_size]

        if (min_avg is None or avg_wait < min_avg):
            min_avg = avg_wait

        if (max_avg is None or avg_wait > max_avg):
            max_avg = avg_wait

    if min_avg is None or max_avg is None:
        return 0.0

    return max_avg - min_avg


def read_seating_csv(file_path: str) -> List[Dict[str, Any]]:
    records: List[Dict[str, Any]] = []

    with open(file_path, "r", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            cleaned: Dict[str, Any] = {}
            for key, value in row.items():
                cleaned[key] = value if value != "" else None
            records.append(cleaned)

    return records


def find_seating_csv_files(search_dir: str) -> List[str]:
    pattern = os.path.join(search_dir, "seating*.csv")
    return sorted(glob.glob(pattern))


def compute_metrics(seating_records: List[Dict[str, Any]]) -> Dict[str, Any]:
    return {
        "avg_wait": avg_wait_time(seating_records),
        "max_wait": max_wait_time(seating_records),
        "groups_served": groups_served(seating_records),
        "seat_util": seat_utilization(seating_records),
        "fairness_gap": fairness_gap_in_average_waiting_time(seating_records),
    }


def write_summary_csv(output_path: str, rows: Iterable[Dict[str, Any]]) -> None:
    fieldnames = [
        "source_file",
        "avg_wait",
        "max_wait",
        "groups_served",
        "seat_util",
        "fairness_gap",
    ]

    with open(output_path, "w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    seating_files = find_seating_csv_files(base_dir)

    summary_rows: List[Dict[str, Any]] = []
    for file_path in seating_files:
        records = read_seating_csv(file_path)
        metrics = compute_metrics(records)
        metrics["source_file"] = os.path.basename(file_path)
        summary_rows.append(metrics)

    output_path = os.path.join(base_dir, "metrics_summary.csv")
    write_summary_csv(output_path, summary_rows)


if __name__ == "__main__":
    main()
