# Changelog

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
