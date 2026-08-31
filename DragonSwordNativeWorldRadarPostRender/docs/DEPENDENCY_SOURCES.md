# Dependency Sources and Release-Clearance Boundary

This document records the dependency evidence for
`DragonSwordNativeWorldRadarPostRender` version
`2.1.0`. It is an engineering provenance record, not a
legal opinion.

## Public-release status

Public binary publication is currently **blocked**. Three independent questions
must be resolved before publishing the release archive:

1. The pinned `UEPseudo` checkout contains generated Unreal Engine declarations,
   has no repository-level license file in the audited checkout, and contains
   Epic Games copyright notices. The project's GPL-3.0 license does not grant
   rights to that material or establish that the generated interface is
   compatible with a public mod binary. Confirm the applicable Epic/Unreal and
   `UEPseudo` terms before distribution.
2. The bundled `e_sqlcipher.dll` has identifiable package and embedded-component
   versions, but the exact source revision, build recipe, compiler options, and
   reproducible source-to-binary chain for this DLL are not recorded. Confirm
   redistribution obligations and obtain the exact corresponding build source
   provenance before publication.
3. The runtime package includes catalogs derived from the installed game PAK and
   a 147-record coordinate table previously validated from MnMRadar. Confirm the
   right to redistribute those derived records, preserve required attribution,
   and document their exact provenance before publishing either archive.

Internal, local gameplay testing is a separate decision and does not clear
either publication blocker.

## Source-archive scope

The final `dist/final-2.1.0` binary allowlist contains no source archive. If a
separate `-source.zip` is generated later, it is **project source only**. Such an
archive may contain this project's source, catalogs, metadata, documentation,
licenses, tests, and verification tools selected by a dedicated source
allowlist. It must exclude the `.sdk` tree, RE-UE4SS, `UEPseudo`, FetchContent
checkouts, Rust crates, build directories, and third-party toolchains, and it
must not be described as a complete dependency source bundle or as complete
Corresponding Source for every component used to produce the binary.

Builders must acquire the pinned dependencies separately. Every publication
candidate must preserve a build receipt and repeat the binary/import/object
audit because a dependency being present in the build graph does not prove that
its code survived the final link.

## Direct SDK inputs

The following commits are the exact revisions required by the audited build.
URLs below distinguish the exact audited build-source origin from any original
upstream project. A local mirror path or remote name is not, by itself, public
provenance evidence.

| Component | Exact revision | Build role | License evidence / boundary |
| --- | --- | --- | --- |
| RE-UE4SS | `1c1a1497f942c707f47ba668db75b25e86f6c08a` | C++ mod API, reflected Unreal wrappers, and `UE4SS` link/import target | MIT license present in the pinned checkout; separately installed UE4SS runtime is not bundled. |
| UEPseudo | `b2e876da82b17254c04304746341c8fde0ddb37c` | Generated Unreal Engine type/interface declarations consumed through RE-UE4SS | **Not cleared for public release.** No repository-level license was found in the audited checkout; generated files carry Epic Games copyright notices. |
| patternsleuth | `da8bfe4c5a464be0ef225c2c9a6ccaa2d9284018` | RE-UE4SS signature-scanning build dependency | Workspace metadata declares `MIT OR Apache-2.0`. |
| ImGuiColorTextEdit | `6d943aba9f7cef05da80b86dbb0253b63818f95c` | RE-UE4SS UI build dependency | MIT license present in the pinned checkout. |
| IconFontCppHeaders | `210b5a399a64270674560d633638952d1e8d804d` | Explicit override that removes RE-UE4SS's floating branch from the build | No single root license file was found in the audited checkout, and generated headers reference multiple icon-font projects. Treat as build-only and do not claim redistribution clearance from the repository alone. |

Audited build-source origins:

- <https://github.com/UE4SS-RE/RE-UE4SS>
- <https://github.com/Re-UE4SS/UEPseudo>
- <https://github.com/trumank/patternsleuth>
- <https://github.com/UE4SS-RE/ImGuiColorTextEdit>
- <https://github.com/juliettef/IconFontCppHeaders>

The ImGuiColorTextEdit checkout is an RE-UE4SS build-source fork; its original
upstream is <https://github.com/BalazsJako/ImGuiColorTextEdit>.

## Pinned FetchContent sources

These clean source checkouts are validated as build inputs. Except for the
documented deterministic `{fmt}` patch, their audited worktrees must be clean.
Most are transitive RE-UE4SS build dependencies; this table does not assert that
each contributes code to the final mod DLL.

