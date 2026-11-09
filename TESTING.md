# Validation

Target: Fallout 4 1.11.240.0, Windows x64, matching F4SE and Address Library.

The tester confirmed that UI/font fixes, screenshot appearance, MCM/SAM integration
and the 2.1.7 view-roll fix work in game. Version 2.1.8 is a cleanup of that working
implementation: it removes temporary diagnostics, counters and unused comments,
without changing the rotation chain, SAM input guard or screenshot path. These
observations are targeted user testing, not certification of every runtime hook.
Remaining ABI risks are recorded in RUNTIME_AUDIT.md.

## Automated checks

Portable checks:

```bash
python3 -m unittest discover -s tests -v
g++ -std=c++23 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude tests/math_tests.cpp -o /tmp/photomode-tests
/tmp/photomode-tests
```

Windows checks after configuring/building XMake as described in README:

```powershell
xmake photomode-hook-tests
xmake run photomode-hook-tests
xmake photomode-image-tests
xmake run photomode-image-tests
```

The workflow compiles releasedbg, checks F4SEPlugin_Load/F4SEPlugin_Version exports,
requires DLL/PDB output, and creates install/corresponding-source ZIPs. Python
checks validate MCM control/default/keybind contracts and install layout, including
license notices and exclusion of user settings/external fonts.

Portable C++ checks cover quaternion roll, image compositing/row bounds, input
ownership, key parsing, overlay discovery and nonnegative 64-bit screenshot indices.
Native Windows fixtures reproduce SAM overwriting an early Photo Mode getter,
then verify late chaining, a single downstream call/contribution, 1000 idempotent
updates without accumulation, changed SAM rotation, zero/exit behavior, later-wrapper
recursion protection and operation without SAM. They retain safe pointer-read checks.

Native DirectXTex/WIC tests cover BGRA/sRGB channel preservation, opaque PNG output
with/without overlays, complete rows, encode failures, overlay decoding on a fresh
worker thread, English defaults, font paths and distinct translated widget IDs.
Screenshot preservation tests cover restart without INI, stale counters, legacy
4294967295 filenames, concurrent writers and uint64 exhaustion. These tests do not
execute Fallout 4, GPU readback or its screenshot hook.

