from typing import Any, Dict, Iterable, List
import csv
import glob
import os


def avg_wait_time(seating_records: List[Dict[str, Any]]) -> float:
    """
    THere must be a field "wait_time" in each record of seating_records, which represents the waiting time for that group.
    """
    # Formula: sum(wait_time) / count(wait_time)
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
    # Formula: max(wait_time)
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
    # Formula: count(records)
    return len(seating_records)


def seat_utilization(seating_records: List[Dict[str, Any]]) -> float:
    # Formula: sum(group_size * dining_duration) / sum(table_capacity * dining_duration)
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
    # Formula: for each group_size g, sum(wait_time_g) / count(wait_time_g)
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


def _has_field(seating_records: List[Dict[str, Any]], field_name: str) -> bool:
    for record in seating_records:
        if field_name in record and record[field_name] is not None:
            return True
    return False


def service_level(seating_records: List[Dict[str, Any]], threshold_minutes: float) -> float:
    # Formula: count(wait_time <= X) / count(wait_time)
    served_within = 0
    count = 0

    for record in seating_records:
        if "wait_time" in record and record["wait_time"] is not None:
            wait_time = float(record["wait_time"])
            count += 1
            if wait_time <= threshold_minutes:
                served_within += 1

    if count == 0:
        return 0.0

    return served_within / count


def median_wait_time(seating_records: List[Dict[str, Any]]) -> float:
    # Formula: middle value of sorted wait_time list (or average of two middle values)
    wait_times: List[float] = []

    for record in seating_records:
        if "wait_time" in record and record["wait_time"] is not None:
            wait_times.append(float(record["wait_time"]))

    if not wait_times:
        return 0.0

    wait_times.sort()
    mid = len(wait_times) // 2

    if len(wait_times) % 2 == 1:
        return wait_times[mid]

    return (wait_times[mid - 1] + wait_times[mid]) / 2.0


def queue_event_times(seating_records: List[Dict[str, Any]]) -> List[float]:
    event_times: List[float] = []

    for record in seating_records:
        if (
            "arrival_time" in record
            and "seating_time" in record
            and record["arrival_time"] is not None
            and record["seating_time"] is not None
        ):
            arrival_time = float(record["arrival_time"])
            seating_time = float(record["seating_time"])
            if seating_time > arrival_time:
                event_times.append(arrival_time)
                event_times.append(seating_time)

    return sorted(set(event_times))


def queue_length_at_time(seating_records: List[Dict[str, Any]], time_point: float) -> int:
    length = 0

    for record in seating_records:
        if (
            "arrival_time" in record
            and "seating_time" in record
            and record["arrival_time"] is not None
            and record["seating_time"] is not None
        ):
            arrival_time = float(record["arrival_time"])
            seating_time = float(record["seating_time"])
            if arrival_time <= time_point < seating_time:
                length += 1

    return length


def final_departure_time(seating_records: List[Dict[str, Any]]) -> float:
    latest_departure = None

    for record in seating_records:
        if "departure_time" in record and record["departure_time"] is not None:
            departure_time = float(record["departure_time"])
            if latest_departure is None or departure_time > latest_departure:
                latest_departure = departure_time

    if latest_departure is None:
        return 0.0

    return latest_departure


def max_queue_length(seating_records: List[Dict[str, Any]]) -> float:
    # Formula: max(queue_length at each event time)
    event_times = queue_event_times(seating_records)
    if not event_times:
        return 0.0

    return float(max(queue_length_at_time(seating_records, time_point) for time_point in event_times))


def average_queue_length(seating_records: List[Dict[str, Any]]) -> float:
    # Formula: sum(queue_length(t) * delta_t) / finish_time
    event_times = queue_event_times(seating_records)
    finish_time = final_departure_time(seating_records)

    if len(event_times) < 2 or finish_time <= 0:
        return 0.0

    area = 0.0
    for current_time, next_time in zip(event_times, event_times[1:]):
        queue_length = queue_length_at_time(seating_records, current_time)
        area += queue_length * (next_time - current_time)

    return area / finish_time