| Component | Exact revision | Canonical source | License in audited checkout | Role |
| --- | --- | --- | --- | --- |
| concurrentqueue | `c68072129c8a5b4025122ca5a0c82ab14b30cb03` | <https://github.com/cameron314/concurrentqueue> | Simplified BSD or Boost Software License 1.0 | RE-UE4SS concurrency dependency. |
| Corrosion | `52844733e14f095c947577627e367ee5f6458af7` | <https://github.com/UE4SS-RE/corrosion> | MIT | CMake-to-Rust build integration for patternsleuth. |
| {fmt} | `40626af88bd7df9a5fb80be7b25ac85b122d6c21` | <https://github.com/fmtlib/fmt> | MIT with the upstream binary-distribution exception | Formatting library; the audited candidate includes a linked `{fmt}` compilation unit. |
| glaze | `3a850807501d98d23bab4bdc5af64d8d4e83e6bc` | <https://github.com/stephenberry/glaze> | MIT | RE-UE4SS JSON/data dependency. |
| GLFW | `e2c92645460f680fd272fd2eed591efb2be7dc31` | <https://github.com/glfw/glfw> | zlib/libpng-style license | RE-UE4SS UI/platform dependency. |
| Dear ImGui | `5d4126876bc10396d4c6511853ff10964414c776` | <https://github.com/ocornut/imgui> | MIT | RE-UE4SS UI dependency. |
| PolyHook 2 | `298d56210b9d9e66cde8f96481d6053925c6ae15` | <https://github.com/stevemk14ebr/PolyHook_2_0> | MIT | RE-UE4SS hooking dependency. |
| raw_pdb | `8c6a7146393c83d27fa101e8bc8017f2a7f151df` | <https://github.com/MolecularMatters/raw_pdb> | BSD-2-Clause | RE-UE4SS PDB parsing dependency. |
| Zydis | `a2278f1d254e492f6a6b39f6cb5d1f5d515659dc` | <https://github.com/zyantific/zydis> | MIT | RE-UE4SS/PolyHook disassembly dependency. |

The RE-UE4SS `{fmt}` macro patch is allowed only when
`include/fmt/ranges.h` has SHA-256
`7146ED70122CCD548AF1BE97794ED069AC1A218B99ADFF4C83D84DFCF4B5276A`.

## Nested pinned sources

The nested revisions below are part of the validated dependency graph and must
not float independently of their parent checkout.

| Parent path | Component | Exact revision | Canonical source | License evidence |
| --- | --- | --- | --- | --- |
| `polyhook2-src/asmjit` | asmjit | `a3199e8857792cd10b7589ff5d58343d2c9008ea` | <https://github.com/asmjit/asmjit> | zlib-style license present. |
| `polyhook2-src/asmtk` | asmtk | `3bce8a48aa895e6d639501d1f1105ab5fe007753` | <https://github.com/asmjit/asmtk> | zlib-style license present. |
| `polyhook2-src/zydis` | Zydis | `a2278f1d254e492f6a6b39f6cb5d1f5d515659dc` | <https://github.com/zyantific/zydis> | MIT. |
| `polyhook2-src/zydis/dependencies/zycore` | Zycore | `0b2432ced0884fd152b471d97ecf0258ff4d859f` | <https://github.com/zyantific/zycore-c> | MIT. |
| `zydis-src/dependencies/zycore` | Zycore | `0b2432ced0884fd152b471d97ecf0258ff4d859f` | <https://github.com/zyantific/zycore-c> | MIT. |

## Bundled SQLCipher runtime

The binary archive includes exactly one non-system runtime library:

- Path: `vendor/sqlcipher/e_sqlcipher.dll`
- SHA-256:
  `32ADED22E9CBAC44AD95FE3D2A1E113BE5DA4A1A81B65D76ECE25762B1BCA196`
- NuGet package: `SQLitePCLRaw.lib.e_sqlcipher` `2.1.10`
- Audited `.nupkg` SHA-256:
  `0430C782E5AC6A06176B76B9C4F24AB3EAF52C6355F738DEC5854F0C5CB0BB8E`
- Package member: `runtimes/win-x64/native/e_sqlcipher.dll`; the staged DLL is
  byte-for-byte identical to that member
- Identifiable embedded components: SQLCipher `4.5.2`, SQLite `3.39.2`, and
  LibTomCrypt `1.18.2`

The package match establishes artifact provenance to the published NuGet
package. It does not identify the exact native source revisions, patches, or
build recipe used to create that package member. The binary archive includes
the recorded Apache-2.0, SQLCipher Community Edition, and LibTomCrypt license
texts plus notices for SQLite's public-domain dedication. Those notices and the
package hash are necessary provenance records, but they do not substitute for
the missing exact source revision and reproducible build chain.

## Publication checklist

Do not change the metadata status from blocked until all of the following are
recorded and reviewed:

- the applicable Unreal Engine and `UEPseudo` redistribution terms;
- a documented compatibility conclusion for the project's GPL-3.0 code and the
  generated Unreal interface used by the binary;
- the exact `e_sqlcipher.dll` source revisions, patches, build configuration,
  compiler/toolchain, and reproducible output hash;
- redistribution rights, attribution, and provenance for the PAK-derived
  catalogs and MnMRadar-derived coordinate table;
- complete notices/license material for every object that survives the final
  link and every bundled runtime library;
- an independently repeated import-table and object/PDB audit for the final DLL;
- a release receipt that binds the published DLL, source snapshot, dependency
  lock, and verification scripts.

Passing tests, package manifests, deployment verification, or gameplay
acceptance does not satisfy this publication-clearance checklist.