The working 2.1.7 source passed Windows compilation, tests, exports and packaging in
[run 41388030376](https://github.com/thecursedcometh2/PhotoModeF4AE/actions/runs/41388030376).
The final cleanup has its own workflow run; use its result when evaluating that build.
Normal startup/error logging remains; temporary roll counters, address traces and
code snapshots are not expected in 2.1.8 logs.

## In-game matrix

Use a backed-up save and record executable/F4SE/Address Library, GPU, resolution,
HDR/SDR and active UI/camera mods. The following remains the full acceptance matrix;
not every combination has been individually recorded as passed.

| Area | Steps and expected outcome |
| --- | --- |
| Activation | Console PhotoMode, optional MCM hotkey and pause entry; UI appears once, all tabs render |
| Camera | First/third-person entry, movement, pitch/yaw/FOV/speed extremes; prior camera/FOV/speed restore |
| Roll | +/-90, zero, move/look, frozen wait, reset, exit/reenter; no accumulating or residual Photo Mode roll |
| SAM | Keep nonzero roll, open/close SAM repeatedly; roll remains applied, SAM position/yaw/pitch/FOV work, LMB/RMB clicks cause no Photo Mode vertical motion |
| Standalone | Repeat roll/exit and controls without SAM; neither mod requires the other |
| MCM | General, keyboard/mouse and controller pages; settings apply on pause close or next entry as documented; bindings persist over restart |
| Screenshots | Plain/overlay PNG alpha is 255, channels correct, all rows present; verify image/screenshot orientation while rolled |
| Preservation | Take photos, restart and take more; original bytes unchanged in custom and normal folders, including legacy 4294967295 names |
| Overlays | Empty/missing/invalid files, nested categories, vignette, varied sizes, intensity and compositing |
| DOF/filters | Enable, near/far/range/strength, reset and exit; original effects restore |
| Input | Keyboard/mouse/Xbox/PlayStation, analog triggers, remapping/focus loss; no stuck keys or movement |
| HUD | Enter with HUD visible/hidden, toggle, screenshot auto-hide and exit; entry visibility restores |
| State | Freeze/time/weather, idle/expression/visibility, exit on menus; original state restores |
| Save/load | Exit, save/load, return to main menu, repeat activations; no persistent camera/input state or crash |
| Assets | External ImGui Fonts mandatory; original overlay art separate; missing translations/fonts retain usable defaults and actionable errors |

Inspect PNG alpha extrema with an image tool. Back up original photos before the
preservation test. The output uses exclusive file creation and never falls back to
an unchecked vanilla writer for failed queued Photo Mode captures. No rollback or
screenshot-counter edits should be necessary.

## Window-message startup crash (2.1.9)

A supplied Addictol log reports execution at 0xFFFF0AC7, USER32 message handling,
and Photo Mode RVA 0x22FB0. The exact 2.1.7 PDB maps that RVA to
ImGui::Renderer::WndProc::thunk; its compiled tail jumps to the saved procedure
pointer in RAX. Windows permits that saved value to be an opaque internal token,
which must be forwarded with CallWindowProc rather than called directly.

2.1.9 uses CallWindowProcA, matching the existing SetWindowLongPtrA subclass API.
It also avoids accessing absent ImGui context on focus loss and has a default
procedure fallback when no previous procedure was captured. Native Windows tests
use hidden ANSI/Unicode/system-default windows with earlier and later subclass
handlers, checking text conversion, return values, single delivery and fallback.
The actual affected user's setup still needs a retry; the stack scan does not
establish which other plugin, if any, selected the previous procedure token.

In game: retry the same startup/save with Photo Mode enabled, then test focus
loss/return, menus, Photo Mode and SAM. Keep the normal DLL dependency set for an
existing save; stripping every F4SE plugin is not an equivalent load test.

## Original MCM translations (2.1.10)

The original config.json and UTF-16 English translation file were supplied by the
user. Matching labels/help/options now reuse their exact original keys; modern
controls have separate English fallback keys. Existing setting IDs/types, ranges,
keybind actions and the optional ESP remain unchanged. The provided English file
is included, with additional modern-control strings. CommonLib's translator file
loader registers English then the selected language with Scaleform independently
of active ESP translation loading. Tests check all displayed keys have English
values and validate the existing MCM contract/package layout.

In game, test EN and an existing original-mod language file, with and without
PhotoMode.esp. Open every MCM page and options list; original entries should use
that language, missing new keys should remain English, and controls should still
apply/persist. Native build checks do not certify the in-game translated UI.

## Missing language setting (2.1.11)

A supplied 2.1.10 crash occurs at Photo Mode RVA 0x554DD, reading [RAX+8]
with RAX=0 immediately after GetINISetting("sLanguage:General"). The matching PDB
places it inside Translation::Manager::GetGameLanguage; caller RVAs map to
BuildTranslationMap and the F4SE data-ready message handler. The plugin log stops
after reading PhotoMode_EN.txt, consistent with this path.

The resolver now handles a missing/empty setting with EN, preserves and normalizes
non-English codes, and caches one resolved language for both file and Scaleform
loading. Portable tests invoke the production resolver with null, empty, lower/
uppercase and multi-character language values. In game, retry affected new-game
and save-load paths and existing translations; actual game confirmation is pending.

## Intermittent third-person movement lock (2.1.12 diagnostic build)

Reported on 2.1.9: after some Photo Mode exits, first-person movement works, but
third person retains the standing/running/crouching/sprinting state from first
person and cannot change it. SAM involvement is unknown. This report is not yet
reproduced here, and the log supplied contains initialization only.

The build prevents repeated activation/deactivation from replacing saved state
or toggling the camera twice, and only toggles free camera off if Photo Mode
entered it and it is still current. These lifecycle corrections are not proof
of a fix for the reported movement problem. It does not alter animation graph
variables, clear third-party input layers, or force all player controls on.

On a disposable save, compare repeated entry/exit from first and third person,
stationary and moving, keyboard and controller, freeze time enabled/disabled,
Pause Menu and hotkey entry, Escape/hotkey exit, and SAM installed/open/absent.
Also test entering when TFC was already active: exit must preserve it. If another
menu changes the camera during Photo Mode, exit must not toggle free camera on.
Try repeated Pause Menu activation while Photo Mode is already active.

After exiting, wait at least two seconds. If movement sticks, switch to first
person, move, switch back, and describe exactly what persists. Supply the full
Photo Mode log (with `Movement trace` lines before entry, before/after exit and
two seconds later) and the SAM version/session sequence. Compare camera flags,
input cache flags, disabled handler indices and first/third-person movement graph
indices. These fields are read-only diagnostics; engine values and animations
still require an affected installation for interpretation. Do not overwrite your
only save while testing; existing stuck saves are not claimed repaired.

## Animation comparison (2.1.13 diagnostic build)

Read-only animation samples run through the F4SE game-thread task queue, at entry/
exit requests and four timed requests approximately 2, 7, 12 and 17 seconds after
exit. Actual execution can be delayed by paused menus; the label identifies the
request and each sample records the camera at execution time. Queued samples do
not represent an exact synchronous before/after snapshot. Actor state, flags,
graph count/active index and optional named bool/float probes are logged. An
unavailable probe is not evidence that a variable is false or zero. Graph names
and manager layout/virtual APIs require affected-runtime validation; these values
are diagnostic leads, not a complete animation graph dump or a repair.

Use the same mod configuration for the affected save and a working new game.
Stand in third person, enter/exit Photo Mode, then hold forward for five seconds,
release for five seconds, switch to first person and repeat, then switch back to
third person. Wait 20 seconds after exit before quitting. If samples miss a step,
repeat entry/exit from that perspective and move immediately after exit. Copy the
full log before starting the second run and note which log represents which save
and which actions got stuck. No console commands or saving are required. Re-enable
Photo Mode to obtain these samples; other mod settings must match between runs.

## Explicit sync recovery experiment (2.1.14)

The paired 2.1.13 logs show bIsSynced=true in every affected third-person graph
sample (activeGraph=0), false in its first-person samples (activeGraph=1), and
false in both perspectives of the working new game. Forward input is recorded
in affected third person, but sampled moveMode/Speed stay zero. This is a strong
correlation, not proof of the original trigger or a verified recovery.

On a disposable copy of the affected save, outside combat/paired animations/
furniture, exit Photo Mode and SAM, switch to third person, open the console and
run `PhotoMode 1` once. It changes only the player's bIsSynced graph variable via
the existing CommonLib setter. It refuses the wrong camera, active Photo Mode/
SAM, synced actor interaction/killmove, animation-driven locomotion, or unavailable
required variables. No automatic reset occurs on entry, exit, load or new game.
The setter may update multiple player graphs; it is not a third-person-only write.

Record the console message, close the console and test starting/stopping/turning,
then first/third-person switching. Send the full log and observed outcome. A false
readback confirms only that the getter reads false, not that movement is repaired
or stays repaired. Do not overwrite the original save. If movement works, save to
a new slot and separately test reloading that slot; persistence is unverified.
The no-argument `PhotoMode` command and MCM toggle must continue to work. Invalid
actions must print usage without toggling or writing graph variables.

## Third Person Fix MCM button (2.1.15)

The user supplied a successful 2.1.14 readback log and confirmed that `PhotoMode 1`
restored movement. This confirms recovery for that affected save, not the original
trigger or persistence after save/reload. MCM's inspected OptionsList.onButtonPressed
supports CallExternalFunction; RunConsoleCommand is a hotkey action and is not a
supported button action. The Settings button calls the po3_PhotoModeF4 Scaleform
plugin's ThirdPersonFix function, which queues the existing recovery routine through
F4SE's game task queue. FunctionHandler uses the SDK Scaleform allocator and its
constructor reference is released after CreateFunction retains it.

Check button visibility/localization and activation using mouse and controller.
In ordinary third person outside an interaction, click the button on an affected
save, close MCM and test starting/stopping/turning and POV switching. Check the
recovery result in the log. On an unaffected save it must report already clear.
First-person, active Photo Mode/SAM, interaction and animation-driven cases must
still refuse. Repeated clicks must remain safe and the ordinary toggle hotkey and
console command must work. No ESP, Papyrus script or MCM binary is bundled. MCM
button dispatch, repeated menu opening/closing and game movement tests are pending.
