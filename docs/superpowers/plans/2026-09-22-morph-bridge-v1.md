# MorphBridge v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a single AviUtl ExEdit2 `MorphBridge.aux2` media object that captures adjacent same-layer endpoint silhouettes, caches validated endpoint data, creates GPU SDFs, and renders a solid-color SDF interpolation.

**Architecture:** A general `.aux2` plug-in registers a `FILTER_PLUGIN_TABLE` media object and owns the `EDIT_HANDLE` used for asynchronous endpoint rendering. Host-independent domain code resolves neighbors, fingerprints dependencies, and controls a coalescing preparation state machine; the filter callback uploads prepared endpoint RGBA, builds named GPU cache resources with compute shaders, and renders the interpolated field with a pixel shader.

**Tech Stack:** C++20, CMake 3.28+, Visual Studio 2022 x64/MSVC, AviUtl2 Plugin SDK, HLSL Shader Model 5.0, Direct3D 11 through `FILTER_PROC_VIDEO`, CTest with a dependency-free test executable.

**Spec:** `docs/superpowers/specs/2026-09-22-morph-bridge-design.md`

## Global Constraints

- The distributable is exactly one `MorphBridge.aux2`; compiled shaders are embedded as byte arrays in the DLL.
- Minimum supported AviUtl2 version is `2.1.8` (`RequiredVersion() == 2'010'800`).
- Windows x64 only; configuration must fail on other platforms or pointer widths.
- Builds do not download dependencies. The exact SDK headers and their upstream revision are vendored under `third_party/aviutl2_sdk`.
- A/B capture uses the previous object's final frame and next object's first frame with `apply_effect=true`.
- Event callbacks only modify atomics. They never call an edit API.
- Host callback pixel memory is copied immediately and never retained.
- No endpoint rendering request starts during final output.
- New production behavior follows red-green-refactor: add one failing test, observe the expected failure, implement the minimum, and rerun all tests.

---

## File map

- `CMakeLists.txt`: build, shader compilation/embedding, plug-in and tests.
- `CMakePresets.json`: reproducible Visual Studio x64 configure/build/test presets.
- `cmake/PlatformGuard.cmake`: Windows/x64 validation.
- `cmake/EmbedBinary.cmake`: converts `.cso` shader output into C++ byte arrays.
- `third_party/aviutl2_sdk/{filter2.h,plugin2.h,README.md}`: pinned SDK boundary.
- `src/domain/types.hpp`: spans, signatures, captured images, transforms, and status types.
- `src/domain/neighbor_resolver.{hpp,cpp}`: same-layer A/B selection.
- `src/cache/signature.{hpp,cpp}`: stable endpoint dependency hash.
- `src/cache/cache_state.{hpp,cpp}`: generation checks and coalescing preparation state.
- `src/capture/endpoint_capture.{hpp,cpp}`: asynchronous SDK request and pitch-aware copying.
- `src/render/canvas.{hpp,cpp}`: shared canvas and future-ready A/B sampling transforms.
- `src/render/standard_transform.{hpp,cpp}`: corrected A/B standard-drawing interpolation and `OBJECT_IMAGE_PARAM` conversion.
- `src/render/sdf_encoding.{hpp,cpp}`: CPU reference for packed GPU seed/distance encoding.
- `src/render/sdf_plan.{hpp,cpp}`: deterministic jump-flood pass plan and resource names.
- `src/render/gpu_renderer.{hpp,cpp}`: endpoint upload, GPU SDF creation, and final interpolation.
- `src/plugin/filter_object.{hpp,cpp}`: controls, per-instance state, and video callback.
- `src/plugin/plugin_entry.cpp`: `.aux2` exports, host registration, events, and lifetime.
- `shaders/{seed,jump_flood,finalize,morph}.hlsl`: GPU SDF pipeline.
- `tests/test_support.hpp`: dependency-free assertions.
- `tests/test_main.cpp`: calls every test group and returns a useful failure code.
- `tests/*_test.cpp`: focused host-independent and SDK-boundary tests.
- `docs/manual-test-checklist.md`: real AviUtl2 verification procedure.

