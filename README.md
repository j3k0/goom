# Goom2k26

Revival of **Goom**, the classic music visualizer engine (the effect that
powered XMMS/Winamp-era visualizations), updated to build and run on modern
systems. Pure C core, C++ engine layers, SDL2 desktop front-end.

Licensed under the **GNU LGPL-2.1** — see [LICENSE](LICENSE) and
[COPYING.README](COPYING.README) for copyright details and attribution.
For static linking into store-distributed app binaries, the
copyright holders' written additional permission is recorded in
[LGPL-ADDITIONAL-PERMISSION.md](LGPL-ADDITIONAL-PERMISSION.md).

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

## Running

`make run` is just a convenience wrapper (`--resources goom-data`); the built
binary can be invoked directly:

```bash
cd sdl
make
./build/Darwin/goom-sdl --resources goom-data --size 1440x900 --control-lan
```

The binary path is `build/<uname>/goom-sdl` (`build/Darwin/` on macOS).
Run with `-h` / `--help` for the built-in usage text.

| Option | Description |
|---|---|
| `--fullscreen` | borderless fullscreen at desktop resolution |
| `--size WxH` | windowed mode size (default 1280x800) |
| `--resources <dir>` | resources root (default: next to the executable); game data is read from `<dir>/data` |
| `--file <path>` | play `<path>` (wav) as the audio source — highest-priority source |
| `--no-mic` | disable microphone input (falls back to the built-in heartbeat) |
| `--exit-after <secs>` | exit automatically after N seconds (testing) |
| `--control[=port]` | serve the parameter control UI over HTTP+JSON on localhost (default port 8090; tries port..port+9) — open http://localhost:8090 while running |
| `--control-lan` | also allow LAN access to the control server (tune parameters from a phone) |
| `-h`, `--help` | show usage and exit |

Audio source priority: `--file <wav>` > microphone > built-in heartbeat.
The control server is off by default (`--control-lan` implies `--control`).

Keyboard controls while running: `s` screenshot (BMP to ~/Desktop),
`f` toggle fullscreen (also F11 / Alt+Enter), `p` pause, `Esc` quit.

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