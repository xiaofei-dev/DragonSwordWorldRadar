# 1.0.17

The 1.0.16 diagnostic proved the fast extractor never reached the decoder:
the candidate filter compared full paths such as `/UnexpectedMissionPlaceData.xml`
against regexes anchored to `UnexpectedMissionPlaceData` without `.xml`.

1.0.17 selects candidates by exact `Entry.File` basename and refuses to continue
unless exactly two entries are found.

The wrapper now copies:
- stage
- internal probe log
- extraction manifest
- directory entry inventory
- decoder evidence

into the normal diagnostics package.

The runtime UScriptStruct route remains as evidence but is bounded to three failed
attempts because UE4SS exposes the MapProperty value pointer without automatically
wrapping it as a Lua TMap in this build.
