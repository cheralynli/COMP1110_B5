from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List


BASE_DIR = Path(__file__).resolve().parent
INPUT_DIR = BASE_DIR / "input" / "default"


@dataclass(frozen=True)
class Scenario:
    pair_id: str
    variation: str
    restaurant: str
    case_study_title: str
    factor_changed: str
    variation_summary: str
    description: str
    config_path: Path
    arrivals_path: Path

    @property
    def name(self) -> str:
        return f"{self.pair_id}_{self.variation}"


SCENARIOS: List[Scenario] = [
    Scenario(
        pair_id="family_dinner",
        variation="A",
        restaurant="Family Dining",
        case_study_title="Balanced vs Large-Table-Heavy Layout",
        factor_changed="table size mix",
        variation_summary="Balanced 2, 4, and 6-seat layout",
        description="Family dinner service with a balanced mix of 2-seat, 4-seat, and 6-seat tables.",
        config_path=INPUT_DIR / "family_dinner_A" / "config.txt",
        arrivals_path=INPUT_DIR / "family_dinner_A" / "arrivals.txt",
    ),
    Scenario(
        pair_id="family_dinner",
        variation="B",
        restaurant="Family Dining",
        case_study_title="Balanced vs Large-Table-Heavy Layout",
        factor_changed="table size mix",
        variation_summary="Large-table-heavy layout",
        description="Family dinner service with more 6-seat tables and fewer small-table options.",
        config_path=INPUT_DIR / "family_dinner_B" / "config.txt",
        arrivals_path=INPUT_DIR / "family_dinner_B" / "arrivals.txt",
    ),
    Scenario(
        pair_id="cafe_lunch",
        variation="A",
        restaurant="Cafe",
        case_study_title="Two-Seat Heavy vs Mixed Cafe Layout",
        factor_changed="table size mix",
        variation_summary="Mostly 2-seat tables",
        description="Cafe lunch rush with mostly 2-seat tables and only one 4-seat table.",
        config_path=INPUT_DIR / "cafe_lunch_A" / "config.txt",
        arrivals_path=INPUT_DIR / "cafe_lunch_A" / "arrivals.txt",
    ),
    Scenario(
        pair_id="cafe_lunch",
        variation="B",
        restaurant="Cafe",
        case_study_title="Two-Seat Heavy vs Mixed Cafe Layout",
        factor_changed="table size mix",
        variation_summary="Mixed 2-seat and 4-seat layout",
        description="Cafe lunch rush with a more flexible mix of 2-seat and 4-seat tables.",
        config_path=INPUT_DIR / "cafe_lunch_B" / "config.txt",
        arrivals_path=INPUT_DIR / "cafe_lunch_B" / "arrivals.txt",
    ),
    Scenario(
        pair_id="fastfood_rush",
        variation="A",
        restaurant="Fast Food",
        case_study_title="Limited vs Expanded Rush Capacity",
        factor_changed="total seating capacity",
        variation_summary="Limited rush seating capacity",
        description="Fast-food rush with tighter seating capacity and stronger queue pressure.",
        config_path=INPUT_DIR / "fastfood_rush_A" / "config.txt",
        arrivals_path=INPUT_DIR / "fastfood_rush_A" / "arrivals.txt",
    ),
    Scenario(
        pair_id="fastfood_rush",
        variation="B",
        restaurant="Fast Food",
        case_study_title="Limited vs Expanded Rush Capacity",
        factor_changed="total seating capacity",
        variation_summary="Expanded rush seating capacity",
        description="Fast-food rush with expanded seating capacity under the same arrival pattern.",
        config_path=INPUT_DIR / "fastfood_rush_B" / "config.txt",
        arrivals_path=INPUT_DIR / "fastfood_rush_B" / "arrivals.txt",
    ),
    Scenario(
        pair_id="sushi_belt",
        variation="A",
        restaurant="Sushi Belt",
        case_study_title="Solo-Seat Priority vs More Shared Seating",
        factor_changed="small-seat vs group-seat emphasis",
        variation_summary="Many 1-seat and 2-seat spots",
        description="Sushi belt lunch layout optimized for solo diners and pairs.",
        config_path=INPUT_DIR / "sushi_belt_A" / "config.txt",
        arrivals_path=INPUT_DIR / "sushi_belt_A" / "arrivals.txt",
    ),
    Scenario(
        pair_id="sushi_belt",
        variation="B",
        restaurant="Sushi Belt",
        case_study_title="Solo-Seat Priority vs More Shared Seating",
        factor_changed="small-seat vs group-seat emphasis",
        variation_summary="Fewer solo spots and more 4-seat tables",
        description="Sushi belt lunch layout that gives up solo spots for more shared-table flexibility.",
        config_path=INPUT_DIR / "sushi_belt_B" / "config.txt",
        arrivals_path=INPUT_DIR / "sushi_belt_B" / "arrivals.txt",
    ),
    Scenario(
        pair_id="kbbq_hotpot",
        variation="A",
        restaurant="KBBQ / Hotpot",
        case_study_title="Medium-Group Focus vs Large-Group Focus",
        factor_changed="layout focus for expected group sizes",
        variation_summary="More 4-seat tables",
        description="KBBQ or hotpot dinner service built around more 4-seat tables for medium groups.",
        config_path=INPUT_DIR / "kbbq_hotpot_A" / "config.txt",
        arrivals_path=INPUT_DIR / "kbbq_hotpot_A" / "arrivals.txt",
    ),
    Scenario(
        pair_id="kbbq_hotpot",
        variation="B",
        restaurant="KBBQ / Hotpot",
        case_study_title="Medium-Group Focus vs Large-Group Focus",
        factor_changed="layout focus for expected group sizes",
        variation_summary="More 6-seat tables",
        description="KBBQ or hotpot dinner service built around more 6-seat tables for large groups.",
        config_path=INPUT_DIR / "kbbq_hotpot_B" / "config.txt",
        arrivals_path=INPUT_DIR / "kbbq_hotpot_B" / "arrivals.txt",
    ),
]


def list_scenarios() -> List[Scenario]:
    return SCENARIOS


def scenarios_by_pair() -> Dict[str, List[Scenario]]:
    grouped: Dict[str, List[Scenario]] = {}
    for scenario in SCENARIOS:
        grouped.setdefault(scenario.pair_id, []).append(scenario)

    for pair_id in grouped:
        grouped[pair_id].sort(key=lambda item: item.variation)

    return grouped


def main() -> None:
    for pair_id, pair_scenarios in scenarios_by_pair().items():
        first = pair_scenarios[0]
        print(f"{first.restaurant} - {first.case_study_title}")
        print(f"  case_id={pair_id}")
        print(f"  factor_changed={first.factor_changed}")
        for scenario in pair_scenarios:
            print(f"  {scenario.variation}: {scenario.variation_summary}")
            print(f"    config={scenario.config_path}")
            print(f"    arrivals={scenario.arrivals_path}")
        print()


if __name__ == "__main__":
    main()
