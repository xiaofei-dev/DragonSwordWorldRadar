# 1.0.34 real TreasureBox actor proximity

The 1.0.33 diagnostic proved:
- PropTreasureBoxData: 1768 rows
- SectionTreasureBoxData: 1693 rows
- 1692/1693 unique static joins

The only Blueprint parser bug was the actual field spelling `BluePrintPath`.

The resulting world TreasureBox actor family contains exactly 11 generated classes:

- TreasureBox01_C
- TreasureBox02_C
- TreasureBox02_Mount_C
- TreasureBox03_C
- TreasureBox03_Mount_C
- TreasureBox03_OnlyFront_C
- TreasureBox04Key_C
- TreasureBox04_C
- TreasureBox05_C
- TreasureBox05_Mount_C
- TreasureBox06_C

1.0.34 uses only those exact classes at runtime.

For each known treasure point within 2500 Unreal units (~25m), it checks whether a
matching exact-class Actor exists near the static coordinate.

Diagnostic state:
- actor present -> `unopened_present`
- missing for 1-2 consecutive samples -> `pending_absence_confirmation`
- missing for >=3 consecutive samples -> `suspected_opened_absent`
- actor reappears -> immediately reset to `unopened_present`

This is deliberately not yet used as production opened state. It is collected
against the existing tb_treasure_box ground truth first.

RespawnCycle XML parsing now uses XmlDocument.Load(path), preventing PowerShell
text-decoding corruption of the Korean UTF-8 XML.
