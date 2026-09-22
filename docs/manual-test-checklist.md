# MorphBridge AviUtl2 smoke-test checklist

Record the environment before checking any item.

- AviUtl2 version: not run
- Windows version: not run
- GPU and driver: not run
- Build/commit: not run

## Loading and basic output

- [ ] AviUtl2 loads `MorphBridge.aux2` without an error.
- [ ] `MorphBridge` appears as a media object, not as an animation effect.
- [ ] Shape -> MorphBridge -> Shape produces a solid silhouette morph.
- [ ] Image -> MorphBridge -> Text produces a solid silhouette morph.
- [ ] Progress 0 matches A and Progress 100 matches B.
- [ ] Alpha threshold and all three SDF scales render successfully.

## Placement and failure handling

- [ ] Missing A gives transparent output and does not crash.
- [ ] Missing B gives transparent output and does not crash.
- [ ] An object on another layer is not selected as A or B.
- [ ] Moving A or B one frame away invalidates the relation as documented.
- [ ] Non-image-producing endpoints fail transparently.

## Cache validation

- [ ] Moving or resizing A rebuilds the matching cache automatically.
- [ ] Editing an effect on B rebuilds the matching cache automatically.
- [ ] Editing an unrelated object does not recapture unchanged A/B endpoints.
- [ ] Rapid edits coalesce without publishing an old result.
- [ ] Undo/redo updates the rendered morph.
- [ ] Scene switching does not reuse another scene's endpoint cache.
- [ ] AviUtl2 cache clear reconstructs GPU data without a manual button.
- [ ] Saving and reopening the project prepares the endpoints again safely.

## Transform behavior

- [ ] A/B position, center, and opacity interpolate from endpoint values.
- [ ] Rotation takes the shortest path across 360/0 degrees.
- [ ] Scale stays positive through the midpoint.
- [ ] A corrections fade out and B corrections fade in with Progress.

## Output

- [ ] A prepared cache renders during final file output.
- [ ] An unprepared cache stays transparent and does not start endpoint capture
      during final file output.
- [ ] Long output does not leak CPU or GPU memory visibly.

## Result

- Overall: **not run**
- Notes: Automated build and host-independent tests do not count as this smoke
  test. Check items only after observing them in AviUtl2.
