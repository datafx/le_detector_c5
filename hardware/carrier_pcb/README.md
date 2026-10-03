# Carrier PCB — WORK IN PROGRESS, NOT FINISHED

Do not fabricate this board as-is. Schematic, footprint layout, and
routing are complete and DRC-clean, but nothing has been physically
test-fit against the real case/display yet — see "What's NOT done."

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

**History: chamfer → notch → narrowed board, each one fitting the real
case better than the last.** The first version used a 6mm diagonal
chamfer at each corner, sized off the display's datasheet hole margins
with no real measurement of the boss itself. A 1:1 scale printout
test-fit against the real case showed it wasn't close - the boss is a
rounded square, not the slim post the chamfer assumed. Measured it
properly off the case STL (top-down render, self-calibrated against the
model's own exact bounding box, not an assumed scale): roughly **7-8mm
across**. Switched to a square notch sized as large as the layout
allowed (7.3mm, capped by J1's own pad clearance) - still about 1mm
short of what the boss needed, an acknowledged unresolved gap at the
time.

**Resolved differently: don't dodge the bosses, don't reach them at
all.** User measured the case's actual clear channel between the two
bosses directly: **36.3mm wide** (the board's full 86mm length is fine,
the case was never tight there). The board is now a **plain rectangle,
36.3mm × 86mm**, centered on J1 (which is the single widest feature at
33.02mm) rather than matching the display's full 50mm width. This
sidesteps the boss problem entirely instead of cutting around it - no
corner notches needed, and it also incidentally resolved a DRC
clearance warning on a user-routed TFT_SCK trace that had been sitting
close to the old, bigger notch. J1 keeps 1.04mm of copper-to-edge
clearance on each side (comfortably above the 0.5mm minimum), and J2/U1/
J3 were already well inside this narrower footprint with no changes
needed. Routing (64 track/via segments, hand-routed in the GUI) was
preserved by editing the outline directly in the routed file rather than
regenerating from the script - confirmed via DRC (0 errors) and a
direct coordinate check (no existing trace/via falls outside the new
boundary).

**Still not physically confirmed** - this is a digital fit against the
user's own case measurement, not yet a hands-on test with the actual
narrowed board. Print and test-fit again before fabricating.

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
  (36.3×86mm rectangle, see above) and the display's main header position
  taken directly from the MSP2807 display's own datasheet, not estimated.
  DRC-verified: every pad is on the correct net (ratsnest-correct), 0 real
  errors.
- **Routing.** Routed interactively in the KiCad GUI (push-and-shove,
  live DRC), using vias to hop layers at the two nets that shorted in the
  earlier automated attempt (TFT_SCK/TFT_CS, TFT_MOSI/TFT_DC). **DRC
  clean: 0 errors, 0 unconnected items** — the only warnings left are the
  same cosmetic ones seen throughout this project (missing-library
  notices for the custom footprints, non-mirrored back-layer reference
  text on U1/J3).

## What's NOT done

- **SD header position** (J2) is re-measured and corrected (see above)
  but still a datasheet-image measurement, not a direct physical check —
  verify against the physical display board before finalizing.
- **No future USB-C panel connector or pigtail** designed yet — the case
  already has a wire-passthrough cutout reserved for this, but the
  connector and cable routing aren't part of this board yet.
- **No physical test-fit yet** against the real case/display with the
  *current* (narrowed) outline — sized against the user's own case
  measurement (36.3mm clear channel), not yet confirmed hands-on with
  the actual board.

## Fits the case in `../../case/` (also WIP — see its README)
