import csv
import os
from typing import Dict, List

import matplotlib.pyplot as plt


def _read_metrics_summary(file_path: str) -> List[Dict[str, float]]:
	rows: List[Dict[str, float]] = []

	with open(file_path, "r", newline="") as handle:
		reader = csv.DictReader(handle)
		for row in reader:
			rows.append({
				"source_file": row["source_file"],
				"avg_wait": float(row["avg_wait"]),
				"max_wait": float(row["max_wait"]),
				"groups_served": float(row["groups_served"]),
				"seat_util": float(row["seat_util"]),
				"fairness_gap": float(row["fairness_gap"]),
			})

	return rows


def _bar_chart(labels: List[str], values: List[float], title: str, y_label: str, output_path: str) -> None:
	plt.figure(figsize=(10, 5))
	plt.bar(labels, values)
	plt.title(title)
	plt.ylabel(y_label)
	plt.xticks(rotation=30, ha="right")
	plt.tight_layout()
	plt.savefig(output_path)
	plt.close()

def visualize_fcfs_metrics():
	base_dir = os.path.dirname(os.path.abspath(__file__))
	output_root = os.path.join(base_dir, "output")
	metrics_path = os.path.join(output_root, "fcfs_metrics_summary.csv")
	rows = _read_metrics_summary(metrics_path)
	labels = [row["source_file"].replace("fcfs_seating_log_", "").replace(".csv", "") for row in rows]
	
	output_dir = os.path.join(output_root, "fcfs_charts")
	os.makedirs(output_dir, exist_ok=True)

	_bar_chart(
        labels,
        [row["avg_wait"] for row in rows],
        "FCFS Average Wait Time",
        "Minutes",
        os.path.join(output_dir, "avg_wait.png"),
    )
	_bar_chart(
        labels,
        [row["max_wait"] for row in rows],
        "FCFS Maximum Wait Time",
        "Minutes",
        os.path.join(output_dir, "max_wait.png"),
    )
	_bar_chart(
		labels,
		[row["seat_util"] for row in rows],
		"FCFS Seat Utilization",
		"Utilization (0-1)",
		os.path.join(output_dir, "seat_util.png"),
	)
	_bar_chart(
		labels,
		[row["fairness_gap"] for row in rows],
		"FCFS Fairness Gap in Average Waiting Time",
		"Minutes",
		os.path.join(output_dir, "fairness_gap.png"),
	)
	_bar_chart(
		labels,
		[row["groups_served"] for row in rows],
		"FCFS Groups Served",
		"Count",
		os.path.join(output_dir, "groups_served.png"),
	)

def visualize_ourAlgo_metrics():
	base_dir = os.path.dirname(os.path.abspath(__file__))
	output_root = os.path.join(base_dir, "output")
	metrics_path = os.path.join(output_root, "metrics_summary.csv")

	rows = _read_metrics_summary(metrics_path)
	labels = [row["source_file"].replace("seating_log_", "").replace(".csv", "") for row in rows]

	output_dir = os.path.join(output_root, "charts")
	os.makedirs(output_dir, exist_ok=True)

	_bar_chart(
		labels,
		[row["avg_wait"] for row in rows],
		"Average Wait Time",
		"Minutes",
		os.path.join(output_dir, "avg_wait.png"),
	)
	_bar_chart(
		labels,
		[row["max_wait"] for row in rows],
		"Maximum Wait Time",
		"Minutes",
		os.path.join(output_dir, "max_wait.png"),
	)
	_bar_chart(
		labels,
		[row["seat_util"] for row in rows],
		"Seat Utilization",
		"Utilization (0-1)",
		os.path.join(output_dir, "seat_util.png"),
	)
	_bar_chart(
		labels,
		[row["fairness_gap"] for row in rows],
		"Fairness Gap in Average Waiting Time",
		"Minutes",
		os.path.join(output_dir, "fairness_gap.png"),
	)
	_bar_chart(
		labels,
		[row["groups_served"] for row in rows],
		"Groups Served",
		"Count",
		os.path.join(output_dir, "groups_served.png"),
	)

def main():
	visualize_fcfs_metrics()
	visualize_ourAlgo_metrics()


if __name__ == "__main__":
	main()

