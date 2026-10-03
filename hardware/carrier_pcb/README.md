# Carrier PCB — WORK IN PROGRESS, NOT FINISHED

Do not fabricate this board as-is. Schematic, footprint layout, and
routing are all DRC-clean (0 errors, 0 unconnected items) - routing was
redone from scratch after U1's footprint was corrected (see below), and
solder-side correctness for all 4 connectors was verified directly from
the footprint layer data, not just visually. Still no physical test-fit
of the current board size - see "What's NOT done."

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
the case was never tight there). The board became a **plain rectangle,
36.3mm × 86mm**, centered on J1 (which is the single widest feature at
33.02mm) rather than matching the display's full 50mm width. This
sidesteps the boss problem entirely instead of cutting around it - no
corner notches needed, and it also incidentally resolved a DRC
clearance warning on a user-routed TFT_SCK trace that had been sitting
close to the old, bigger notch. J1 keeps 1.04mm of copper-to-edge
clearance on each side (comfortably above the 0.5mm minimum), and J2/U1/
J3 were already well inside this narrower footprint with no changes
needed.

**Trimmed again: top edge pulled in to put J2 right on the edge too,
mirroring J1.** J1 sits 2.0mm (pin-center-to-edge) from the bottom edge.
J2 was still 4.96mm from the top edge, leaving unused board above it.
Pulled the top edge in to the same 2.0mm margin: new top edge at
**y=2.96** (was y=0), so the board is now **36.3mm × 83.04mm**. J2 gets
the same 1.40mm copper-to-edge clearance J1 already had. Both edits
(width and this height trim) were applied by editing the outline
directly in the routed file rather than regenerating from the script,
each time confirming no existing trace/via fell outside the new
boundary before committing to it - preserved all 64 of the user's
hand-routed segments through both changes, verified via DRC (0 errors)
each time.

**Still not physically confirmed** - this is a digital fit against the
user's own case measurement, not yet a hands-on test with the actual
board at its current size. Print and test-fit again before fabricating.

## U1 (XIAO socket): footprint was wrong, now matches the real device

The original U1 footprint was a generic guess — two rows of 7 pins,
7.62mm apart, pins at `i*2.54mm` within each row — never checked against
the real XIAO ESP32-C5's actual pinout, and not mirrored for back-side
mounting. User caught both problems: wrong footprint, and backwards for
how the device actually gets plugged in (XIAO mounts on the board's
back, opposite the display).

**Fixed using Seeed's own official files**, not another guess:
downloaded their published KiCad footprint
(`XIAO-ESP32-C5-DIP.kicad_mod`) and schematic symbol from the
[XIAO KiCad library](https://wiki.seeedstudio.com/XIAO_BLE/) (same
family covers every XIAO board). The real footprint has **15.24mm**
between the two pin rows, not 7.62mm — a completely different part of
the board was being used. Cross-referenced the symbol to get the
real pin-number-to-signal mapping (D0–D6 on one row, D7–D10 + 3V3_OUT/
GND/VBUS on the other, in that specific order — not a clean split).

**Mirroring**: a part mounted on the back layer needs its footprint
mirrored left-right relative to how it would sit on the front — this is
what KiCad does automatically when you flip a footprint in the GUI, but
since this one was hand-authored as raw coordinates, that mirroring had
to be applied by hand too, and wasn't. Verified the mirror direction
using Seeed's own front and back pinout diagrams for the XIAO (they
publish both specifically because this flip is easy to get backwards) -
confirmed the two diagrams are exact left-right mirrors of each other
before trusting the derivation.

**Routing cleared, not preserved, for this one.** Unlike the earlier
outline-only changes (width, height trims), this moves the pads
themselves, and nearly every net touches U1 directly or routes through
it - preserving any of the old routing wasn't realistic. Pulled all 64
track/via segments rather than leave something uncertain in place.
Board is back to a clean, DRC-verified ratsnest (0 errors, 12
unconnected items, matching the exact pre-routing baseline) - **needs a
full re-route**.

**Slid toward the board's right edge** for USB-C cable clearance, as
far as the *real device's physical body* allows - not just its pads.
The pads alone would have permitted sliding further right, but the
actual XIAO module extends ~2.8mm beyond its own pin span on each side;
checking pads only would have let the real part hang ~1.3mm off the
board edge. Capped at **x=32** (board right edge minus the real
courtyard extent minus a 0.5mm margin), not x=34.

