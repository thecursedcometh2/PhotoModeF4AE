# Runtime audit

Target: Fallout 4 **1.11.240.0**. Other runtimes are rejected.
The CommonLib header is checked at compile time. Version checks do not prove ABI compatibility.
There is no local game executable or Address Library database to inspect.
User traces provide the binary evidence noted below.

## Hooks

| Hook | Address / expected signature | Status |
| --- | --- | --- |
| Roll | FreeCameraState live vtable slot 0x0C; void(const FreeCameraState*, NiQuaternion&) | SDK table checked. User trace proves dispatch. Visible roll confirmed. |
| DOF | ImageSpaceEffectDepthOfField slot 0x08; bool(ImageSpaceEffectDepthOfField*) | SDK IsActive slot. Runtime layout/dispatch needs testing. |
| Bokeh | ImageSpaceEffectBokehDepthOfField slot 0x08; bool(ImageSpaceEffectDepthOfField*) | SDK base slot. Runtime testing pending. |
| Pause entry | PauseMenu slot 0x01; void(PauseMenu*, const GFx::FunctionHandler::Params&) | SDK Call slot. Keeps userData=16 and entry checks. SWF paths need testing. |
| Menu input | MenuOpenHandler slot 0x08; void(MenuOpenHandler*, ButtonEvent*) | SDK OnButtonEvent slot. Pointer constness leaves ABI unchanged. Check event policy. |
| Idle IDs | TESIdleForm slot 0x3B; bool(TESIdleForm*, const char*) | SDK SetFormEditorID slot. Caches non-paired idles. Check load timing. |
| Screenshot | REL::ID(2228913)+0x2C6; void(const char*, BSGraphics::TextureFileFormat, uint32_t) | Original Main::Swap call. E8 opcode checked. Instruction boundary/ABI needs binary analysis. |
| UI draw | HUDMenu slot 0x06; void(IMenu*) | SDK PostDisplay slot. Check hidden HUD, context ownership and resize. |
| Window input | GWLP_WNDPROC; LRESULT(HWND,UINT,WPARAM,LPARAM) | Win32 hook. CallWindowProcA forwards the saved procedure. Check window recreation. |
| Console | SCRIPT_FUNCTION entry TogglePathLine; SDK execute signature | Replaced with PhotoMode. Optional integer action parsed by SDK. User confirmed recovery command. |
| MCM | F4SE Scaleform callback; GFx::FunctionHandler::Call(const Params&) | Queues the same recovery. User confirmed dispatch. New labels need testing. |

## Camera evidence

The original roll hook used REL::ID(2248281)+0x1B.
Early replacement hooks received no calls in the affected setup. They were removed.
A user trace found SAM's getter at ScreenArcherMenu.dll+62550 in live slot 0x0C.
The original EXE getter at RVA 0x1022520 was bypassed.

The supplied state.Update bytes show these calls:

- RVA 0x102243A loads the state vtable.
- RVA 0x102243D loads the output buffer. RCX holds the state.
- RVA 0x1022444: FF 50 60 calls slot 0x0C.
- RVA 0x1022451: FF 50 68 calls slot 0x0D for translation.

These are observed RVAs, not patch offsets. SAM's inspected public source lacks
that replacement getter. Its source cannot certify the installed binary.
The hook captures the live getter on activation. It calls downstream once.
It adds local-Y roll while Photo Mode is active. Exit removes that contribution.
It refuses to re-chain a later wrapper that could cause recursion.
No SAM DLL address is hardcoded. The tester confirmed visible roll works.

## Capture

Source fixes cover alpha, missing rows, zero hardware concurrency and failed writes.
Images use straight RGBA8. Every row uses rowPitch. PNG alpha is flattened to 255.
DirectXTex handles readback and channel conversion. WIC handles PNG encoding.
Capture is synchronous. No worker outlives its image buffer.

RenderTargetManager.renderTargetID[63] selects the capture target.
RendererData supplies the device, context and texture.
Target meaning, layouts, HDR, MSAA and typeless textures still need game checks.
The user confirmed normal screenshot appearance. That does not cover every GPU mode.

