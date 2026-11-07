"""Windows real-executable UTF-8 argv regression; no converter-only oracle."""

import argparse
import os
from pathlib import Path
import subprocess
import uuid


def run(executable, *arguments, timeout=15):
    result = subprocess.run(
        [str(executable), *arguments], stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, timeout=timeout,
    )
    return result, result.stdout.decode("utf-8"), result.stderr.decode("utf-8")


def check_invalid_utf16(executable):
    for invalid in ("\ud800", "\udc00"):
        result, out, err = run(executable, invalid)
        if result.returncode != 2 or "Invalid UTF-16 command line" not in err:
            raise AssertionError(f"malformed UTF-16 was not rejected: {result.returncode} {out!r} {err!r}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    args = parser.parse_args()
    if os.name != "nt":
        raise RuntimeError("this regression must launch the Windows executable")
    check_invalid_utf16(args.exe)
    name = "\u65e5\u672c \U0001f680 \"252\"-" + uuid.uuid4().hex[:8]
    result, out, err = run(args.exe, "--source", "name:" + name, "--target", name)
    if result.returncode != 0:
        raise AssertionError(f"status failed: {out!r} {err!r}")
    lines = out.splitlines()
    if "Source: name:" + name not in lines or "Target sender: " + name not in lines:
        raise AssertionError(f"status did not preserve exact names: {out!r}")
    result, out, err = run(args.exe, "--source", "", "--target", name)
    if result.returncode != 0 or "Source: " not in out.splitlines() or "Target sender: " + name not in out.splitlines():
        raise AssertionError(f"empty argument changed argv positions: {out!r} {err!r}")
    print("PASS Windows Unicode source/target status and strict rejection; no Spout host/GPU claim")


if __name__ == "__main__":
    main()