---

### Task 1: Reproducible build and test harness

**Files:**
- Create: `CMakeLists.txt`
- Create: `CMakePresets.json`
- Create: `cmake/PlatformGuard.cmake`
- Create: `cmake/EmbedBinary.cmake`
- Create: `third_party/aviutl2_sdk/filter2.h`
- Create: `third_party/aviutl2_sdk/plugin2.h`
- Create: `third_party/aviutl2_sdk/README.md`
- Create: `tests/test_support.hpp`
- Create: `tests/test_main.cpp`
- Create: `src/domain/types.hpp`

**Interfaces:**
- Produces: `morph_bridge_core`, `MorphBridge.aux2`, `morph_bridge_tests`, and `run_all_tests()`.
- Produces: `morph_bridge::ObjectSpan`, `EndpointPair`, `RgbaImage`, `SamplingTransform`, and `CacheStatus`.

- [ ] **Step 1: Add a configure-time platform test that fails before the guard exists**

Create `cmake/PlatformGuard.cmake` initially with the intended callable contract missing, and add this call near the top of `CMakeLists.txt`:

```cmake
include(cmake/PlatformGuard.cmake)
morph_bridge_require_windows_x64("${WIN32}" "${CMAKE_SIZEOF_VOID_P}")
```

Configure with:

```powershell
cmake --preset dev
```

Expected: configuration fails with `Unknown CMake command "morph_bridge_require_windows_x64"`.

- [ ] **Step 2: Implement the platform guard and minimal targets**

Implement:

```cmake
function(morph_bridge_require_windows_x64 is_windows pointer_size)
  if(NOT is_windows)
    message(FATAL_ERROR "MorphBridge supports Windows only")
  endif()
  if(NOT pointer_size EQUAL 8)
    message(FATAL_ERROR "MorphBridge supports x64 only")
  endif()
endfunction()
```

Define an `aviutl2_sdk` interface target, a C++20 `morph_bridge_core` static library, a `MorphBridge` module with `PREFIX ""` and `SUFFIX ".aux2"`, and a `morph_bridge_tests` executable registered with CTest. Add `/utf-8`, `NOMINMAX`, and `/wd4828` where the SDK requires them.

- [ ] **Step 3: Vendor and identify the current SDK snapshot**

Copy `filter2.h` and `plugin2.h` from the current `aviutl2_sdk_mirror` revision. Record the source URL, commit SHA, retrieval date, and MIT license reference in `third_party/aviutl2_sdk/README.md`. Confirm `FILTER_PROC_VIDEO::exec_computeshader_data`, `HOST_APP_TABLE::register_filter_plugin`, `HOST_APP_TABLE::register_event_listener`, and `EDIT_HANDLE::rendering_object_video` exist.

- [ ] **Step 4: Add the dependency-free test harness**

Use this assertion shape in `tests/test_support.hpp`:

```cpp
#define MB_CHECK(expr) do { if (!(expr)) throw std::runtime_error(\
  std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #expr); } while (false)
```

`tests/test_main.cpp` calls declared test groups inside one `try/catch`, prints the exception to `std::cerr`, and returns `1` on failure or `0` on success.

- [ ] **Step 5: Add domain value types**

Define exact value types without SDK headers:

```cpp
namespace morph_bridge {
struct ObjectSpan { std::int64_t token; int layer; int start; int end; friend bool operator==(const ObjectSpan&, const ObjectSpan&) = default; };
struct EndpointPair { ObjectSpan before; ObjectSpan after; int before_frame; int after_frame; friend bool operator==(const EndpointPair&, const EndpointPair&) = default; };
struct RgbaImage { int width{}; int height{}; std::vector<std::byte> pixels; };
struct SamplingTransform { float tx{}, ty{}, rotation{}; float sx{1}, sy{1}; friend bool operator==(const SamplingTransform&, const SamplingTransform&) = default; };
enum class CacheStatus { Empty, Dirty, Capturing, CpuReady, GpuReady, Error };
}
```

