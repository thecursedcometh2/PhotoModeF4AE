# Fallout 4 1.11.240.0 audit

The tester confirmed the 2.1.7 view-roll fix. Version 2.1.8 retains that hook and
removes temporary diagnostics. Historical investigation notes below describe
superseded builds, not additional hooks in the current DLL.

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
| Camera rotation, Hooks.cpp | Live FreeCameraState VTABLE[0], slot 0x0C; void(const FreeCameraState*, NiQuaternion&) | Live table matches SDK. User-supplied bytes at EXE RVA 0x1022444 decode to FF 50 60, confirming dispatch through slot 0x0C; target is SAM, not original REL::ID(2248281). On activation capture current callback, publish original before writing slot with REL::WriteSafeData, then call downstream once and compose local-Y roll. No code/DLL offsets patched. Runtime hook conflict/callsite established by trace; corrected in-game effect remains pending. |
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

Original roll reached a matrix conversion through a CALL at
REL::ID(2248281)+0x1B in the authoritative upstream Hooks.cpp. The first migration
replaced that with a virtual GetRotation hook. Version 2.1.4 tried a PlayerCamera
Update vtable hook and a scene overlay. The tester reports a changing roll value
with `camera updates=0, roll applications=0`. This establishes that the installed
Update vtable hook is not called in that installation; it does not establish
whether the getter is inlined, called directly, or replaced by another camera mod.
Both unsuccessful virtual-hook approaches have now been removed.

The 2.1.5 entry detour also received zero calls. The 2.1.6 trace then established
why: the live table itself matches FreeCameraState, but slot 0x0C contains
ScreenArcherMenu.dll+62550. The original EXE getter at RVA 0x1022520 is bypassed.
The supplied state.Update bytes disassemble to the following call sequence:

- EXE RVA 0x102243A loads the state's vtable into RAX.
- RVA 0x102243D loads an output buffer into RDX; RCX receives the state pointer.
- RVA 0x1022444: `FF 50 60`, call qword ptr [RAX+0x60] (slot 0x0C).
- RVA 0x1022451: `FF 50 68`, translation getter (slot 0x0D).
- The quaternion output is converted to a matrix and stored into the camera root.

These are observations from the user's actual code snapshot, disassembled with
objdump in the development environment. They are not patch offsets or signatures.
The public SAM master inspected earlier lacks this replacement; do not claim its
old source verifies the installed binary's implementation. The observed ABI and
live dispatch plus the SDK declaration justify chaining the opaque callback.

2.1.7 removes the unused entry detour and MinHook. First activation, after toggling
free camera, checks that the live state is free and its vtable matches the SDK.
It captures slot 0x0C's current function (SAM or vanilla), publishes that downstream
pointer, then writes Photo Mode's wrapper with CommonLib Shared REL::WriteSafeData.
The wrapper calls that function exactly once, preserving its complete quaternion,
and composes the Photo Mode roll while active, including with SAM open. It writes
no stored angles/scene transforms; inactive output is the downstream result.

Frame checks are idempotent and never capture the Photo Mode wrapper as its own
original. If another mod later replaces the slot, the code logs and refuses blind
re-chaining: that wrapper may already call Photo Mode and form a cycle. No lifecycle
unpatch erases SAM's changes. Native fixtures reproduce an early wrapper overwritten
by a SAM-style replacement, then test the actual late-chain helper, exactly-once
composition, idempotence, changed SAM baseline, inactive/zero output, later-wrapper
recursion protection and vanilla operation. Actual visible roll/sign and SAM menu
open/close behavior still require the in-game matrix.

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

## Unused entry confirmed; 2.1.6 diagnostic

The tester subsequently reports `entry hook=true, rotation calls=0` for 2.1.5.
That invalidates the GetRotation entry as a demonstrated live interception point
in that installation. Whether another live vtable/type, direct alternate routine,
or inlining explains this remains unknown. No replacement hook is introduced.
A one-time read-only trace now compares the SDK's expected vtables with actual
PlayerCamera/current-state/render-camera objects and captures their declared
method entries plus bounded main-image executable bytes. This is binary-analysis
input, not compatibility proof. The methods are read, never invoked by the trace;
the existing hook's real call counter is not artificially incremented. SAM does
not reset or suppress the selected roll. Further hook changes require inspecting
this live evidence or the target executable, as the previous audit already noted.

## Trace resolution (2.1.7)

The diagnostic trace requested above was supplied. It identifies SAM's replacement
getter and confirms the game dispatches to it from state.Update; the assumption
that camera virtual dispatch itself was unused was incomplete. The unused target
was the original EXE getter after SAM replaced the slot. The late-chain fix above
uses this evidence; the specific observed SAM RVA is never hardcoded.

## Final cleanup (2.1.8)

The user confirmed that view roll now works and requested a final build without
temporary diagnostics. Rotation/application counters, slider logging/state, module
address descriptions and executable code sampling are removed. Safe pointer reads
remain in the hook-chain helper; failure checks and error logs remain. The late
vtable capture, downstream call, quaternion composition, SAM input guard and reset
behavior are unchanged. The user confirmation covers the reported regression,
not every hook or every row of the broader in-game matrix.

## Window procedure forwarding correction (2.1.9)

The supplied crash target 0xFFFF0AC7 is consistent with an opaque Win32 procedure
value. PDB S_PUB32 record for the exact 2.1.7 DLL places Renderer::WndProc::thunk
at section 1 offset 0x21FB0, plus .text RVA 0x1000 = 0x22FB0, matching the log.
Disassembly confirms a tail jump through the saved procedure value. Replace that
direct invocation with CallWindowProcA; no engine relocation or hook slot changes.
Microsoft documents that saved WNDPROC values may be internal tokens and that
CallWindowProc performs ANSI/Unicode conversion. Source:
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-callwindowproca

This is a concrete forwarding defect supported by the crash, not proof that
DearModdingUI, mirrors, Community Shaders or another scanned module is at fault.
Native window-chain checks validate the API correction; game confirmation remains
pending. Camera roll and screenshot behavior are unchanged.
