#!/usr/bin/env python3
"""One-command wrapper: convert a captured CSV and launch the Renode
stm32f4_accelerometer_example demo replaying it -- no firmware rebuild
required.

Runs entirely against the already-built build/stm32-renode/firmware.elf
(see ../CMakeLists.txt's ACCEL_SOURCE=synthetic preset). This script:
  1. Reads that ELF's synthetic_accel_mg buffer capacity (its compiled-in
     maximum, currently 100 samples -- see
     ../src/generated/synthetic_accel_data.h).
  2. Converts --csv to the raw injection format via csv_to_bin.py,
     truncated to that capacity.
  3. Launches Renode with renode/accel_demo.resc, overriding $csv_file/
     $csv_samples so the injected data replaces the compiled-in default for
     this run only.

Usage:
    python3 tools/run_accel_demo.py --csv testdata/sample_capture.csv

Then, in another terminal, same as running accel_demo.resc by hand:
    python3 tools/uart_capture.py --tcp localhost:3456 --out capture.csv --samples 20
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_CSV = PROJECT_ROOT / "testdata" / "sample_capture.csv"
FIRMWARE_ELF = PROJECT_ROOT / "build" / "stm32-renode" / "firmware.elf"
DEMO_RESC = PROJECT_ROOT / "renode" / "accel_demo.resc"


def find_renode():
    renode = shutil.which("renode")
    if renode:
        return renode
    sys.exit("renode not found on PATH -- install it or add it to PATH first.")


def get_capacity(elf_path):
    """Read synthetic_accel_mg's compiled-in size (samples) from the ELF
    symbol table, so this script always matches whatever capacity the
    current build was compiled with -- no hardcoded assumption."""
    readelf = shutil.which("arm-none-eabi-readelf")
    if not readelf:
        sys.exit("arm-none-eabi-readelf not found on PATH.")
    out = subprocess.check_output([readelf, "-s", str(elf_path)], text=True)
    for line in out.splitlines():
        if "synthetic_accel_mg" in line and "synthetic_accel_len" not in line:
            fields = line.split()
            size_bytes = int(fields[2], 16)
            return size_bytes // 6  # 3 int16 (x,y,z) per sample
    sys.exit("Could not find synthetic_accel_mg symbol in firmware.elf -- "
              "was it built with the stm32-renode preset?")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--csv", default=str(DEFAULT_CSV),
                    help="Captured CSV to replay (default: testdata/sample_capture.csv)")
    p.add_argument("--elf", default=str(FIRMWARE_ELF),
                    help="firmware.elf to run (default: build/stm32-renode/firmware.elf)")
    args = p.parse_args()

    elf_path = Path(args.elf)
    if not elf_path.exists():
        sys.exit(f"{elf_path} not found -- build it first:\n"
                  f"  cmake --preset stm32-renode && cmake --build --preset stm32-renode")

    renode = find_renode()
    max_samples = get_capacity(elf_path)

    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as tmp:
        bin_path = Path(tmp.name)

    convert_script = Path(__file__).resolve().parent / "csv_to_bin.py"
    subprocess.check_call([
        sys.executable, str(convert_script), args.csv,
        "--output", str(bin_path),
        "--max-samples", str(max_samples),
    ])
    n_samples = bin_path.stat().st_size // 6
    if n_samples < max_samples:
        print(f"Note: {args.csv} has only {n_samples} samples "
              f"(buffer capacity is {max_samples}) -- it will loop.")

    print(f"Launching Renode with {args.csv} ({n_samples} samples)...")
    cmd = [
        renode, "--disable-xwt",
        "-e", f"$bin = @{elf_path}",
        "-e", f"$csv_file = @{bin_path}",
        "-e", f"$csv_samples = {n_samples}",
        "-e", f"s @{DEMO_RESC}",
    ]
    try:
        subprocess.run(cmd, cwd=str(PROJECT_ROOT))
    finally:
        bin_path.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