- [ ] **Step 6: Verify configure, build, and empty test pass**

Run:

```powershell
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Expected: all commands succeed and `MorphBridge.aux2` exists in the preset output directory.

- [ ] **Step 7: Commit**

```powershell
git add CMakeLists.txt CMakePresets.json cmake third_party src/domain/types.hpp tests
git commit -m "build: scaffold MorphBridge aux2 project"
```

---

### Task 2: Same-layer neighbor resolution

**Files:**
- Create: `src/domain/neighbor_resolver.hpp`
- Create: `src/domain/neighbor_resolver.cpp`
- Create: `tests/neighbor_resolver_test.cpp`
- Modify: `tests/test_main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `ObjectSpan`, `EndpointPair`.
- Produces: `std::optional<EndpointPair> resolve_neighbors(const ObjectSpan& bridge, const std::function<std::optional<ObjectSpan>(int,int)>& find_at)`.

- [ ] **Step 1: Write failing boundary tests**

Cover these concrete cases:

```cpp
MB_CHECK(resolve_neighbors({2, 4, 10, 19}, find_exact).value() ==
         EndpointPair{{1,4,0,9},{3,4,20,29},9,20});
MB_CHECK(resolve_neighbors({2, 4, 10, 19}, find_next_at_20).has_value());
MB_CHECK(resolve_neighbors({2, 4, 10, 19}, find_next_at_21).has_value());
MB_CHECK(!resolve_neighbors({2, 4, 10, 19}, find_next_at_22));
MB_CHECK(!resolve_neighbors({2, 4, 10, 19}, find_wrong_layer));
MB_CHECK(!resolve_neighbors({2, 4, 10, 19}, find_self));
```

Run `cmake --build --preset dev` and confirm compilation fails because `resolve_neighbors` is missing.

- [ ] **Step 2: Implement the resolver**

Query A at `bridge.start - 1`. Query B at `bridge.end`, then `bridge.end + 1`. Validate layer, non-self token, A ending before the bridge, and B starting within the allowed boundary. Return endpoint frames `A.end` and `B.start`.

- [ ] **Step 3: Run all tests**

Run `ctest --preset dev --output-on-failure`. Expected: all neighbor tests pass.

- [ ] **Step 4: Commit**

```powershell
git add src/domain/neighbor_resolver.* tests/neighbor_resolver_test.cpp tests/test_main.cpp CMakeLists.txt
git commit -m "feat: resolve adjacent timeline objects"
```

---

### Task 3: Endpoint signatures and lazy invalidation

