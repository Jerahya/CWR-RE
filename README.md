# CWR-RE

CWR-RE is a community fork of the *Poseidon* engine and game source code that
Bohemia Interactive released for *Arma: Cold War Assault* (originally *Operation
Flashpoint: Cold War Crisis*, 2001). The original source release lives at
<https://github.com/BohemiaInteractive/CWR>.

**This is a modified version of that program.** It is not the original program, it is
not an official Bohemia Interactive product, and it is not affiliated with, endorsed by,
or associated with Bohemia Interactive or Electronic Arts. See the commit history for the
modifications.

Three things are worth keeping separate:

**Source code (this repository)**

The engine and game executables, licensed under GPL-3.0-or-later with additional terms
under Section 7 (see [`LICENSE`](LICENSE)). You may use, study, modify, and redistribute
it, provided it stays GPL and you follow those terms.

**The name and brand**

"ARMA", "Operation Flashpoint", and their logos are not granted by the license and are not
used as this project's name or branding. They are mentioned here only to identify where the
source code came from.

**Game data (separate)**

Models, textures, sounds, missions, and voices are not in this repository and are not GPL.
They are released separately by Bohemia Interactive under the APL-SA license. To run what you
build, use the game data from your own Steam copy of the game:

- *Arma: Cold War Assault Remastered* demo on Steam: <https://store.steampowered.com/app/4819000>
- *Arma: Cold War Assault Remastered* full game on Steam: <https://store.steampowered.com/app/65790>

This project does not distribute game data.

## Quick Start

```sh
cmake --preset win-x64-clang-rwdi
cmake --build build/win-x64-clang-rwdi
```

On GNU/Linux, use the matching `linux-x64-clang-rwdi` preset.

## Nintendo Switch (homebrew)

CWR-RE builds as a Switch homebrew application (`.nro`). It boots, renders through Mesa
(OpenGL 4.3), plays audio, and runs single-player missions with a controller. Performance
work is ongoing, and the mission editor's controller support is incomplete.

No compiled Switch binaries are distributed, and no game data, keys, or console
modification instructions are provided. You build it yourself, use the game data from your
own Steam copy of the game (see [Game data](#game-data--assets--arma-public-license-share-alike-apl-sa)),
and run it on hardware you are able to run homebrew on.

### Requirements

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the `switch-dev` group and
  these portlibs: `switch-mesa switch-glad switch-openal-soft switch-curl switch-freetype
  switch-libvorbis switch-libogg switch-libopus switch-libzstd switch-mbedtls switch-zlib
  switch-sdl2`
- vcpkg (`VCPKG_ROOT`) and CMake 3.25+, as for the desktop build. On a Windows host, use the
  native Windows CMake rather than the MSYS2 one shipped with devkitPro, and keep devkitPro's
  `msys2\usr\bin` after it on `PATH`.

### Build

```sh
cmake --preset switch-aarch64-rwdi
cmake --build build/switch-aarch64-rwdi --target PoseidonGame_nro
```

The output is `build/switch-aarch64-rwdi/switch/PoseidonGame/PoseidonGame.nro`.

### Run

1. Copy `PoseidonGame.nro` to `sdmc:/switch/` on the SD card.
2. Copy the contents of the `Remastered` folder from your Steam installation of the game to
   `sdmc:/switch/cwr-re/data/`.
3. Launch it from the homebrew menu. Settings, saves and logs go to `sdmc:/switch/cwr-re/`.

The game needs the full application memory budget; in the limited applet mode it runs out
of memory while loading. For development, `nxlink -s PoseidonGame.nro` sends the build over
the network and streams the game log back to the PC.

## Layout

- [Apps](apps/README.md) - executable targets
- [Engine](engine/README.md) - engine libraries and Rust Trident tooling
- [Master server tools](mserver/README.md) - Rust service and CLI crates
- [Tests](tests/README.md) - test source trees; CI currently compiles them only
- `cmake/` - presets, toolchains, vcpkg triplets, and overlay ports
- `docker/` - container support for service and runtime environments
- `packages/` - ignored local game data staging area
- `resources/` - application icon resources
- `thirdparty/` - vendored third-party headers and sources

## Project Notes

- [Contributing](CONTRIBUTING.md)
- [Credits](CREDITS.md)
- [Third-party notices](THIRD_PARTY_NOTICES.md)
- [Vendored dependencies](thirdparty/README.md)

## License

The source in this repository is licensed under the **GNU General Public License
v3.0 *or later***, with additional terms under **Section 7** of the GPL. See
[`LICENSE`](LICENSE) for the full text, including the original copyright notice and
the additional terms, which must accompany any copy of this program.
This license does not grant you any right to use "ARMA" or any other Bohemia Interactive
trademark.

The [`thirdparty/`](thirdparty) directory is **excluded** from the project's GPL
license: it contains vendored third-party code (glad, the RenderDoc API header)
under their own respective licenses — see [`thirdparty/README.md`](thirdparty/README.md).
Dependencies pulled in via vcpkg ([`vcpkg.json`](vcpkg.json)) likewise remain under
their own licenses.

*"ARMA" is a registered trademark of BOHEMIA INTERACTIVE a.s. "OPERATION FLASHPOINT" is a
registered trademark of Electronic Arts Inc. See [`LICENSE`](LICENSE) for information
concerning trademarks. This notice is informational and does not constitute any grant
and/or waiver of rights.*

### Game data / assets — Arma Public License Share Alike (APL-SA)

Game data and assets (models, textures, sounds, missions, etc.) are **not part of
this repository** and are **not** covered by the GPL. They are released separately
by Bohemia Interactive under the **Arma Public License Share Alike (APL-SA)**:

- APL-SA license text: <https://www.bohemia.net/community/licenses/arma-public-license-share-alike>

Get the game data from Steam:

- *Arma: Cold War Assault Remastered* demo on Steam: <https://store.steampowered.com/app/4819000>
- *Arma: Cold War Assault Remastered* full game on Steam: <https://store.steampowered.com/app/65790>

Whatever you do with assets is governed by the APL-SA linked above; whatever you do
with this source is governed by the GPL with additional terms per Section 7 in
[`LICENSE`](LICENSE).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).
