# Windows Spout runtime research notes

Scope: issue #123, Spout side only. These notes are based on primary source inspection of `leadedge/Spout2` release `2.007.017` (`f49e2f469f8cb25f559a6eaa61a3f5b8173fc100`).

## Source and license

- Upstream: <https://github.com/leadedge/Spout2>
- Release: `2.007.017`, published 2025-10-22.
- Audited commit: `f49e2f469f8cb25f559a6eaa61a3f5b8173fc100`.
- License in upstream `LICENSE`: BSD 2-Clause.
- Redistribution impact: binary packages must reproduce the BSD 2-Clause notice in documentation or bundled notices.

## Relevant upstream APIs

Spout exposes two plausible integration surfaces:

1. `SPOUTSDK/SpoutDirectX/SpoutDX/SpoutDX.h`
   - `OpenDirectX11(ID3D11Device *pDevice = nullptr)`
   - `SendTexture(ID3D11Texture2D *pTexture)`
   - `ReceiveTexture()` / `ReceiveTexture(ID3D11Texture2D **ppTexture)`
   - `GetSenderTexture()`
   - `GetSenderHandle()`
   - `GetSenderFormat()`
2. `SPOUTSDK/SpoutLibrary/SpoutLibrary.h`
   - DLL factory export: `GetSpout`
   - COM-like C++ interface with OpenGL, D3D11, sender discovery, and format methods.

`SpoutDX` is the cleaner target for a real nozzle bridge because it exposes D3D11 texture methods directly. `SpoutLibrary.dll` is useful for deployment/probing, but its C++ virtual interface is not a stable ABI unless this repo pins and builds the exact SDK version.

## Runtime path assessment

### Spout sender -> nozzle

Preferred path:

1. Use `spoutDX::OpenDirectX11(...)` with an explicit D3D11 device, or let Spout create one only if the resulting device ownership is documented.
2. Select the sender using `SetReceiverName` / sender discovery.
3. Call `ReceiveTexture()` or `ReceiveTexture(ID3D11Texture2D **)`.
4. Inspect `GetSenderWidth`, `GetSenderHeight`, `GetSenderFormat`, `GetSenderFrame`, `GetSenderFps`.
5. Publish to nozzle through the D3D11 path if the texture lifetime and share semantics are compatible; otherwise copy into a nozzle-owned texture/frame and report it as copy-based.

Blocked evidence:

- No host smoke yet proves Spout's returned `ID3D11Texture2D *` can be retained beyond the frame call. Treat it as borrowed until measured.
- No evidence yet proves channel order, origin, alpha, or synchronization behavior.
- No zero-copy claim is justified.

### nozzle -> Spout sender

Preferred path:

1. Acquire or receive a nozzle frame.
2. Convert/copy/wrap into `ID3D11Texture2D` with a Spout-compatible `DXGI_FORMAT`.
3. Call `spoutDX::SendTexture(ID3D11Texture2D *)`.
4. Record whether the transfer is direct shared texture, GPU copy, or CPU copy.

Blocked evidence:

- Current app scaffold has no D3D11 device/context ownership model.
- Current packaging does not vendor or build Spout2.
- Sender name collision behavior is not implemented.

## Current implementation level

The Windows source now performs only a conservative runtime discovery probe:

- attempts to load `SpoutLibrary.dll` on Windows;
- checks for the `GetSpout` export;
- keeps `Bridge available: no` because DLL discovery is not a texture bridge;
- always reports `Runtime supported: no` because no texture transfer path is implemented.

This is deliberately not a runtime bridge. It separates:

- scaffold: CLI/status app exists;
- compile-time integration: pending pinned Spout SDK build/link decision;
- manual host smoke: pending real Spout sender/receiver tests.

## Manual smoke required before support claims

For each direction, test at least `320x240` and `641x479` with a non-symmetric RGBA pattern:

- Spout sender -> bridge -> nozzle-viewer
- nozzle sender -> bridge -> Spout receiver
- no vertical flip
- no red/blue swap
- alpha behavior documented
- sender disconnect/reconnect
- sender-name collision behavior
- frame counter and FPS sanity