**Files:**
- Create: `src/cache/signature.hpp`
- Create: `src/cache/signature.cpp`
- Create: `src/cache/cache_state.hpp`
- Create: `src/cache/cache_state.cpp`
- Create: `tests/signature_test.cpp`
- Create: `tests/cache_state_test.cpp`
- Modify: `tests/test_main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `EndpointDescriptor`, `EndpointSignature`, `make_signature()`, and `PreparationState`.
- `PreparationState::observe(global_generation, signature)` returns `Keep`, `StartCapture`, or `WaitForCapture`.
- `PreparationState::complete(request_id, signature, images)` accepts only the latest matching request.

- [ ] **Step 1: Write failing signature tests**

Define the desired descriptor API in the test:

```cpp
EndpointDescriptor a{{1,2,0,9}, 9, "alias-a"};
EndpointDescriptor b{{3,2,20,29}, 20, "alias-b"};
const auto base = make_signature(7, 1920, 1080, a, b, 50, 50, 1);
MB_CHECK(base == make_signature(7, 1920, 1080, a, b, 50, 50, 1));
b.alias_utf8 = "alias-b-changed";
MB_CHECK(base != make_signature(7, 1920, 1080, a, b, 50, 50, 1));
```

Also verify scene dimensions, endpoint frames, threshold, scale, and cache-format version each change the signature. Build and observe the missing-symbol failure.

- [ ] **Step 2: Implement deterministic FNV-1a-based signatures**

Serialize every integer explicitly as little-endian bytes and hash alias bytes exactly. Do not hash struct padding or pointer values. `EndpointSignature` is two `std::uint64_t` lanes using distinct FNV offsets to reduce accidental collisions.

- [ ] **Step 3: Write failing state-machine tests**

Test these transitions:

```cpp
PreparationState state;
MB_CHECK(state.observe(1, sig_a).action == CacheAction::StartCapture);
const auto request = state.active_request();
MB_CHECK(state.observe(2, sig_a).action == CacheAction::WaitForCapture);
MB_CHECK(state.complete(request, sig_a, images));
MB_CHECK(state.observe(3, sig_a).action == CacheAction::Keep);
MB_CHECK(state.observe(4, sig_b).action == CacheAction::StartCapture);
MB_CHECK(!state.complete(request, sig_a, images));
```

Add a case where generation changes but the signature does not: the ready cache must remain ready.

- [ ] **Step 4: Implement coalescing and stale-result rejection**

Keep `observed_generation`, `desired_signature`, `ready_signature`, monotonically increasing `request_id`, status, and immutable `std::shared_ptr<const PreparedEndpoints>`. Protect compound state with one mutex; the global edit generation remains atomic outside this class.

- [ ] **Step 5: Run all tests and commit**

Run `ctest --preset dev --output-on-failure`, then:

```powershell
git add src/cache tests/signature_test.cpp tests/cache_state_test.cpp tests/test_main.cpp CMakeLists.txt
git commit -m "feat: validate and coalesce endpoint caches"
```

---

### Task 4: Asynchronous endpoint capture

**Files:**
- Create: `src/capture/endpoint_capture.hpp`
- Create: `src/capture/endpoint_capture.cpp`
- Create: `tests/endpoint_capture_test.cpp`
- Modify: `tests/test_main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: SDK `EDIT_HANDLE`, temporary `OBJECT_HANDLE`s, `EndpointSignature`, and request ID.
- Produces: `EndpointCapture::request(before, before_frame, after, after_frame, completion)`.
- Completion type: `std::function<void(CaptureResult)>`, where `CaptureResult` contains request ID, signature, or a typed `CaptureError`.

- [ ] **Step 1: Write a failing pitch-copy test**

Expose a pure helper:

```cpp
auto image = copy_rgba_rows(source.data(), 2, 2, 12);
MB_CHECK(image.width == 2);
MB_CHECK(image.height == 2);
MB_CHECK(image.pixels.size() == 16);
MB_CHECK(image.pixels[8] == source[12]);
```

Also test null data, zero dimensions, pitch smaller than `width*4`, and overflow-sized dimensions return `CaptureError::InvalidGeometry`. Observe failure before implementing.

- [ ] **Step 2: Implement validated immediate copying**

Use checked `std::size_t` multiplication, copy exactly `width*4` bytes from each source row, and return an owning `RgbaImage`.

- [ ] **Step 3: Write a failing two-endpoint coordination test**

Use a fake request function that records `(object, frame, apply_effect)` and exposes callbacks. Verify both requests use `apply_effect=true`, completion fires exactly once after both callbacks, and any rejection or invalid callback completes with an error.

- [ ] **Step 4: Implement `EndpointCapture`**

Allocate one `std::shared_ptr<Job>` containing both image slots, signature, request ID, an atomic remaining count, a mutex, and an atomic completed flag. SDK callbacks own a heap-held `shared_ptr<Job>` wrapper and delete that wrapper after copying. Never store `OBJECT_HANDLE` after the request is queued.

- [ ] **Step 5: Run tests and commit**

```powershell
ctest --preset dev --output-on-failure
git add src/capture tests/endpoint_capture_test.cpp tests/test_main.cpp CMakeLists.txt
git commit -m "feat: capture endpoint RGBA asynchronously"
```

---

### Task 5: Shared canvas and packed SDF math

