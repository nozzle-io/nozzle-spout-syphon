#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 4 ]; then
  echo "usage: scripts/package-release.sh PLATFORM PACKAGE_NAME PACKAGE_ROOT BUILD_DIR" >&2
  exit 2
fi

platform="$1"
package_name="$2"
package_root="$3"
build_dir="$4"

if [ "$platform" != "macos" ]; then
  echo "unsupported platform for shell packager: $platform" >&2
  exit 2
fi

app_bundle="$(find "$build_dir" -type d -name 'nozzle-spout-syphon.app' | sort | head -n 1)"
if [ -z "$app_bundle" ]; then
  echo "missing nozzle-spout-syphon.app under $build_dir" >&2
  exit 1
fi

rm -rf package "$package_name" package-contents.txt verify-package
mkdir -p "package/$package_root"
cp -R "$app_bundle" "package/$package_root/nozzle-spout-syphon.app"
cp README.md "package/$package_root/README.md"
cp LICENSE "package/$package_root/LICENSE"
cp THIRD-PARTY-NOTICES.md "package/$package_root/THIRD-PARTY-NOTICES.md"

codesign --force --deep --sign - "package/$package_root/nozzle-spout-syphon.app"
scripts/verify-package.sh macos "package/$package_root"

(
  cd package
  zip -r "../$package_name" "$package_root"
)

test -f "$package_name"
unzip -l "$package_name" > package-contents.txt
grep -F "$package_root/nozzle-spout-syphon.app/Contents/MacOS/nozzle-spout-syphon" package-contents.txt
grep -F "$package_root/nozzle-spout-syphon.app/Contents/Info.plist" package-contents.txt
grep -F "$package_root/README.md" package-contents.txt
grep -F "$package_root/LICENSE" package-contents.txt
grep -F "$package_root/THIRD-PARTY-NOTICES.md" package-contents.txt
