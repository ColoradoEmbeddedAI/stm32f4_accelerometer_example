# stm32f4_accelerometer_example

Reads the STM32F4 Discovery board's onboard 3-axis MEMS accelerometer over
SPI and streams X/Y/Z acceleration (in mg) over UART, on demand via a
`START <n>` command.

Bare-metal (no HAL/RTOS), hand-written startup code, no vendor
startup files, direct register access via `src/stm32f407.h`. `-nostdlib` is
used (no newlib) since the firmware needs no libc beyond what it implements
itself (`uart_print_int`/`uart_print_hex` in `src/hal.c`).

The accelerometer source is swappable at build time (`ACCEL_SOURCE=real` or
`synthetic`, see "Building" below): `src/accel_real.c` is the real SPI1
driver, `src/accel_synthetic.c` replays a CSV-derived sample array instead,
for running under Renode (which has no model for either accelerometer
chip — see "Renode" below). `src/main.c` and `src/hal.c` are identical
between both builds.

## What's here

`src/accel_real.c` auto-detects which accelerometer chip is fitted to the
board (both variants exist across Discovery board revisions) by reading its
`WHO_AM_I` register over SPI1:

| Chip     | Board revision | `WHO_AM_I` | Output      | Full scale |
|----------|-----------------|------------|-------------|------------|
| LIS3DSH  | MB997C / MB997D | `0x3F`     | 16-bit      | ±2g        |
| LIS302DL | MB997A / MB997B | `0x3B`     | 8-bit       | ±2.3g      |

Board wiring:

| Signal      | Pin  | Notes                              |
|-------------|------|-------------------------------------|
| SPI1_SCK    | PA5  | AF5 (real-hardware build only)       |
| SPI1_MISO   | PA6  | AF5 (real-hardware build only)       |
| SPI1_MOSI   | PA7  | AF5 (real-hardware build only)       |
| CS          | PE3  | GPIO output, active-low (real-hardware build only) |
| USART2_TX   | PA2  | AF7 — connect a USB-UART adapter here |
| USART2_RX   | PA3  | AF7 — connect a USB-UART adapter here (for the `START <n>` command) |
| LED (green) | PD12 | toggles once per reading, heartbeat  |

UART is plain ASCII text at **115200 8N1**, both directions. After a startup
banner and chip ID line, the firmware idles and prints `READY`, then waits
for a `START <n>` command on USART2_RX (PA3) before it captures anything:

```
STM32F4-DISCOVERY IMU demo
Chip: LIS3DSH  WHO_AM_I=0x3F  CTRL_REG4=0x67 OK
Format: X=<mg>  Y=<mg>  Z=<mg>  (200 ms)

READY (chip=LIS3DSH) (Send "START <n>" to capture <n> samples.)
START 5               <- typed by the user; the firmware echoes each
                          character back so it's visible in picocom
START ACK 5 chip=LIS3DSH
X=-988  Y=32  Z=-16 mg
X=-984  Y=36  Z=-8 mg
X=-981  Y=30  Z=-12 mg
X=-990  Y=34  Z=-14 mg
X=-986  Y=33  Z=-10 mg
DONE
READY (chip=LIS3DSH) (Send "START <n>" to capture <n> samples.)   <- idles again
```

Send `START <n>\r\n` (a bare number of samples, e.g. `START 1000`) to trigger
a capture of exactly `<n>` readings at the 200 ms print rate; the firmware
prints `START ACK <n> chip=<CHIP>`, streams `<n>` reading lines, prints
`DONE`, then returns to `READY` and waits for the next command. Both `READY`
and `START ACK` report the detected chip (`LIS3DSH` or `LIS302DL`) so a host
script always knows which sensor it's talking to, even if it connects after
boot and misses the startup banner. An unrecognized line gets
`ERR unrecognized command` and the firmware stays idle. This lets a host
script kick off a bounded, scripted capture instead of free-running forever.

