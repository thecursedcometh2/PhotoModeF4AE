# Fallout 4 1.11.240.0 audit

**Not runtime-certified.** No Fallout4.exe, matching Address Library database,
GPU/game session or disassembler evidence is available in this environment.
The pinned CommonLibF4 header explicitly defines RUNTIME_1_11_240 and selects it
as RUNTIME_LATEST. The plugin rule advertises that version; a static assertion
prevents silently following a future version. Load also rejects other runtimes.
These checks constrain the target; they do not certify game ABI correctness.

## Hook inventory

All listed hooks preserve original behavior outside Photo Mode unless noted.
The addresses below are source identifiers, not observed executable addresses.
No new relocation IDs or instruction offsets have been guessed.

| Hook/location | Address/slot and expected signature | Purpose, rationale, verification |
| --- | --- | --- |
| Camera rotation, Hooks.cpp | FreeCameraState VTABLE[0], slot 0x0C; void(const FreeCameraState*, NiQuaternion&) | Calls original then post-multiplies local-Y roll. Slot/signature follow pinned TESCameraState.h. Removes original REL::ID(2248281)+0x1B internal CALL, whose presence on 1.11.240 was unverified. Quaternion math tested; slot dispatch, axis/sign and visible effect require binary/game verification. |
| DOF_IsActive | ImageSpaceEffectDepthOfField VTABLE[0], 0x08; bool(ImageSpaceEffectDepthOfField*) | Suppresses vanilla DOF during Photo Mode. Pinned ImageSpaceEffect.h declares IsActive at 08. Runtime dispatch/layout unverified. |
| Bokeh_IsActive | ImageSpaceEffectBokehDepthOfField VTABLE[0], 0x08; bool(ImageSpaceEffectDepthOfField*) | Enables controlled DOF; same base method as above. Runtime unverified. |
| Pause-menu handler | PauseMenu VTABLE[0], 0x01; void(PauseMenu*, const Scaleform::GFx::FunctionHandler::Params&) | Retains upstream callback and userData=16 test and selectedEntry marker/disabled checks (display text uses the plugin translation map). SWFToCodeFunctionHandler.h declares Call at 01 in the primary menu vtable. Userdata and SWF paths still require target verification. |
| Menu input | MenuOpenHandler VTABLE[0], 0x08; void(MenuOpenHandler*, ButtonEvent*) | Calls original and allows VATS handling in Photo Mode. BSInputEventUser.h declares OnButtonEvent(const ButtonEvent*) at 08; upstream thunk mutates handled. Pointer constness does not change ABI, but event policy needs gameplay verification. |
| Idle editor IDs | TESIdleForm VTABLE[0], 0x3B; bool(TESIdleForm*, const char*) | Caches non-paired idles where TESDataHandler lacks an array. TESForm.h declares SetFormEditorID at 3B; target binary and form-load timing unverified. |
| Screenshot call | REL::ID(2228913) + 0x2C6; void(const char*, BSGraphics::TextureFileFormat, uint32_t) | Retained upstream Main::Swap CALL. **Requires binary analysis before release.** Installer checks E8 opcode and fails closed if mismatched; this is not an ABI or instruction-boundary proof. Vanilla capture runs only for unqueued requests. Queued capture failures notify/log instead of falling through to an unchecked writer. |
| UI rendering | HUDMenu VTABLE[0], 0x06; void(IMenu*) | Runs ImGui before original PostDisplay. IMenu.h declares PostDisplay at 06; HUD-hidden behavior and render-thread/context ownership require in-game verification. |
| Window input | GWLP_WNDPROC, LRESULT(HWND,UINT,WPARAM,LPARAM) | Standard Win32 API hook, original procedure forwarded; clears keys on focus loss. Window lifecycle/reinitialization still needs testing. |
| Console command | SCRIPT_FUNCTION table entry named TogglePathLine; upstream execute ABI | Replaces table entry with PhotoMode command; table layout and command coexistence require target validation. |

CommonLib's VTABLE/RTTI/ID tables resolve through its Address Library infrastructure.
The project uses no separate hardcoded RTTI constants. The write_vfunc helper
writes VTABLE[0] only. The call helper uses REL::GetTrampoline, initialized once
through F4SE::Init (128 bytes), and the current write_call<5> API. No signature
scanning was added. The unused branch helper now uses write_jmp<5>.

## Camera analysis

Original roll reached a matrix conversion only through a CALL patched at +0x1B.
If newer code inlines/reorders that operation, changing the UI value cannot affect
the returned rotation. That is a plausible failure mechanism, **not a reproduced
root cause** without the executable. The replacement hooks the declared virtual
GetRotation output and composes roll after the game supplies pitch/yaw. It never
writes FreeCameraState's stored angles; inactive mode returns the original result.
Camera::RevertState resets the roll value to zero. Math identity/inverse tests pass;
visible roll, sign, and restoring pre-existing TFC remain pending.

## Capture and compositing analysis

Original code copied capture alpha unchanged, blended only RGB, ignored failed
conversion/encoding, divided by hardware_concurrency (which may be zero), and
ignored remainder rows. These are source-confirmed defects. New code normalizes
capture and overlay to straight RGBA8 before byte operations, resizes either-axis
mismatches, blends all rows with rowPitch, and flattens PNG alpha to 255 even with
no overlay. Queued Photo Mode capture uses exclusive PNG creation and never falls through to the vanilla writer on a save failure. Bounds
checks reject invalid render-target indices/null textures. SDR sRGB views are converted without accidentally decoding then merely retagging
the stored RGB bytes. DirectXTex performs
GPU readback; WIC performs PNG encoding synchronously on the original caller.
No worker threads survive encoding or buffer destruction.

