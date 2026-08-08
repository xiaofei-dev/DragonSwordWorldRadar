DBConnectionProbe 0.6.1 — source package

Purpose
- Passively enumerate modules loaded by DSClient-Win64-Shipping.exe.
- Detect dynamically exported SQLite/SQLCipher APIs.
- Identify whether sqlite3_key/sqlite3_key_v2 is exported.
- Attempt a read-only sqlite_master query only when a compatible exported API is found.
- Produce evidence for the next step: exported-function interception or static-link signature scanning.

Important limitation
This package contains source code, not a precompiled DLL. A reliable DLL cannot be produced here without:
- the exact compiler/Windows SDK environment used for the target,
- confirmation of the game's module loader/injection path,
- and, if SQLite is statically linked, the matching DSClient-Win64-Shipping.exe for signature analysis.

Build
1. Install Visual Studio 2022 with Desktop development with C++ and CMake.
2. Open PowerShell in native/DBConnectionProbe.
3. Run:
   .\build.ps1

Output
native/DBConnectionProbe/bin/DBConnectionProbe.dll

Loading
The DLL must be loaded into the game process by the same trusted native-mod mechanism used by your UE4SS C++ mods.
Do not rename it to an arbitrary game DLL or use proxy-DLL replacement.

Log
runtime/logs/native-db-probe.log

Expected outcomes
1. SQLITE_API_MODULE appears:
   The game dynamically loads SQLite/SQLCipher. Next version can intercept open/key/prepare calls and retain the existing decrypted connection.
2. No SQLITE_API_MODULE:
   SQLite is likely statically linked or hidden behind a game wrapper. The next step requires signature scanning against the exact game executable.
3. DIRECT_SQLITE_MASTER_PREPARE succeeds:
   The exported module applies the database key implicitly, and direct read-only querying is already possible.
