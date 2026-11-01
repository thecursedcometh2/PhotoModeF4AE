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

Install the original Photo Mode runtime distribution for its MCM, localization,
pause-menu integration and overlays, then replace `Data/F4SE/Plugins/po3_PhotoModeF4.dll`
with this build. The authoritative source checkout contains **no** ESP/ESL,
Papyrus scripts, SWF, MCM JSON, localization files, or overlay assets. These cannot
be reconstructed or bundled from this checkout. Do not mistake the DLL archive
for the full original mod. No original tracked assets have been removed.

## Building

Use VS2022 C++ tools and Windows SDK, XMake **3.0.3**, Git and Python 3:

```powershell
git submodule update --init --recursive
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
DLL, PDB, dependency lock, a corresponding-source ZIP, and a DLL-only installation
ZIP with documentation/notices. Build failure
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