Typed characters are echoed back by the firmware as they're received, so a
terminal with local echo off (picocom's default) still shows what you type.

### Changing the capture/print rate

Two rates are involved, and they're set independently:

- **Sensor output data rate (ODR)** — how fast the chip itself samples, set
  in `accel_init()` in `src/main.c`:
  - LIS3DSH: the low nibble of the `CTRL4` (`0x20`) write, currently
    `0x67` = 100 Hz, X/Y/Z enabled, BDU off. See the ST datasheet for other
    `ODR[3:0]` encodings.
  - LIS302DL: bit 7 of the `CTRL1` (`0x20`) write, currently `0x47` = active
    mode at 100 Hz. LIS302DL only supports 100 Hz or 400 Hz (bit 7).
- **Print/loop rate** — how often `main()`'s loop reads and prints a sample,
  set by the `last_print += 200U;` line near the bottom of `main()`
  (milliseconds, driven by the 1 kHz SysTick). Currently 200 ms (5 Hz).
  This can be lowered independently of the ODR (each SPI read just returns
  the chip's most recent sample), but raising it much above the ODR period
  means printing stale/duplicate readings, since `accel_read_mg()`
  deliberately doesn't poll the `STATUS`/`ZYXDA` bit (see the comment in
  `accel_real.c`'s `accel_read_mg()` — reading `STATUS` on the LIS3DSH
  clears the data-ready flag as a side effect, which this code avoids
  relying on). The synthetic build (see "Renode" below) has no ODR of its
  own — it just hands back the next embedded sample on every call, however
  often `main()` calls it.

## Building

Real hardware (SPI1 accelerometer capture):
```
cmake --preset stm32
cmake --build --preset stm32
```

Renode (CSV-derived synthetic source — see "Renode" below):
```
cmake --preset stm32-renode
cmake --build --preset stm32-renode
```

Both produce `build/<preset>/firmware.elf` / `firmware.bin`.

## Flashing (real hardware)

```
cmake --build --preset stm32 --target flash
```
or directly:
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
    -c "transport select hla_swd" \
    -c "program build/stm32/firmware.bin 0x08000000 verify reset exit"
```

## Running

Connect a USB-UART adapter's RX pin to PA2 and its TX pin to PA3 (and GND to
a board GND) — both directions are needed now, since triggering a capture
requires sending the `START <n>` command to the board. Then read the stream
with any of the following.

### picocom

```
picocom -b 115200 /dev/ttyUSB0
```
- Exit with `Ctrl-A` then `Ctrl-X`.
- Reset the board (or power-cycle it) after opening picocom if you want to
  see the startup banner — picocom doesn't reset the board itself, so if the
  board was already running you'll join the stream mid-way through.
- picocom's own `-b` here is baud rate, not to be confused with the
  firmware's per-sample line format.
- To trigger a capture, type `START 1000` and press Enter (picocom sends
  `\r`, which the firmware's line parser accepts). picocom has local echo
  off by default, but the firmware echoes each character back as it's
  received, so you'll see `START 1000` appear as you type it. The board
  replies `START ACK 1000 chip=<CHIP>`, then streams 1000 readings, prints
  `DONE`, and goes back to `READY` for the next command.

### Python capture tool

`tools/uart_capture.py` sends the `START <n>` command for you (`<n>` =
`--samples`), then reads exactly that many reading lines and writes them to
`--out` as CSV, exiting once the capture completes — useful for
scripted/automated captures where typing into picocom isn't convenient.

**One-time setup** — create a venv in the project and install `pyserial`
into it:
```
python3 -m venv venv
source venv/bin/activate
pip install pyserial
```

**Each run** — activate the venv (if not already active in this shell) and
run the script:
```
source venv/bin/activate
python3 tools/uart_capture.py --out capture.csv --samples 100
```

Both `--out` and `--samples` are required. `--port` defaults to
`/dev/ttyUSB0` (override with `--port /dev/ttyUSB1` etc. if your adapter
enumerates elsewhere — check `dmesg` after plugging it in). Non-reading
lines (`READY`, `START ACK ...`, `DONE`, the startup banner) are printed to
stdout for visibility but are not written to the CSV — only the `<n>`
captured readings are, one row per sample.

The same script also works against a Renode synthetic-accelerometer run
(see "Renode" below) with `--tcp` in place of `--port`:
```
python3 tools/uart_capture.py --tcp localhost:3456 --out capture.csv --samples 20
```

The CSV's first line is a `#`-prefixed comment describing the capture
hardware (board + detected accelerometer chip, taken from the `START ACK`
the firmware sends for this capture), followed by the header row and the
data rows:

```
# device: STM32F4-DISCOVERY board, accelerometer chip: LIS3DSH
sample_num,x,y,z
1,-988,32,-16
2,-984,36,-8
...
```

Load it with pandas using `comment='#'` so the leading comment line is
skipped:

```python
import pandas as pd
df = pd.read_csv('capture.csv', comment='#')
```

## Debugging with GDB

```
# terminal 1: start OpenOCD as a GDB server
cmake --build --preset stm32 --target debug
# (equivalent to: openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "transport select hla_swd")

# terminal 2: connect GDB
arm-none-eabi-gdb build/stm32/firmware.elf
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) break accel_init
(gdb) continue
(gdb) print/x id
```
(`accel_init`/`id` are in `src/accel_real.c` — this only applies to the
`stm32` real-hardware build.)

If the board hangs in `Default_Handler` (e.g. after a bad register write),
inspect the captured fault context:
```
(gdb) info registers r1 r2 r3
```
`r1` = PC at fault, `r2` = LR at fault, `r3` = CFSR. Resolve the faulting
address with:
```
arm-none-eabi-addr2line -e build/stm32/firmware.elf 0x<r1>
```
Common CFSR values: `0x00080000` = NOCP (FPU not enabled — shouldn't happen,
`Reset_Handler` enables it first), `0x00020000` = UNALIGNED, `0x00000001` =
IACCVIOL.

## Renode

The locally installed Renode build (v1.16.1) has no model for either
accelerometer chip (`LIS3DSH`/`LIS302DL`) or any generic SPI sensor — its
`Sensors` namespace only ships `ICP_101xx` (pressure) and `MAX86171`
(biosensor), and its `SPI` namespace only has flash-oriented peripherals
(`NORFlash`, `SFDP`). The SPI1 *controller* itself is modeled
(`SPI.STM32SPI` in `platforms/cpus/stm32f4.repl`), so the STM32 side works —
there's just nothing to answer it as a slave.

Instead, the `stm32-renode` build links `src/accel_synthetic.c` in place of
`src/accel_real.c` — it implements the identical `accel_init()`/
`accel_name()`/`accel_read_mg()` interface (declared in `src/accel.h`), but
replays a CSV-derived X/Y/Z array (`src/generated/synthetic_accel_data.h`)
instead of touching SPI1/GPIOE. `src/main.c` is unchanged between the two
builds, and no SPI hardware is modeled or needed.

**The CSV format is the same one `tools/uart_capture.py` writes** (an
optional `#` comment line, a `sample_num,x,y,z` header, then one row per
sample) — so a capture taken from real hardware can be replayed later under
Renode unmodified, and a Renode run captured with `uart_capture.py --tcp`
(see below) round-trips back into that same format.

The committed default embeds `testdata/sample_capture.csv` (100 samples,
synthetic reference data — a board resting roughly flat, `Z≈1000mg` with a
slow tilt drift and noise; not a real capture) as the buffer's *compiled-in
capacity*. That capacity is fixed at build time (see "Changing the buffer
capacity" below), but which CSV actually plays back is selected separately,
at Renode invocation time, with no rebuild: `synthetic_accel_len` (a plain
non-const global in `src/accel_synthetic.c`) controls the loop point, and
`renode/accel_demo.resc` overwrites both the buffer contents and that
length in-place via `sysbus LoadBinary` / `sysbus WriteDoubleWord` right
after `sysbus LoadELF`, before `start`. This `LoadBinary` call is
unconditional — it runs even if you launch the demo with no overrides at
all (falling back to `renode/default_synthetic_accel.bin`) — so only the
array's compiled-in *capacity* (`SYNTHETIC_ACCEL_LEN`) matters to the
Renode flow; the values baked into `synthetic_accel_data.h` are always
replaced before anything reads them.

The easiest way to use this is the one-command wrapper, which converts a
CSV and launches Renode in one step:
```
python3 tools/run_accel_demo.py --csv /path/to/capture.csv
```
Then, in another terminal:
```
python3 tools/uart_capture.py --tcp localhost:3456 --out out.csv --samples 20
```
`out.csv` should match `/path/to/capture.csv` — the file just injected
above, *not* the compile-time default described earlier (that default's
sample values never survive to be played, as explained above). If
`--samples` asks for more rows than got injected, `accel_synthetic.c` wraps
back to the start instead of erroring, so `out.csv` will contain the
capture followed by it repeating.

To do the same thing by hand instead of via the wrapper (e.g. to reuse a
pre-converted `.bin` across multiple runs):
```
python3 tools/csv_to_bin.py /path/to/capture.csv \
    --output /tmp/injected.bin --max-samples 100
renode --disable-xwt \
    -e '$csv_file = @/tmp/injected.bin' \
    -e '$csv_samples = 100' \
    -e 's @renode/accel_demo.resc'
```
`$csv_samples` must equal the actual number of samples written (i.e. the
`.bin` file size in bytes / 6), not necessarily the full buffer capacity —
a shorter capture just loops sooner. Running `renode renode/accel_demo.resc`
directly with no overrides falls back to the committed default
(`renode/default_synthetic_accel.bin`, a pre-converted copy of the same CSV
baked into flash, kept in sync with `src/generated/synthetic_accel_data.h`).

### Changing the buffer capacity

The 100-sample capacity itself is still a compile-time constant
(`SYNTHETIC_ACCEL_LEN` in `src/generated/synthetic_accel_data.h`) — no CSV
can exceed it without a rebuild. To raise it (e.g. for longer captures):
```
python3 tools/csv_to_c_array.py /path/to/capture.csv \
    --output src/generated/synthetic_accel_data.h
python3 tools/csv_to_bin.py /path/to/capture.csv \
    --output renode/default_synthetic_accel.bin
cmake --build --preset stm32-renode
```
(both commands must point at the same reference CSV so the committed
default and the compiled-in capacity stay consistent). Every CSV played
back afterward, up to the new capacity, still needs no further rebuild.

## Python tools (reference)

All scripts are pure standard library (Python 3); `pyserial` is only needed
for `--port` (real hardware), not `--tcp` (Renode). Run them from the
project root.

| Script | What it does | Usage |
|--------|--------------|-------|
| `tools/uart_capture.py` | Host-side capture (see "Python capture tool" above). Sends `START <n>`, reads exactly `<n>` reading lines from a serial port or a Renode UART/TCP bridge, and writes them to a CSV. | `python3 tools/uart_capture.py --out capture.csv --samples 100`<br>`python3 tools/uart_capture.py --tcp localhost:3456 --out capture.csv --samples 20` |
| `tools/csv_to_c_array.py` | Build-time converter: turns a captured CSV into `src/generated/synthetic_accel_data.h` (a C array of X/Y/Z mg triples) used by `src/accel_synthetic.c` in the Renode build. | `python3 tools/csv_to_c_array.py capture.csv --output src/generated/synthetic_accel_data.h` |
| `tools/csv_to_bin.py` | Runtime converter: same CSV parsing, but emits a raw little-endian `int16` `.bin` (x,y,z interleaved) that `renode/accel_demo.resc` injects into an already-built `firmware.elf` via `sysbus LoadBinary`. `--max-samples` must not exceed `SYNTHETIC_ACCEL_LEN`. | `python3 tools/csv_to_bin.py capture.csv --output /tmp/injected.bin --max-samples 100` |
| `tools/run_accel_demo.py` | One-command wrapper around the two above plus Renode: reads the ELF's buffer capacity, converts `--csv` with `csv_to_bin.py`, and launches `renode/accel_demo.resc` with the injection overrides — no firmware rebuild. | `python3 tools/run_accel_demo.py --csv testdata/sample_capture.csv` |

## Key files

- `src/main.c` — application: USART2 command loop, chip-agnostic capture
  logic. Identical between the real and synthetic builds.
- `src/hal.h` / `src/hal.c` — shared board bring-up (LED, USART2, SysTick)
  and UART helpers, used by both builds.
- `src/accel.h` — accelerometer source interface, implemented by exactly
  one of the following (selected by `ACCEL_SOURCE`, see "Building"):
  - `src/accel_real.c` — real SPI1 driver for the onboard LIS3DSH/LIS302DL
  - `src/accel_synthetic.c` — CSV-derived stand-in for Renode (see "Renode")
- `src/generated/synthetic_accel_data.h` — auto-generated embedded CSV
  data for the synthetic build (do not hand-edit; regenerate with
  `tools/csv_to_c_array.py`)
- `src/stm32f407.h` — hand-written peripheral register map
- `startup.c` — vector table and `Reset_Handler` (FPU enable, `.data`
  copy, `.bss` zero, no libc init — this project is `-nostdlib`)
- `STM32F407VG.ld` — linker script (FLASH/SRAM1/SRAM2/CCM regions)
- `arm-none-eabi.cmake` — Cortex-M4 toolchain file
- `testdata/sample_capture.csv` — committed reference CSV (synthetic, not a
  real capture) used as the Renode build's default embedded dataset
- `renode/stm32f4_discovery_full.repl` — Renode platform description
  (corrected SRAM/flash sizes, `systickFrequency` matched to this
  project's 16 MHz HSI clock)
- `renode/accel_demo.resc` — Renode launch script: loads the firmware,
  injects a CSV-derived sample buffer, and bridges USART2 to TCP port 3456
- `renode/default_synthetic_accel.bin` — pre-converted copy of
  `testdata/sample_capture.csv`, the fallback Renode injects with no
  `-e` overrides
- `tools/uart_capture.py` / `tools/csv_to_c_array.py` / `tools/csv_to_bin.py`
  / `tools/run_accel_demo.py` — host-side tools (see "Python tools" above)
