# Changelog

## 0.1.11 - 2026-09-23

- Generate anti-aliasing from the final morphed SDF using its screen-space
  derivative, keeping the edge width stable through scale and aspect changes.
- Reconstruct a continuous distance field with bilinear SDF sampling before
  measuring that derivative, including corrected and rotated endpoints.
- Center the SDF zero crossing between inside and outside boundary pixels,
  preventing a half-transparent zero-distance band from growing when scaled up.
- Use a smooth cubic coverage curve instead of the previous fixed-width linear
  ramp.
- Remove source partial-alpha blending; the threshold defines geometry and the
  resulting silhouette remains solid apart from its generated anti-aliased edge.

## 0.1.10 - 2026-09-23

- Preserve captured A/B alpha, including semi-transparent and anti-aliased
  edges, while the SDF continues to control morph geometry.
- Reproduce the exact endpoint alpha at 0% and 100%; normalize intermediate
  alpha by endpoint SDF coverage so the middle of the morph does not become a
  simple crossfade or unintentionally thin out.
- Use the nearer endpoint alpha during extrapolation while retaining the
  existing extrapolated SDF geometry.

## 0.1.9 - 2026-09-23

- Apply each endpoint object's scale and aspect ratio to its SDF before
  interpolation, so size and aspect changes participate in the silhouette morph.
- Preserve endpoint center X/Y semantics by carrying scale-around-center movement
  into the corresponding SDF sampling transform.
- Keep the MorphBridge output scale and aspect neutral to avoid applying endpoint
  scaling twice; position, rotation, center, and opacity remain interpolated as
  output-object transforms.
- Retain only the interpolated depth scale on the output transform so center Z
  continues to behave correctly with X/Y rotation.

## 0.1.8 - 2026-09-23

- Rename the correction groups to `前オブジェクト補正` and
  `後オブジェクト補正`, and shorten `アルファしきい値` to `しきい値`.
- Increase correction precision to 0.001 for scale and 0.01 for X, Y,
  rotation, and the silhouette threshold; aspect uses 0.001 precision.
- Expand X/Y to -100000–100000, scale to 0–10000, and aspect to -100–100.
  Scale now uses 100 as its neutral default and interpolates from that value.

## 0.1.7 - 2026-09-22

- Extend signed distance continuously beyond each SDF texture boundary instead
  of substituting the maximum distance, removing straight-edged holes caused by
  corrected sampling coordinates leaving an endpoint texture.

## 0.1.6 - 2026-09-22

- Rebuild transient GPU SDF resources before every draw so AviUtl2 never
  receives a stale image-resource name, eliminating the alternating blank
  frames and `executePixelShader invalid image resource` warnings.
- Expand the SDF canvas automatically around weighted A/B X, Y, scale,
  rotation, and aspect corrections so corrected silhouettes are not clipped.

## 0.1.5 - 2026-09-22

- Rebuild endpoint captures when the edit generation changes, including edits
  that arrive while an asynchronous capture is still running.
- Stop probing warm GPU cache sizes every frame, eliminating repeated invalid
  image-resource warnings while retaining cache-clear recovery.
- Add adaptive SDF padding and bound extrapolated contours to the destination
  silhouette neighborhood to suppress canvas-edge fragments.

## 0.1.4 - 2026-09-22

- Allow Progress easing to extrapolate beyond 0% and 100% in the SDF and
  standard transform interpolation paths.
- Apply A/B correction tracks only to SDF sampling, weighted by Progress and
  one minus Progress respectively, including extrapolation.
- Use image-centered X, Y, scale, rotation, and aspect correction controls with
  Japanese labels and unique namespaced project keys.
- Remove the SDF Scale control and always build the SDF at 100% resolution.

## 0.1.3 - 2026-09-22

- Create all persistent SDF image resources before using them as compute-shader
  targets, preventing `invalid target resource` failures.
- Give B correction controls unique persisted names so A/B values no longer
  produce duplicate project keys.

## 0.1.2 - 2026-09-22

- Upload endpoint images through SDK-supported transient `resource:` names so
  the first SDF build no longer fails with an invalid image resource.
- Skip missing-cache size queries on the first build and rebuild automatically
  after AviUtl2 clears image caches.
- Keep one GPU cache tracker per MorphBridge instance to reuse completed SDFs.

## 0.1.1 - 2026-09-22

- Replaced `FILTER_ITEM_TRACK_GROUP` with compatible grouped track items to
  prevent AviUtl2 from crashing while loading the registered media object.
- Added a registration regression test that rejects the crashing item type.

## 0.1.0 - 2026-09-22

- Added the MorphBridge AviUtl2 media object.
- Added adjacent A/B endpoint discovery and effect-applied asynchronous capture.
- Added automatic signature-based CPU/GPU cache validation.
- Added GPU jump-flood SDF generation and solid-color distance interpolation.
- Added automatic endpoint transform interpolation and A/B correction controls.
