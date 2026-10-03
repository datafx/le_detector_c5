# Carrier PCB — Bill of Materials

## Parts to populate on the carrier PCB

Generated from the schematic (`kicad-cli sch export bom`) — see `bom.csv`
for the raw export.

| Ref | Qty | Description |
|---|---|---|
| J1 | 1 | 1×14 female header, 2.54mm pitch — socket for MSP2807 display main header |
| J2 | 1 | 1×4 female header, 2.54mm pitch — socket for MSP2807 SD header (**mechanical only, not wired** — see README) |
| J3 | 1 | 1×3 **right-angle (90°)** female header, 2.54mm pitch — socket for 3-pin active buzzer module, lets the buzzer board lie flat against the carrier instead of standing up on pins. **Standoff height not yet chosen** — must clear a USB-C cable plugged into U1, see README. |
| U1 | 1 | 2×7 female header, 2.54mm pitch — socket for Seeed XIAO ESP32-C5 |

## Mounting hardware

None. This board isn't screwed to the case — it's held by the header
engagement with the display board above it, which keeps its own original
4 screws/standoffs into the case's corner bosses unchanged. A dab of hot
glue between the carrier PCB and the display board is the fallback if
testing ever shows the header connection alone isn't enough — not
included here since it's a maybe-needed-later item, not a planned part.

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
