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
