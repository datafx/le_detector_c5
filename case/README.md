# Case — WORK IN PROGRESS, NOT FINAL

3D-printable enclosure (Tinkercad export), built and tested with the
display/XIAO/buzzer wired via breadboard jumpers — confirmed everything
fits and works, but this is not the final case.

- `Extended_case.stl` / `Lid.stl` — current working revision.
- **Width and height are final** (matched to the display's own 50×86mm
  board, with the lid already cut with a viewing window for it).
- **Depth will be shortened** once the carrier PCB (see
  `../hardware/carrier_pcb/`, also still WIP) is finished and the actual
  component stack height is known — the current depth has far more
  clearance than needed.
- **GoPro-style mount + USB panel connector cutout added** (2026-10-16)
  — extends the case body by ~23mm in length for the mount tab. Panel
  connector cutout is for the future USB-C passthrough already referenced
  in the project notes (carrier PCB plugs in internally via a pigtail,
  not designed yet — see `../hardware/carrier_pcb/BOM.md`).
