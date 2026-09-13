# Additional Permission for Static Linking (LGPL-2.1 Section 6)

Status: **DRAFT — not effective until signed by every Grantor listed in §5
and passed by the owner and legal counsel.**

Drafted: 2026-09-13 by Fovea.cc, as the first artifact of the GoomApple
iOS port (FOV-1260). This document is the audit-trail record of a
written additional permission from the copyright holders of the
LGPL-licensed Goom code, per LGPL-2.1 §6.

## 1. Problem

The Goom engine (this repository, Goom2k26) is licensed under the GNU
Lesser General Public License, version 2.1 (see `LICENSE` and
`COPYING.README`). LGPL-2.1 §6 allows a work using the library to be
distributed under terms other than §6's default only if the user of the
Combined Work is given, per §6(a), object files plus relinking
information, or per §6(b), a shared library suitable for and locatable by
the user for relinking.

A signed application bundle distributed through an app store (Google
Play, Apple App Store) can satisfy neither mechanism: the store sealing
process fixes the binary, there is no user-writable library path, and
relinking instructions are meaningless without a toolchain on the
device. GoomAndroid already distributes the Goom core statically linked
inside `libgoom.so` on Google Play; GoomApple will do the same inside a
signed App Store bundle.

This document records the copyright holders' written permission for
that distribution form. It waives nothing except the §6 mechanics
described in §3; all other LGPL-2.1 obligations — notably the notice
duty — are preserved and reinforced in §4.

This is a license statement by the copyright holders, not legal advice;
it requires an owner and legal-counsel pass before it is relied upon
for distribution.

## 2. Covered Software

The "Covered Software" is every LGPL/GPL-licensed file of Fovea
copyright that is, or may be, compiled into a Goom application binary:

- **Goom core** — the C sources and headers of `Goom/` in the Goom2k26
  tree (goom_core.c, filters.c, zoom_filter_neon.c, ifs.c, tentacle3d.c,
  surf3d.c, convolve_fx.c, lines.c, drawmethods.c, v3d.c,
  config_param.c, flying_stars_fx.c, gfontlib.c, gfontrle.c,
  plugin_info.c, cpu_info.c, sound_tester.c, mathtools.c, goom_tools.c,
  graphic.c, motif_goom1.h, motif_goom2.h, and the public headers
  goom.h, goom_config.h, goom_config_param.h, goom_filters.h,
  goom_fx.h, goom_graphic.h, goom_plugin_info.h, goom_tools.h,
  goom_typedefs.h, goom_visual_fx.h, lines.h, ifs.h, v3d.h, surf3d.h,
  gfontlib.h, gfontrle.h, cpu_info.h, mathtools.h, drawmethods.h,
  sound_tester.h, config_param.h, flying_stars_fx.h, convolve_fx.h,
  plugin_info.h).
- **Engine layers** — the compiled subsets of `gametools/` and `iosfc/`
  from the Goom2k26 tree, as enumerated by the GoomAndroid build
  (`GoomAndroid/app/src/main/cpp/CMakeLists.txt:32-53`) and any future
  equivalent subset selected for the iOS target (the iOS v1 binary
  compiles no `sdl/` sources and no `libs/` sources).
- **iOS glue** — the Fovea-authored application glue being re-imported
  into Goom2k26 from the 2012 codebase for the GoomApple port
  (`IPhoneDrawContext.mm`, `EAGLView.mm`, `IPhoneEventManager.mm`,
  the new `GoomAppDelegate`, the audio sources, `GoomWidget.cpp`).
  These files carry `__MyCompanyName__` placeholder headers in the
  2012 sources; §5 fixes the attribution of record.
- **2026 revival contributions** — all modifications made to the above
  during the Goom2k26 revival, including `Goom/zoom_filter_neon.c` and
  the `sdl/` desktop binding.

Not covered (no Fovea-held LGPL copyright to grant — see §6): the
third-party components with their own licenses.

## 3. Grant

Each Grantor in §5, solely for the copyright that Grantor holds in the
Covered Software (the "Granted Portion"), hereby grants Fovea.cc and
every recipient of the Covered Software a **perpetual, irrevocable,
worldwide, royalty-free, non-exclusive** permission to:

1. statically link the Granted Portion into a Combined Work and
   distribute the resulting signed binary through an app store or any
   equivalent channel where the LGPL-2.1 §6(a) and §6(b) mechanisms
   cannot be satisfied by the distribution channel itself;
2. reproduce, modify, and distribute the Granted Portion as part of
   such a Combined Work without the need to satisfy §6(a) or §6(b).

The permission applies retroactively to all past Goom distributions
through those channels and to all future ones.

Where a file credits multiple authors, this permission is effective for
that file once every credited holder of that file has signed below.

Nothing else in LGPL-2.1 is waived. A recipient who obtains the
Covered Software itself (from this repository) retains every
LGPL-2.1 right, including the right to remove this additional
permission from their own modified versions, whereupon LGPL-2.1 §6
applies in full to that version.

## 4. Conditions (notice duty survives)

The permission in §3 is conditioned on the distributing application:

1. keeping the complete corresponding source of the Covered Software
   publicly available at no charge — the Goom2k26 repository,
   <https://github.com/j3k0/goom>;
