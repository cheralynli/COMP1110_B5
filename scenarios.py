from dataclasses import dataclass
from pathlib import Path
from typing import List


BASE_DIR = Path(__file__).resolve().parent


@dataclass(frozen=True)
class Scenario:
    name: str
    restaurant_type: str
    demand_level: str
    description: str
    config_path: Path
    arrivals_path: Path


SCENARIOS: List[Scenario] = [
    Scenario(
        name="fastfood_non_peak",
        restaurant_type="fastfood",
        demand_level="non_peak",
        description="Fast food with light traffic, mostly solo diners and pairs, and short stays.",
        config_path=BASE_DIR / "fastfood_non_peak" / "config.txt",
        arrivals_path=BASE_DIR / "fastfood_non_peak" / "arrivals.txt",
    ),
    Scenario(
        name="fastfood_peak",
        restaurant_type="fastfood",
        demand_level="peak",
        description="Fast food rush with dense small-party arrivals and very quick table turnover.",
        config_path=BASE_DIR / "fastfood_peak" / "config.txt",
        arrivals_path=BASE_DIR / "fastfood_peak" / "arrivals.txt",
    ),
    Scenario(
        name="cafe_non_peak",
        restaurant_type="cafe",
        demand_level="non_peak",
        description="Cafe with lighter traffic, mostly solo diners and pairs.",
        config_path=BASE_DIR / "cafe_non_peak" / "config.txt",
        arrivals_path=BASE_DIR / "cafe_non_peak" / "arrivals.txt",
    ),
    Scenario(
        name="cafe_peak",
        restaurant_type="cafe",
        demand_level="peak",
        description="Cafe rush with dense arrivals and more queue pressure.",
        config_path=BASE_DIR / "cafe_peak" / "config.txt",
        arrivals_path=BASE_DIR / "cafe_peak" / "arrivals.txt",
    ),
    Scenario(
        name="family_non_peak",
        restaurant_type="family",
        demand_level="non_peak",
        description="Family dining with moderate traffic, larger groups, and longer meal durations.",
        config_path=BASE_DIR / "family_non_peak" / "config.txt",
        arrivals_path=BASE_DIR / "family_non_peak" / "arrivals.txt",
    ),
    Scenario(
        name="family_peak",
        restaurant_type="family",
        demand_level="peak",
        description="Family dining rush with many medium and large groups arriving close together.",
        config_path=BASE_DIR / "family_peak" / "config.txt",
        arrivals_path=BASE_DIR / "family_peak" / "arrivals.txt",
    ),
]


def list_scenarios() -> List[Scenario]:
    return SCENARIOS


def main() -> None:
    for scenario in SCENARIOS:
        print(
            f"{scenario.name}: {scenario.restaurant_type} ({scenario.demand_level})\n"
            f"  config={scenario.config_path}\n"
            f"  arrivals={scenario.arrivals_path}"
        )


if __name__ == "__main__":
    main()
