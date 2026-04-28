from dataclasses import dataclass
from pathlib import Path
from typing import List


BASE_DIR = Path(__file__).resolve().parent
INPUT_DIR = BASE_DIR / "input"


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
        description="Full-day fast food service with light breakfast, moderate lunch, and a mild dinner lift.",
        config_path=INPUT_DIR / "fastfood_non_peak" / "config.txt",
        arrivals_path=INPUT_DIR / "fastfood_non_peak" / "arrivals.txt",
    ),
    Scenario(
        name="fastfood_peak",
        restaurant_type="fastfood",
        demand_level="peak",
        description="Full-day fast food service with strong lunch and dinner rushes and short lull periods.",
        config_path=INPUT_DIR / "fastfood_peak" / "config.txt",
        arrivals_path=INPUT_DIR / "fastfood_peak" / "arrivals.txt",
    ),
    Scenario(
        name="cafe_non_peak",
        restaurant_type="cafe",
        demand_level="non_peak",
        description="Full-day cafe service with steady but manageable traffic from morning through early evening.",
        config_path=INPUT_DIR / "cafe_non_peak" / "config.txt",
        arrivals_path=INPUT_DIR / "cafe_non_peak" / "arrivals.txt",
    ),
    Scenario(
        name="cafe_peak",
        restaurant_type="cafe",
        demand_level="peak",
        description="Full-day cafe service that stays busy across brunch, lunch, and late-day surges.",
        config_path=INPUT_DIR / "cafe_peak" / "config.txt",
        arrivals_path=INPUT_DIR / "cafe_peak" / "arrivals.txt",
    ),
    Scenario(
        name="family_non_peak",
        restaurant_type="family",
        demand_level="non_peak",
        description="Full-day family dining with spread-out lunch and dinner business and long table holds.",
        config_path=INPUT_DIR / "family_non_peak" / "config.txt",
        arrivals_path=INPUT_DIR / "family_non_peak" / "arrivals.txt",
    ),
    Scenario(
        name="family_peak",
        restaurant_type="family",
        demand_level="peak",
        description="Full-day family dining with heavy lunch and dinner demand from medium and large parties.",
        config_path=INPUT_DIR / "family_peak" / "config.txt",
        arrivals_path=INPUT_DIR / "family_peak" / "arrivals.txt",
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
