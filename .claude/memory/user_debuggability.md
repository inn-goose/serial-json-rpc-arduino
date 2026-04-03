---
name: User values debuggability via Serial Monitor
description: User's primary reason for choosing JSON-RPC is ability to test via Arduino Serial Monitor without any CLI tooling
type: feedback
---

JSON-RPC was chosen for debuggability: sending raw JSON text via Serial Monitor in Arduino IDE without any CLI. This is especially important during initial development stages.

**Why:** User explicitly stated this is the reason for the design choice. The convenience of zero-tooling testing during hardware bringup outweighs the performance cost.

**How to apply:** Never propose replacing JSON-RPC with a binary-only protocol. Any performance improvements must be additive (e.g., opt-in binary fast-path) while preserving the JSON-RPC path for Serial Monitor testing.
