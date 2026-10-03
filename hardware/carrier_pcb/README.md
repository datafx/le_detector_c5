# Carrier PCB — WORK IN PROGRESS, NOT FINISHED

Do not fabricate this board as-is. This is a first-draft schematic and
footprint layout, not a finished design.

## Mounting: held by the header, not screwed to the case

This board has **no mounting holes**. Earlier drafts gave it the same 4
holes as the display module, so it could bolt to the case's existing
corner bosses — but that only works by stacking both boards on the same
4 screws, which needs standoffs/spacers between them and a case
modification to deepen the bosses. Given how light this assembly is
(PCB + XIAO + buzzer), that's not worth it: the board is instead held
purely by its header engagement with the display board above it (which
keeps screwing to the case exactly as it did before this board existed).
If real-world vibration ever proves that insufficient, a dab of hot glue
between the two boards is the cheap fix — not designed in up front.

Each of the 4 corners is chamfered (6mm legs) instead of square, because
the case's corner bosses (for the display's own screws) occupy that
corner area and this board must not collide with them. The chamfer size
is sized off the display's own datasheet hole positions with clearance
to spare from the nearest header (J1's end pins sit ~4.5mm clear of the
chamfer line) — not measured against the actual boss shape/diameter,
which is unverified. **Test-fit against the physical case before
fabricating.**

## J2 (SD header socket): populated, not wired

J2 is on the board purely so the display module has somewhere to plug
its SD header into for mechanical support — pulling one corner of the
display without it would leave that corner unsupported by anything but
J1. It carries **no electrical connections**: all 4 pins (and U1's
SD_CS pin, which would otherwise drive it) are explicit no-connects.
SD card bring-up (mount, read, CSV config/logging) is a real future
goal, not abandoned, but it's down the road enough that wiring it now
would just be unused complexity - revisit as part of a later PCB
revision, likely alongside touch support.

**Position re-measured and corrected.** The first placement (18.40,
11.00) was a rough visual estimate. Re-measuring the datasheet image
found the estimate was based on an incorrect pixel-to-mm scale (assumed
a literal 1:1 300dpi-to-mm render; the PDF's actual scale, derived from
the two known hole-spacing dimensions, is ~9.05px/mm, not ~11.81).
Corrected position: **(21.15, 4.96)** — the 4 SD pads sit at essentially
the same height as the top mounting holes, not ~11mm down as first
estimated. Pitch was already right (standard 2.54mm). Still a datasheet
*image* measurement, not a direct physical check — reasonably confident
now, but confirm against the real board before fabricating.

## What's done and verified

- **Schematic** (`carrier_pcb.kicad_sch`) — complete, ERC-clean (0 errors).
  Every net matches the locked pin table in the project's own notes.
- **PCB footprint placement** (`carrier_pcb.kicad_pcb`) — board outline
  (chamfered, see above) and the display's main header position taken
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
- **SD header position** (J2) is re-measured and corrected (see above)
  but still a datasheet-image measurement, not a direct physical check —
  verify against the physical display board before finalizing.
- **Board is a standard 2-layer design** (F.Cu + B.Cu, both already
  declared) — nothing needed there. Routing is free to use either layer
  per segment; the two known problem crossings (TFT_SCK/TFT_CS and
  TFT_MOSI/TFT_DC) need a via to hop layers rather than running straight,
  per the routing walkthrough.
- **Corner chamfer vs. the real case bosses** — sized against the
  display's datasheet hole positions, not the actual boss geometry inside
  the case (unverified — see above). Test-fit before fabricating.
- **No future USB-C panel connector or pigtail** designed yet — the case
  already has a wire-passthrough cutout reserved for this, but the
  connector and cable routing aren't part of this board yet.

## Fits the case in `../../case/` (also WIP — see its README)
