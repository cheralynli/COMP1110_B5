# Case Study Tables

These tables are generated from `results.csv` and are meant to be report-friendly summaries.

## Family Dining: Balanced vs Large-Table-Heavy Layout

- Case ID: `family_dinner`
- Factor changed: table size mix
- Same in A and B: arrivals, group sizes, and dining-duration pattern

| Variation | Setup summary | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length |
|---|---|---:|---:|---:|---:|---:|---:|
| A | Balanced 2, 4, and 6-seat layout | 32.75 | 71.0 | 12 | 43.86% | 25.0% | 7 |
| B | Large-table-heavy layout | 34.5 | 71.0 | 12 | 48.36% | 16.67% | 8 |

Suggested interpretation:
- Variation A: Balanced 2, 4, and 6-seat layout
- Variation B: Large-table-heavy layout
- Compare which side lowers wait and queue length without causing poor utilization.
- Because the arrivals are the same, differences come from the restaurant setup rather than demand.

## Cafe: Two-Seat Heavy vs Mixed Cafe Layout

- Case ID: `cafe_lunch`
- Factor changed: table size mix
- Same in A and B: arrivals, group sizes, and dining-duration pattern

| Variation | Setup summary | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length |
|---|---|---:|---:|---:|---:|---:|---:|
| A | Mostly 2-seat tables | 49.33 | 117.0 | 12 | 26.61% | 0.0% | 10 |
| B | Mixed 2-seat and 4-seat layout | 44.0 | 79.0 | 12 | 33.64% | 0.0% | 10 |

Suggested interpretation:
- Variation A: Mostly 2-seat tables
- Variation B: Mixed 2-seat and 4-seat layout
- Compare which side lowers wait and queue length without causing poor utilization.
- Because the arrivals are the same, differences come from the restaurant setup rather than demand.

## Fast Food: Limited vs Expanded Rush Capacity

- Case ID: `fastfood_rush`
- Factor changed: total seating capacity
- Same in A and B: arrivals, group sizes, and dining-duration pattern

| Variation | Setup summary | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length |
|---|---|---:|---:|---:|---:|---:|---:|
| A | Limited rush seating capacity | 44.06 | 65.0 | 16 | 43.15% | 0.0% | 14 |
| B | Expanded rush seating capacity | 39.12 | 57.0 | 16 | 36.81% | 0.0% | 14 |

Suggested interpretation:
- Variation A: Limited rush seating capacity
- Variation B: Expanded rush seating capacity
- Compare which side lowers wait and queue length without causing poor utilization.
- Because the arrivals are the same, differences come from the restaurant setup rather than demand.

## Sushi Belt: Solo-Seat Priority vs More Shared Seating

- Case ID: `sushi_belt`
- Factor changed: small-seat vs group-seat emphasis
- Same in A and B: arrivals, group sizes, and dining-duration pattern

| Variation | Setup summary | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length |
|---|---|---:|---:|---:|---:|---:|---:|
| A | Many 1-seat and 2-seat spots | 28.21 | 69.0 | 14 | 32.34% | 0.0% | 9 |
| B | Fewer solo spots and more 4-seat tables | 32.71 | 45.0 | 14 | 29.37% | 0.0% | 10 |

Suggested interpretation:
- Variation A: Many 1-seat and 2-seat spots
- Variation B: Fewer solo spots and more 4-seat tables
- Compare which side lowers wait and queue length without causing poor utilization.
- Because the arrivals are the same, differences come from the restaurant setup rather than demand.

## KBBQ / Hotpot: Medium-Group Focus vs Large-Group Focus

- Case ID: `kbbq_hotpot`
- Factor changed: layout focus for expected group sizes
- Same in A and B: arrivals, group sizes, and dining-duration pattern

| Variation | Setup summary | Avg wait | Max wait | Groups served | Table utilization | Service level <=15 min | Max queue length |
|---|---|---:|---:|---:|---:|---:|---:|
| A | More 4-seat tables | 16.8 | 57.0 | 10 | 58.2% | 50.0% | 4 |
| B | More 6-seat tables | 22.2 | 64.0 | 10 | 67.26% | 40.0% | 5 |

Suggested interpretation:
- Variation A: More 4-seat tables
- Variation B: More 6-seat tables
- Compare which side lowers wait and queue length without causing poor utilization.
- Because the arrivals are the same, differences come from the restaurant setup rather than demand.

## Overall Summary

| Restaurant | Case study | Better variation | Main trade-off to discuss |
|---|---|---|---|
| Family Dining | Balanced vs Large-Table-Heavy Layout | A | Explain the wait/utilization trade-off seen in the table above. |
| Cafe | Two-Seat Heavy vs Mixed Cafe Layout | B | Explain the wait/utilization trade-off seen in the table above. |
| Fast Food | Limited vs Expanded Rush Capacity | B | Explain the wait/utilization trade-off seen in the table above. |
| Sushi Belt | Solo-Seat Priority vs More Shared Seating | A | Explain the wait/utilization trade-off seen in the table above. |
| KBBQ / Hotpot | Medium-Group Focus vs Large-Group Focus | A | Explain the wait/utilization trade-off seen in the table above. |
