# Photo Mode for Fallout 4 1.11.240

A modern port of [powerofthree's Photo Mode](https://github.com/powerof3/PhotoModeF4).
Built for Windows x64. Uses libxse/CommonLibF4 with XMake.

## Install

Required:

- Fallout 4 **1.11.240.0**. Use matching F4SE and Address Library builds.
- Microsoft Visual C++ 2015–2022 x64 Redistributable.
- [ImGui Fonts](https://www.nexusmods.com/fallout4/mods/88107).
- A compatible [MCM](https://github.com/reg2k/f4mcm) build for the settings menu.

Install the enclosed **PhotoModeF4-1.11.240.zip** through your mod manager.
Include all Data files. Keep your existing MCM settings when upgrading.
The package does not overwrite user settings.

MCM is optional if you use the pause menu or `PhotoMode` console command.
SAM is also optional. Neither mod requires the other.
No PhotoMode ESP is needed for MCM. An existing `PhotoMode_IsActive` global is
updated if present.

## Use

Open Photo Mode from the pause menu. Or set its hotkey in MCM.
**Settings** has entry options, screenshot settings, controller labels and recovery.
The other pages set keyboard or controller bindings. Avoid duplicate bindings.
Close the pause menu to apply settings. Entry options apply next time.
Legacy INI bindings still work unless a newer binding overrides them.

While SAM is open, Photo Mode hides its UI. SAM clicks do not move Photo Mode's
camera vertically. The selected roll stays applied. It resets on Photo Mode exit.

For stuck third-person movement, switch to third person. Exit Photo Mode and SAM.
Use **MCM → Photo Mode → Settings → Third Person Fix [Run]** outside an interaction.
Click the highlighted row. MCM's native button has no separate right-hand widget.
The same fix is available as `PhotoMode 1`. It never runs automatically.
The tester confirmed both the command and MCM action restore movement.
The original cause is unknown. Save/reload persistence still needs testing.

## Fonts, overlays and translations

Install fonts under `Data/Interface/ImGuiFonts`.
Defaults use Roboto-Regular.ttf and Font Awesome solid. `fonts.ini` is optional.
Missing fonts use a fallback. The external font mod is still required.
Controller prompts use text labels. The original embedded glyph data was unavailable.

Get overlay artwork from the original mod archive. Copy only its overlays folder.
Vignette belongs at `Data/Interface/PhotoMode/overlays/EFFECTS/VIGNETTE.png`.
Keep this port's DLL. Fonts and artwork are not bundled.

Translation files use UTF-16LE with a BOM. Existing `$PM_...` keys are preserved.
Load order is MCM English fallback, PhotoMode_EN.txt, then PhotoMode_<LANGUAGE>.txt.
The fallback supplies new MCM keys when an older translation lacks them.
Keep `PhotoMode_MCMFallback_EN.txt` from this package.

Photos use opaque alpha. New files never replace existing photos.
Numbering is recovered from the output folder at startup. Legacy high-numbered
filenames are supported.

## Build

Use VS2022 C++ tools, Windows SDK, XMake **3.0.3**, Git and Python 3.
No game installation is needed.

```powershell
git submodule update --init --recursive
python -m pip install --no-deps --only-binary=:all: --require-hashes -r scripts/ci-build-tools.txt
xmake g --pkg_searchdirs="$((Get-Location).Path)/vendor"
xmake f -p windows -a x64 -m releasedbg --toolchain=msvc -y
xmake -y -vD
python scripts/package_release.py --dll build/windows/x64/releasedbg/po3_PhotoModeF4.dll --pdb build/windows/x64/releasedbg/po3_PhotoModeF4.pdb --output dist --source-bundle
```

CI builds the DLL/PDB. It checks exports, runs native tests, then makes install
and source ZIPs. See [DEPENDENCIES.md](DEPENDENCIES.md) for pins.
Linux supports editing and portable tests. The plugin needs a Windows build.
See [TESTING.md](TESTING.md) for checks. Runtime limits are in [RUNTIME_AUDIT.md](RUNTIME_AUDIT.md).

## Credits and license

Original author: powerofthree. Thanks to libxse and the library authors.
The original MIT notice stays in LICENSE.txt. The combined DLL uses
GPL-3.0-or-later with exceptions. Ship its matching source and notices.
Read [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), LICENSE and EXCEPTIONS.
License text and copyright notices are unchanged.

This port was updated with Codex 6.1 Sol.
