# Licensing and redistribution

Photo Mode's original source is Copyright (c) powerofthree, MIT; the unchanged
original notice is LICENSE.txt. This modernization does not declare the combined
DLL MIT-only.

libxse/CommonLibF4 and CommonLib Shared are GPL-3.0-or-later with the modding and
linking exceptions copied into licenses/. The modding exception permits interaction
with the proprietary game; it does not remove obligations for library code.
The linking exception requires Corresponding Source for linked Modding Libraries
as well as covered work. For distribution, provide the exact plugin source,
build scripts, pinned submodules, and source of linked dependencies (including
modifications), with their notices and exceptions. Do not treat a DLL upload as
a complete source-compliance package. Keep the MIT notice for original Photo Mode
files and all original third-party notices.

Other linked code: Dear ImGui (Omar Cornut and contributors, MIT), powerofthree's
ImGui extensions (retain upstream MIT notice), CLibUtil (powerofthree, MIT),
DirectXTex (Microsoft, MIT), FreeType (The FreeType Project, FreeType License
option; acknowledge use of FreeType), spdlog (Gabi Melman and contributors, MIT),
RapidFuzz (Max Bachmann and contributors, MIT), unordered_dense (Martin Leitner-Ankerl,
MIT), SRELL (Nozomu Katoo, BSD-2-Clause). CLibUtil embeds SimpleIni
(MIT) and XoshiroCpp (MIT); retain their header notices. Corresponding source
must retain those notices. This software is based in part on the work of the
FreeType Team (https://www.freetype.org). The corresponding-source ZIP includes
the tracked plugin/library build inputs and the six checksum-verified linked
package source archives; no optional ImGui sample font binaries are included. Dependency license files must accompany final releases.

Font Awesome icon-name header carries its existing attribution. No third-party
font files or Bethesda controller glyph data are distributed. Install ImGui Fonts
(Nexus 88107) separately under its own terms. The missing ignored BGSFonts.h is
not part of that external mod dependency: it was referenced as compressed embedded
controller TTF data. Its provenance and redistribution rights could not be verified;
plain controller button labels replace it.

ImGui's built-in emergency fallback embeds ProggyClean (Copyright 2004, 2005
Tristan Grimmer, MIT). Its notice is included separately. This permitted fallback
is distinct from the mandatory externally installed ImGui Fonts assets and the
unavailable Bethesda controller glyphs; no standalone font binary is packaged.
ImGui's included stb helpers (Sean Barrett and contributors) retain their MIT
alternative notices in licenses/ and their complete source headers in the source
bundle. The external font requirement is unchanged.