**Files:**
- Create: `src/render/canvas.hpp`
- Create: `src/render/canvas.cpp`
- Create: `src/render/sdf_encoding.hpp`
- Create: `src/render/sdf_encoding.cpp`
- Create: `src/render/standard_transform.hpp`
- Create: `src/render/standard_transform.cpp`
- Create: `tests/canvas_test.cpp`
- Create: `tests/sdf_encoding_test.cpp`
- Create: `tests/standard_transform_test.cpp`
- Modify: `tests/test_main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `CanvasLayout make_canvas_layout(a_width, a_height, b_width, b_height, scale_percent, margin)`.
- Produces: per-endpoint `SamplingTransform` and integer placement rectangles.
- Produces: 16-bit normalized seed-coordinate and signed-distance pack/unpack helpers mirrored exactly in HLSL.
- Produces: `StandardTransform interpolate_corrected_transform(a, a_correction, b, b_correction, progress)`.

- [ ] **Step 1: Write failing canvas tests**

Verify a 100x50 endpoint and 60x120 endpoint at 50% scale with 4 scaled pixels of margin produce a centered shared canvas, even dimensions, and independent identity transforms. Verify invalid dimensions and unsupported scales are rejected.

- [ ] **Step 2: Implement canvas layout**

Scale dimensions with ceiling division, take per-axis maxima, add twice the margin, round each axis to an even integer, and center each endpoint with deterministic left/top bias for odd differences.

- [ ] **Step 3: Write failing encoding and interpolation tests**

Test exact round trips for seed coordinates `0`, `1`, `255`, `256`, and `4095`; reserve `0xffff` as invalid. Test signed-distance encode/decode at `-max`, `0`, and `+max` within one quantization unit. Test the CPU reference interpolation:

```cpp
MB_CHECK(inside(interpolate_sdf(-10.0f, 10.0f, 0.25f)));
MB_CHECK(!inside(interpolate_sdf(-10.0f, 10.0f, 0.75f)));
MB_CHECK(alpha_from_sdf(0.0f, 1.0f) > 0.49f);
```

- [ ] **Step 4: Implement packed RGBA8 encoding**

Pack 16-bit X into R/G and Y into B/A for seed textures. Pack biased normalized signed distance into R/G, leaving B/A available for the inside mask and version marker. Clamp dimensions below the reserved value and use the same rounding formula that the shaders will use.

- [ ] **Step 5: Write failing standard-transform tests**

Define `StandardTransform` with X/Y/Z, center X/Y/Z, rotation X/Y/Z, scale, aspect, and opacity, plus the same fields except opacity in `TransformCorrection`. Verify:

```cpp
const auto at_a = interpolate_corrected_transform(a, a_fix, b, b_fix, 0.0);
MB_CHECK(at_a.x == a.x + a_fix.x);
const auto at_b = interpolate_corrected_transform(a, a_fix, b, b_fix, 1.0);
MB_CHECK(at_b.x == b.x + b_fix.x);
MB_CHECK(interpolate_angle_degrees(350.0, 10.0, 0.5) == 0.0);
MB_CHECK_NEAR(interpolate_positive_scale(50.0, 200.0, 0.5), 100.0, 0.001);
```

At progress 0.5, verify A and B additive corrections contribute with weights 0.5 and 0.5. Verify scale/aspect corrections are applied to endpoint percentages before conversion to positive X/Y multipliers, and opacity is linearly interpolated without a correction control.

- [ ] **Step 6: Implement corrected interpolation**

Clamp progress to `[0,1]`. Add each correction to its matching endpoint value, wrap rotations through the shortest signed delta, convert scale/aspect to positive X/Y factors, interpolate positive factors geometrically with `exp(lerp(log(a),log(b),p))`, and interpolate position, center, and opacity linearly. Keep this file free of SDK types; `filter_object.cpp` converts the result into `OBJECT_IMAGE_PARAM`.

- [ ] **Step 7: Run tests and commit**

```powershell
ctest --preset dev --output-on-failure
git add src/render/canvas.* src/render/sdf_encoding.* src/render/standard_transform.* tests/canvas_test.cpp tests/sdf_encoding_test.cpp tests/standard_transform_test.cpp tests/test_main.cpp CMakeLists.txt
git commit -m "feat: define SDF canvas and endpoint transforms"
```

---

### Task 6: Jump-flood shaders and deterministic GPU dispatch plan

**Files:**
- Create: `src/render/sdf_plan.hpp`
- Create: `src/render/sdf_plan.cpp`
- Create: `tests/sdf_plan_test.cpp`
- Create: `shaders/seed.hlsl`
- Create: `shaders/jump_flood.hlsl`
- Create: `shaders/finalize.hlsl`
- Create: `shaders/morph.hlsl`
- Modify: `cmake/EmbedBinary.cmake`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `SdfDispatchPlan build_sdf_plan(width, height, resource_prefix)`.
- Produces embedded arrays `seed_cso`, `jump_flood_cso`, `finalize_cso`, and `morph_cso` with corresponding sizes.

- [ ] **Step 1: Write failing dispatch-plan tests**

For a 320x180 canvas, require jump steps `{256,128,64,32,16,8,4,2,1}`, thread-group counts `ceil(width/8)` by `ceil(height/8)`, alternating ping/pong resources, and distinct A/B resource prefixes. Observe the missing-symbol failure.

- [ ] **Step 2: Implement the dispatch plan**

Select the largest power of two greater than or equal to `max(width,height)`, then halve through one. The final result resource must be deterministic regardless of whether the pass count is odd or even.

- [ ] **Step 3: Add shaders with shared encoding formulas**

Use `[numthreads(8,8,1)]` for compute shaders. `seed.hlsl` compares alpha against the supplied threshold and emits self coordinates only at inside/outside boundaries. `jump_flood.hlsl` examines the 3x3 neighborhood at the current jump distance and retains the closest valid seed. `finalize.hlsl` computes Euclidean pixel distance and applies the mask sign. `morph.hlsl` decodes both fields, applies separate A/B inverse sampling transforms, linearly interpolates the distances, and outputs premultiplied solid color with a one-pixel smooth transition.

- [ ] **Step 4: Compile and embed shaders**

Add custom commands using `fxc /T cs_5_0` for compute files and `fxc /T ps_5_0` for `morph.hlsl`. `EmbedBinary.cmake` reads each `.cso` as hex and emits `generated/shaders.hpp`; the plug-in never loads external shader files.

- [ ] **Step 5: Verify shader compilation and CPU/HLSL constants**

Run `cmake --build --preset dev --target MorphBridge`. Expected: every shader compiles without warnings and `generated/shaders.hpp` is nonempty. Keep encoding constants in one generated header included by both C++ and HLSL compilation so sentinel and quantization values cannot drift.

- [ ] **Step 6: Run tests and commit**

```powershell
ctest --preset dev --output-on-failure
git add shaders src/render/sdf_plan.* tests/sdf_plan_test.cpp cmake/EmbedBinary.cmake CMakeLists.txt
git commit -m "feat: add GPU jump-flood SDF pipeline"
```

---

### Task 7: GPU resource renderer

**Files:**
- Create: `src/render/gpu_renderer.hpp`
- Create: `src/render/gpu_renderer.cpp`
- Create: `tests/gpu_renderer_contract_test.cpp`
- Modify: `tests/test_main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `FILTER_PROC_VIDEO`, `PreparedEndpoints`, `CanvasLayout`, progress, color, threshold, and effect ID.
- Produces: `GpuRenderer::render(RenderRequest) -> RenderResult`.
- Uses per-instance names `cache:MorphBridge.<effect_id>.<signature>.<role>`.

