# CWR-RE

CWR-RE is a community fork of the *Poseidon* engine and game source code that
Bohemia Interactive released for *Arma: Cold War Assault* (originally *Operation
Flashpoint: Cold War Crisis*, 2001). The original source release lives at
<https://github.com/BohemiaInteractive/CWR>.

**This is a modified version of that program.** It is not the original program, it is
not an official Bohemia Interactive product, and it is not affiliated with, endorsed by,
or associated with Bohemia Interactive or Electronic Arts. See [What changed](#what-changed)
and the commit history for the modifications.

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
build, use game data from your own copy of the game: the full game, or the free Demo on
Steam (<https://store.steampowered.com/app/4819000>).
This project does not distribute game data.

## What changed

Compared with the original source release, CWR-RE:

- is renamed (window and tool titles, user folders, icons) and carries no original branding;
- uses its own settings, saves and logs folders (`CWR-RE`), separate from the official game.

Further engine and platform work is tracked in the commit history.

## Quick Start

```sh
cmake --preset win-x64-clang-rwdi
cmake --build build/win-x64-clang-rwdi
```

On GNU/Linux, use the matching `linux-x64-clang-rwdi` preset.

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

Whatever you do with assets is governed by the APL-SA linked above; whatever you do
with this source is governed by the GPL with additional terms per Section 7 in
[`LICENSE`](LICENSE).

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).
