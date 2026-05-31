# Syphon runtime research

## Decision

`nozzle-syphon` uses Syphon Framework's Metal API:

- `SyphonMetalClient` for Syphon -> nozzle.
- `SyphonMetalServer` for nozzle -> Syphon.
- `nozzle::sender::publish_native_texture(...)` for Syphon texture snapshot publication into nozzle.
- `nozzle::receiver::acquire_frame(...)` plus `nozzle::metal::get_texture(...)` for nozzle frame publication into Syphon.

The macOS app is separate from the Windows app. Keeping a single executable name would hide platform-specific runtime constraints and packaging requirements.

## Pinned dependency

- Upstream: <https://github.com/Syphon/Syphon-Framework>
- Submodule path: `deps/Syphon-Framework`
- Commit: `71351d4b484cd2d1917867f7846a5cdca724552d`

Tag `5` was rejected for this integration because it does not expose the `SyphonMetalClient` / `SyphonMetalServer` API needed for the Metal runtime path. Building the historical tag also hits obsolete deployment-target friction on current Xcode. The pinned upstream commit builds as a universal framework with `MACOSX_DEPLOYMENT_TARGET=12.0`.

## CI coverage boundary

CI validates:

1. Syphon Framework universal build.
2. `nozzle-syphon.app` universal CMake build.
3. CTest shape/status checks.
4. Package shape, embedded `Syphon.framework`, bundle id, and ad-hoc codesign verification.

CI does not prove live host interoperability. Real support still requires host smoke evidence for both directions, non-square resolution, flip, channel order, alpha, and reconnect behavior.

## Known risks

- The diagnostics intentionally report `Runtime supported: no` until host smoke evidence is attached, even though the bridge code is compiled.
- Syphon -> nozzle currently uses `publish_native_texture(...)`, which snapshots into nozzle-owned storage. Do not claim zero-copy.
- nozzle -> Syphon publishes the acquired Metal texture into `SyphonMetalServer` using a command buffer and waits for completion. This is conservative, not tuned.
- Source matching accepts Syphon server name or app name; duplicate names are not disambiguated yet.
- Source retry-before-create is not claimed yet; start with the source already visible.
- `frame_limit=0` means run until externally stopped.
