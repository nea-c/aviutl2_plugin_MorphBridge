# MorphBridge Design

## Goal

MorphBridge is an AviUtl ExEdit2 media-object plug-in placed on one timeline layer as `A -> MorphBridge -> B`. It renders a solid-color silhouette whose shape changes from the final frame of A to the first frame of B by interpolating signed-distance fields (SDFs).

The v1 deliverable is a single `MorphBridge.aux2`. It does not require `.obj2`, `.mod2`, or `.auf2` companion files.

## User workflow

1. Place visual object A, MorphBridge, and visual object B consecutively on the same layer.
2. Set MorphBridge's progress track from 0 to 100 using any AviUtl2 movement mode. A newly placed MorphBridge starts at the fixed value 0.
3. MorphBridge finds A and B automatically and displays the interpolated silhouette.
4. Editing either endpoint invalidates only the affected cached result after a lightweight signature check.

If either neighbor is missing or cannot be rendered, MorphBridge outputs a transparent 1x1 image and writes a rate-limited diagnostic log entry.

## Packaging and registration

`MorphBridge.aux2` exports the general plug-in entry point and registers a `FILTER_PLUGIN_TABLE` through `HOST_APP_TABLE::register_filter_plugin()`.

The filter flags are:

- `FLAG_VIDEO`: MorphBridge produces video.
- `FLAG_INPUT`: MorphBridge appears as a timeline media object.
- `FLAG_USERDATA`: each MorphBridge instance owns independent cache state.

The general plug-in also creates one `EDIT_HANDLE`, registers for `UPDATE_OBJECT`, `CHANGE_EDIT_SCENE`, and host cache-clear notifications, and owns the asynchronous endpoint-render queue.

## Neighbor resolution

During MorphBridge video processing, its current layer and global start/end frames come from `OBJECT_INFO`.

- A is the object returned at `bridge_start - 1`. It must be on the same layer and end before the bridge starts.
- B is searched at `bridge_end` and `bridge_end + 1` to tolerate AviUtl2's observed one-frame placement gap. It must be on the same layer and start no later than `bridge_end + 1`.
- MorphBridge itself is rejected as either endpoint.

The resolver returns endpoint handles only for the duration of the supplied `EDIT_SECTION`. Handles are never dereferenced later or used as persistent identity.

## Endpoint capture

For A, request `EDIT_HANDLE::rendering_object_video(A, A_end, true, ...)`. For B, request `rendering_object_video(B, B_start, true, ...)`.

The API is asynchronous. The callback immediately copies the returned premultiplied RGBA rows using the supplied width, height, and pitch. It never retains the host buffer pointer.

No endpoint request is issued from final-output processing. Endpoint preparation occurs during editing. If an output frame reaches an unprepared cache, v1 returns transparent output and logs one warning per MorphBridge instance rather than blocking or starting recursive rendering.

## Cache invalidation

`UPDATE_OBJECT` provides no changed-object identifier. Its listener therefore only increments a global atomic edit generation. It does not immediately erase caches or call editing APIs from the event-notification thread.

Each MorphBridge instance stores the last generation it inspected and an endpoint signature. On its next editable render after the generation changes, it resolves A/B again and computes a signature from:

- scene ID, width, and height;
- A and B layer/start/end spans;
- A and B endpoint frame numbers;
- hashes of both complete object aliases;
- alpha threshold and SDF resolution settings;
- plug-in cache-format version.

If the signature is unchanged, the existing SDFs remain valid and the instance only advances its observed generation. If it changed, the instance marks its cache stale and coalesces new A/B capture requests. Only one preparation job per instance runs at a time. If another update arrives during preparation, the completed result is accepted only when its signature still matches; otherwise one new job is queued for the newest signature.

Scene changes and explicit host cache clearing discard all instance caches.

## Silhouette and SDF generation

The endpoint alpha channel defines the silhouette. Pixels with alpha greater than or equal to the configurable threshold are inside.

Each endpoint is centered in a shared object-local canvas large enough for both endpoint images plus a small distance-field margin. Endpoint metadata retains independent sampling transforms so later image-alignment controls can be added without changing the cache format's public interface.

SDF generation runs on the GPU through AviUtl2's compute-shader resource API:

1. Convert endpoint alpha into an inside mask and boundary-seed texture.
2. Run jump-flood passes from the largest power-of-two step down to one pixel.
3. Convert nearest-boundary coordinates and the inside mask to signed distance.
4. Store A and B fields in cache resources.

