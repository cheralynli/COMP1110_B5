from typing import Any, Dict, List
import glob
import os

from metrics import compute_metrics, read_seating_csv, write_summary_csv


def find_fcfs_seating_csv_files(search_dir: str) -> List[str]:
    pattern = os.path.join(search_dir, "fcfs_seating_log_*.csv")
    return sorted(glob.glob(pattern))


def main() -> None:
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(base_dir, "output")
    os.makedirs(output_dir, exist_ok=True)
    seating_files = find_fcfs_seating_csv_files(output_dir)

    summary_rows: List[Dict[str, Any]] = []
    for file_path in seating_files:
        records = read_seating_csv(file_path)
        metrics = compute_metrics(records)
        metrics["source_file"] = os.path.basename(file_path)
        summary_rows.append(metrics)

    output_path = os.path.join(output_dir, "fcfs_metrics_summary.csv")
    write_summary_csv(output_path, summary_rows)


if __name__ == "__main__":
    main()
