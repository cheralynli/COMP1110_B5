import csv
import os
from typing import Dict, List

import matplotlib.pyplot as plt


def _parse_float(value: str) -> float:
	if value is None or value == "":
		return 0.0
	return float(value)


def _read_metrics_summary(file_path: str) -> List[Dict[str, float]]:
	rows: List[Dict[str, float]] = []

	with open(file_path, "r", newline="") as handle:
		reader = csv.DictReader(handle)
		for row in reader:
			rows.append({
				"source_file": row["source_file"],
				"avg_wait": _parse_float(row.get("avg_wait", "")),
				"max_wait": _parse_float(row.get("max_wait", "")),
				"median_wait": _parse_float(row.get("median_wait", "")),
				"groups_served": _parse_float(row.get("groups_served", "")),
				"seat_util": _parse_float(row.get("seat_util", "")),
				"true_seat_util": _parse_float(row.get("true_seat_util", "")),
				"service_level_15": _parse_float(row.get("service_level_15", "")),
				"max_queue_length": _parse_float(row.get("max_queue_length", "")),
				"avg_queue_length": _parse_float(row.get("avg_queue_length", "")),
				"fairness_gap": _parse_float(row.get("fairness_gap", "")),
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


def _comparison_chart_three(labels: List[str], series_a: List[float], series_b: List[float],
						series_c: List[float], title: str, y_label: str, output_path: str,
						label_a: str, label_b: str, label_c: str) -> None:
	bar_width = 0.25
	indices = list(range(len(labels)))

	plt.figure(figsize=(12, 5))
	plt.bar([i - bar_width for i in indices], series_a, width=bar_width, label=label_a)
	plt.bar(indices, series_b, width=bar_width, label=label_b)
	plt.bar([i + bar_width for i in indices], series_c, width=bar_width, label=label_c)
	plt.title(title)
	plt.ylabel(y_label)
	plt.xticks(indices, labels, rotation=30, ha="right")
	plt.legend()
	plt.tight_layout()
	plt.savefig(output_path)
	plt.close()


def _charts_dir(output_root: str, folder_name: str) -> str:
	output_dir = os.path.join(output_root, folder_name)
	os.makedirs(output_dir, exist_ok=True)
	return output_dir

def visualize_fcfs_metrics():
	base_dir = os.path.dirname(os.path.abspath(__file__))
	output_root = os.path.join(base_dir, "output")
	metrics_path = os.path.join(output_root, "fcfs_metrics_summary.csv")
	rows = _read_metrics_summary(metrics_path)
	labels = [row["source_file"].replace("fcfs_seating_log_", "").replace(".csv", "") for row in rows]
	
	output_dir = _charts_dir(output_root, "fcfs_charts")

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
		[row["median_wait"] for row in rows],
		"FCFS Median Wait Time",
		"Minutes",
		os.path.join(output_dir, "median_wait.png"),
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
		[row["true_seat_util"] for row in rows],
		"FCFS True Seat Utilization",
		"Utilization (0-1)",
		os.path.join(output_dir, "true_seat_util.png"),
	)
	_bar_chart(
		labels,
		[row["service_level_15"] for row in rows],
		"FCFS Service Level (<= 15 min)",
		"Share (0-1)",
		os.path.join(output_dir, "service_level_15.png"),
	)
	_bar_chart(
		labels,
		[row["max_queue_length"] for row in rows],
		"FCFS Max Queue Length",
		"Groups",
		os.path.join(output_dir, "max_queue_length.png"),
	)
	_bar_chart(
		labels,
		[row["avg_queue_length"] for row in rows],
		"FCFS Average Queue Length",
		"Groups",
		os.path.join(output_dir, "avg_queue_length.png"),
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

	output_dir = _charts_dir(output_root, "ourAlgo_charts")

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
		[row["median_wait"] for row in rows],
		"Median Wait Time",
		"Minutes",
		os.path.join(output_dir, "median_wait.png"),
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
		[row["true_seat_util"] for row in rows],
		"True Seat Utilization",
		"Utilization (0-1)",
		os.path.join(output_dir, "true_seat_util.png"),
	)
	_bar_chart(
		labels,
		[row["service_level_15"] for row in rows],
		"Service Level (<= 15 min)",
		"Share (0-1)",
		os.path.join(output_dir, "service_level_15.png"),
	)
	_bar_chart(
		labels,
		[row["max_queue_length"] for row in rows],
		"Max Queue Length",
		"Groups",
		os.path.join(output_dir, "max_queue_length.png"),
	)
	_bar_chart(
		labels,
		[row["avg_queue_length"] for row in rows],
		"Average Queue Length",
		"Groups",
		os.path.join(output_dir, "avg_queue_length.png"),
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


def visualize_size_metrics():
	base_dir = os.path.dirname(os.path.abspath(__file__))
	output_root = os.path.join(base_dir, "output")
	metrics_path = os.path.join(output_root, "size_metrics_summary.csv")

	rows = _read_metrics_summary(metrics_path)
	labels = [row["source_file"].replace("size_seating_log_", "").replace(".csv", "") for row in rows]

	output_dir = _charts_dir(output_root, "size_charts")

	_bar_chart(
		labels,
		[row["avg_wait"] for row in rows],
		"Size Average Wait Time",
		"Minutes",
		os.path.join(output_dir, "avg_wait.png"),
	)
	_bar_chart(
		labels,
		[row["max_wait"] for row in rows],
		"Size Maximum Wait Time",
		"Minutes",
		os.path.join(output_dir, "max_wait.png"),
	)
	_bar_chart(
		labels,
		[row["median_wait"] for row in rows],
		"Size Median Wait Time",
		"Minutes",
		os.path.join(output_dir, "median_wait.png"),
	)
	_bar_chart(
		labels,
		[row["seat_util"] for row in rows],
		"Size Seat Utilization",
		"Utilization (0-1)",
		os.path.join(output_dir, "seat_util.png"),
	)
	_bar_chart(
		labels,
		[row["true_seat_util"] for row in rows],
		"Size True Seat Utilization",
		"Utilization (0-1)",
		os.path.join(output_dir, "true_seat_util.png"),
	)
	_bar_chart(
		labels,
		[row["service_level_15"] for row in rows],
		"Size Service Level (<= 15 min)",
		"Share (0-1)",
		os.path.join(output_dir, "service_level_15.png"),
	)
	_bar_chart(
		labels,
		[row["max_queue_length"] for row in rows],
		"Size Max Queue Length",
		"Groups",
		os.path.join(output_dir, "max_queue_length.png"),
	)
	_bar_chart(
		labels,
		[row["avg_queue_length"] for row in rows],
		"Size Average Queue Length",
		"Groups",
		os.path.join(output_dir, "avg_queue_length.png"),
	)
	_bar_chart(
		labels,
		[row["fairness_gap"] for row in rows],
		"Size Fairness Gap in Average Waiting Time",
		"Minutes",
		os.path.join(output_dir, "fairness_gap.png"),
	)
	_bar_chart(
		labels,
		[row["groups_served"] for row in rows],
		"Size Groups Served",
		"Count",
		os.path.join(output_dir, "groups_served.png"),
	)


def visualize_combined_metrics():
	base_dir = os.path.dirname(os.path.abspath(__file__))
	output_root = os.path.join(base_dir, "output")
	fcfs_path = os.path.join(output_root, "fcfs_metrics_summary.csv")
	ours_path = os.path.join(output_root, "metrics_summary.csv")
	size_path = os.path.join(output_root, "size_metrics_summary.csv")

	fcfs_rows = _read_metrics_summary(fcfs_path)
	ours_rows = _read_metrics_summary(ours_path)
	size_rows = _read_metrics_summary(size_path)

	fcfs_by_key = {
		_scenario_key(row["source_file"], "fcfs_seating_log_"): row for row in fcfs_rows
	}
	ours_by_key = {
		_scenario_key(row["source_file"], "seating_log_"): row for row in ours_rows
	}
	size_by_key = {
		_scenario_key(row["source_file"], "size_seating_log_"): row for row in size_rows
	}

	labels = sorted(set(fcfs_by_key) & set(ours_by_key) & set(size_by_key))
	output_dir = _charts_dir(output_root, "charts")

	metrics = [
		("avg_wait", "Average Wait Time", "Minutes"),
		("max_wait", "Maximum Wait Time", "Minutes"),
		("median_wait", "Median Wait Time", "Minutes"),
		("seat_util", "Seat Utilization", "Utilization (0-1)"),
		("true_seat_util", "True Seat Utilization", "Utilization (0-1)"),
		("service_level_15", "Service Level (<= 15 min)", "Share (0-1)"),
		("max_queue_length", "Max Queue Length", "Groups"),
		("avg_queue_length", "Average Queue Length", "Groups"),
		("fairness_gap", "Fairness Gap in Average Waiting Time", "Minutes"),
		("groups_served", "Groups Served", "Count"),
	]

	for key, title, y_label in metrics:
		series_fcfs = [fcfs_by_key[label][key] for label in labels]
		series_ours = [ours_by_key[label][key] for label in labels]
		series_size = [size_by_key[label][key] for label in labels]
		_comparison_chart_three(
			labels,
			series_fcfs,
			series_ours,
			series_size,
			f"FCFS vs Our Algo vs Size: {title}",
			y_label,
			os.path.join(output_dir, f"{key}.png"),
			"FCFS",
			"Our Algo",
			"Size",
		)


def _scenario_key(source_file: str, prefix: str) -> str:
	return source_file.replace(prefix, "").replace(".csv", "")


def _split_scenario_key(key: str) -> Dict[str, str]:
	if key.endswith("_non_peak"):
		return {"scenario": key[:-9], "setting": "non_peak"}
	if key.endswith("_peak"):
		return {"scenario": key[:-5], "setting": "peak"}
	if "_" not in key:
		return {"scenario": key, "setting": ""}

	scenario, setting = key.rsplit("_", 1)
	return {"scenario": scenario, "setting": setting}


def _setting_sort_key(setting: str):
	if setting == "A":
		return (0, setting)
	if setting == "B":
		return (1, setting)
	if setting == "non_peak":
		return (0, setting)
	if setting == "peak":
		return (1, setting)
	return (2, setting)


def visualize_scenario_comparison_metrics():
	base_dir = os.path.dirname(os.path.abspath(__file__))
	output_root = os.path.join(base_dir, "output")
	fcfs_path = os.path.join(output_root, "fcfs_metrics_summary.csv")
	ours_path = os.path.join(output_root, "metrics_summary.csv")
	size_path = os.path.join(output_root, "size_metrics_summary.csv")

	fcfs_rows = _read_metrics_summary(fcfs_path)
	ours_rows = _read_metrics_summary(ours_path)
	size_rows = _read_metrics_summary(size_path)

	fcfs_by_key = {
		_scenario_key(row["source_file"], "fcfs_seating_log_"): row for row in fcfs_rows
	}
	ours_by_key = {
		_scenario_key(row["source_file"], "seating_log_"): row for row in ours_rows
	}
	size_by_key = {
		_scenario_key(row["source_file"], "size_seating_log_"): row for row in size_rows
	}

	available_keys = sorted(set(fcfs_by_key) & set(ours_by_key) & set(size_by_key))
	by_scenario: Dict[str, List[str]] = {}
	for key in available_keys:
		split = _split_scenario_key(key)
		scenario = split["scenario"]
		setting = split["setting"]
		by_scenario.setdefault(scenario, []).append(setting)

	metrics = [
		("avg_wait", "Average Wait Time", "Minutes"),
		("max_wait", "Maximum Wait Time", "Minutes"),
		("median_wait", "Median Wait Time", "Minutes"),
		("seat_util", "Seat Utilization", "Utilization (0-1)"),
		("true_seat_util", "True Seat Utilization", "Utilization (0-1)"),
		("service_level_15", "Service Level (<= 15 min)", "Share (0-1)"),
		("max_queue_length", "Max Queue Length", "Groups"),
		("avg_queue_length", "Average Queue Length", "Groups"),
		("fairness_gap", "Fairness Gap in Average Waiting Time", "Minutes"),
		("groups_served", "Groups Served", "Count"),
	]

	root_dir = _charts_dir(output_root, "scenario_comparison_charts")
	os.makedirs(root_dir, exist_ok=True)

	for scenario, settings in by_scenario.items():
		settings_sorted = sorted(set(settings), key=_setting_sort_key)
		labels = [f"{scenario}_{setting}" if setting else scenario for setting in settings_sorted]
		comparison_dir = os.path.join(root_dir, scenario)
		os.makedirs(comparison_dir, exist_ok=True)

		for key, title, y_label in metrics:
			series_fcfs = []
			series_ours = []
			series_size = []
			for setting in settings_sorted:
				combined_key = f"{scenario}_{setting}" if setting else scenario
				series_fcfs.append(fcfs_by_key[combined_key][key])
				series_ours.append(ours_by_key[combined_key][key])
				series_size.append(size_by_key[combined_key][key])

			file_name = f"{key}_comparison.png"
			_comparison_chart_three(
				labels,
				series_fcfs,
				series_ours,
				series_size,
				f"{scenario}: FCFS vs Our Algo vs Size - {title}",
				y_label,
				os.path.join(comparison_dir, file_name),
				"FCFS",
				"Our Algo",
				"Size",
			)

def main():
	visualize_fcfs_metrics()
	visualize_ourAlgo_metrics()
	visualize_size_metrics()
	visualize_combined_metrics()
	visualize_scenario_comparison_metrics()


if __name__ == "__main__":
	main()

