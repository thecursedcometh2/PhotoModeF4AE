# Dependency inventory

Target: Fallout 4 **1.11.240.0**, matching F4SE and Address Library database.
The following git dependencies are submodules, not branch downloads:

| Project | Commit | Purpose |
| --- | --- | --- |
| libxse/commonlibf4 | 7c8c6f8a349ac85abf6fc8a7e5a71732d9339954 | Game/F4SE ABI, official plugin rule |
| libxse/commonlib-shared | 29fbdb0e2dc548c9ab22f6964981d75090dc9094 | Recorded recursive gitlink; REL/REX |
| powerof3/imgui | 55dc9c8badfbd7464cff5da4639b249b91cca225 | Original custom text shadows and dynamic fonts |
| powerof3/CLibUtil | e1eabe6a063a3abcd3e5b1e739e6ca842fc9a67c | Original INI, RNG, string helpers |
| xmake-io/xmake-repo | 56066557b57047b65d60d354bdb9f80e7b878704 | Package recipes and archive checksums |

CommonLib Shared main 79bccb6430237eac766c7e45c9c7a0523bdf92e4 is **not** used:
no reason to override CommonLibF4's tested API pairing was established.
The CommonLibF4 version header names 1.11.240 as latest; that does not verify
Photo Mode's hook instruction offsets or game behavior.

Build tool: XMake 3.0.3, upstream commit de7aca41098886112f693dbbb590c2facf7fa533.
Windows CI installs the fixed release through a commit-pinned official setup action.
MSVC v143 / VS2022 and Windows SDK come from windows-2022. Hosted images receive
updates, so the compiler/SDK patch versions are not byte-for-byte pinned; CI prints
the selected compiler. C++23, MD runtime, static dependency libraries, releasedbg.
No game directory is used during compilation.

Package archives: DirectXTex 2025.07 (MIT), FreeType 2.13.3 (FTL/GPL dual license;
select FTL), RapidFuzz v3.3.3 (MIT), SRELL 4.063 (BSD-2-Clause),
unordered_dense v4.8.1 (MIT), spdlog v1.16.0 (MIT, compiled with std::format and
wchar support by CommonLib Shared). Recipes and their expected archive hashes
are fixed by the xmake-repo gitlink. CMake is an internal third-party package
builder only, not the Photo Mode build system. The Windows-generated
`xmake-requires.lock` is committed: requested transitive tool versions are CMake
3.31.6, Meson 1.12.1, Ninja 1.13.2, pkgconf 2.5.1 and Python 3.14.3.
XMake may satisfy build-tool probes with compatible preinstalled host tools;
those host binaries follow windows-2022 image updates, not package archive pins.
The linked library versions and checked archive hashes remain fixed.
Do not update recipes or git dependencies implicitly for a release.

Original source baseline: powerof3/PhotoModeF4 d47942eadd0004f9266027ad23ea49c49393f955,
identical to this repository's starting commit. No experimental Photo Mode fork
was incorporated. The original CMake/vcpkg integration was removed only after Windows XMake
compilation, linking, export checks and packaging succeeded in CI.

`scripts/dependency_sources.json` records the exact six source archive URLs and
SHA256 values used for the corresponding-source bundle. Git submodule sources
(including Shared) and package recipes are included recursively without Git
metadata or optional sample font binaries. CI source packaging verifies every
archive checksum and fails if matching source cannot be retrieved.

The original vcpkg manifest also listed rsm-mmio and xbyak. Photo Mode has no
rsm-mmio API uses or generated Xbyak code; the unused legacy Xbyak include was
removed. Hooks use CommonLib Shared's REL trampoline, initialized through F4SE,
so neither unused package is a requirement of the migrated plugin.

## Optional integrations inspected

These are source references, not linked dependencies or bundled components:

| Integration | Inspected revision | Contract used |
| --- | --- | --- |
| reg2k/f4mcm | e2b187a82e87e63571f6f59ec3c29d505e02779e | Config/settings/keybinds JSON, typed INI settings, RunConsoleCommand action |
| maximusmaxy/ScreenArcherMenu | 97ce9c286b2ffcd91eed9d51f847631ff51664d6 | Native menu name `ScreenArcherMenu`; independent input and direct free-camera controls |

MCM's SettingStore.cpp, MCMKeybinds.cpp and MCMInput.cpp were inspected, along
with the shipped `dist/Data/Interface/MCM.swf` control contract (script inspection
using JPEXS 26.4.0). FO4 control types include switcher, slider, dropdown, keyinput
and hotkey; Skyrim MCM Helper types are not interchangeable. New JSON/INI files
are authored here. No MCM or SAM source, Flash assets, or runtime offsets are copied.
The inspected MCM backend targets 1.10.984; its revision is **not** proof of
1.11.240 compatibility. Install independently maintained compatible MCM/SAM builds.
Their own runtime support cannot be established by building Photo Mode.

As of 2.1.7, MinHook is no longer linked or required. Versions 2.1.5/2.1.6 used
c3fcafdc10146beb5919319d0683e44e3c30d537 for the unsuccessful early entry detour.
The trace proved the live virtual getter belongs to SAM instead. The replacement
uses CommonLib Shared's REL::WriteSafeData to chain that vtable slot on activation;
no instruction relocation library or new Git dependency is required.

The inspected SAM master at 97ce9c286b2ffcd91eed9d51f847631ff51664d6 does not contain
the installed binary's replacement getter. Its behavior is established by the
user's 1.11.240 trace (ScreenArcherMenu.dll+62550), not attributed to that source
revision. The fix captures the current slot at runtime; that observed DLL offset
is documentation only and is never used as a dependency address.

## SRELL archive availability

The upstream SRELL 4.063 URLs now return HTML rather than the ZIP.
`vendor/srell-4.063.zip` preserves the original source archive recovered from
the successful 2.1.10 corresponding-source bundle. Its SHA256 remains
`15aa0e193bace7677f1a4a7fe75071b1a95b64d45cd67ea2a23d0a48ce5911ff`,
identical to the pinned XMake recipe; no dependency code or checksum changed.
CI and the documented build command configure XMake's package search directory
to use this archive. Source packaging verifies the same hash locally. The archive
contains the upstream BSD-2-Clause notice, also retained in `licenses/SRELL-BSD.txt`.

CI installs host Meson 1.12.1, setuptools 80.9.0 and wheel 0.45.1 using exact
wheel hashes from `scripts/ci-build-tools.txt`. This lets the pinned pkgconf recipe
use the host Meson instead of downloading the XMake Python tool archive, whose
upstream asset no longer matches its recipe checksum. Linked library revisions
and checksums are unchanged; these Python packages are build tools only.
