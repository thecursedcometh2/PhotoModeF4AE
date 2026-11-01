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
