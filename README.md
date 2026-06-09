# nozzle-spout-syphon

`nozzle-spout-syphon` is the repository for standalone bridges between [nozzle](https://github.com/nozzle-io/nozzle) and established live-visual texture-sharing ecosystems.

The deliverables are intentionally split by platform instead of pretending that one app name fits both runtimes:

- macOS app: `nozzle-syphon`
- Windows executable: `nozzle-spout`

## Current status

| Platform | Bridge target | CI package | Runtime bridge | Notes |
| --- | --- | --- | --- | --- |
| macOS | Syphon | `nozzle-syphon` app zip | Implemented but not support-claimed | Builds against Syphon Framework commit `71351d4b484cd2d1917867f7846a5cdca724552d`; the diagnostics intentionally keep `Runtime supported: no` until host smoke evidence is attached. |
| Windows | Spout | `nozzle-spout.exe` zip | Implemented but not support-claimed | Builds and links the pinned Spout2 `SpoutDX` D3D11 path at `f49e2f469f8cb25f559a6eaa61a3f5b8173fc100`; diagnostics keep `Runtime supported: no` until host smoke evidence is attached. |
| Linux | N/A | N/A | Out of scope | Spout/Syphon do not define the Linux target for this issue. |

Do not read a CI artifact as end-user runtime proof. CI proves buildability, package shape, and static behavior. Manual host smoke tests still decide whether a direction/platform is actually usable.

## Bridge modes

```text
external-to-nozzle  # Syphon/Spout sender -> nozzle sender
nozzle-to-external  # nozzle sender -> Syphon/Spout sender
```

Common CLI controls:

- `--mode external-to-nozzle|nozzle-to-external`
- `--source SELECTOR`
- `--target NAME`
- `--publish on|off` (`--publish off` is diagnostics-only and is rejected with `--run`)
- `--width N --height N` for diagnostics
- `--list` to list visible platform sources
- `--run` to run the platform bridge
- `--frames N` where `0` means run until stopped
- `--timeout-ms N`
- `--idle-sleep-ms N`

External source selection is explicit and deterministic:

- `--source default` is a convenience selector. It picks the first visible source after deterministic sorting by displayed source fields. This is useful for quick local smoke, not a production-grade stable identity.
- `--source name:<exact-name>` selects an exact Syphon server name or Spout sender name.
- `--source app:<exact-app>` selects an exact Syphon app name. Spout currently exposes sender names only in this bridge, so `app:` is rejected on Windows instead of guessing.
- `--source value` is preserved for compatibility and is treated as `name:value`.
- Missing sources fail with a clear selector error.
- Duplicate `name:` or `app:` matches fail as ambiguous and list the matching candidates. The bridge does not silently choose the first duplicate.
- With `--run`, missing `external-to-nozzle` sources are polled until `--timeout-ms` before failing. Ambiguous or unsupported selectors fail immediately.
- Once a source has been selected, the bridge keeps that selected target sticky. It does not re-run `default` selection every frame and does not silently switch to a different source when the visible source list changes.
- If the selected source disappears while running, the current bridge waits on the selected target path until the existing frame/no-frame timeout logic fails; it does not auto-select another source.

Bare Syphon app-name matching from older builds is no longer claimed. Use `app:<exact-app>` for app-name selection.

`--list` prints quoted selector evidence. Values are double-quoted with `"` and `\` escaped; pass the selector value to the shell as one argument, for example `--source "name:Main Output"`.

Example Syphon output:

```text
Syphon app="Resolume Arena" name="Main Output" index=0 selector="name:Main Output" name_ambiguous=no selector="app:Resolume Arena" app_ambiguous=no
```

Example Spout output:

```text
Spout name="Spout Sender" index=0 selector="name:Spout Sender" name_ambiguous=no
```

If two Syphon servers share the same server name, `--list` marks `name_ambiguous=yes`; selecting `name:<that-name>` then fails instead of hiding the ambiguity. If two Syphon servers share an app name, `app_ambiguous=yes` is reported and `app:<that-app>` fails.

## Build

```sh
git clone --recursive https://github.com/nozzle-io/nozzle-spout-syphon.git
cd nozzle-spout-syphon
```

### macOS universal `nozzle-syphon`

Syphon Framework must be built before CMake configuration:

```sh
xcodebuild \
  -project deps/Syphon-Framework/Syphon.xcodeproj \
  -scheme Syphon \
  -configuration Release \
  -derivedDataPath build/syphon-derived \
  CODE_SIGNING_ALLOWED=NO \
  MACOSX_DEPLOYMENT_TARGET=12.0 \
  ARCHS="arm64 x86_64" \
  ONLY_ACTIVE_ARCH=NO \
  build

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DNOZZLE_SPOUT_SYPHON_BUILD_TESTS=ON
cmake --build build --config RelWithDebInfo
ctest --test-dir build --output-on-failure -C RelWithDebInfo
```

### Windows x64 `nozzle-spout`

```powershell
cmake -S . -B build -A x64 -DNOZZLE_SPOUT_SYPHON_BUILD_TESTS=ON
cmake --build build --config RelWithDebInfo
ctest --test-dir build --output-on-failure -C RelWithDebInfo
```

Linux configuration intentionally fails because Linux is out of scope for this bridge.

## Usage

Diagnostics:

```sh
./build/nozzle-syphon.app/Contents/MacOS/nozzle-syphon --help
./build/nozzle-syphon.app/Contents/MacOS/nozzle-syphon --list
```

Syphon sender to nozzle sender. The named Syphon source must already be visible; missing-source retry is not claimed yet:

```sh
./build/nozzle-syphon.app/Contents/MacOS/nozzle-syphon \
  --mode external-to-nozzle \
  --source name:syphon_sender_name \
  --target nozzle_bridge \
  --run
```

nozzle sender to Syphon sender. The named nozzle source must already exist before startup; missing-source retry is not claimed yet:

```sh
./build/nozzle-syphon.app/Contents/MacOS/nozzle-syphon \
  --mode nozzle-to-external \
  --source nozzle_sender_name \
  --target syphon_bridge \
  --run
```

Windows builds and links the SpoutDX D3D11 bridge. `--list` enumerates visible Spout senders. `--run` attempts the compiled bridge path, but production runtime support is not claimed until host smoke evidence exists.

## Release artifacts

Latest release assets from `main` use product-specific names:

```text
nozzle-syphon-latest-<short_sha>-macos.zip
nozzle-spout-latest-<short_sha>-windows-x64.zip
```

Versioned tag assets use:

```text
nozzle-syphon-<tag>-macos.zip
nozzle-spout-<tag>-windows-x64.zip
```

The zips contain the app/executable plus `README.md`, `LICENSE`, and `THIRD-PARTY-NOTICES.md`. The macOS zip also embeds `Syphon.framework` inside `nozzle-syphon.app/Contents/Frameworks`.

## Manual smoke evidence still required

Before this repo claims production bridge support, attach evidence for:

1. Syphon app -> `nozzle-syphon` -> nozzle-viewer.
2. nozzle-viewer/nozzle-mixer -> `nozzle-syphon` -> Syphon receiver.
3. Spout sender -> `nozzle-spout` -> nozzle-viewer.
4. nozzle sender -> `nozzle-spout` -> Spout receiver.
5. Non-square resolution.
6. No vertical flip.
7. No R/B swap.
8. Alpha behavior documented.
9. Sender disconnect/reconnect behavior.

Performance claims must remain direction/platform-specific. Do not claim zero-copy until the exact path is proven. Current Windows CI proves Spout2 API compilation/linkage and package shape, not live GPU interop.
