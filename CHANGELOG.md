# Changelog

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
