# nozzle-spout-syphon

`nozzle-spout-syphon` is the planned standalone bridge between [nozzle](https://github.com/nozzle-io/nozzle) and the established live-visual texture-sharing ecosystems:

- macOS: Syphon
- Windows: Spout

This repository is an initial public scaffold. It builds and packages a CLI/app shell with platform-separated Syphon and Spout placeholders, but it does **not** implement runtime texture bridging yet.

## Current status

| Platform | Bridge target | CI package | Runtime bridge | Notes |
| --- | --- | --- | --- | --- |
| macOS | Syphon | Planned by GitHub Actions | Not implemented | Syphon SDK source/version/license still pending Phase 0 approval. |
| Windows | Spout | Planned by GitHub Actions | Not implemented | Spout SDK source/version/license still pending Phase 0 approval. |
| Linux | N/A | N/A | Out of scope | Spout/Syphon do not define the Linux target for this issue. |

Do not read a CI artifact as runtime support. CI currently proves only buildability, package shape, and that the scaffold reports a clear unavailable bridge state.

## Intended modes

The app shell reserves these two bridge directions:

```text
external-to-nozzle  # Syphon/Spout sender -> nozzle sender
nozzle-to-external  # nozzle sender -> Syphon/Spout sender
```

The CLI exposes the product controls required by the roadmap:

- source sender name selection;
- target sender name;
- publish enabled toggle;
- requested resolution display;
- frame-rate/status diagnostics fields;
- explicit error state when the platform bridge is unavailable.

Because runtime SDK integration is not implemented, every run reports `Bridge available: no` and exits successfully after printing diagnostics unless argument parsing fails.

## Build

```sh
git clone --recursive https://github.com/nozzle-io/nozzle-spout-syphon.git
cd nozzle-spout-syphon
cmake -S . -B build -DNOZZLE_SPOUT_SYPHON_BUILD_TESTS=ON
cmake --build build --config RelWithDebInfo
ctest --test-dir build --output-on-failure -C RelWithDebInfo
```

macOS universal CI builds use:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DNOZZLE_SPOUT_SYPHON_BUILD_TESTS=ON
cmake --build build --config RelWithDebInfo
```

Windows x64 CI builds use:

```powershell
cmake -S . -B build -A x64 -DNOZZLE_SPOUT_SYPHON_BUILD_TESTS=ON
cmake --build build --config RelWithDebInfo
ctest --test-dir build --output-on-failure -C RelWithDebInfo
```

Linux configuration intentionally fails because Linux is out of scope for this bridge.

## Usage

```sh
./build/nozzle-spout-syphon --help
./build/nozzle-spout-syphon \
  --mode external-to-nozzle \
  --source syphon_or_spout_sender \
  --target nozzle_bridge \
  --publish on \
  --width 1920 \
  --height 1080
```

The current output is a status report, not a live bridge. Runtime support requires the Phase 0 SDK decision and per-direction smoke evidence.

## Release artifacts

Latest release assets from `main` are expected to use:

```text
nozzle-spout-syphon-latest-<short_sha>-macos.zip
nozzle-spout-syphon-latest-<short_sha>-windows-x64.zip
```

Versioned tag assets use:

```text
nozzle-spout-syphon-<tag>-macos.zip
nozzle-spout-syphon-<tag>-windows-x64.zip
```

The zips contain the app/executable plus `README.md`, `LICENSE`, and `THIRD-PARTY-NOTICES.md`.

## Manual smoke evidence still required

Before this repo claims real bridge support, attach manual evidence for:

1. Syphon app -> bridge -> nozzle-viewer.
2. nozzle-viewer/nozzle-mixer -> bridge -> Syphon receiver.
3. Spout sender -> bridge -> nozzle-viewer.
4. nozzle sender -> bridge -> Spout receiver.
5. Non-square resolution.
6. No vertical flip.
7. No R/B swap.
8. Alpha behavior documented.
9. Sender disconnect/reconnect behavior.

Performance claims must remain direction/platform-specific. Do not claim zero-copy until the exact path is proven.
