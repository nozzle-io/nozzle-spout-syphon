# Spout runtime research

## Decision

`nozzle-spout` uses the pinned Spout2 `SpoutDX` D3D11 helper rather than a host-provided `SpoutLibrary.dll` probe.

- Spout -> nozzle: `spoutDX::ReceiveTexture()` / `spoutDX::GetSenderTexture()` followed by `nozzle::sender::publish_native_texture(...)`.
- nozzle -> Spout: `nozzle::receiver::acquire_frame(...)`, `nozzle::d3d11::get_texture(...)`, then `spoutDX::SendTexture(...)`.

This is intentionally Windows-only. macOS remains the separate `nozzle-syphon` app.

## Pinned dependency

- Upstream: <https://github.com/leadedge/Spout2>
- Submodule path: `deps/Spout2`
- Commit: `f49e2f469f8cb25f559a6eaa61a3f5b8173fc100`
- Release: `2.007.017`
- Library target: `SpoutDX_static`

## CI coverage boundary

CI validates:

1. Spout2 submodule checkout.
2. `SpoutDX_static` compilation/linkage into `nozzle-spout.exe` on Windows x64.
3. CTest status checks that the bridge is compiled but runtime support remains unclaimed.
4. Package shape, PE/MZ binary check, `--help`, and `--list` execution.

CI does not prove live host interoperability. Real support still requires host smoke evidence for both directions, non-square resolution, flip, channel order, alpha, sender reconnect, and behavior with actual Spout sender/receiver applications.

## Known risks

- `SpoutDX::ReceiveTexture()` and `SpoutDX::SendTexture()` require a working D3D11 desktop environment; GitHub hosted CI only proves build and non-live command execution.
- Spout -> nozzle uses `publish_native_texture(...)`, so do not claim zero-copy.
- nozzle -> Spout uses SpoutDX's internal D3D11 copy path; do not claim zero-copy.
- `Runtime supported: no` is deliberate until host smoke evidence is attached.
