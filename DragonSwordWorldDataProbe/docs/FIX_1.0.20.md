# 1.0.20

1.0.19 proved `ReadDataEntry` itself succeeded. The failure occurred only while
serializing debug information because the compiled `DataEntry` model does not
expose a `PayloadOffset` member.

That diagnostic exception occurred before `Test-DataEntryMatchesCompact`, so
`direct_match=false` in 1.0.19 was not a real mismatch.

1.0.20 makes debug snapshots fully optional and evaluates matching before
diagnostic serialization. Decoder semantics are otherwise unchanged.