**Flagging, not deciding, one open question**: with this as a straight
slide (no rotation), the XIAO's USB-C port faces *inward* (toward
x≈21-23, the board's open left-center area), not toward the right edge
itself - it's the *blank far end* of the device that now sits near the
right edge. This still frees up real approach space for a cable (the
whole left side of U1's row is now open board), but if the port itself
needs to be at/past the edge, that needs a 180° rotation on top of the
slide, which hasn't been done - flagging it rather than guessing, since
it changes which end of the board needs clearance.

## J3 (buzzer socket): pin order changed, moved left, now a right-angle header

**Pin order**: was VCC/GND/SIG (pins 1/2/3). Changed to **GND/SIG/VCC**
(pins 1/2/3) to match the physical pinout of the actual buzzer module
being used. Schematic symbol's pin names updated to match (so the
symbol doesn't show "pin 1 = VCC" while actually wired to GND) - J3's
physical pad *positions* didn't change for this part, only which net
each pad carries.

**Position**: moved from x=20 to **x=9** (pins span 9.00-14.08), clear
of both the board's left edge (1.55mm pad clearance) and U1's real
device body (6.77mm clear of U1's courtyard at x≈21.45) - U1 having
moved right made room for this.

**Right-angle (90°) header, not straight** - so the buzzer module can
lie flat against the back of the carrier instead of standing up
perpendicular on pins. This is a BOM/part-choice change, not a PCB
layout change: a right-angle header's hole pattern is the same 3 pads
at the same pitch, just a different physical part populated there -
reflected in `BOM.md`, `bom.csv`, and the schematic's own description
field for J3.

**Standoff height: not chosen - a real open question, not a guess I'm
comfortable making.** The buzzer board needs to lie flat without
touching a USB-C cable plugged into U1's port. Getting that number
right needs several real measurements I don't have verified: U1's own
header standoff height (never specified - it's just "a 2x7 female
header" with no particular part/height chosen either), the XIAO's USB-C
receptacle height above its own PCB (datasheet has no Z-axis dimension,
only the 21x17.8mm footprint), and critically, **the actual cable's
plug profile** - which isn't a XIAO spec at all, it varies by which
cable gets used, from slim low-profile plugs to bulky ones. A header
spec sheet can't tell me that part. **Recommend a physical mockup**
once the real parts are in hand (XIAO + a header + the actual cable
likely to be used) rather than trusting a number derived from
incomplete datasheets - or tell me the exact cable/header part numbers
if there's a specific one in mind and I can look up real dimensions for
those two at least.

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

## Silkscreen: functional labels, pin-1 markers, and a real bug fixed

Added on request - before this, silkscreen was just 4 bare reference
designators (J1/J2/J3/U1), nothing else, no graphics at all.

- **Functional labels** on all 4 connectors, stating what they are *and*
  which side of the board the part goes on: `DISPLAY — FRONT` (J1),
  `SD (NC) — FRONT` (J2), `XIAO — BACK` (U1), `BUZZER — BACK` (J3).
- **Pin-1 markers**: pin 1 of every connector is now a square pad
  instead of round (standard convention) - J2 and J3 didn't strictly
  need it since they're asymmetric/small, but did it for all 4 for
  consistency and because this board has already had two real
  orientation bugs this session (U1's mirroring, J3's pin order) - cheap
  insurance against a third one during assembly.
- **Board title**: `LE Detector C5 by datafx`, 2.2mm text (noticeably
  larger than the 1mm connector labels), vertical along the left edge,
  top-justified. Placed on the back (B.SilkS) rather than the front -
  the display covers nearly the full front face once plugged into J1
  (its own board is as tall as this carrier, header near its bottom
  edge), so front silkscreen would be hidden after assembly; the back
  stays mostly open even with U1/J3 populated. Positioned clear of both
  U1 and J3, confirmed via DRC and a true flipped-back render (not just
  the see-through view) before trusting it, per the mirroring lesson
  just below.

**Found and fixed a real bug while verifying this, not a new one
introduced by it: back-layer silkscreen text was never actually
mirrored.** DRC has been flagging `nonmirrored_text_on_back_layer` on
U1's and J3's reference labels since they were first created, and it
was dismissed every single time as the same cosmetic baseline as the
`lib_footprint_issues` warnings - it isn't. Rendered the board with
`kicad-cli`'s `-m` flag (true "physically flip the board and look at
the back" view, not the usual "see-through from the front" renders used
throughout this project) and the text came out genuinely backwards -
`U1`, `J3`, and both new back-side labels all read right-to-left,
garbled. This would have printed wrong on the real board. Fixed by
adding `(justify mirror)` to all 4 back-layer text elements (the 2
existing reference labels plus the 2 new functional labels) - verified
again with the same true-flip render, now reads correctly. **Re-check
any future back-layer text added to this board the same way** (render
with `-m`, don't trust the normal see-through view) - this bug is easy
to reintroduce since hand-authored S-expressions don't get KiCad's
automatic GUI-flip mirroring for free.

## What's done and verified

- **Schematic** (`carrier_pcb.kicad_sch`) — complete, ERC-clean (0 errors).
  Every net matches the locked pin table in the project's own notes.
- **PCB footprint placement** (`carrier_pcb.kicad_pcb`) — board outline
  (36.3×83.04mm rectangle, see above) and the display's main header
  position taken directly from the MSP2807 display's own datasheet, not
  estimated. DRC-verified: every pad is on the correct net
  (ratsnest-correct), 0 real errors.
- **U1 footprint** now matches Seeed's official XIAO-ESP32-C5-DIP
  footprint, correctly mirrored for back-side mounting (see above).
- **Silkscreen** — functional labels + pin-1 markers on all 4
  connectors, back-layer text verified correctly mirrored (see above).
- **Routing** — redone from scratch in the KiCad GUI after U1's
  footprint was corrected. DRC clean: 0 errors, 0 unconnected items.
- **Solder side for all 4 connectors** — verified directly from each
  footprint's stored layer field (not just visually): J1/J2 on `F.Cu`
  (front), U1/J3 on `B.Cu` (back), matching their silkscreen labels.

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
