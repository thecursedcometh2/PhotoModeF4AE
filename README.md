# Photo Mode for Fallout 4 1.11.240

Modernization of [powerofthree's Photo Mode](https://github.com/powerof3/PhotoModeF4).
Target: Fallout 4 **1.11.240.0**, Windows x64. Compatibility is under validation;
see [runtime audit](RUNTIME_AUDIT.md) and [test matrix](TESTING.md) before use.

## Requirements and installation

Install the current Microsoft Visual C++ 2015–2022 x64 Redistributable, matching
F4SE, the Address Library database for 1.11.240, and the mandatory
[ImGui Fonts mod](https://www.nexusmods.com/fallout4/mods/88107). Fonts are loaded
from `Data/Interface/ImGuiFonts` (default Roboto-Regular.ttf and Font Awesome solid).
Do not bundle fonts with this plugin. Missing fonts produce a logged fallback,
not a replacement for the external dependency. Controller prompts use readable
Xbox/PlayStation labels because the original embedded glyph header is absent.

Install this build's **PhotoModeF4-1.11.240.zip** through your mod manager, including
its `Data/MCM/Config/PhotoMode` files. Installing only the DLL does not install MCM
support. Install a working **1.11.240-compatible** [Mod Configuration Menu](https://github.com/reg2k/f4mcm)
separately to use the settings pages and optional global hotkey. This package does
not port or bundle MCM's native plugin. Pause-menu entry and the `PhotoMode`
console command also work without MCM.

Original overlay artwork is still a separate installation from the original mod
archive; no third-party fonts, artwork, ESP/ESL, scripts or SWFs are bundled.
The authored MCM settings use the native plugin directly and need no Photo Mode
quest or ESP. Preserve existing `Data/MCM/Settings/PhotoMode.ini` and MCM's global
keybind saves when upgrading; the package supplies defaults, never user overrides.

In **MCM > Photo Mode**, General offers camera speed, freeze-on-entry, pause entry,
screenshot options, controller prompts, hold duration and an optional global
Toggle Photo Mode hotkey (initially unbound). Keyboard / Mouse and Controller
pages rebind the six in-mode actions. Controller buttons use dropdowns because
the inspected MCM key-capture widget only captures keyboard/mouse input. Avoid
binding actions to the same key or to free-camera movement controls. Escape and
Tab retain UI navigation roles. Close the pause menu to reload settings; entry
options apply on the next activation. Legacy `i*Key` / `i*GamePad` INI bindings
remain readable; the new MCM bindings take precedence within the same file.

While Screen Archer Menu is open, Photo Mode hides its UI and suspends its own
input, releasing vertical motion it previously synthesized. It leaves SAM's
input handler, input layer and camera manipulation enabled. Closing SAM resumes
Photo Mode. This behavior still requires in-game verification with your SAM build.

## Building

Use VS2022 C++ tools and Windows SDK, XMake **3.0.3**, Git and Python 3:

```powershell
git submodule update --init --recursive
xmake g --pkg_searchdirs="$((Get-Location).Path)/vendor"
xmake f -p windows -a x64 -m releasedbg --toolchain=msvc -y
xmake -y -vD
python scripts/package_release.py --dll build/windows/x64/releasedbg/po3_PhotoModeF4.dll --pdb build/windows/x64/releasedbg/po3_PhotoModeF4.pdb --output dist --source-bundle
```

The official CommonLibF4 plugin rule generates F4SE metadata and Windows resources.
No local game installation, VCPKG_ROOT, CommonLibF4Path or personal secrets are
needed. Git submodules pin CommonLibF4, its recorded Shared dependency, original
custom ImGui, CLibUtil and package recipes. See [DEPENDENCIES.md](DEPENDENCIES.md).
The Photo Mode CMake/vcpkg integration has been removed after a successful Windows
XMake build. CMake remains only an internal builder used by some package recipes.

GitHub Actions runs on pushes, PRs and manual dispatch, checks exports and uploads
DLL, PDB, dependency lock, a corresponding-source ZIP, and an installation ZIP including MCM configuration
and documentation/notices. Build failure
is a release blocker. Review the workflow logs and perform TESTING.md before
publishing a compatible release. Portable checks on Linux:

```bash
python3 -m unittest discover -s tests -v
g++ -std=c++23 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude tests/math_tests.cpp -o /tmp/photomode-tests
/tmp/photomode-tests
```

Linux is supported for editing and portable tests,
not for building the F4SE plugin.

Original author: powerofthree. Thanks to libxse, Dear ImGui and the dependency
authors listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Original MIT
copyright remains in LICENSE.txt; the linked DLL is not advertised as MIT-only.

English UI text is built in. An optional UTF-16LE/BOM
`Data/Interface/Translations/PhotoMode_<LANGUAGE>.txt` overrides defaults (EN is
loaded before the selected language). Missing keys retain readable defaults.
`Data/Interface/PhotoMode/fonts.ini` is optional: bare font names are resolved
under `Data/Interface/ImGuiFonts` even when that configuration file is absent.

To restore the original overlay artwork, extract **only** the original mod's
`Data/Interface/PhotoMode/overlays` folder into the active game/mod-manager Data
folder. For example, vignette must be installed as
`Data/Interface/PhotoMode/overlays/EFFECTS/VIGNETTE.png`. Keep the newly built DLL
when copying original assets. The DLL package does not supply these PNG files.
The plugin logs the overlay directory, successful load count and load failures.

Screenshot filenames are recovered from the selected output directory on every
startup, independently of optional MCM settings. New PNG files are created
exclusively, so stale settings or simultaneous captures cannot replace existing
photos. Legacy Screenshot4294967295.png files are retained and numbering can
continue above that value using a 64-bit counter.

## AI Disclaimer
This was updated using Codex 6.1 Sol.

## License
powerof3's Photo Mode is distributed under the MIT license. libxse's CommonLibF4 is distributed under GPL-3.0 with a Modding Exception. This mod is distributed under GPL-3.0 with a modding exception. See [MIT LICENSE](https://github.com/thecursedcometh2/PhotoModeF4AE/blob/master/MIT%20LICENSE), [LICENSE](https://github.com/thecursedcometh2/PhotoModeF4AE/blob/master/LICENSE), and [EXCEPTIONS](https://github.com/thecursedcometh2/PhotoModeF4AE/blob/master/EXCEPTIONS).

SAM remains optional: without SAM, Photo Mode chains the vanilla game getter.
Neither mod requires the other to be installed.

Version 2.1.9 retains the user-tested roll fix and corrects window-message
forwarding through CallWindowProcA, including Windows opaque procedure tokens
and ANSI/Unicode subclass chains. Photo Mode
chains the live rotation getter after startup, preserving SAM's rotation and adding
the selected Photo Mode roll. The selected roll remains applied while SAM is open;
its mouse-input guard remains active. Exiting Photo Mode removes its contribution.

Temporary camera tracing, per-slider diagnostics and counters have been removed.
Normal startup and error logging remain. Windows regression checks cover both
SAM-style replacement and vanilla getters; see TESTING.md for validation details.

MCM labels reuse the original mod's `$PM_...` translation keys. Install existing
`PhotoMode_<LANGUAGE>.txt` translations over this package as usual. Shipped UTF-16
English strings provide fallback for newer controls; older translations need not
be complete. The plugin loads these files into Scaleform without requiring an ESP.
Settings IDs, bindings, numeric ranges and the optional global hotkey are unchanged.

If third-person movement is stuck repeating the last first-person movement, switch
to third person, exit Photo Mode and SAM, then open **MCM → Photo Mode → Settings →
Third Person Fix** outside an interaction. This runs the same manual recovery as
`PhotoMode 1`; it is never applied automatically. The affected user confirmed
movement recovery with the command. The MCM button still needs in-game confirmation.
