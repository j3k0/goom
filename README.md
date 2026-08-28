# Goom2k26

Revival of **Goom**, the classic music visualizer engine (the effect that
powered XMMS/Winamp-era visualizations), updated to build and run on modern
systems. Pure C core, C++ engine layers, SDL2 desktop front-end.

Licensed under the **GNU LGPL-2.1** — see [LICENSE](LICENSE) and
[COPYING.README](COPYING.README) for copyright details and attribution.

## Layout

```
Goom/        Goom core — C effect engine (goom_core.c, filters.c,
             flying_stars_fx.c, tentacle3d.c, ifs.c, NEON zoom filter, ...)
sdl/         SDL2 desktop port (macOS arm64 today; Windows/Linux intended)
gametools/   Fovea2D engine core (rendering, game loop, UI, audio)
iosfc/       cross-platform primitives (threads, sockets, file I/O)
libs/        vendored: soil (image loading + freetype text rendering)
tests/       standalone conformance + stress harness for the NEON
             zoom filter (tests/zoom_filter; `make test` / `make bench`)
```

## Build (macOS)

Requirements: Xcode command-line tools, SDL2, freetype (`brew install sdl2 freetype`).

```bash
cd sdl
make            # build/Darwin/goom-sdl
make run        # build, then run against staged goom-data resources
```

Audio source priority: `--file <wav>` > microphone > built-in heartbeat.
Useful flags: `--file <path>`, `--no-mic`, `--fullscreen`, `--size WxH`,
`--resources <dir>`, `--control` (HTTP control server on port 8090,
live parameter tuning — open http://localhost:8090 while running).

## Tests

```bash
make -C tests/zoom_filter test    # NEON zoom filter vs C reference
make -C tests/zoom_filter bench   # stress benchmark at real resolutions
```

## History

Goom core: (c)2000-2003 Jean-Christophe Hoelt, Guillaume Borios
(released under the informal group name "iOS-software", no relation
to Apple). Engine layers: (c)2003-2012 Fovea.cc.
Goom2k26 revival: 2026.

## Third-party components

soil (public domain/MIT, incl. GLFreeType) · freetype2 (FTL/GPL-2.0 dual) ·
SDL2 (zlib)