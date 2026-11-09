# Licensing and redistribution

Original Photo Mode: Copyright (c) powerofthree. MIT.
Keep the unchanged notice in LICENSE.txt.
The combined DLL is not MIT-only.

CommonLibF4 and CommonLib Shared use GPL-3.0-or-later with modding/linking exceptions.
The modding exception permits interaction with the proprietary game.
It does not remove source obligations for library code.
The linking exception requires Corresponding Source for linked Modding Libraries.
Ship the exact plugin source, build files, pinned libraries and their changes.
Include dependency source, notices and exceptions. A DLL alone is not enough.
The source ZIP includes tracked inputs plus six hash-checked package archives.
Keep the license files with releases. Full license text is unchanged in licenses/.

| Code | Authors | License |
| --- | --- | --- |
| Dear ImGui | Omar Cornut and contributors | MIT |
| ImGui extensions | powerofthree | MIT |
| CLibUtil | powerofthree | MIT |
| DirectXTex | Microsoft | MIT |
| FreeType | The FreeType Project | FreeType License option |
| spdlog | Gabi Melman and contributors | MIT |
| RapidFuzz | Max Bachmann and contributors | MIT |
| unordered_dense | Martin Leitner-Ankerl | MIT |
| SRELL | Nozomu Katoo | BSD-2-Clause |

CLibUtil embeds SimpleIni and XoshiroCpp (MIT). Keep their header notices.
This software is based in part on the work of the FreeType Team:
https://www.freetype.org.

The Font Awesome icon-name header keeps its attribution.
Install ImGui Fonts (Nexus 88107) separately under its own terms.
No standalone third-party fonts or Bethesda controller glyphs are bundled.
The missing BGSFonts.h held embedded controller TTF data. Its rights were unknown.
Text labels replace it. That header is separate from the external font requirement.

ImGui's emergency fallback embeds ProggyClean.
Copyright 2004, 2005 Tristan Grimmer. MIT. Its notice is included.
It does not replace the required ImGui Fonts mod.
Optional ImGui sample font binaries are excluded from the source ZIP.
ImGui's stb helpers are by Sean Barrett and contributors.
Their MIT notices remain in licenses/ and the source headers.
