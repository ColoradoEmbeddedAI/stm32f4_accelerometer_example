#!/usr/bin/env python3
"""Trigger and capture an accelerometer burst from the stm32f4_accelerometer_example
firmware over UART, writing the readings to a CSV file.

The firmware (see src/main.c) idles waiting for a start command:
    READY (chip=LIS3DSH) (Send "START <n>" to capture <n> samples.)
    START 1000                     <- sent by this script
    START ACK 1000 chip=LIS3DSH
    X=-988  Y=32  Z=-16 mg
    X=-984  Y=36  Z=-8 mg
    ...
    DONE

This script opens the serial port (or a Renode UART bridge, with --tcp),
sends "START <samples>\r\n", then reads exactly --samples reading lines and
writes them to --out as CSV. The first line is a "#"-prefixed comment
describing the capture hardware, followed by the header row and one data
row per sample:

    # device: STM32F4-DISCOVERY board, accelerometer chip: LIS3DSH
    sample_num,x,y,z
    1,-988,32,-16
    2,-984,36,-8
    ...

The leading "#" comment line means the CSV loads straight into pandas with:

    import pandas as pd
    df = pd.read_csv('data.csv', comment='#')

Non-reading lines (READY / START ACK / DONE / banner text) are echoed to
stdout for visibility but are not written to the CSV.

Usage:
    python3 tools/uart_capture.py --out capture.csv --samples 1000
    python3 tools/uart_capture.py --port /dev/ttyUSB1 --out capture.csv --samples 500

Against a Renode synthetic-accelerometer run (see ../renode/accel_demo.resc
and ../README.md's "Renode" section) instead of real hardware:
    python3 tools/uart_capture.py --tcp localhost:3456 --out capture.csv --samples 20

A CSV captured this way is itself valid input to tools/csv_to_c_array.py /
tools/csv_to_bin.py, so a real-hardware capture can be replayed later under
Renode.
"""

import argparse
import csv
import re
import socket
import sys


BAUD_RATE = 115200
SAMPLE_LINE_RE = re.compile(r"^X=(-?\d+)\s+Y=(-?\d+)\s+Z=(-?\d+) mg$")
ACK_LINE_RE = re.compile(r"^START ACK (\d+) chip=(\S+)$")


class TcpSerialAdapter:
    """Thin wrapper over a TCP socket exposing the subset of pyserial's
    Serial API this script uses, so the same capture logic works against a
    Renode `CreateServerSocketTerminal` bridge (see ../renode/accel_demo.resc)."""

    def __init__(self, host, port, timeout):
        self._sock = socket.create_connection((host, port), timeout=timeout)
        self._buf = bytearray()
        self.timeout = timeout

    @property
    def timeout(self):
        return self._timeout

    @timeout.setter
    def timeout(self, value):
        self._timeout = value
        self._sock.settimeout(value)

    def readline(self):
        while b"\n" not in self._buf:
            try:
                chunk = self._sock.recv(4096)
            except socket.timeout:
                line, self._buf = bytes(self._buf), bytearray()
                return line
            if not chunk:
                line, self._buf = bytes(self._buf), bytearray()
                return line
            self._buf.extend(chunk)
        idx = self._buf.index(b"\n") + 1
        line = bytes(self._buf[:idx])
        del self._buf[:idx]
        return line

    def write(self, data):
        self._sock.sendall(data)

    def flush(self):
        pass  # sendall() already blocks until written

    def close(self):
        self._sock.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


def parse_args():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--port", default=None,
                   help="Serial port for the USB-UART adapter on PA2/PA3 "
                        "(default if neither --port nor --tcp given: /dev/ttyUSB0)")
    p.add_argument("--tcp", default=None, metavar="HOST:PORT",
                   help="Connect to a Renode UART bridge over TCP instead of a serial "
                        "port, e.g. --tcp localhost:3456 (see ../renode/accel_demo.resc)")
    p.add_argument("--out", required=True,
                   help="CSV output file to write captured samples to (required)")
    p.add_argument("--samples", type=int, required=True,
                   help="Number of accelerometer samples to request and capture (required)")
    return p.parse_args()


def open_transport(args):
    if args.tcp:
        host, _, port_s = args.tcp.partition(":")
        if not port_s:
            sys.exit("--tcp must be HOST:PORT, e.g. --tcp localhost:3456")
        print(f"Connecting to Renode UART bridge at {host}:{port_s} ...", flush=True)
        return TcpSerialAdapter(host, int(port_s), 10)

    port = args.port or "/dev/ttyUSB0"
    try:
        import serial
    except ImportError:
        sys.exit("pyserial not found. Install with:  pip install pyserial")
    print(f"Opening {port} at {BAUD_RATE} baud ...", flush=True)
    try:
        return serial.Serial(port, BAUD_RATE, timeout=10)
    except serial.SerialException as e:
        sys.exit(f"Could not open serial port: {e}")


def start_capture(ser, n_samples):
    cmd = f"START {n_samples}\r\n".encode("ascii")
    ser.write(cmd)
    ser.flush()


def wait_for_ack(ser, n_samples):
    """Reads lines until the firmware's START ACK for this capture arrives,
    returning the accelerometer chip name it reports (e.g. "LIS3DSH").

    Older READY/banner lines sent before this script opened the port may
    have already been lost off the wire, so the chip name is read from the
    ACK for *this* capture rather than the boot banner or READY line."""
    while True:
        raw = ser.readline()
        if not raw:
            sys.exit("\nSerial read timed out waiting for START ACK.\n"
                     "  - Is the board powered and flashed with this firmware (or the "
                     "Renode demo running)?\n"
                     "  - Is --port/--tcp correct? (check `dmesg` after plugging in the "
                     "USB-UART adapter)")
        line = raw.decode("ascii", errors="replace").rstrip("\r\n")
        print(line, flush=True)
        m = ACK_LINE_RE.match(line)
        if m and int(m.group(1)) == n_samples:
            return m.group(2)


def capture(ser, n_samples, csv_writer):
    samples_seen = 0
    while samples_seen < n_samples:
        raw = ser.readline()
        if not raw:
            sys.exit(f"\nSerial read timed out after {samples_seen} / {n_samples} samples.\n"
                     "  - Is the board powered and flashed with this firmware (or the "
                     "Renode demo running)?\n"
                     "  - Is --port/--tcp correct? (check `dmesg` after plugging in the "
                     "USB-UART adapter)")
        line = raw.decode("ascii", errors="replace").rstrip("\r\n")
        m = SAMPLE_LINE_RE.match(line)
        if not m:
            print(line, flush=True)
            continue
        samples_seen += 1
        x, y, z = m.group(1), m.group(2), m.group(3)
        print(line, flush=True)
        csv_writer.writerow([samples_seen, x, y, z])


def main():
    args = parse_args()

    with open_transport(args) as ser, open(args.out, "w", newline="") as out_f:
        start_capture(ser, args.samples)
        chip = wait_for_ack(ser, args.samples)
        out_f.write(f"# device: STM32F4-DISCOVERY board, accelerometer chip: {chip}\n")
        writer = csv.writer(out_f)
        writer.writerow(["sample_num", "x", "y", "z"])
        capture(ser, args.samples, writer)

    print(f"\nCaptured {args.samples} samples -> {args.out}")
    print(f"Read with pandas:  pd.read_csv({args.out!r}, comment='#')")


if __name__ == "__main__":
    main()