Photo names use uint64_t. Startup scans the chosen folder even without settings.
Encoded PNGs use CreateFileW(CREATE_NEW). Existing files cause a retry.
Failed writes clean up through the owned handle. Exhaustion fails without wrapping.
Queued failures never fall through to the vanilla writer.

## State and structures

| Area | Accesses and limits |
| --- | --- |
| Camera | PlayerCamera state/FOV, Main::SetCameraFOV, speed setting and DOF instance. Check restoration. |
| Input | Event fields, ControlMap TFC mapping and FreeCameraState worldZDirection. Check device behavior. |
| Controls | Input layers and handler flags. Restore only recorded handlers. Keep prior save restrictions. |
| HUD | menuSystemVisible, HUD modes and pause-menu SWF paths. Check hidden-HUD entry/exit. |
| Time | Main freeze flag, BSTimer, Calendar and Sky weather fields. Check restoration. |
| Character | Actor angles, position, visibility, process idles/packages and effects. Check game behavior. |
| Filters | ImageSpaceManager overrides and tint. Restore only an override still owned by Photo Mode. |
| Rendering | Swap chain, render window and graphics dimensions. Check focus and resize. |
| Translation | BSScaleformManager loader and translator. Missing language settings fall back to EN. |

The Address Library resolves SDK IDs, RTTI and vtables. No separate RTTI offsets
or signature scanner are used. The trampoline is initialized through F4SE (128 bytes).
A valid ID alone does not prove an instruction offset or function ABI.

SAM's native menu name is ScreenArcherMenu at the inspected revision in DEPENDENCIES.md.
Photo Mode suspends its drawing/input while that menu is open. SAM's controls stay active.
Vertical input is cleared only if the current state and last written direction match.
A saved state pointer is never dereferenced during that check.

## Crash evidence

The 2.1.7 crash target 0xFFFF0AC7 matched a Win32 procedure token.
Its PDB placed Renderer::WndProc::thunk at RVA 0x22FB0.
Disassembly showed a direct jump through the saved token.
The fix forwards with CallWindowProcA. Native ANSI/Unicode tests pass.
The affected user confirmed the startup crash was resolved.
Reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-callwindowproca

The 2.1.10 crash at RVA 0x554DD dereferenced [RAX+8] with RAX=0.
Its PDB mapped that address to GetGameLanguage. Callers were BuildTranslationMap
(0x55212) and MessageHandler (0x2CD53). The missing sLanguage:General lookup was
unchecked. The EN fallback fixed that user's crash. One language result is reused
for file and Scaleform loading.

## Movement recovery

The lock persisted with Photo Mode, SAM and tested movement mods disabled.
The affected third-person samples had bIsSynced=true. Working samples had false.
Forward input arrived, but affected movement mode and graph speed stayed zero.
This is evidence for a sync-state problem. It does not identify the original trigger.

The manual fix calls the existing player graph getter/setter.
It refuses invalid camera/mode/interaction states and animation-driven movement.
It writes only bIsSynced=false. The setter may update multiple player graphs.
It never runs automatically. The user confirmed command recovery and MCM dispatch.
Recovery persistence after save/reload is still unverified.

The MCM callback uses F4SE's Scaleform registration. It queues the same fix through
the game task interface. FunctionHandler uses the SDK allocator/refcount lifecycle.
No ESP, Papyrus script or new engine offset is needed.

## Translation follow-up

Missing modern keys all followed a comment row in the shipped English file.
2.1.16 removes comment rows. It normalizes entries to tab-separated UTF-16LE.
A separate MCM English file loads first. It keeps new keys available when an older
PhotoMode_EN.txt overrides the main file. Localized files still load last.
The parser's exact failure behavior needs binary inspection. The corrected file
format and older-file fallback need in-game confirmation.

MCM's inspected native button is a clickable label row. It has no right-hand widget.
The action now includes [Run]. No MCM SWF is modified.

## Report limits

Upstream GitHub issues were empty during the original review. Nexus access returned
HTTP 403. Those sources did not establish additional reproducible bugs.
The missing BGSFonts.h held embedded controller glyph data. Its rights were unknown.
Text labels replace it. External ImGui Fonts remains required.
See TESTING.md for the remaining game checks.
