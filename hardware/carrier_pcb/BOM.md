# Carrier PCB — Bill of Materials

## Parts to populate on the carrier PCB

Generated from the schematic (`kicad-cli sch export bom`) — see `bom.csv`
for the raw export.

| Ref | Qty | Description |
|---|---|---|
| J1 | 1 | 1×14 female header, 2.54mm pitch — socket for MSP2807 display main header |
| J2 | 1 | 1×4 female header, 2.54mm pitch — socket for MSP2807 SD header |
| J3 | 1 | 1×3 female header, 2.54mm pitch — socket for 3-pin active buzzer module |
| U1 | 1 | 2×7 female header, 2.54mm pitch — socket for Seeed XIAO ESP32-C5 |

## Mounting hardware

| Qty | Part | Notes |
|---|---|---|
| 4 | M3 screw | Length TBD — case depth is being trimmed once the actual stack height (display + PCB + XIAO/buzzer clearance) is known, see `CLAUDE.md` |
| 4 | M3 standoff or nut (depending on case boss threading) | Matches the case's existing 4 corner bosses — confirm thread/clearance against the physical case before ordering |

## Plug-in modules (not populated on the carrier PCB)

These mate with the sockets above rather than being soldered to the
carrier board. Already documented with sourcing links in the project
[README](../../README.md#hardware) — not duplicated here to avoid two
copies drifting out of sync:

- Seeed Studio XIAO ESP32-C5 → plugs into U1
- MSP2807 2.8" display module → plugs into J1 + J2
- 3-pin active buzzer module → plugs into J3

## Not yet in this BOM

- Future USB-C panel-mount connector + internal pigtail cable (per the
  wire-passthrough cutout already in the case) — not designed yet.
