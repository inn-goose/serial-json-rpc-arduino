---
name: Project context and backport status
description: Relationship to eeprom-programmer, backport history, and improvement ideas
type: project
---

This template was backported from the EEPROM Programmer project (../eeprom-programmer), which vendors the header locally as `serial_json_rpc_lib.h` and is the primary real-world consumer.

**Why:** The core design choice is JSON-RPC for debuggability — you can type raw JSON in Arduino Serial Monitor to test board logic with zero tooling, especially valuable during initial hardware bringup.

**How to apply:** Any proposed changes must preserve Serial Monitor testability. Performance optimizations (binary protocol, reduced polling) should be additive/opt-in, not replace the JSON-RPC path.

Backported from eeprom-programmer on 2026-04-06:
1. Fix `json_array_to_byte_array()` return 0 instead of -1 (unsigned type bug)
2. Compact JSON encoding with `separators=(',', ':')`
3. Fix response timeout to use `RESPONSE_READ_TIMEOUT_SEC` instead of `init_timeout`
4. Always include `"params": []` in requests (board rejects missing params key)

After this backport, the only remaining difference between template and eeprom-programmer vendored copies is the file name (`serial_json_rpc.h` vs `serial_json_rpc_lib.h`).
