# LE Gear Detector — XIAO ESP32-C5 Fork

Passive 5GHz/2.4GHz WiFi + BLE scanner for law-enforcement equipment,
rebuilt for the Seeed XIAO ESP32-C5 to add 5GHz detection. The original
project ([`datafx/le_detector`](https://github.com/datafx/le_detector),
ESP32-WROOM-32U) is 2.4GHz only and can't see the highest-value target
found in the field: Axon Fleet Hub units broadcasting their own hidden APs
on 5GHz. A match signals with a flashing red screen border + buzzer,
faster as the signal strengthens.

## Hardware

| Part | Notes | Link |
|---|---|---|
| Seeed Studio XIAO ESP32-C5 | RISC-V, dual-band WiFi 6 (2.4+5GHz), BLE 5, USB-C | [Seeed Studio](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C5-p-6609.html) |
| 2.8" ILI9341 TFT, SPI, 240×320, with XPT2046 touch + microSD slot | Standard combo board sold under many names; touch and SD are present on the board but not yet used by this firmware | search "2.8 inch ILI9341 SPI touch SD" |
| Active buzzer module, 3-pin (VCC/GND/signal) | Must have its own driver transistor onboard. Confirmed **active-low** on this build — check `BUZZER_ACTIVE_LOW` in `config.h` against yours | search "active buzzer module 3 pin" |
| Alfa APA-M25 dual-band antenna | 8 dBi @2.4GHz / 10 dBi @5GHz, RP-SMA | [Alfa Network](https://alfa-network.eu/apa-m25) |
| U.FL → RP-SMA pigtail | Connects the XIAO's onboard U.FL to the external antenna | search "U.FL RP-SMA pigtail" |
| Breadboard + jumper wires | | |
| USB-C cable | Power + programming | |

The display, buzzer, and pigtail are generic commodity parts sold by many
sellers under many names — there's no single canonical listing worth
linking (and one that works today can vanish tomorrow), so a search term
is given instead of a storefront link.

## Wiring

Pin assignments confirmed against the physical board — see `CLAUDE.md`
decision #6 for the full reasoning, including which pins were deliberately
avoided and why (boot-strap pins, reserved for a future GPS UART, etc.).

| Signal | XIAO pin | GPIO | Notes |
|---|---|---|---|
| Display `VCC` | `5V` | — | needs the 5V/VBUS pin specifically, not `3V3` |
| Display `GND` | `GND` | — | |
| Display `LED` (backlight) | `5V` | — | tied permanently on |
| Display `CS` | `D2` | 25 | |
| Display `DC` | `D3` | 7 | |
| Display `RESET` | `D4` | 23 | |
| Display `SDI`/MOSI | `D10` | 10 | shared SPI bus |
| Display `SCK` | `D8` | 8 | shared SPI bus |
| Display `SDO`/MISO | `D9` | 9 | shared SPI bus |
| `SD_CS` | `D5` | 24 | wired and reserved — SD support isn't built yet |
| `SD_MOSI`/`SD_MISO`/`SD_SCK` | `D10`/`D9`/`D8` | 10/9/8 | shared with the display |
| Buzzer `VCC` | `3V3` | — | |
| Buzzer `GND` | `GND` | — | |
| Buzzer signal | `D0` | 1 | |

Touch (`T_CLK`/`T_CS`/`T_DIN`/`T_DO`/`T_IRQ`) is present on the display
board but intentionally unwired — planned for future bench-configuration
tooling, not for driving controls.

## Build & flash

PlatformIO has no ESP32-C5 support as of this writing — this project
builds via the Arduino IDE / `arduino-cli` instead.

### Arduino IDE

1. **Install the ESP32 board package.** File → Preferences → Additional
   Boards Manager URLs, add:

       https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

   Then Tools → Board → Boards Manager, search "esp32", install **esp32 by
   Espressif Systems**, version **3.3.5 or higher** (built and tested
   against 3.3.11).

2. **Select the board:** Tools → Board → esp32 → **XIAO_ESP32C5**.

3. **Install libraries** via Library Manager: `Adafruit GFX Library`,
   `Adafruit BusIO`, `Adafruit ILI9341`. BLE support is bundled with the
   esp32 core itself — no separate BLE library install needed.

4. **Open the sketch:** `firmware/le_detector_c5/le_detector_c5.ino`.
   Every `.cpp`/`.h` file in that same folder is compiled and linked
   automatically — Arduino sketches don't use PlatformIO's `src`/`include`
   split, so everything lives flat in one folder.

5. **Plug in the board** via USB-C, select its port, and upload.

### arduino-cli

    arduino-cli core update-index
    arduino-cli config set board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    arduino-cli core install esp32:esp32@3.3.11
    arduino-cli lib install "Adafruit GFX Library" "Adafruit ILI9341"
    arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C5 firmware/le_detector_c5
    arduino-cli upload -p /dev/ttyACM0 --fqbn esp32:esp32:XIAO_ESP32C5 firmware/le_detector_c5

### If upload fails, or the board doesn't show up as a serial port

The C5 uses its SoC's own native USB-Serial/JTAG peripheral, not a separate
USB-serial chip — no CH340/CP210x driver needed — but that comes with its
own gotchas, all hit and resolved during this project's own bring-up:

- **Linux: the `cdc_acm` kernel module.** If `/dev/ttyACM0` never appears,
  check `lsmod | grep cdc_acm`; if missing, `sudo modprobe cdc_acm` (if
  *that* fails with "module not found," your running kernel doesn't match
  what's installed on disk — reboot). Your user also needs permission on
  the port: group `dialout` on Debian/Ubuntu, `uucp` on Arch-based distros.
- **Upload fails with "Invalid head of packet" or "Serial data stream
  stopped: possible serial noise or corruption."** Looks like a code or
  board problem, usually isn't — a USB hub between the board and the
  computer was the actual cause here. Plug directly into a root/motherboard
  port before suspecting anything else.
- **No serial monitor output.** `arduino-cli monitor` produced nothing in
  testing even with the board clearly running. On Linux, plain
  `cat /dev/ttyACM0` worked reliably instead, and conveniently triggers a
  fresh reset on open. This firmware doesn't print anything during normal
  operation anyway (the display is the real status output), so this mostly
  matters if you're debugging.

## Watchlist

**62 OUI entries** across law-enforcement radio, body/dash cam, vehicle
router/MDT, radar/lidar, ALPR, and lightbar equipment, plus **6 SSID
patterns** (5 real + one test/demo row). The full list, provenance, and
vendor-collision reasoning live as comments in the source files themselves
— those are what actually runs, so they're the version that can't drift out
of sync with reality:

- [`oui_table.cpp`](firmware/le_detector_c5/oui_table.cpp) — MAC OUI
  watchlist
- [`ssid_table.cpp`](firmware/le_detector_c5/ssid_table.cpp) — SSID
  watchlist (beacons, probe requests, probe responses)

A few vendors are **disabled by default** after field testing showed they
false-positive more than they help — commented out, not deleted, so each
is one edit away from re-enabling if that read ever turns out wrong:
**CradlePoint**, **Peplink**, **Pepwave**, **Vantiva**, **PRO-VISION**.
Comment a row back in (or out) to change what's active for most entries;
CradlePoint specifically is gated by an `EXCLUDE_VENDOR_CRADLEPOINT` toggle
at the top of `oui_table.cpp`. Run `test/test_oui.cpp` after any edit.

Want to test a local addition before submitting it upstream? Drop it in
`user_oui_table.cpp` instead — unsorted, just appended to.

### SSID watchlist

| Pattern | Label | Category |
|---|---|---|
| `PSP-MVR`    | PSP in-car video          | Body/car cam |
| `SP-MVR`     | PSP paired virtual AP     | Other |
| `PSP-TEST`   | PSP facility              | Other |
| `PSPWLAN`    | PSP facility              | Other |
| `PSP_UC`     | PSP facility voice        | Other |
| `LEDET-TEST` | TEST/DEMO — not real gear | Other |

All PSP rows are prefix matches, deliberately — a substring `CONTAINS
"PSP"` rule tested against real survey data matched several unrelated
civilian networks near the same facilities (a pet store, a retailer, a
private residence). The full prefix string, not the shared `PSP` fragment,
is what makes these safe to alert on. Same logic for `SP-MVR` vs.
`PSP-MVR`: kept as two explicit rows rather than one `CONTAINS "SP-MVR"`
rule, which would also match `PSP-MVR` (it contains "SP-MVR" as a
substring).

**Provenance:** confirmed via WiGLE survey data cross-checked against
three independent Pennsylvania State Police barracks in three different
troops, roughly 280km apart, all on matching Cisco hardware.
`PSP-MVR`/`SP-MVR` (Mobile Video Recorder, PSP's in-car video system) and
`PSP-TEST` were confirmed at all three sites; cross-site BSSID correlation
— matching Cisco OUI sub-ranges across independently surveyed locations,
not just a shared naming convention — confirms a single statewide
deployment, not coincidence. `PSPWLAN`/`PSP_UC` were observed at one site
only, so carry lower confidence until cross-checked elsewhere. Exact survey
locations and dates are kept in local, non-public notes rather than this
file.

**Known limitation:** these are fixed APs at barracks buildings, not
vehicle-mounted equipment. A hit on the road means a cruiser's in-car
client is directedly probing for one of these SSIDs while out of range of
the real AP — whether PSP in-car systems actually do that is untested in
the field. Treat a hit here as barracks-proximity, not confirmed mobile
detection, until that's verified.

## How detection works

A three-band phase machine — only one of WiFi-5GHz / WiFi-2.4GHz / BLE can
be up at a time (one radio behind a dual-band diplexer, and it can't do
WiFi and BLE simultaneously either). Scheduling is 5GHz-dominant, based on
real drive data and a measured radio-switch cost on this specific board:

- **5GHz** sweeps continuously (9 non-DFS channels, ~2.7s/sweep) — the
  only long-range, continuous signal (beacons every ~100ms), and the
  primary detection path.
- **BLE** gets a short 400ms dip every 3rd 5GHz sweep, not every cycle —
  its realistic job is a close/stopped encounter lasting several seconds,
  not a sub-second highway pass, so it doesn't need constant attention.
- **2.4GHz** gets a brief 3-channel token check (1/6/11 only) every 8th
  sweep — contributes almost nothing on the road per the drive data (a
  barracks-proximity signal, not a mobile one), kept only as an occasional
  check.

Measured WiFi↔BLE switch cost on this board: **103ms** to switch into BLE,
**9ms** back out, with zero measured variance across repeated runs. See
[`firmware/05_radio_switch_timing/`](firmware/05_radio_switch_timing/) to
re-measure on different hardware. The "every N sweeps" cadence constants
(`BLE_EVERY_N_SWEEPS`, `WIFI_2G_EVERY_N_SWEEPS` in `config.h`) are starting
points to tune empirically, not a measured optimum.

A match's alert shows which band/radio caught it — e.g. "via 5GHz WiFi
OUI" or "SSID beacon (2.4GHz): PSP-MVR" — useful for telling a long-range
hit apart from a close BLE encounter at a glance.

## Known limits

- **One radio, three bands, time-sliced.** Only one of 5GHz WiFi / 2.4GHz
  WiFi / BLE is ever listening — during any other phase, that band is
  invisible. See "How detection works" above for the current schedule.
- **MAC randomisation.** Only public BLE addresses and non-randomised WiFi
  MACs carry a real OUI — randomised addresses are skipped, since their
  vendor bits are meaningless. SSID matching runs independently of this
  and is unaffected.
- **PSP SSID rows are barracks-proximity, not confirmed mobile detection.**
  See SSID watchlist provenance above.
- **SSID matching depends on what's actually broadcast.** A wildcard probe
  or hidden-SSID beacon carries no SSID bytes at all — nothing to match
  either way. Directed probe requests are a declining signal on modern
  phones (randomised MAC + wildcard probes only, roughly iOS/Android 2014-
  2017 onward), so this increasingly only catches older/embedded clients.
  Beacon/probe-response matching is unaffected by that trend.
- **No SD card support yet.** The display board has a microSD slot and the
  select pin (`SD_CS`) is wired, but CSV-based config/logging isn't built —
  the firmware runs entirely on its compiled-in tables for now.

## Tuning

Everything lives in `firmware/le_detector_c5/config.h`: band/channel
lists, sweep cadence, flash-rate endpoints (`RSSI_WEAK`/`RSSI_STRONG`,
`FLASH_PERIOD_SLOW`/`FAST`), alert hold time, and `BUZZER_ENABLED` for
silent bench testing.

## License

GPLv3 (see `LICENSE`), matching the companion `rf_stalker` and
`police_oui_watchlist` projects, and the upstream `le_detector` project
this is forked from.

[`firmware/le_detector_c5/detector.cpp`](firmware/le_detector_c5/detector.cpp)
carries its own `SPDX-License-Identifier: MIT` line for the portions
adapted from nyanBOX — see the header comment in that file for exactly
what was adapted and what changed. Everything else in this repo is GPLv3.