The interpolated field is `lerp(sdf_a, sdf_b, progress)`. The output alpha is derived from a one-pixel antialiased transition around distance zero and multiplied by the selected color's alpha. RGB is emitted premultiplied by alpha.

## Standard transform interpolation

MorphBridge reads the following `標準描画` tracks from A at A's final frame and B at B's first frame:

- X, Y, and Z;
- center X, Y, and Z;
- X-, Y-, and Z-axis rotation;
- scale and aspect ratio;
- opacity.

The A-side correction values are added to A's endpoint values, and the B-side correction values are added to B's endpoint values. MorphBridge then interpolates the corrected endpoint values using the same progress `p` used for the SDF morph.

- Position and center are interpolated linearly.
- Each rotation uses the shortest signed angular delta.
- Scale is converted to a positive multiplicative factor and interpolated geometrically; invalid or zero values are clamped to a small positive factor.
- Aspect ratio is converted to independent X/Y scale before interpolation.
- Opacity is interpolated linearly and multiplies the output silhouette alpha.

The interpolated result is written to `OBJECT_IMAGE_PARAM`, so AviUtl2 applies the complete output transform after the object-local SDF morph. Algebraically, A corrections have weight `1-p` and B corrections have weight `p`: at progress 0 only the corrected A transform is present, at 50 both endpoint corrections contribute equally, and at 100 only the corrected B transform is present.

## Controls

v1 exposes:

- `Progress`: track, 0 to 100, fixed default 0. Users select the desired AviUtl2 movement mode and endpoint value.
- `Color`: solid silhouette color, default white.
- `Alpha threshold`: numeric value from 0 to 100, default 50.
- `SDF scale`: selection of 25%, 50%, or 100%, default 50%, controlling preparation cost and detail.
- `A position correction`: grouped X/Y/Z tracks, default 0.
- `A center correction`: grouped X/Y/Z tracks, default 0.
- `A rotation correction`: grouped X/Y/Z tracks, default 0 degrees.
- `A scale/aspect correction`: grouped scale/aspect tracks, default 0 percentage points.
- `B position correction`: grouped X/Y/Z tracks, default 0.
- `B center correction`: grouped X/Y/Z tracks, default 0.
- `B rotation correction`: grouped X/Y/Z tracks, default 0 degrees.
- `B scale/aspect correction`: grouped scale/aspect tracks, default 0 percentage points.

Cache rebuilding is completely automatic. v1 exposes neither a manual rebuild button nor a status field.

## Threading and lifetime

- Event listeners only update atomics.
- SDK edit reads occur only inside valid edit/read sections.
- Endpoint callbacks copy pixels and publish immutable results under instance synchronization.
- GPU resource creation and shader execution occur only inside `FILTER_PROC_VIDEO` processing.
- Instance state is created and destroyed with `FLAG_USERDATA`; outstanding callbacks hold safe shared job state and never access a destroyed instance directly.

## Error handling

Expected errors include missing neighbors, unsupported/nonvisual endpoints, rejected asynchronous requests, invalid callback geometry, shader/resource failure, and stale results. Errors do not crash or block AviUtl2. MorphBridge produces transparent output and emits rate-limited diagnostic logging.

## Testing

Host-independent C++ tests cover:

- preceding/following neighbor boundary rules;
- signature equality and inequality;
- global generation handling without unnecessary invalidation;
- preparation coalescing and stale-result rejection;
- shared-canvas placement and sampling-transform math;
- standard-transform endpoint correction, shortest-angle interpolation, and geometric scale interpolation;
- SDF interpolation sign behavior and alpha conversion using a CPU reference.

SDK-boundary tests use small fake tables to verify registration, callback copying with non-tight pitch, event handling, and transparent fallback. Shader sources are compiled during the build, and a manual AviUtl2 checklist verifies shape/image/text endpoints, endpoint edits, scrubbing, undo/redo, project reload, and final output with prepared caches.

## v1 non-goals

- Color or texture interpolation from A/B.
- Path or semantic correspondence beyond SDF interpolation.
- Audio morphing.
- Cross-layer endpoint selection.
- Group-control additions that the object-render API does not include.
- Starting asynchronous endpoint renders during final output.
- Perspective-aware independent A/B deformation inside the SDF canvas; v1 interpolates endpoint transforms on the completed morph image through `OBJECT_IMAGE_PARAM`.
