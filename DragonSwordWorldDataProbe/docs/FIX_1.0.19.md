# 1.0.19 WinPS uint literal fix

The 1.0.18 diagnostic proved the XOR16 path stopped before decoding `bits`
because Windows PowerShell 5.1 does not support the C#-style unsigned literal
`1u`.

Only these expressions were changed:

- `(1u -shl 22)` -> `([uint32]1 -shl 22)`
- `(1u -shl 31)` -> `([uint32]1 -shl 31)`
- `(1u -shl 30)` -> `([uint32]1 -shl 30)`
- `(1u -shl 29)` -> `([uint32]1 -shl 29)`

No decoder logic or validation policy was changed.
