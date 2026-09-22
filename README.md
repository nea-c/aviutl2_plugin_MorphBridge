# MorphBridge

MorphBridge is an AviUtl2 2.1.8+ media-object plug-in for morphing the
silhouettes of two adjacent objects with a signed distance field (SDF).
It is Windows x64-only and distributed as one `MorphBridge.aux2` file.

## Placement

Place all three objects on the same layer without another object between them:

```text
A object -> MorphBridge -> B object
```

MorphBridge captures the final frame of A and the first frame of B with their
effects applied. `Progress` is an ordinary AviUtl2 track: leave its first value
at 0, set the second value to 100, and select linear or another standard movement
mode. There is intentionally no automatic-progress switch.

## Controls

- `Progress`: 0–100 silhouette interpolation.
- `Color`: solid output color.
- `Alpha threshold`: converts the captured alpha into a silhouette.
- `SDF scale`: 25%, 50%, or 100% working resolution.
- `A/B Position`, `Center`, `Rotation`, and `Scale / Aspect`: additive endpoint
  corrections. A correction fades out toward B; B correction fades in from A.

The endpoint position, center, rotation, scale/aspect, and opacity are read at
the endpoint frames and interpolated automatically. Rotation uses the shortest
path and positive scale uses geometric interpolation.

## Cache behavior

`UPDATE_OBJECT` marks the project generation as changed. On the next render,
MorphBridge compares the complete A/B aliases, endpoint frames, scene size,
alpha threshold, SDF scale, and cache format. It captures again only when that
signature changed. Rapid edits are coalesced and stale asynchronous results are
discarded. AviUtl2 cache clearing keeps the owned CPU endpoints and reconstructs
missing GPU resources automatically.

## Build

Use a Visual Studio x64 developer environment with CMake 3.28+, Ninja, the
Windows SDK `fxc.exe`, and the vendored AviUtl2 SDK headers.

```powershell
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

For a release package:

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
cmake --build --preset release --target package
```

## v1 limitations

- Output is a single solid-color silhouette; source texture/color blending is
  not included.
- A and B must be image-producing objects on the same layer around MorphBridge.
- Endpoint capture is not started during file output. Prepare the cache once in
  the editor before final output.
- Added effects outside the endpoint object, such as a separate Group Control,
  are outside the v1 capture contract.
- Real AviUtl2/GPU smoke-test results are tracked in
  `docs/manual-test-checklist.md`; an unchecked item is not a claimed pass.
