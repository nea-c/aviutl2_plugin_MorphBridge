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
effects applied. `進捗` is an ordinary AviUtl2 track: leave its first value
at 0, set the second value to 100, and select linear or another standard movement
mode. Easing overshoot is extrapolated instead of clamped. There is intentionally
no automatic-progress switch.

## Controls

- `進捗`: silhouette interpolation, including values outside 0–100.
- `色`: solid output color.
- `しきい値`: converts the captured alpha into a silhouette.
- `前オブジェクト補正` / `後オブジェクト補正`: image-centered X, Y,
  scale, rotation, and aspect tracks. The previous-object correction is weighted
  by Progress; the next-object correction is weighted by one minus Progress.
  These tracks affect SDF sampling only and extrapolate with Progress. Scale is
  an absolute percentage where 100 means no correction.

The SDF is always generated at 100% working resolution.

The endpoint position, center, rotation, scale/aspect, and opacity are read at
the endpoint frames and interpolated automatically. Rotation uses the shortest
path and positive scale uses geometric interpolation.

## Cache behavior

`UPDATE_OBJECT` marks the project generation as changed. On the next render,
MorphBridge compares the complete A/B aliases, endpoint frames, scene size,
alpha threshold, fixed SDF resolution, cache format, and edit generation. It
captures again when either the signature or generation changes. Rapid edits are
coalesced and stale asynchronous results are discarded. CPU endpoint images are
retained, while transient GPU SDF resources are reconstructed before each draw
because AviUtl2 does not preserve those image resources between render calls.

The SDF canvas expands automatically to contain the currently weighted A/B
corrections, including translation, scale, rotation, and aspect changes. When
inverse correction sampling reaches beyond an endpoint SDF texture, its outside
distance is extended continuously from the nearest texture edge.

Extrapolated progress uses an adaptive transparent SDF margin. Beyond 0% or
100%, the contour remains limited to a size-dependent neighborhood around the
corresponding endpoint, preventing distant canvas-edge fragments while keeping
typical elastic overshoot visible.

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
