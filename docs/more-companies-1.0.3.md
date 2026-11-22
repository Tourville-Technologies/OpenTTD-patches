# More Companies 1.0.3

More Companies 1.0.3 is based on JGRPP 0.73.5 and retains the 240-company and RGB company-colour extensions.

## Cargo income and aging

The new **Minimum cargo fare** setting (`economy.cargo_payment_floor`) sets a floor on ordinary cargo income as a percentage of its fresh fare. It defaults to **0**, preserving the standard decay curve and feeder accounting. When enabled, cargo still loses value with transit age until it reaches the floor, so faster service earns more per delivery as well as making more trips.

A 25% floor uses the smallest fixed-point factor above that fraction: 64 out of 255, or approximately 25.1%. Fresh cargo can earn almost four times the minimum. This protects useful base revenue on very large maps without disabling the speed premium.

With the floor enabled, transfer credits use the guaranteed minimum fare at each transfer's distance from the cargo origin. Final delivery receives the remaining base fare plus the speed premium. Credits remain part of the original total fare; transfers do not create additional company income. Forward-moving routes avoid negative credits caused by later aging, including fast early feeders followed by very late delivery. Routes that backtrack toward the origin still use ordinary distance accounting.

NewGRF profit callbacks retain their own payment rules and are excluded from the floor and conservative feeder calculation. Construction costs, operating costs, acceptance, capacity, and station ratings still determine actual route profit.

Recommended settings for the supplied 16k North America scenario are:

| Setting | Value |
| --- | --- |
| Economy speed reduction factor | 7 |
| Timekeeping | Existing calendar mode; about 85 minutes per normal-speed year |
| Cargo aging rate | 14% |
| Minimum cargo fare | 25% |
| Feeder payment share | 50% |
| Infrastructure maintenance | Enabled, linear scaling |
| NATS2 construction and maintenance parameters | Low |
| NARS reduced vehicle costs | Enabled |

These are scenario settings, not new defaults for unrelated games. Use 1.0.3 for the floor and feeder protection; older executables do not implement them. Use matching builds for all players in multiplayer.

Cargo income arithmetic now preserves large distance/count products and inflated rates without 32-bit truncation. The final result saturates only when it cannot fit the game's signed 64-bit money type. Signed NewGRF profit callback results remain signed.

## Vehicle colours

The vehicle palette guard replaces unused magenta indices D7-E2 with the normal primary company shade if they remain in the vehicle's remap table. It preserves legitimate pink company colours, secondary colours, transparency, and deliberate NewGRF recolouring to valid colours.

NARS 2.52 purchase sprites, including the Mogul, Consolidation, Ten-Wheeler, and Express, rendered without unused-magenta pixels in OpenGFX 8.0 tests using both 8bpp-optimized and 40bpp-anim. The exact previously reported screenshot corruption was not reproduced, so this release does not claim to fix every possible source of pink sprites.

## Validation

* Full game suite: 72,961 assertions in 210 test cases passed.
* Native scenario checks: 2,880 payment cases across all 32 NAIS cargo types, distances up to 32,766 tiles, packet sizes up to 65,535 units, and ages up to 65,535 transit periods. Aging reduced fares in 1,908 cases, with no negative payments or minimum-fare violations.
* All 192 tested forward-moving transfer chains had nonnegative credits and final income, including short final legs and very late delivery after fast early feeders.
* The balanced North America scenario retains the original terrain, water, towns, populations, and industry footprints. The game release does not require that scenario or its NewGRFs.
