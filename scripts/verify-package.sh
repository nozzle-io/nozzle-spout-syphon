#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 2 ]; then
  echo "usage: scripts/verify-package.sh PLATFORM PACKAGE_ROOT_DIR" >&2
  exit 2
fi

platform="$1"
package_root_dir="$2"

if [ ! -d "$package_root_dir" ]; then
  echo "missing package root: $package_root_dir" >&2
  exit 1
fi

test -f "$package_root_dir/README.md"
test -f "$package_root_dir/LICENSE"
test -f "$package_root_dir/THIRD-PARTY-NOTICES.md"

case "$platform" in
  macos)
    app="$package_root_dir/nozzle-spout-syphon.app"
    info_plist="$app/Contents/Info.plist"
    binary="$app/Contents/MacOS/nozzle-spout-syphon"
    test -d "$app"
    test -f "$info_plist"
    test -f "$binary"
    /usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$info_plist" | grep -F 'org.nozzle-io.spout-syphon'
    file "$binary" | tee package-binary-file.txt
    grep -F 'Mach-O' package-binary-file.txt
    codesign --verify --deep --strict --verbose=4 "$app"
    ;;
  *)
    echo "unsupported platform: $platform" >&2
    exit 2
    ;;
esac