- [ ] **Step 1: Write a failing resource-plan contract test**

Test a pure `build_gpu_resource_plan()` first. Assert that A/B upload names differ, ping/pong targets never alias a sampled source in the same pass, an unchanged signature reuses names, and a changed signature changes all cache resource names.

- [ ] **Step 2: Implement resource naming and readiness checks**

Use lowercase hexadecimal effect ID and signature with fixed-width formatting. Before rebuilding, call `get_image_resource_size()` for both final SDF resources and require exact canvas dimensions. Missing resources trigger GPU rebuilding from retained CPU endpoints, not new asynchronous endpoint capture.

- [ ] **Step 3: Write a failing SDK-call-order test**

Provide a small fake `FILTER_PROC_VIDEO` table that records calls. Require this order for a cold cache: set object size, upload A/B resources, seed A/B, all jump passes, finalize A/B, morph into `object`. For a warm cache require only object sizing and morph. Return false from each SDK operation in separate cases and require a typed `RenderError` rather than later calls.

- [ ] **Step 4: Implement the renderer**

Use `set_image_data(nullptr,width,height)` to establish the output. Upload centered endpoint RGBA with `set_image_resource_data`. Execute embedded compute shader bytes with `exec_computeshader_data`, and the final embedded pixel shader with `exec_pixelshader_data`. Pass only trivially copyable, 16-byte-aligned constant-buffer structs and assert their sizes at compile time.

