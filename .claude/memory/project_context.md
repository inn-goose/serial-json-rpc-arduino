---
name: Project context and bottlenecks
description: Why this project exists, its relationship to eeprom-programmer, performance bottlenecks, and design rationale for JSON-RPC
type: project
---

This template was backported from the EEPROM Programmer project (../eeprom-programmer), which vendors the header locally as `serial_json_rpc_lib.h` and is the primary real-world consumer.

**Why:** The core design choice is JSON-RPC for debuggability — you can type raw JSON in Arduino Serial Monitor to test board logic with zero tooling, especially valuable during initial hardware bringup.

**How to apply:** Any proposed changes must preserve Serial Monitor testability. Performance optimizations (binary protocol, reduced polling) should be additive/opt-in, not replace the JSON-RPC path.

Key bottlenecks identified from eeprom-programmer profiling:
- ArduinoJson serialization is 62% of per-page CPU time on MEGA
- Python client 50ms polling sleep accumulates ~12s over 512 pages
- JSON inflates 64 bytes to ~323 bytes on wire
- Fully synchronous (no pipelining)
- Projected 10-13x speedup with binary protocol for bulk transfers

Potential improvements discussed (2026-04-03):
1. Reduce Python polling interval (low-hanging fruit)
2. Add bulk transfer example (show send_result_bytes/longs usage)
3. ArduinoJson v6/v7 compatibility (DynamicJsonDocument removed in v7)
4. Optional binary fast-path alongside JSON-RPC (hybrid approach)
5. Client-side wrapper pattern example (like EepromProgrammerClient)
