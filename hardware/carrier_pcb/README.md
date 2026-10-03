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

Each of the 4 corners has a **square notch** cut out (not a diagonal
chamfer), because the case's corner bosses (for the display's own
screws) occupy that corner area and this board must not collide with
them.

**History: chamfer → measured → too small → notch, sized against the
real boss.** The first version used a 6mm diagonal chamfer sized off the
display's datasheet hole margins, with no real measurement of the boss
itself. User printed the board at true 1:1 scale and physically test-fit
it against the case: **not close** — the real boss is a rounded square,
not the slim post the chamfer assumed. Measured it properly off the case
STL with a top-down render, self-calibrated against the model's own
exact bounding box (not an assumed scale, same discipline as the J2
datasheet fix): the boss is roughly **7-8mm across**. Switched from a
diagonal chamfer to a square notch (clears a rounded post with less
material removed, and is simpler to verify by eye) and sized it as large
as the layout allows.

**That maximum is 7.3mm, not the ~8.3mm the measurement suggests is
needed** — capped by J1's own end pads, which sit close enough to the
corners (8.49mm pin-center inset, 0.6mm pad radius) that anything larger
violates the board's 0.5mm copper-to-edge clearance rule. J1 can't move
— its position is fixed by the real display module's own header
location. **This is a real, acknowledged shortfall (~1mm), not yet
resolved** — if 7.3mm still doesn't clear the real boss on test-fit, the
realistic fix is trimming a sliver off the boss itself, not shrinking
the board further.

One more thing the larger notch exposed: it now runs close enough to a
user-routed **TFT_SCK** trace near the bottom-left corner that DRC flags
~0.19mm clearance there (needs 0.5mm) — nudge that trace segment
(around x=7.6, y=83-84) slightly away from the edge in the GUI before
fabricating.

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
  (notched, see above) and the display's main header position taken
  directly from the MSP2807 display's own datasheet, not estimated.
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
- **Corner notch may still be ~1mm short of the real boss** — the layout
  caps it at 7.3mm (J1's pad clearance), the boss measurement suggests
  ~8.3mm is needed. Acknowledged gap, not resolved — see above. Trimming
  the boss is the likely fix if test-fit confirms it's still tight.
- **TFT_SCK trace near the bottom-left notch needs a nudge** — DRC flags
  ~0.19mm clearance (needs 0.5mm) now that the notch is bigger. Quick fix
  in the GUI, not yet done.
- **No future USB-C panel connector or pigtail** designed yet — the case
  already has a wire-passthrough cutout reserved for this, but the
  connector and cable routing aren't part of this board yet.
- **No physical test-fit yet** against the real case/display with the
  *current* (notched) outline — the first chamfered version was test-fit
  and found insufficient, which is what prompted this revision.

## Fits the case in `../../case/` (also WIP — see its README)
