# Additional Permission for Static Linking (LGPL-2.1 §6 / GPL-2.0)

Status: **DRAFT — not effective until (i) every Grantor listed in §5 has
signed, and (ii) the owner and legal counsel have recorded their pass in
§7.**

Drafted: 2026-09-13 by Fovea.cc, as the first artifact of the GoomApple
iOS port (FOV-1260). Revised 2026-09-13 after review (PR #1): dual-track
grant (the compiled corpus is mixed LGPL/GPL), §6 proviso addressed,
holder census corrected, covered set made determinate (Appendix A/B),
third-party inventory completed. This document is the audit-trail record
of a written additional permission from the copyright holders of the
Goom code, per LGPL-2.1 §6 and GPL-2.0 §2(b).

This is a license statement by the copyright holders, not legal advice;
it requires the owner and legal-counsel pass recorded in §7 before it
is relied upon for distribution.

## 1. Problem

The licensing of the Goom code that compiles into an application binary
is mixed, and the per-file headers disagree with the repository's
blanket notice:

- `Goom/` core files carry "LGPL Licence" / "(c) iOS-software" notices,
  and the repository notice (`COPYING.README`) licenses the whole
  Goom2k26 tree under **GNU LGPL-2.1**.
- Several `gametools/` and `iosfc/` files carry **GPL-2.0-or-later**
  headers from the 2002–2004 FloboPuyo/iosfc era (`GTPak.cpp`,
  `FPDataPathManager.cpp`, `LocalizedDictionary.cpp`, `DataPathManager.h`,
  `vec3.h`, `ScreenTransition.h`, and the iosfc layer headers:
  `ios_exception.h`, `ios_memory.h`, `ios_filepath.h`, `ios_stream.h`,
  and their .cpp files).

**Ownership of record (2026-09-13, FOV-1260):** Jean-Christophe
Hoelt, owner of Fovea.cc, has attested that Fovea owns all copyright
in the code covered by this document — including the multi-author
2002-2004 files — so Fovea may license or relicense it as it pleases;
no co-author consent problem exists. The signatures in §5 are
confirmatory acknowledgments by the named authors, not a consent
round. The two-track grant below is kept because the per-file license
*notices* still say what they say, and a downstream recipient must be
able to rely on the text rather than on private ownership knowledge.
This document therefore takes the per-file header reality as it
stands and grants what distribution actually needs, on two tracks:

1. For the LGPL-2.1 files: LGPL-2.1 §6 permits a combined work under
   terms of the distributor's choice only if the user is given, per
   §6(a), object files plus relinking information, or per §6(b), a
   user-replaceable shared library. A signed application bundle
   distributed through an app store (Google Play, Apple App Store) can
   satisfy neither: the store sealing process fixes the binary, there
   is no user-writable library path, and relinking is meaningless
   without a toolchain on the device. GoomAndroid already distributes
   the Goom core statically linked inside `libgoom.so` on Google Play;
   GoomApple will do the same inside a signed App Store bundle. (The
   engineering facts: App Store code signing forbids substituting or
   side-loading frameworks into a sealed bundle; dynamic linking was
   evaluated and rejected in the port plan — FOV-1260. Counsel
   confirms.)
2. For the GPL-2.0 files: GPL-2.0 §2(b)/§3 makes static linking into a
   proprietary signed bundle a distribution problem this doc must also
   solve for the holders' own files.

## 2. Covered Software

The "Covered Software" is the Fovea-copyright work listed in the
schedules of Appendix A (the compiled set of the GoomAndroid 2026.2
Google Play binary, generated from `GoomAndroid/app/src/main/cpp/
CMakeLists.txt:31-99` and exact as of the 2026.2 release) and Appendix
B (the iOS v1 set; pinned before the first iOS release — see §7), in
each case comprising:

- **Goom core** — the `Goom/` C sources and headers of the Goom2k26
  tree (all files listed in Appendix A; `ifs.c` itself is excluded
  from the grant — third-party, see §6).
- **Engine layers** — the compiled `gametools/` and `iosfc/` subsets
  from the Goom2k26 tree, plus every header they include.
- **Desktop binding** — the `sdl/` sources (2026 revival work; the
  desktop `goom-sdl` binary is itself a distribution of the Covered
  Software even though no mobile binary compiles them).
- **iOS glue** — the Fovea-authored application glue being re-imported
  into Goom2k26 from the 2012 codebase for the GoomApple port
  (`IPhoneDrawContext.*`, `EAGLView.*`, `IPhoneEventManager.*`, the
  new `GoomAppDelegate`, the audio sources, `GoomWidget.cpp`).
- **Android glue** — `goom_android.cpp`, `GoomAndroidAppDelegate.cpp`,
  `GoomAndroidMicSource.cpp`, `GoomAndroidPrefs.cpp`,
  `GoomAndroidCloud.cpp` (Fovea 2026, currently headerless — the
  attribution of record is "(c) 2026 Fovea.cc / Jean-Christophe
  Hoelt").
- **Fovea modifications to vendored files** — the changes Fovea made
  to `libs/soil/SOIL.c` (the `GTOpenGl.h` include and the disabled
  native GL header block) and `libs/soil/GLFreeType.cpp` (the
  `namespace gametools` wrapping). The upstream bases of those files
  remain third-party (§6); only the Fovea modifications are Covered
  Software.
- **2026 revival contributions** — all modifications made to the
  above during the Goom2k26 revival, including `Goom/zoom_filter_neon.c`.

If a future release compiles a file not in Appendix A or B, the
appendix is amended and committed with that release; the permission is
effective for an appended file under §3's per-file rule once every
credited holder of that file has signed. The granted file set at any
time is exactly the then-current appendices.

Attribution of record (replacing every `__MyCompanyName__` placeholder
in the Goom2k26 tree — 24 files: `AchievementsManager.h`,
`AppDelegate.*`, `CommanderResources.h`, `FontPackage.*`, `Frame.h`,
`GTPlatform.*`, `GTRenderer.*`, `GTScreenOrientation.*`,
`GTSlaveEventManager.*`, `GameTools.h`, `LocalAchievementsManager.*`,
`MusicPackage.*`, `OpenGlDrawContext.cpp`, `OverlayManager.*`,
`audiomanager.cpp`, and the 2012 iOS glue files): each file is
attributed to **"Copyright (c) <years from the file's own 'Created by'
line> Fovea.cc (the named author)"**. The placeholder cleanup is
carried out in the normal course of port work; this paragraph is the
authoritative attribution until then.

## 3. Grant

Each Grantor in §5, solely for the copyright that Grantor holds in the
Covered Software (the "Granted Portion"), hereby grants Fovea.cc (the
company operating fovea.cc — legal form and registration to be
completed in §8 before signature) and every recipient of the Covered
Software a **perpetual, irrevocable, worldwide, royalty-free,
non-exclusive** permission, retroactive to all past Goom
distributions:

1. **LGPL track.** For Granted Portions licensed under LGPL-2.1:
   permission to copy, modify, and statically link the Granted Portion
   into a Combined Work and to reproduce and distribute the resulting
   combined binary under terms of the distributor's choice, without
   satisfying the mechanics of LGPL-2.1 §6(a) or §6(b). To the extent
   LGPL-2.1 §6's opening proviso (that the terms permit modification
   for the customer's own use and reverse engineering for debugging
   such modifications) conflicts with the standard terms of a
   distribution channel (App Store / Play EULAs, code-signing and
   anti-tamper requirements), that proviso is waived for the Combined
   Work as distributed through that channel. Nothing else in LGPL-2.1
   is waived: LGPL-2.1 applies in full to the Covered Software as
   distributed on its own from the repository, and a recipient may
   remove this additional permission from their own modified versions,
   whereupon LGPL-2.1 §6 applies in full to that version.
2. **GPL track.** For Granted Portions licensed under GPL-2.0 (the
   2002–2004-era headers listed in §1): a Classpath-style linking
   exception — permission to link the Granted Portion, statically or
   dynamically, with other modules to produce a Combined Work
   distributed under terms of the distributor's choice, provided that
   the Combined Work, when distributed, remains accompanied by the
   source availability and notice conditions of §4, and the Granted
   Portion itself remains available under GPL-2.0-or-later from the
   repository.

Where a file credits multiple authors, this permission is effective
for that file once every credited holder of that file has signed §5.
Signatures act for the Grantor's own copyright only; they do not
transfer ownership.

## 4. Conditions (notice duty survives)

The permissions in §3 are conditioned on the distributing application:

1. keeping the complete corresponding source of the Covered Software
   (Fovea code) publicly available at no charge — the Goom2k26
   repository, <https://github.com/j3k0/goom> — and, for the
   third-party components compiled alongside it (§6), the source
   availability required by each component's own license (freetype
   FTL; the vendored 2.4.2 tree is preserved in the GoomAndroid build
   tree — the offering mechanism for it is an SACompliance item);
