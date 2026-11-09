# Dependencies

Target: Fallout 4 **1.11.240.0**, Windows x64.
Use matching F4SE and Address Library builds.

## Git pins

All build dependencies below are recursive submodules.

| Project | Commit | Purpose |
| --- | --- | --- |
| libxse/commonlibf4 | 7c8c6f8a349ac85abf6fc8a7e5a71732d9339954 | Game/F4SE ABI, official plugin rule |
| libxse/commonlib-shared | 29fbdb0e2dc548c9ab22f6964981d75090dc9094 | Recorded recursive gitlink; REL/REX |
| powerof3/imgui | 55dc9c8badfbd7464cff5da4639b249b91cca225 | Original custom text shadows and dynamic fonts |
| powerof3/CLibUtil | e1eabe6a063a3abcd3e5b1e739e6ca842fc9a67c | Original INI, RNG, string helpers |
| xmake-io/xmake-repo | 56066557b57047b65d60d354bdb9f80e7b878704 | Package recipes and archive checksums |

CommonLib Shared uses CommonLibF4's recorded gitlink.
The newer main revision 79bccb6430237eac766c7e45c9c7a0523bdf92e4 is not used.
A version header is not proof of hook compatibility. See RUNTIME_AUDIT.md.

Original source: powerof3/PhotoModeF4 d47942eadd0004f9266027ad23ea49c49393f955.
This matches the repository's starting source. No experimental fork was imported.

## Tools

XMake **3.0.3**: de7aca41098886112f693dbbb590c2facf7fa533.
CI uses a commit-pinned setup action.
VS2022/MSVC v143 and Windows SDK come from windows-2022.
Hosted compiler/SDK patch versions can change. CI prints the selected versions.
Build settings: C++23, x64, MD runtime, static libraries, Release with symbols.
No game installation is needed.

The lock requests CMake 3.31.6, Meson 1.12.1, Ninja 1.13.2, pkgconf 2.5.1 and
Python 3.14.3. XMake can use compatible host tools instead.
CI supplies Meson 1.12.1, setuptools 80.9.0 and wheel 0.45.1.
Their wheel hashes are in scripts/ci-build-tools.txt.
This avoids a changed XMake Python mirror archive. Hash checks remain enabled.
CMake builds dependencies only. Photo Mode itself uses XMake.

## Linked packages

| Package | Version | License |
| --- | --- | --- |
| DirectXTex | 2025.07 | MIT |
| FreeType | 2.13.3 | FreeType License option |
| RapidFuzz | v3.3.3 | MIT |
| SRELL | 4.063 | BSD-2-Clause |
| unordered_dense | v4.8.1 | MIT |
| spdlog | v1.16.0 | MIT |

The pinned recipes fix archive hashes. CommonLib Shared configures spdlog with
std::format and wchar support. scripts/dependency_sources.json records the six
linked source archives. Source packaging verifies their hashes.
Keep xmake-requires.lock. Do not update dependencies implicitly for a release.

SRELL's old URLs now return HTML. vendor/srell-4.063.zip preserves the original
archive from the successful 2.1.10 source bundle. Its SHA256 is unchanged:
15aa0e193bace7677f1a4a7fe75071b1a95b64d45cd67ea2a23d0a48ce5911ff.
The documented XMake package search path uses that archive.
Its BSD notice is also in licenses/SRELL-BSD.txt.

MinHook, rsm-mmio and Xbyak are not linked. Hooks use CommonLib Shared's trampoline.
The old CMake/vcpkg files were removed after the Windows XMake build passed.

## Optional integrations

These are inspected references. They are not linked or bundled.

| Integration | Inspected revision | Contract used |
| --- | --- | --- |
| reg2k/f4mcm | e2b187a82e87e63571f6f59ec3c29d505e02779e | Config/settings/keybinds JSON, typed INI settings, RunConsoleCommand action |
| maximusmaxy/ScreenArcherMenu | 97ce9c286b2ffcd91eed9d51f847631ff51664d6 | Native menu name `ScreenArcherMenu`; independent input and direct free-camera controls |

MCM source and its shipped SWF were inspected with JPEXS 26.4.0.
Buttons support CallExternalFunction. RunConsoleCommand applies to hotkeys.
The native button renderer draws a clickable row. It has no right-hand widget.
The inspected MCM backend targets 1.10.984. Use a compatible maintained build.

SAM's installed replacement getter was identified by the user's runtime trace.
The inspected public source lacks it. Its observed DLL RVA is never hardcoded.
The live callback is chained. SAM remains optional.

No third-party font or artwork files are bundled.
See THIRD_PARTY_NOTICES.md for attribution and source distribution requirements.