The capture still uses RenderTargetManager.renderTargetID[63], RendererData device,
context, renderTargets[index].texture. The selector's meaning, game layout,
MSAA/typeless texture handling, HDR tone mapping, and whether the selected surface
already contains the visible overlay require target GPU/game validation. No
premultiplied-alpha source is intentionally supported: installed PNG overlays are
loaded through WIC without a premultiply flag. Capture RGB is treated as already
flattened, so it is not divided by an unrelated render-target alpha. RGBA/BGRA
channel conversion is delegated to DirectXTex, not manual channel guessing.

## Other structure and state dependencies

| Source | Runtime-sensitive accesses / outstanding checks |
| --- | --- |
| Camera.cpp | PlayerCamera worldFOV/currentState, Main::SetCameraFOV, INI translation speed, ImageSpaceModifierInstanceDOF flags/data/strength; validate all controls and restore. |
| Input.cpp | Input event device/key/analog fields, ControlMap TFC mapping, FreeCameraState worldZDirection; now checks actual camera state before cast. ImGui analog argument order and mouse button index corrected; device navigation still needs testing. |
| PhotoMode/Manager.cpp | UI/menuSystemVisible, context stack, PlayerControls handlers, BSInputEnableManager layers, savingAllowed, active global, pause SWF entries. Empty context guard added. HUD restores entry visibility, saving restores prior permission, untouched VATS INI no longer forced. Input handler enable flags now restore their recorded values only on the same handler objects, and text-entry requests are balanced; game/mod coexistence remains unverified. |
| Time.cpp | Main freezeTime, BSTimer multiplier, Calendar global values, Sky weather override/update fields; all need target state restoration tests. |
| Character.cpp | Actor data angle/position, 3D visibility, currentProcess idle/package APIs, temp effect handles and shader/art effects; target layouts and restoring the recorded visibility is implemented; gameplay effects need testing. Fixed temporary string lifetime in display label. |
| Filters.cpp | ImageSpaceManager base/override pointers and cinematic/tint fields; original override pointer is now restored if Photo Mode still owns the override; mod coexistence remains unverified. |
| ImGui/Renderer.cpp | RendererData renderWindow[0], swapchain/device/context and BSGraphics::State dimensions; needs HUD-hidden, focus and resolution-change testing. |
| Translation.cpp | BSScaleformManager loader/translator table; handles unavailable translator and empty/malformed lines safely. |
| Fonts/Manager.cpp | Original ignored BGSFonts.h supplied compressed ControllerButtons_Data/Size, absent in original Git tree. Provenance/rights unavailable. Text labels replace it; external Roboto/Font Awesome remain mandatory and unbundled. |

## Report evidence

Original GitHub issue API (`powerof3/PhotoModeF4/issues?state=all&per_page=100`)
returned an empty list during this review. Nexus 88107 bugs page returned HTTP 403;
no Nexus report contents could be reviewed. User-reported roll/transparency/UI/DOF
regressions remain reports, not reproduced gameplay failures. Source-confirmed
bugs and portable tests above must not be represented as in-game fixes verified.

## Screenshot preservation follow-up

Startup scanning is no longer conditional on a successful optional INI load.
Selected-folder filename suffixes and persisted hints use nonnegative uint64_t
values; existing Screenshot4294967295.png is handled without signed overflow.
The PNG is encoded in memory before CreateFileW(CREATE_NEW). Existing files cause
a retry at the next number; no pathname is opened with truncation or replacement.
Partial-write cleanup uses the newly owned handle rather than deleting a pathname.
The manager serializes captures and UINT64_MAX exhaustion fails without wrapping.
The lower-level SaveToPNG encoder is used only for caller-owned test paths;
production captures call SaveNewPNG. See TESTING.md for restart and race tests.

## Screen Archer Menu coexistence

No new relocation or vtable hook. The `ScreenArcherMenu` native menu name is
verified in SAM/constants.h at 97ce9c286b2ffcd91eed9d51f847631ff51664d6.
Photo Mode's existing MenuOpenClose sink and frame/input paths query CommonLib
UI::GetMenuOpen. While open, they skip Photo Mode drawing/input before the normal
context validity check (SAM uses context 0x22, otherwise causing deactivation).
The VATS event policy also leaves SAM events alone. SAM/camera.cpp directly
updates FreeCameraState position/angles/FOV and is not intercepted or disabled.

Photo Mode already synthesizes FreeCameraState::worldZDirection (int16) for TFC
WorldZUp/Down. It now tracks the state pointer and last direction it wrote, and
clears that direction once on suspension/exit only if both still match. It never
dereferences a saved pointer; further SAM-owned changes are untouched. TFC mapping
lookups are refreshed instead of cached indefinitely. No global handler or SAM
input layer is toggled by this guard. Existing structure-layout assumptions are
unchanged. Pure ownership-policy checks pass; actual SAM clicks, camera controls,
menu ordering and game-thread behavior require the in-game matrix in TESTING.md.