- [ ] **Step 5: Run tests and commit**

```powershell
ctest --preset dev --output-on-failure
git add src/render/gpu_renderer.* tests/gpu_renderer_contract_test.cpp tests/test_main.cpp CMakeLists.txt
git commit -m "feat: render cached SDF morphs on GPU"
```

---

### Task 8: Media object, plug-in registration, and host integration

**Files:**
- Create: `src/plugin/filter_object.hpp`
- Create: `src/plugin/filter_object.cpp`
- Create: `src/plugin/plugin_entry.cpp`
- Create: `tests/plugin_registration_test.cpp`
- Create: `tests/filter_object_test.cpp`
- Modify: `tests/test_main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces exported `GetCommonPluginTable`, `InitializePlugin`, `RegisterPlugin`, `UninitializePlugin`, and `RequiredVersion`.
- Produces a filter table named `MorphBridge` with `FLAG_VIDEO | FLAG_INPUT | FLAG_USERDATA`.
- Per-instance state owns effect ID, `PreparationState`, source CPU images, sampling transforms, and warning limiter.

- [ ] **Step 1: Write a failing registration test**

Use a fake `HOST_APP_TABLE` and assert `RegisterPlugin()`:

```cpp
MB_CHECK(registered_filter != nullptr);
MB_CHECK((registered_filter->flag & FILTER_PLUGIN_TABLE::FLAG_INPUT) != 0);
MB_CHECK(created_edit_handle);
MB_CHECK(events.contains(EVENT_TYPE::UPDATE_OBJECT));
MB_CHECK(events.contains(EVENT_TYPE::CHANGE_EDIT_SCENE));
MB_CHECK(clear_cache_handler != nullptr);
MB_CHECK(RequiredVersion() == 2'010'800);
```

Observe link failures before the exports are implemented.

- [ ] **Step 2: Implement registration and atomic event handling**

Store the host-created `EDIT_HANDLE*` for the DLL lifetime. `UPDATE_OBJECT` increments `std::atomic_uint64_t edit_generation`. Scene change and cache clear increment generation and a separate `clear_generation`. Register the filter table through `register_filter_plugin()`.

- [ ] **Step 3: Write failing filter-instance tests**

Verify `func_create(effect_id)` creates isolated state and `func_destroy()` safely releases it. Verify missing neighbors, null SDK callbacks, output mode with no prepared cache, and capture rejection produce transparent 1x1 output plus one rate-limited log entry. Verify progress is clamped to `[0,1]`, color is premultiplied, and a generation change with the same signature does not issue capture requests. Verify the A/B `標準描画` values are read at their endpoint frames and the corrected interpolated result is copied into `OBJECT_IMAGE_PARAM`.

- [ ] **Step 4: Implement controls and render orchestration**

Define:

```cpp
FILTER_ITEM_TRACK progress{L"Progress", 0.0, 0.0, 100.0, 0.01};
FILTER_ITEM_COLOR color{L"Color", 0xffffff};
FILTER_ITEM_TRACK alpha_threshold{L"Alpha threshold", 50.0, 0.0, 100.0, 0.1};
FILTER_ITEM_SELECT sdf_scale{L"SDF scale", 50, scale_items};
```

Add eight `FILTER_ITEM_TRACK_GROUP` controls: A position XYZ, A center XYZ, A rotation XYZ, A scale/aspect, and the matching four B groups. Every correction track defaults to zero. In editable rendering, resolve neighbors inside `video->edit`, copy aliases immediately into descriptors, read A/B `標準描画` endpoint tracks, compute the signature, and feed `PreparationState`. Queue capture only for `StartCapture`. Publish capture completions back into the instance state. If CPU endpoints are ready, call `GpuRenderer`; otherwise output transparent. Never call `rendering_object_video()` when `EDIT_HANDLE::get_edit_state()` reports outputting.

- [ ] **Step 5: Keep Progress under normal AviUtl2 track control**

Keep the registered default at the fixed value zero. Do not add an automatic-progress checkbox and do not mutate the project to install a movement mode. The user can select linear or any other standard movement and set the second endpoint to 100 in AviUtl2.

- [ ] **Step 6: Run all tests and build the DLL**

```powershell
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Expected: tests pass and the module target is named exactly `MorphBridge.aux2`.

