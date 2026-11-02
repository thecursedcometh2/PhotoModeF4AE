# Validation

Target is Fallout 4 1.11.240.0, Windows x64, matching F4SE and Address Library.
Windows compilation and in-game results must be recorded separately. A green
compile is not evidence that any hook is ABI-compatible.

## Automated checks

Run `python -m unittest discover -s tests -v` and the standalone image math test
as described in README. Windows Actions configures XMake, builds releasedbg,
checks the required exports with dumpbin, requires DLL and PDB, and packages them.
It also builds/runs `photomode-image-tests`: real DirectXTex/WIC PNG roundtrips
check sRGB BGRA channel preservation, opaque output with and without overlays,
row coverage and failed-encode propagation. These tests do not exercise GPU
readback or the game's screenshot hook.
Linux cannot compile this MSVC/Windows SDK project. A Windows build of commit a555342 passed configuration, compilation, linking,
export checks, native PNG regression tests, artifact generation and corresponding-source checks in
[Actions run 41335473652](https://github.com/thecursedcometh2/PhotoModeF4AE/actions/runs/41335473652).
That establishes build readiness for that commit, not runtime compatibility.
The final revision must pass its own CI. Hook risks are recorded in
RUNTIME_AUDIT.md; do not mark unrun tests passed.

## Manual matrix (all pending)

Use a backed-up disposable save and record executable, F4SE, Address Library,
GPU, resolution, HDR/SDR, and other UI/camera mods. Check the plugin log first.

| Area | Steps and expected outcome |
| --- | --- |
| Activation | Console PhotoMode, MCM hotkey, pause menu; UI appears once and all tabs render |
| Camera | First/third-person entry, freecam movement, pitch/yaw/FOV/speed extremes; restore previous camera/FOV/speed |
| Roll | +/-90, zero, reset, move/look while rolled, exit/reenter; no residual roll in gameplay or pre-existing TFC |
| Screenshots | Plain and transparent overlays at 0/.5/1 intensity; PNG alpha must be 255 everywhere, correct RGB channels, bottom row included |
| Capture formats | SDR RGBA/BGRA, HDR if supported, odd resolutions; failed capture/encode must preserve vanilla screenshot behavior |
| Overlays | Empty folder, missing/invalid PNG, mismatched dimensions, one-axis-only resolution mismatch, category changes |
| DOF/filters | Enable/disable, near/far/range/strength, reset and exit; restore original image-space settings |
| Input | Keyboard/mouse/Xbox/PlayStation, analog triggers, remapping, focus loss; readable button labels and no stuck keys |
| HUD | Enter with HUD visible and already hidden, toggle repeatedly, screenshot auto-hide, exit; restore entry visibility |
| State | Freeze/time/weather, character idle/expression/visibility, exit on menus; restore original state |
| Save/load | Saving unavailable only during mode; exit, save/load, main-menu return, repeat 20 times; no crash or persistent camera/input state |
| Fonts/localization | Required external mod installed, missing fonts, missing/empty translation lines; log errors and retain usable fallback UI |

Use an image tool to inspect alpha extrema in every generated PNG, including
fully transparent overlay pixels. Compare captured colors against a known scene;
alpha alone cannot rule out HDR conversion or premultiplied-color errors.

## Follow-up from user testing

On runtime 1.11.240 the tester reports normal screenshot appearance and working
view roll, with a UI regression: absent translations made every label identical,
so controls shared ImGui IDs. This is user observation, not a complete runtime
certification. Built-in English labels and state-based widget IDs now cover the
missing-file case. The optional fonts.ini previously controlled whether the font
asset directory was prefixed; font path resolution now works without that file.
Windows regression checks exercise English defaults, default/qualified font paths,
stable missing-key string lifetime, and distinct ImGui IDs for equal labels.
Retest each camera slider separately, then tabs, checkboxes, and controller
navigation, both with and without optional translation/font configuration files.

The tester subsequently confirmed the translation/slider/font fixes. A raw
`$PM_Title_Menu` pause entry now uses the plugin translation map; activation uses
a separate marker instead of comparing displayed text. Missing vignette artwork
was confirmed absent from the active installation and present only in the original
archive. Install the original overlays separately and retest category selection,
vignette intensity, screenshot compositing, and localized pause-menu activation.
Portable tests cover the exact EFFECTS/VIGNETTE.png layout, nested categories,
uppercase PNG extensions and missing directories. Windows tests decode a real PNG
on a fresh worker thread using the overlay COM initialization helper.

## Screenshot filename preservation regression

The tester reported restart overwrites with Screenshot4294967295.png and
Screenshot0.png. The old signed -1 counter became UINT32_MAX in filenames;
startup scanning was incorrectly inside an optional INI callback and never ran
without that file. Initialization now always scans the selected destination,
parses nonnegative 64-bit suffixes (including legacy UINT32_MAX files), and treats
persisted settings only as hints. Production PNG output uses CREATE_NEW, retries
collisions without truncating existing files, and fails instead of wrapping at
UINT64_MAX. File writes are serialized within the manager. Failed writes mark only
the newly owned handle for deletion; existing filenames are never deleted.

Portable tests cover negative/overflowed hints, legacy filenames, custom/normal
prefixes and upper-case extensions. Windows tests save two actual PNGs, reset
the counter to simulate restart without settings, save two more, and compare the
original bytes. They also force stale-counter collisions, legacy 4294967295, two
concurrent writers starting at zero, and UINT64_MAX exhaustion.

In game: back up existing photos, take two shots, exit/restart, take two more in
both custom and normal-folder modes. Confirm four distinct images and unchanged
original file hashes. Repeat without PhotoMode INI files and with an existing
Screenshot4294967295.png; numbering may legitimately continue above that value.

Queued Photo Mode capture failures display a failure notification and suppress
the unchecked vanilla writer, avoiding any fallback that could reuse a stale
filename. Native screenshots not queued by Photo Mode retain their normal path.

## MCM and Screen Archer Menu integration

Automated: validate FO4 MCM control types, unique setting IDs, typed INI defaults,
controller dropdown ranges, console hotkey registration and packaged MCM files.
Portable C++ tests exercise captured key parsing, unbinding, rejection of modifiers
and invalid device codes, controller dropdown conversion, and release of only the
vertical direction owned by Photo Mode. These tests do not execute either mod.

Run with working 1.11.240 builds of MCM and SAM. **All tests below remain pending.**

| Test | Expected result |
| --- | --- |
| Install the full ZIP; open MCM > Photo Mode | General, Keyboard / Mouse, Controller pages display; no missing quest/ESP requirement |
| Change speed, entry freeze, screenshot folder/hiding/repeat, hold time and prompt scheme | Close pause menu; log reports reload; relevant behavior changes, entry options on next activation |
| Rebind all six keyboard actions; clear one; reset it | Updated prompts/actions, cleared action inactive, independent bindings; avoid duplicate/movement keys |
| Change all controller dropdowns | Correct buttons and prompts; navigation remains usable with nonconflicting choices |
| Bind global Toggle Photo Mode, including a modifier | Starts/stops outside pause menu via MCM's action; unbinding stops it; persists over restart |
| Restart, reopen MCM, enter Photo Mode | User values and bindings persist; shipped defaults unchanged; screenshots still never overwrite |
| Enter Photo Mode, then open SAM; click and right-click SAM controls repeatedly | No Photo Mode ascent/descent, screenshots or hotkey actions; Photo Mode UI hidden without exiting it |
| Hold LMB or RMB to move vertically, then open SAM before releasing | Previously latched Photo Mode vertical direction stops; no drift |
| With SAM open, use its position, rotation, FOV and camera drag controls | SAM camera controls still work; no camera reset caused by Photo Mode's context check |
| Close SAM; press fresh LMB/RMB and Photo Mode keys | Photo Mode resumes without a latched click/key; vertical controls work again |
| Repeat opening/closing SAM and Photo Mode, then save/load | Camera, HUD, controls, freeze and save state restore normally; no stuck input |

The SAM guard checks its registered native menu name through CommonLib UI, never
its old binary addresses. It does not disable the free-camera handler or SAM's
input layer. Another mod may replace a menu or change input ordering; verify these
transitions in game before claiming compatibility. MCM itself must already work
on 1.11.240; the inspected upstream backend targets an earlier runtime.

Screenshot counters are not written to MCM: its `i*` values use signed 32-bit
`std::stoi`, whereas filenames can have 64-bit suffixes. Existing counter hints
are read without lowering the live counter; filename scanning/exclusive creation
remain authoritative. No new counter default is registered with MCM.

## View roll regression follow-up (2.1.4)

The tester reports that MCM/SAM integration works, but roll has no visible effect
immediately after startup while the numeric value changes. This supersedes the
earlier partial roll observation; the cause has not been reproduced here.
The 2.1.4 candidate moved application to PlayerCamera::Update and passed math and
build checks, but the user's subsequent zero-call log shows that callback was not
reached. Its scene overlay and SAM roll suspension have been superseded by 2.1.5.

## Direct-entry roll follow-up (2.1.5)

The tester's `camera updates=0, roll applications=0` confirms that 2.1.4's camera
update callback was never reached. Its compilation/math success did not fix the
runtime problem. This version replaces it with a decoded GetRotation entry hook,
checked against the upstream ID/vtable, to intercept direct as well as virtual calls.
A new Windows executable tests real MinHook entry detours (not just quaternion math)
for both call types, original trampoline, repeated calls without accumulation,
changed original rotation, zero setting, inactive output and hook removal.

**In-game checks remain pending:**

1. Confirm version 2.1.5 and `View roll entry hook installed` in the log. An explicit
   ID/vtable mismatch or decoder failure requires investigation, not guessed offsets.
2. From fresh startup, change roll to +30, -30, +90, -90 and zero. Confirm visible
   rotation and screenshots. The log should report a nonzero rotation-call count
   and `View roll applied through FreeCameraState::GetRotation entry hook`.
3. Keep a nonzero roll, move/look and wait frozen. It must remain stable without drift.
4. Open SAM: the selected roll and its visible effect must remain. Change SAM
   position, yaw, pitch and FOV; click LMB/RMB controls repeatedly. SAM works and
   Photo Mode does not move vertically or consume its clicks. Close/reopen SAM:
   the Photo Mode roll setting must not reset or disappear.
5. Exit Photo Mode, including with pre-existing TFC: Photo Mode's roll contribution
   disappears, gameplay rotation remains correct, and reentering starts at zero.
   Explicit camera/tab/all reset is also allowed to return the setting to zero.

If the entry hook is installed but rotation calls still remain zero, the selected
function is bypassed too: inspecting the 1.11.240 executable or a runtime call trace
is required. Do not label an uncalled hook a successful runtime fix.

## Live camera trace (2.1.6 diagnostic)

The tester confirmed `entry hook=true, rotation calls=0, roll applications=0` on
2.1.5. Entry detouring works in the native fixture but its selected game function
is unused in that installation. **Roll is still unresolved.** No new interception
point is selected until live-object/code evidence can be inspected.

Build 2.1.6 logs a one-time trace from the known-working slider/renderer path:
player/current-state/render-camera vtables, comparisons to the generated tables,
declared update/getter slots and code samples (at most 192 bytes each). Original
getter bytes are captured before the existing detour is installed. Reads use
ReadProcessMemory with range checks; code snapshots require executable MEM_IMAGE
pages belonging to the main EXE. Only image basenames and RVAs are printed, never
installation paths. There are no new method calls to the traced targets, probes
that inflate the counters, camera-memory writes, or new hooks.

Native checks verify live fixture-vtable reads, invalid pointers, slot overflow,
basename-only image labels, bounded executable snapshots, and rejection of stack/
data pages. These validate the trace utility, not a Fallout call path.

In game, confirm 2.1.6, enter Photo Mode and change roll once. Collect the entire
`Roll trace BEGIN` through `Roll trace END` block, existing entry-hook installation
line and camera-mod names. Repeated activation must not flood the log (one block
per process). Normal MCM, screenshots, SAM clicks/camera controls and exit behavior
must remain unchanged. The actual roll fix remains blocked on this runtime evidence
or direct analysis of Fallout4.exe 1.11.240; do not guess another hook from a green CI.

## SAM replacement getter identified; late chain (2.1.7)

The tester supplied the full trace: SDK/live tables match, but state.GetRotation
is ScreenArcherMenu.dll+62550. Disassembly of the supplied state.Update bytes
confirms the indirect call through slot 0x0C at EXE RVA 0x1022444. This explains the
zero original-entry counts and supersedes the preceding unresolved diagnosis.

Native Windows tests now reproduce the actual ordering failure: an early Photo
Mode wrapper is overwritten by a SAM-style getter that does not call it. The real
late-chain helper captures that replacement and preserves its output while adding
Photo Mode roll exactly once. Checks cover 1000 idempotent installs/calls without
self-recursion or accumulation, SAM changing its baseline, Photo Mode exit/reset,
later wrappers that already delegate to Photo Mode, guarded restoration, and
operation without SAM. The old entry detour tests/dependency are replaced.

In game (pending):

1. With SAM installed, enter Photo Mode. Confirm version 2.1.7 and `View roll chained
   after ScreenArcherMenu.dll+...` (the offset may differ by SAM version).
2. Change roll through +/-30, +/-90 and zero. Confirm visible rotation/screenshots,
   nonzero rotation-call counts and `View roll applied through the chained live
   rotation getter`. Wait frozen, move/look: no accumulating roll.
3. Keep nonzero roll, open SAM and use its position/yaw/pitch/FOV controls. The
   selected Photo Mode roll and its effect remain. LMB/RMB SAM clicks do not move
   Photo Mode vertically. Close/reopen SAM repeatedly; no reset or disappearance.
4. Exit/reenter Photo Mode, also with pre-existing TFC: only Photo Mode's contribution
   disappears on exit, SAM's own baseline survives, selected roll starts at zero.
5. Repeat activation several times; the hook is not stacked and angles do not double.
   Test without SAM as well: log captures Fallout4.exe's getter instead.
6. Any `View roll chain unavailable` warning needs investigation; do not force a
   later callback back into the slot because that can create a delegation cycle.

The runtime trace confirms the cause/callsite. It does not certify the corrected
DLL's in-game behavior; native success and the above pending checks remain separate.