2. carrying an easily accessible notice (the "About" screen) that
   names the Goom copyright holders and directs the user to the
   source repository and to the licenses. Suggested notice text, to
   be carried verbatim or substantively equivalent:

   > This application includes the Goom music visualizer.
   > Copyright (c) 2000-2003 Jean-Christophe Hoelt and Guillaume
   > Borios, with contributions from Florent Boudet.
   > Copyright (c) 2003-2012 Fovea.cc — Jean-Christophe Hoelt,
   > Guillaume Borios, Florent Boudet.
   > Copyright (c) 2026 the Goom2k26 contributors.
   > Licensed under the GNU LGPL, version 2.1 or later.
   > Source code: https://github.com/j3k0/goom

3. preserving all copyright and license notices contained in the
   Covered Software.

**Commencement:** the notice condition binds new releases from the
effective date (§7). Distributions already made (the GoomAndroid
listing on Google Play) are blessed retroactively by §3, provided the
next released update of the application carries the §4.2 notice.

## 5. Grantors — attribution map and signatures

### Jean-Christophe Hoelt — jeko@fovea.cc (Fovea.cc)

Holds copyright in: the Goom core (primary author; the "(c)2000-2003
iOS-software" and "(c) iOS / iOS Software" entity notices are the
Fovea founders' own group name — see §1); all 2026 revival work
(`zoom_filter_neon.c`, the `sdl/` tree, engine fixes); `vec3.h`
((c) 2004); `ios_hash.h` ((c) 2004); gametools files authored
2010–2012 (`FontPackage`, `GTPlatform`, `GTScreenOrientation`,
`MusicPackage`, `LocalAchievementsManager`, …); the 2012 iOS glue
(`GoomMain.mm`, `GoomAppDelegate`, `GoomWidget.cpp`,
`GoomAudioSourceHeartBeat`, `GoomAudioSource.h`); co-author of the
2004 gametools files and the iosfc layer; the Android glue (2026);
the Fovea modifications to `SOIL.c` and `GLFreeType.cpp` (2026).

Signature: ______________________  Date: ____________

### Guillaume Borios — gyom@fovea.cc (Fovea.cc)

Holds copyright in: Goom core files (`cpu_info.c`/`cpu_info.h`,
`ppc_drawings.h`, `ppc_zoom_ultimate.h`, (c) 2003 iOS); 2004
gametools files (`GTPreferences`, co-author of `DataPathManager`,
`GTPak`, `LocalizedDictionary`, `ScreenTransition`); co-author of
the iosfc layer.

Signature: ______________________  Date: ____________

### Florent Boudet — flobo@fovea.cc (Fovea.cc)

Holds copyright in: `gametools/OpenGlDrawContext.cpp` and the GL
draw-context family ((c) 2009); `iosfc/posix/ios_unixmuteximpl.cpp`
((c) 2002); co-author of the iosfc layer and the 2004 gametools
files; the 2012 iOS glue (`IPhoneDrawContext.mm`, `EAGLView.mm`,
`IPhoneEventManager.mm`, `GoomAudioSourceMic.cpp` — currently
`__MyCompanyName__` placeholders, attributed per §2).

Signature: ______________________  Date: ____________

*Ownership of record: Jean-Christophe Hoelt, owner of Fovea.cc, has
attested (FOV-1260, 2026-09-13) that Fovea owns all copyright in the
Covered Software, including the iosfc layer in its entirety. The
credit line "Michel Metzger — participate during the developement of
Shubunkin!" that appeared in some iosfc headers was boilerplate from
an unrelated project (Shubunkin); it has been removed from the tree
and no third-party copyright is claimed in the iosfc layer.*

*Each signature is the Grantor's act for their own copyright only.
Where a Grantor's contribution was made in the course of employment
and copyright vests in an employer, counsel confirms the chain (§8)
before that signature is relied upon. Fovea.cc records and publishes
the signed version of this file in the public Goom2k26 repository as
the audit trail.*

## 6. Third-party components (no Fovea grant needed)

These files compile into the same binaries but carry their own
licenses; they are not part of the grant and need no Grantor
signature:

- `Goom/ifs.c` — (c) 1997 Massimino Pascal; permissive license (notice
  + permission in the header). Ships as-is. Not part of the Goom-core
  grant in §2 despite living in `Goom/`.
- `gametools/mt19937ar.cpp` — (c) 1997-2002 Makoto Matsumoto and
  Takuji Nishimura, (c) 2005 Mutsuo Saito; BSD-style license in the
  header. Ships as-is.
- `libs/soil/` upstream bases — `SOIL.c`/`stb_image_aug` public domain
  (Jonathan Dummer 2007 / Sean Barret; the `SOIL.c` Fovea
  modifications are Covered Software per §2); `image_DXT.c` public
  domain (Dummer 2007); **`image_helper.c` is MIT licensed** (Dummer).
- `iosfc/google/sparsehash` headers — (c) 2005 Google Inc., BSD;
  used header-only by `ios_hash.cpp`.
- **freetype 2.4.2, vendored** — compiled into the Android binary from
  `GoomAndroid/android/external/freetype` (17 source files,
  `CMakeLists.txt:67-74`); FreeType License (FTL, BSD-style with
  attribution) or GPL-2.0 at the option used. Vendored, not linked —
  `COPYING.README` is corrected in this PR accordingly.

**Compliance flag (outside this document's grant, routed to
SACompliance):** `libs/soil/GLFreeType.cpp` — Sven Olsen's 2003
nehe.gamedev.net tutorial code, **no license statement** — is
compiled into the GoomAndroid binary today (`CMakeLists.txt:62`).
The iOS target excludes it by design (no TTF text is ever drawn).
The Android exposure stands until the file is dropped from
`SOIL_SRC` (it is dead code there for the same reason). The Fovea
`namespace gametools` modifications to the file are Covered
Software; the unlicensed upstream base is not, and no Fovea grant
can cure it.

**Not compiled, for the record:** `iosfc/patools/` (including
`pacharbuffer.cpp`, which credits **David Gayerie** as co-author
with JC Hoelt, (c) 2000-2001 iOS software) is in the public source
tree but **not** in any compiled set (`IOSFC_SRC` is exactly
`ios_exception.cpp ios_filepath.cpp ios_hash.cpp ios_memory.cpp
ios_stream.cpp ios_time.cpp posix/ios_unixmuteximpl.cpp`;
nothing in the compiled set includes patools). No Gayerie grant is
needed for the covered binaries; **if** a future release compiles
patools, his signature is required under §2's appendix-amendment
rule before that release ships. Source distribution of patools
under its GPL header is unaffected.

## 7. Record-keeping

- **Effective date: the later of (i) the date the last Grantor signs
  §5 and (ii) the date the owner/legal-counsel pass is recorded
  below.** Both are required; the document is a DRAFT until then.
- Owner / legal-counsel pass recorded by: ______________________
  Date: ____________
- The signed version of this file is committed to the public Goom2k26
  repository; the commit hash is the audit reference.
- Appendix B is pinned and committed before the first iOS release;
  the release commit and this document are amended together.
- Source of record: <https://github.com/j3k0/goom>.
- Related: `COPYING.README` (copyright and licensing notice),
  `LICENSE` (LGPL-2.1 full text), `README.md` (license section).

## 8. Formalities for counsel (complete before signature)

- Legal identity of the grantee "Fovea.cc" (form, registration,
  address) and of the "iOS-software" entity in the 2000-2003 core
  notices.
- Employment/WFH chain for the Grantors' 2002-2012 contributions:
  superseded by the §1 ownership attestation (Fovea owns all
  covered code); kept only as a non-blocking counsel confirmation.
- Successors-and-assigns; warranty-of-title wording; severability;
  governing law; notice address for §4 cure questions.
- Confirmation that the §3 GPL-track exception language and the
  §6 proviso waiver are correctly scoped for the target stores.

## Appendix A — GoomAndroid 2026.2 compiled set (exact)

Generated from `GoomAndroid/app/src/main/cpp/CMakeLists.txt` at the
2026.2 release; the Fovea-copyright grant covers these files (plus
every header they include) except the third-party files marked [*]:

- `gametools/` (30): AppDelegate.cpp, BufferedDrawTarget.cpp,
  Commander.cpp, CompositeDrawContext.cpp, DataPathManager.cpp,
  FPDataPathManager.cpp, FontPackage.cpp, GTAnisoSurface.cpp,
  GTAtlasLoader.cpp, GTLog.cpp, GTPak.cpp, GTPlatform.cpp,
  GTPreferences.cpp, GTRenderer.cpp, GTScreenOrientation.cpp,
  GTTestGroup.cpp, GTUI.cpp, LocalAchievementsManager.cpp,
  LocalizedDictionary.cpp, MusicPackage.cpp, NotifyCenter.cpp,
  OpenGlDrawContext.cpp, OverlayManager.cpp, audio.cpp,
  audiomanager.cpp, drawcontext.cpp, gameloop.cpp, gameui.cpp,
  GTSlaveEventManager.cpp, graphicsmanager.cpp — plus
  `mt19937ar.cpp` [*, third-party BSD].
- `iosfc/` (7): ios_exception.cpp, ios_filepath.cpp, ios_hash.cpp,
  ios_memory.cpp, ios_stream.cpp, ios_time.cpp,
  posix/ios_unixmuteximpl.cpp.
- `libs/soil/` (5): SOIL.c, image_DXT.c, image_helper.c [*MIT],
  stb_image_aug.c [*public domain], GLFreeType.cpp [*unlicensed —
  compliance flag, §6].
- `Goom/` (22): goom_core.c, filters.c, zoom_filter_neon.c,
  ifs.c [*, Massimino Pascal 1997], tentacle3d.c, surf3d.c,
  convolve_fx.c, lines.c, drawmethods.c, v3d.c, config_param.c,
  flying_stars_fx.c, gfontlib.c, gfontrle.c, plugin_info.c,
  cpu_info.c, sound_tester.c, mathtools.c, goom_tools.c, graphic.c,
  GoomWidget.cpp, GoomAudioSourceHeartBeat.cpp.
- Android glue (5): goom_android.cpp, GoomAndroidAppDelegate.cpp,
  GoomAndroidMicSource.cpp, GoomAndroidPrefs.cpp,
  GoomAndroidCloud.cpp.
- Desktop (not in the mobile binary; covered for the `goom-sdl`
  distribution): the `sdl/` tree.
- Also compiled, third-party, no grant: freetype 2.4.2 (17 files,
  FTL/GPL, vendored in the GoomAndroid tree).

## Appendix B — iOS v1 compiled set

**To be pinned and committed before the first iOS release.** It is
the GoomAndroid set's engine core + iosfc subset + Goom core (no
soil/GLFreeType — target-excluded; no freetype; no Android glue),
plus the iOS glue enumerated in §2. Amending this appendix and
releasing are one commit, per §2 and §7.