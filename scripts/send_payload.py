#!/usr/bin/env python3
"""Upload one ELF payload to elfldr and return after the bytes are sent."""

import socket
import sys


def main() -> int:
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} HOST PORT PAYLOAD.elf", file=sys.stderr)
        return 2

    host, port_text, path = sys.argv[1:]
    try:
        port = int(port_text)
    except ValueError:
        print(f"Invalid port: {port_text}", file=sys.stderr)
        return 2

    total = 0
    try:
        with open(path, "rb") as payload:
            with socket.create_connection((host, port), timeout=5) as sock:
                sock.settimeout(30)
                while chunk := payload.read(1024 * 1024):
                    sock.sendall(chunk)
                    total += len(chunk)
                sock.shutdown(socket.SHUT_WR)
    except OSError as exc:
        print(f"Upload failed: {exc}", file=sys.stderr)
        return 1

    print(f"Uploaded {total} bytes.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
