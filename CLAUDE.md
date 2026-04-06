# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A JSON-RPC 2.0 template for Arduino-to-computer communication over serial. Not a library — meant to be forked and extended with custom methods on both sides. The example implementation controls the built-in LED.

Backported from the [EEPROM Programmer](https://github.com/inn-goose/eeprom-programmer) project, which is the primary real-world consumer of this template.

## Architecture

Two sides communicate via JSON-RPC 2.0 over serial (newline-delimited JSON at 115200 baud):

- **Board side** (`board/`): Arduino sketch + header-only library
  - `serial_json_rpc.h` — `SerialJsonRpcBoard` class (header-only, all implementation inline). Handles serial buffering, JSON-RPC parsing, and response serialization. Uses ArduinoJson `DynamicJsonDocument` (v6 API — note this was removed in ArduinoJson v7). Buffer limit is 350 bytes.
  - `board.ino` — Sketch that defines a `rpc_processor` callback dispatching on method name. Params are passed as `String[]` (not a map) due to Arduino memory constraints.

- **Python client** (`py-cli/`): pySerial-based JSON-RPC client
  - `serial_json_rpc/client.py` — `SerialJsonRpcClient` class. Manages serial connection, request ID auto-increment, JSON encoding/decoding with timeout-based polling (50ms sleep between checks).
  - `cli.py` — CLI entry point using argparse. Maps high-level command names (e.g. `led_on`) to RPC method calls.

## Arduino Hardware Constraints

- **350-byte buffer limit** (`_JSON_RPC_BUFFER_SIZE`): Hard ceiling due to Arduino UNO R3's ~2KB RAM. Any JSON-RPC message exceeding this is rejected.
- **Positional params only**: Params are `String[]` arrays, not named JSON objects — saves RAM by avoiding HashMap/key overhead.
- **Three result types only**: `send_result_string()`, `send_result_bytes()`, `send_result_longs()`. Other types require manual serialization.
- **Auto-reset on serial connect**: Arduino resets every time a serial connection opens, requiring ~2-3 second init timeout and losing all board state between sessions.

## Design Rationale

JSON-RPC exists for **debuggability**: you can send raw JSON text via Arduino Serial Monitor to test board-side logic with zero tooling. Never replace JSON-RPC with a binary-only protocol — any performance optimizations (binary fast-path, reduced polling) should be additive/opt-in.

## Python Environment Setup

```bash
pip3 install virtualenv
PATH=${PATH}:~/Library/Python/3.9/bin/ ./env/init.sh
source venv/bin/activate
```

Dependencies: `pyserial==3.4` (in `env/requirements_cli.txt`).

## Running the CLI

```bash
source venv/bin/activate
PYTHONPATH=./:$PYTHONPATH python3 ./py-cli/cli.py <port> <method>
# e.g.: PYTHONPATH=./:$PYTHONPATH python3 ./py-cli/cli.py /dev/cu.usbmodem2101 led_on
```

List available serial ports: `python -m serial.tools.list_ports`

## Adding New RPC Methods

1. **Board**: Add a new branch in `rpc_processor()` in `board.ino`, matching on method name string. Use `rpc_board.send_result_string()`, `send_result_bytes()`, or `send_result_longs()` for responses.
2. **Client**: Add a new `Method` enum value in `cli.py` and map it to a `send_request()` call in `execute_method()`.
3. Params are positional arrays (JSON arrays), not named objects, on both sides.

## Board Compilation

The Arduino sketch requires the [ArduinoJson](https://arduinojson.org/) library (currently using v6 API). Compile and upload via Arduino IDE or `arduino-cli`.