def true_seat_utilization(seating_records: List[Dict[str, Any]]) -> float:
    # Formula: used_seat_minutes / (total_seats * service_duration)
    used_seat_minutes = 0.0
    table_capacity_by_id: Dict[str, float] = {}
    min_arrival = None
    max_departure = None

    for record in seating_records:
        if (
            "group_size" in record
            and "table_capacity" in record
            and "dining_duration" in record
            and record["group_size"] is not None
            and record["table_capacity"] is not None
            and record["dining_duration"] is not None
        ):
            group_size = float(record["group_size"])
            table_capacity = float(record["table_capacity"])
            dining_duration = float(record["dining_duration"])
            used_seat_minutes += group_size * dining_duration

            table_id = str(record.get("table_id", ""))
            if table_id:
                prev_capacity = table_capacity_by_id.get(table_id)
                if prev_capacity is None or table_capacity > prev_capacity:
                    table_capacity_by_id[table_id] = table_capacity

        if "arrival_time" in record and record["arrival_time"] is not None:
            arrival_time = float(record["arrival_time"])
            if min_arrival is None or arrival_time < min_arrival:
                min_arrival = arrival_time

        if "departure_time" in record and record["departure_time"] is not None:
            departure_time = float(record["departure_time"])
            if max_departure is None or departure_time > max_departure:
                max_departure = departure_time

    if not table_capacity_by_id or min_arrival is None or max_departure is None:
        return 0.0

    service_duration = max_departure - min_arrival
    if service_duration <= 0:
        return 0.0

    total_seats = sum(table_capacity_by_id.values())
    total_possible = total_seats * service_duration
    if total_possible == 0:
        return 0.0

    return used_seat_minutes / total_possible


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


def find_ourAlgo_csv_files(search_dir: str) -> List[str]:
    pattern = os.path.join(search_dir, "seating*.csv")
    return sorted(glob.glob(pattern))

def find_fcfs_csv_files(search_dir: str) -> List[str]:  
    pattern = os.path.join(search_dir, "fcfs_seating_log_*.csv")
    return sorted(glob.glob(pattern))

def compute_metrics(seating_records: List[Dict[str, Any]]) -> Dict[str, Any]:
    return {
        "avg_wait": avg_wait_time(seating_records),
        "max_wait": max_wait_time(seating_records),
        "median_wait": median_wait_time(seating_records),
        "groups_served": groups_served(seating_records),
        "seat_util": seat_utilization(seating_records),
        "true_seat_util": true_seat_utilization(seating_records),
        "service_level_15": service_level(seating_records, 15.0),
        "max_queue_length": max_queue_length(seating_records),
        "avg_queue_length": average_queue_length(seating_records),
        "fairness_gap": fairness_gap_in_average_waiting_time(seating_records),
    }


def write_summary_csv(output_path: str, rows: Iterable[Dict[str, Any]]) -> None:
    fieldnames = [
        "source_file",
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
    ]

    with open(output_path, "w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

def run_metrics_computation_fcfs():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(base_dir, "output")
    os.makedirs(output_dir, exist_ok=True)
    seating_files = find_fcfs_csv_files(output_dir)

    summary_rows: List[Dict[str, Any]] = []
    for file_path in seating_files:
        records = read_seating_csv(file_path)
        metrics = compute_metrics(records)
        metrics["source_file"] = os.path.basename(file_path)
        summary_rows.append(metrics)

    output_path = os.path.join(output_dir, "fcfs_metrics_summary.csv")
    write_summary_csv(output_path, summary_rows)

def run_metrics_computation_ourAlgo():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(base_dir, "output")
    os.makedirs(output_dir, exist_ok=True)
    seating_files = find_ourAlgo_csv_files(output_dir)

    summary_rows: List[Dict[str, Any]] = []
    for file_path in seating_files:
        records = read_seating_csv(file_path)
        metrics = compute_metrics(records)
        metrics["source_file"] = os.path.basename(file_path)
        summary_rows.append(metrics)

    output_path = os.path.join(output_dir, "metrics_summary.csv")
    write_summary_csv(output_path, summary_rows)

def main():
    run_metrics_computation_fcfs()
    run_metrics_computation_ourAlgo()


if __name__ == "__main__":
    main()
