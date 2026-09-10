# F6 raster build fonts

These unmodified font files are build inputs for deterministic UI text images.
The game loads generated TGA pixels; runtime payloads do not contain or load
these font binaries. SG-12 rasterizes main labels, popup language names, hover
help, confirmations and numeric values for all 11 supported languages. Each
script uses a pinned regular sans-serif face; fixed main labels share the same
32-reference-unit base and role scales, with no artificial bold outline or
per-language shrinking. French/Spanish name-only images remain in the payload.

| Input | Source and license | SHA-256 |
| --- | --- | --- |
| `LiberationSans-Regular.ttf` | Unmodified `pdfjs-dist/standard_fonts` input; SIL Open Font License 1.1 in `LICENSE_LIBERATION` | `F8ACE1F892B2BD9DC1792BA7F097FA7588F84FED48321480E04DE5390828221F` |
| `NotoSansThai-Variable.ttf` | [Google Fonts Noto Sans Thai](https://github.com/google/fonts/tree/main/ofl/notosansthai); SIL Open Font License 1.1 in `LICENSE_NOTO_THAI` | `5A1C559BB539583C8A1FD99D1C5B9491E5E14478C9CD2BD0970D5C3096CC9EF8` |

Noto Sans Thai was downloaded unmodified on 2026-09-09 from
`ofl/notosansthai/NotoSansThai[wdth,wght].ttf` (218,652 bytes). The accompanying
upstream metadata identifies Noto Sans Thai v2.002, source repository
https://github.com/notofonts/thai and commit
`f8f3f024703f9d939d02f4e2fe16f1d5a39ca963`. Copyright 2022 The Noto Project
Authors. `LICENSE_NOTO_THAI` is the unmodified upstream OFL.txt (4,380 bytes;
SHA-256 `2E98FD23A52D253DB8612CD5942C8F2FF4111B21D2367050FDCA91D8CCC374A0`).

CJK glyphs use the existing SDK build input
`.sdk/RE-UE4SS/deps/fonts/droid/DroidSansFallback.ttf`, pinned to SHA-256
`05D71B179EF97B82CF1BB91CEF290C600A510F77F39B4964359E3EF88378C79D`.
It is licensed under Apache 2.0; see the project third-party notices.

Build and verification tools check hashes, required glyph masks, measured text
bounds and emitted pixels. Thai combining characters stay attached to their
base when wrapping. No game font is extracted for these assets.