- [ ] **Step 7: Commit**

```powershell
git add src/plugin tests/plugin_registration_test.cpp tests/filter_object_test.cpp tests/test_main.cpp CMakeLists.txt
git commit -m "feat: expose MorphBridge media object"
```

---

### Task 9: Packaging and real-host verification

**Files:**
- Create: `README.md`
- Create: `CHANGELOG.md`
- Create: `LICENSE`
- Create: `docs/manual-test-checklist.md`
- Create: `package/Plugin/MorphBridge/.gitkeep`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `package/Plugin/MorphBridge/MorphBridge.aux2` and an installable `MorphBridge-v0.1.0.au2pkg.zip`.

- [ ] **Step 1: Add a failing package-layout test**

Add a CTest script that fails unless the staged package contains exactly:

```text
Plugin/MorphBridge/MorphBridge.aux2
package.ini
package.txt
```

and contains no `.obj2`, `.mod2`, `.auf2`, `.pdb`, or source file.

- [ ] **Step 2: Add package generation**

Create a `package` target that stages the Release DLL, writes UTF-8 package metadata, and uses `cmake -E tar --format=zip` to produce `MorphBridge-v0.1.0.au2pkg.zip`.

- [ ] **Step 3: Document operation and limitations**

README must state the A -> MorphBridge -> B same-layer workflow, automatic cache validation, supported v1 silhouette output, requirement to let endpoints prepare before final output, and known exclusion of Group Control additions. The manual checklist must cover shape/image/text pairs, missing neighbors, moving A/B, editing filters, unrelated-object edits, rapid slider changes, undo/redo, scene switch, cache clear, project reload, prepared output, and unprepared output.

- [ ] **Step 4: Run automated verification**

```powershell
cmake --build --preset release
ctest --preset release --output-on-failure
cmake --build --preset release --target package
```

Expected: all tests pass, package-layout test passes, and the zip is generated.

- [ ] **Step 5: Run AviUtl2 smoke tests**

Copy the staged DLL to a dedicated test plug-in folder, launch AviUtl2, and execute every item in `docs/manual-test-checklist.md`. Record AviUtl2 version, GPU, pass/fail, and any observed SDK-contract differences. Do not claim the host integration works until this checklist has actual results.

- [ ] **Step 6: Final repository verification and commit**

Run `git status --short`, confirm only intended files remain, then:

```powershell
git add README.md CHANGELOG.md LICENSE docs package CMakeLists.txt
git commit -m "docs: package and verify MorphBridge v1"
```
