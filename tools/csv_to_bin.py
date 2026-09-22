#!/usr/bin/env python3
"""Convert a captured accelerometer CSV into a raw little-endian int16
binary, for injecting into a running Renode simulation (see
../renode/accel_demo.resc and ../src/accel_synthetic.c).

Row format is identical to csv_to_c_array.py, which embeds a CSV at *build*
time. This script instead produces a raw .bin that ../renode/accel_demo.resc
pokes directly into an already-compiled firmware.elf's synthetic_accel_mg
buffer via `sysbus LoadBinary`, so a single build can replay any captured
CSV (up to the buffer's compiled-in capacity) selected at Renode invocation
time, with no rebuild. Each sample is written as three little-endian int16
values (x, y, z), matching synthetic_accel_mg[i][3] in
src/generated/synthetic_accel_data.h.

Usage:
    python3 tools/csv_to_bin.py capture.csv --output /tmp/injected.bin \\
        --max-samples 100

--max-samples must match (or be smaller than) SYNTHETIC_ACCEL_LEN in
src/generated/synthetic_accel_data.h -- the firmware.elf currently in use
cannot be made to accept more samples than it was built with.
"""

import argparse
import csv
import struct
import sys


def load_samples(path, max_samples):
    samples = []
    with open(path, newline="") as f:
        for row in csv.reader(f):
            if not row or row[0].startswith("#"):
                continue
            if row[0] == "sample_num":
                continue  # header row
            if len(row) < 4:
                continue
            samples.append((int(row[1]), int(row[2]), int(row[3])))
            if max_samples is not None and len(samples) >= max_samples:
                break
    return samples


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("input", help="Captured CSV to convert (as written by tools/uart_capture.py)")
    p.add_argument("--output", required=True, help="Output raw binary .bin path")
    p.add_argument("--max-samples", type=int, default=None,
                    help="Truncate to this many samples (must not exceed the "
                         "target firmware's compiled-in buffer capacity)")
    args = p.parse_args()

    samples = load_samples(args.input, args.max_samples)
    if not samples:
        sys.exit(f"No samples found in {args.input} -- check the file is in "
                  "uart_capture.py's CSV format.")

    with open(args.output, "wb") as f:
        for x, y, z in samples:
            f.write(struct.pack("<3h", x, y, z))

    print(f"Wrote {len(samples)} samples ({len(samples) * 6} bytes) to {args.output}")


if __name__ == "__main__":
    main()
