# Case Study Analysis Template

Use one table per case study. Each table should compare variation `A` and variation `B` for the same restaurant story and the same arrivals.

## How To Read `results.csv`

For one case study:

- choose one `algorithm` to discuss, or compare multiple algorithms in separate tables
- take the two rows for the matching `scenario_name` pair, such as `pair_1_A` and `pair_1_B`
- copy the values for:
  - `avg_wait`
  - `max_wait`
  - `groups_served`
  - `table_utilization`
  - `service_level_15`
  - `max_queue_length_if_available`

## Detailed Table Template

Copy this template once for each case study.

### Case Study: [Restaurant Name]

Context:
- Same arrivals in A and B: [yes, describe the rush or meal period]
- What changes between A and B: [one clear setting difference]
- Algorithm discussed: [custom / fcfs / size_queue]

| Restaurant | Variation | Setup summary | Factor changed | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length | Key takeaway |
|---|---|---|---|---:|---:|---:|---:|---:|---:|---|
| [Restaurant] | A | [brief setup] | [factor] | [x] | [x] | [x] | [x]% | [x]% | [x] | [1 short sentence] |
| [Restaurant] | B | [brief setup] | [factor] | [x] | [x] | [x] | [x]% | [x]% | [x] | [1 short sentence] |

Short analysis:
- Both versions used the same arrivals, so differences come from the restaurant setup rather than customer demand.
- Variation [A/B] had the lower average wait, which suggests [why].
- Variation [A/B] had the better utilization, which suggests [why].
- The main trade-off revealed here is [example: better support for small groups vs more flexibility for large groups].

## Example Structure

### Case Study: Fast Food Peak Rush

Context:
- Same arrivals in A and B: peak fast-food lunch rush
- What changes between A and B: seating capacity and table mix
- Algorithm discussed: custom

| Restaurant | Variation | Setup summary | Factor changed | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length | Key takeaway |
|---|---|---|---|---:|---:|---:|---:|---:|---:|---|
| Fast Food | A | small-table-heavy, tighter capacity | table mix / capacity | 12.4 | 35 | 20 | 71% | 82% | 6 | good for solo diners but queues grow faster |
| Fast Food | B | more balanced layout, slightly larger capacity | table mix / capacity | 8.1 | 22 | 20 | 78% | 94% | 3 | lower waits and better rush handling |

Short analysis:
- Both variations used the same peak-rush arrivals.
- Variation B reduced both average and maximum wait time.
- Variation B also kept utilization high, so the better wait times were not caused by leaving tables idle.
- This suggests the more balanced fast-food layout handled the lunch rush better.

## Overall Summary Table

After making one detailed table per case study, finish with one short overall table.

| Case study | Algorithm | Better variation | Main reason | Main trade-off |
|---|---|---|---|---|
| [Cafe lunch rush] | [custom] | [A/B] | [short reason] | [short trade-off] |
| [Fast food peak rush] | [custom] | [A/B] | [short reason] | [short trade-off] |
| [Family dinner] | [custom] | [A/B] | [short reason] | [short trade-off] |
| [Sushi belt] | [custom] | [A/B] | [short reason] | [short trade-off] |
| [KBBQ / hotpot] | [custom] | [A/B] | [short reason] | [short trade-off] |

## Suggested Writing Prompts

For each case study, answer:

1. What restaurant situation is this trying to represent?
2. What stays the same between A and B?
3. What restaurant setting changes between A and B?
4. Which metrics improved?
5. What trade-off does the comparison reveal?
6. What limitation is still present in the simulation?

## Important Reminder

Do not compare unrelated rows.

Good comparison:
- `pair_X_A` vs `pair_X_B`

Bad comparison:
- `pair_1_A` vs `pair_4_B`

Inside one case study, the arrival pattern should stay the same so the comparison is fair.
