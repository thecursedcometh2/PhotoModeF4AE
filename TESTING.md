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
