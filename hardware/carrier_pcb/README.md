# Carrier PCB — WORK IN PROGRESS, NOT FINISHED

Do not fabricate this board as-is. This is a first-draft schematic and
footprint layout, not a finished design.

## What's done and verified

- **Schematic** (`carrier_pcb.kicad_sch`) — complete, ERC-clean (0 errors).
  Every net matches the locked pin table in the project's own notes.
- **PCB footprint placement** (`carrier_pcb.kicad_pcb`) — board outline,
  4 mounting holes, and the display's main header position all taken
  directly from the MSP2807 display's own datasheet, not estimated.
  DRC-verified: every pad is on the correct net (ratsnest-correct), 0 real
  errors.

## What's NOT done

- **Routing.** The board currently has zero copper traces — just
  footprints and a verified ratsnest. An automated first attempt at
  routing produced actual electrical shorts between adjacent signals, so
  it was deliberately left undone rather than shipped broken. Route this
  interactively in the KiCad GUI (live DRC, push-and-shove routing) before
  doing anything else with this board.
- **SD header position** (J2) is a visual estimate from the datasheet
  drawing, not a labeled dimension — verify against the physical display
  board before finalizing.
- **Mounting hole diameter** — the datasheet shows both ⌀3.2mm and
  ⌀4.7mm; 3.2mm (M3 clearance) was assumed as the actual hole. Confirm
  against the physical case/display before fabricating.
- **No future USB-C panel connector or pigtail** designed yet — the case
  already has a wire-passthrough cutout reserved for this, but the
  connector and cable routing aren't part of this board yet.

## Fits the case in `../../case/` (also WIP — see its README)