2. carrying an easily accessible notice (the "About" screen) that
   names the Goom copyright holders and directs the user to the
   source repository and to the LGPL-2.1 license. Suggested notice
   text, to be carried verbatim or substantively equivalent:

   > This application includes the Goom music visualizer.
   > Copyright (c) 2000-2003 Jean-Christophe Hoelt and Guillaume Borios.
   > Copyright (c) 2003-2012 Fovea.cc — Jean-Christophe Hoelt,
   > Guillaume Borios, Florent Boudet, Michel Metzger (iosfc).
   > Copyright (c) 2026 the Goom2k26 contributors.
   > Licensed under the GNU LGPL, version 2.1 or later.
   > Source code: https://github.com/j3k0/goom

3. preserving all copyright and license notices contained in the
   Covered Software.

## 5. Grantors — attribution map and signatures

The attribution of record for the Covered Software, replacing all
`__MyCompanyName__` placeholders that appear in the 2012 and engine
sources (`OpenGlDrawContext.cpp`, `FontPackage.cpp`, `GTPlatform.h`,
`IPhoneDrawContext.mm`, `EAGLView.mm`, `IPhoneEventManager.mm`,
`GoomAudioSourceMic.*`, and the 2012 `GoomAppDelegate.*`): each such
file is attributed to **"Copyright (c) 2009-2012 Fovea.cc
(Jean-Christophe Hoelt, Florent Boudet)"** or, where the file credits
only one of them, that author alone, with the year(s) from the original
"Created by" line. The placeholder cleanup is carried out at restore
time in the iOS port; this map is the authoritative attribution until
then.

### Jean-Christophe Hoelt — jeko@fovea.cc (Fovea.cc)

Holds copyright in: the Goom core (primary author, (c)2000-2003
iOS-software and all 2026 revival work: zoom_filter_neon.c, sdl/ tree,
engine fixes); gametools/ files authored 2010-2012
(FontPackage, Commander, GTUI, OverlayManager, graphicsmanager, ...);
iosfc files (ios_hash, ios_time, co-author of the rest); vec3.h
((c) 2004); the 2012 iOS glue (GoomMain.mm, GoomAppDelegate,
GoomWidget.cpp, GoomAudioSourceHeartBeat, GoomAudioSource.h).

Signature: ______________________  Date: ____________

### Guillaume Borios — gyom@fovea.cc (Fovea.cc)

Holds copyright in: Goom core files (cpu_info.c/cpu_info.h,
ppc_drawings.h, ppc_zoom_ultimate.h, (c) 2003 iOS); 2004 gametools
files (GTPreferences, co-author of DataPathManager, GTPak,
LocalizedDictionary, ScreenTransition); co-author of the iosfc layer.

Signature: ______________________  Date: ____________

### Florent Boudet — flobo@fovea.cc (Fovea.cc)

Holds copyright in: gametools/OpenGlDrawContext.cpp and the GL draw
context family ((c) 2009); iosfc (ios_unixmuteximpl.cpp (c) 2002,
co-author of the layer); the 2012 iOS glue (IPhoneDrawContext.mm,
EAGLView.mm, IPhoneEventManager.mm, GoomAudioSourceMic.cpp, (c)
2009-2012, all currently carrying the `__MyCompanyName__` placeholder).

Signature: ______________________  Date: ____________

### Michel Metzger

Holds copyright in: the iosfc layer headers that credit him as a
co-author ("participate during the developement of Shubunkin!"):
ios_exception.h, ios_memory.h, ios_filepath.h, ios_stream.h — all four
compile into the shipped Android binary today and are expected in the
iOS binary.

Signature: ______________________  Date: ____________

*Each signature is the Grantor's act for their own copyright only;
signatures do not transfer ownership. Fovea.cc records and publishes
the signed version of this file in the public Goom2k26 repository as
the audit trail.*

## 6. Third-party components (no Fovea grant needed)

These files compile into the same binaries but carry their own
permissive licenses; they are not part of the LGPL grant and need no
Grantor signature:

- `Goom/ifs.c` — (c) 1997 Massimino Pascal; permissive license (notice
  + permission in the header). Ships as-is.
- `gametools/mt19937ar.cpp` — (c) 1997-2002 Makoto Matsumoto and
  Takuji Nishimura, (c) 2005 Mutsuo Saito; BSD-style license in the
  header. Ships as-is.
- `libs/soil/` (SOIL.c, image_DXT, image_helper, stb_image_aug) —
  public domain, Jonathan Dummer 2007 / Sean Barret. Verified
  unmodified since vendoring (no Fovea-authored "soil additions"
  exist in the tree; the §6 inventory item resolves to none).
- `iosfc/google/sparsehash` headers (dense_hash_map et al.) — (c) 2005
  Google Inc., BSD; used header-only by ios_hash.cpp.

**Compliance flag (outside this document's grant, for the SACompliance
lane):** `libs/soil/GLFreeType.cpp` — Sven Olsen's 2003 nehe.gamedev.net
tutorial code — carries **no license statement**. The iOS target
excludes it by design (no TTF text is ever drawn). It is, however,
still compiled into the GoomAndroid binary today
(`GoomAndroid/app/src/main/cpp/CMakeLists.txt:62`), which is an open
Android compliance item independent of this port.

## 7. Record-keeping

- Effective date: the date the last Grantor signs (§5).
- The signed version of this file is committed to the public Goom2k26
  repository; the commit hash is the audit reference.
- Source of record: <https://github.com/j3k0/goom>.
- Related: `COPYING.README` (copyright and licensing notice),
  `LICENSE` (LGPL-2.1 full text).