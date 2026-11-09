# Testing

Target: Fallout 4 **1.11.240.0**, Windows x64.
Use matching F4SE and Address Library builds. Install ImGui Fonts.
Use compatible MCM/SAM builds for those tests.

## Automated checks

On Linux:

```bash
python3 -m unittest discover -s tests -v
g++ -std=c++23 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude tests/math_tests.cpp -o /tmp/photomode-tests
/tmp/photomode-tests
```

Windows CI runs the Python checks and portable C++ tests. It also checks:

- DLL linking, F4SE exports and PDB output.
- Window forwarding: ANSI/Unicode, procedure tokens and subclass chains.
- Camera chaining: SAM replacement, vanilla, repeated frames and recursion guards.
- PNG output: channels, opacity, overlays, padded rows and failed encoding.
- File naming: restart, stale settings, old high indices and concurrent writes.
- UI labels, default fonts and unique widget IDs.
- Install layout, source bundle, licenses and dependency hashes.

A passing build does not certify game behavior. Fallout 4 cannot run here.

## In-game matrix

Use a copy of your save. Keep a backup. Record the DLL version and enabled mods.

| Area | Check |
| --- | --- |
| Entry | Use pause entry, hotkey and console. Repeat from first/third person. |
| Camera | Move on all axes. Test FOV, speed and roll. Exit restores normal gameplay. |
| Roll with SAM | Change roll. Open SAM. Roll stays set. SAM camera controls still work. |
| SAM input | Click SAM controls with LMB/RMB. Photo Mode must not move vertically. |
| Screenshots | Check PNG alpha is 255. Check colors, overlays and image edges. |
| Existing photos | Take photos, restart, take more. Earlier files must stay unchanged. |
| Overlays | Install vignette. Test categories, intensity and reset. |
| DOF | Enable, adjust, reset and exit. Check normal game DOF returns. |
| UI | Hide/show HUD. Alt-tab. Change resolution. Check mouse focus. |
| Input | Test keyboard, Xbox and PlayStation labels. Test capture, rebinding and navigation. |
| Character | Test visibility, transforms, idles and effects. Exit restores prior values. |
| Time | Test freeze, time scale and weather. Exit restores the prior state. |
| Exit | Exit with Escape/hotkey. Try menus during Photo Mode. Check movement and saving. |
| Pre-existing TFC | Enter with free camera already active. Exit must keep it active. |
| Save/load | Save to a new slot after exit. Reload. Repeat entry/exit. |

## Third Person Fix

The tester confirmed recovery through `PhotoMode 1` and the MCM button.
The affected third-person graph had bIsSynced=true. First person had false.
The working new game had false in both perspectives.
This identifies a useful recovery. It does not identify the original trigger.

Switch to third person. Exit Photo Mode and SAM. Use the Settings action outside
an interaction. Close MCM. Test starting, stopping, turning and POV switching.
On a working save, it should report no change needed.
First-person, active Photo Mode/SAM and animation-driven states must refuse.
Repeat menu opening/closing. Test controller activation too.
If recovery works, save to a new slot. Reload that slot to test persistence.
Temporary movement/animation traces were removed in 2.1.16.
Normal logs include startup, errors and the recovery result.

## Translations and MCM labels

In 2.1.16, check all Settings help and every gamepad option.
Check `Third Person Fix [Run]` is readable and clickable.
MCM draws this as a row. A right-hand button is not part of its native renderer.
Repeat with an older original-mod translation. New keys should fall back to English.
Also test an older PhotoMode_EN.txt override. Keep the separate fallback file installed.
Do not change original key names. After editing English text, run:

```bash
python3 scripts/update_mcm_fallback.py
python3 -m unittest discover -s tests -v
```

Both files must have a BOM. Nonempty rows must use `$key<TAB>value`.
Do not add comment rows. The tests check keys, formatting and fallback contents.
A missing game language setting must fall back to EN without a crash.

## Status

User-confirmed: camera roll, screenshot appearance, SAM input guard, startup crash
repairs, sync recovery and MCM recovery dispatch.
Pending: 2.1.16 label fixes, full matrix coverage and recovery save/reload.
Native tests cannot replace those checks.
