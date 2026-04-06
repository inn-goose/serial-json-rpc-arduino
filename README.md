# Serial JSON-RPC for Arduino

A minimal [JSON-RPC 2.0](https://www.jsonrpc.org/specification) implementation for computer-to-Arduino communication over Serial. It hides all encoding/decoding and serial-transfer details, allowing you to focus on defining RPC methods and their handlers.

This is a **template**, not a library. Fork or copy it into your project and extend both sides with your own methods. The included example controls the built-in LED.

> Blog post: [Project: Serial JSON-RPC for Arduino](https://goose.sh/blog/serial-json-rpc/)

## Why JSON-RPC over Serial?

Working with the raw Serial protocol quickly introduces ambiguity: unclear field boundaries, no standard error reporting, and ad-hoc message formats that don't scale across projects. JSON-RPC solves this with a well-defined request/response structure, named methods, typed parameters, and built-in error codes.

The key advantage is **debuggability**: you can paste raw JSON directly into Arduino's Serial Monitor and get structured responses back, with no client tooling required. This makes hardware bringup and iterative development significantly faster.

```
Serial Monitor (115200 baud) >  {"jsonrpc":"2.0","id":0,"method":"set_builtin_led","params":[1]}
                             <  {"jsonrpc":"2.0","id":0,"result":"OK: builtin LED is ON"}
```

## Architecture

```
+-----------------+          Serial / 115200 baud          +-------------------+
|  Python Client  |  <--- newline-delimited JSON-RPC --->  |  Arduino Board    |
|  (py-cli/)      |                                        |  (board/)         |
+-----------------+                                        +-------------------+
```

**Board** (`board/`) -- Arduino sketch + header-only C++ library

| File | Role |
|------|------|
| `serial_json_rpc.h` | `SerialJsonRpcBoard` class. Serial buffering, JSON-RPC parsing, response serialization. Header-only, all implementation inline. |
| `board.ino` | Example sketch. Defines an `rpc_processor` callback that dispatches on method name. |

**Client** (`py-cli/`) -- Python CLI over pySerial

| File | Role |
|------|------|
| `serial_json_rpc/client.py` | `SerialJsonRpcClient` class. Serial connection management, request ID auto-increment, JSON encoding/decoding with timeout-based polling. |
| `cli.py` | CLI entry point. Maps high-level commands (`led_on`, `led_off`) to RPC method calls. |

## Real-World Usage

This template was extracted from the [EEPROM Programmer](https://github.com/inn-goose/eeprom-programmer) project, where it drives all communication between a Python CLI and an Arduino MEGA/DUE for reading and writing AT28Cxx EEPROM chips.

In that project, the JSON-RPC layer handles 7 methods (`init_chip`, `read_page`, `write_page`, etc.) using all three board-side response types -- strings for status messages, byte arrays for memory dumps, and long arrays for performance counters. The Python client wraps `SerialJsonRpcClient` in a domain-specific class that manages pagination, chip initialization, and bulk data transfer across 512+ pages.

The recommended pattern for performance-sensitive applications: keep JSON-RPC for control commands (debuggable, human-readable) and add an optional binary fast-path only for bulk data transfer.

## Getting Started

### Requirements

- Arduino board (tested on UNO R3, MEGA, DUE)
- [ArduinoJson](https://arduinojson.org/) library
- Python 3 with `pyserial`

### Board

Open `board/board.ino` in the Arduino IDE, install the ArduinoJson library, and upload to your board.

### Client

```bash
pip3 install virtualenv

# get python version, use X.Y part below
python3 --version

PATH=${PATH}:~/Library/Python/3.9/bin/ ./env/init.sh
source venv/bin/activate
```

### Run

```bash
# find your serial port
python3 -m serial.tools.list_ports

# turn LED on
PYTHONPATH=./:$PYTHONPATH python3 ./py-cli/cli.py /dev/cu.usbmodem2101 led_on

# turn LED off
PYTHONPATH=./:$PYTHONPATH python3 ./py-cli/cli.py /dev/cu.usbmodem2101 led_off
```

## Adding New Methods

**1. Board side** -- add a branch in `rpc_processor()` in `board.ino`:

```cpp
void rpc_processor(int request_id, const String &method, const String params[], int params_size) {
  if (method == "my_method") {
    // validate params
    if (params_size != 1) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "expected 1 param");
      return;
    }

    // do work...

    // respond with one of three result types:
    rpc_board.send_result_string(request_id, "done");
    // rpc_board.send_result_bytes(request_id, buffer, size);
    // rpc_board.send_result_longs(request_id, buffer, size);

  } else {
    rpc_board.send_error(request_id, -32601, "Method not found", method.c_str());
  }
}
```

**2. Client side** -- add a `Method` enum value in `cli.py` and map it in `execute_method()`:

```python
class Method(Enum):
    MY_METHOD = "my_method"

def execute_method(json_rpc_client, method):
    if method == Method.MY_METHOD:
        return json_rpc_client.send_request("my_method", ["arg1"])
```

**3. Test via Serial Monitor** -- paste into the monitor at 115200 baud:

```json
{"jsonrpc":"2.0","id":0,"method":"my_method","params":["arg1"]}
```

Parameters are always positional arrays, not named objects, on both sides.

## Constraints

| Constraint | Detail |
|-----------|--------|
| **Buffer limit** | 350 bytes (`_JSON_RPC_BUFFER_SIZE`). Hard ceiling for UNO R3's ~2 KB RAM. Messages exceeding this are rejected. |
| **Positional params** | `String[]` arrays only -- no named JSON objects. Saves RAM by avoiding HashMap overhead. |
| **Three result types** | `send_result_string()`, `send_result_bytes()`, `send_result_longs()`. Other types require manual serialization. |
| **Auto-reset** | Arduino resets on every serial connection open, requiring ~2-3 sec init timeout. Board state is lost between sessions. |

## License

MIT
