# License Scope

## First-party work

Unless a file states otherwise, original source code, scripts, tests,
first-party metadata, and original documentation in this repository are
licensed under the GNU General Public License version 3 only
(`GPL-3.0-only`). The copyright in each contribution remains with its
respective contributor.

The copies of `LICENSE` in independently packaged project directories contain
the same GPL version 3 text and are provided so that each source or binary
package can carry the applicable license with it.

## Material outside that grant

The GPL grant above applies only where this project or its contributors own
the necessary rights. It does not grant any rights to:

- DragonSword: Awakening or other game binaries, cooked or extracted assets,
  data tables, screenshots, names, artwork, audio, icons, or trademarks;
- Unreal Engine, generated Unreal declarations, `UEPseudo`, or other Epic
  Games material;
- RE-UE4SS, SQLCipher, CUE4Parse, UAssetAPI, Newtonsoft.Json,
  Microsoft.Bcl.Memory, ZstdSharp, Oodle, or any other third-party component;
- generated catalogs, coordinates, reference datasets, decoded files, or
  other material derived from the game or another Mod; or
- a file that carries its own license, notice, or copyright terms.

Those items remain under their respective owners' terms. See the root
`THIRD_PARTY_NOTICES.txt`, each project's notices, and PostRender's
`docs/DEPENDENCY_SOURCES.md` for the currently recorded boundaries.

`DragonSwordUE4SSCompatibilityRuntime` is a packaging and compatibility
project. Its original project scripts and documentation use GPL-3.0-only,
while the UE4SS payload and bundled upstream dependencies retain their own
licenses. The release builder must continue to include the upstream license
tree.

`DragonSwordWorldDataProbe` contains references to the historical local
`PakReaderCore.exe` and `ooz.exe` extraction chain. No source or independent
redistribution grant for those binaries is established by this repository.
They are excluded from the GPL grant and from new public commits until their
provenance and redistribution rights are separately cleared.

## Distribution rule

A distributed project package must include its applicable `LICENSE` and all
relevant third-party notices and license files. A public source checkout is
not evidence that every optional binary-release dependency has been cleared
for redistribution.

Previously sealed binary artifacts and the hashes recorded for them predate
this repository-wide license normalization. Do not relabel or republish those
artifacts as newly GPL-compliant packages without rebuilding their release
layout to include the applicable license material and repeating the release
gates.

This repository is unofficial and is not affiliated with or endorsed by the
game publisher, Epic Games, or the listed third-party projects.
