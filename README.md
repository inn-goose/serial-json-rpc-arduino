# Serial JSON RPC for Arduino

## TLDR

The basic serial protocol between a computer and an Arduino board is often insufficient for board control and/or complex data exchange. Implementing primitive command handlers on top of the basic serial protocol is also inconvenient, so this project implements the existing [JSON-RPC protocol](https://www.jsonrpc.org/specification) to perform remote command execution on Arduino.

This is a template, not a ready-to-use library. You can use it as a foundation to build both the Arduino side and the client side. To do so, define the set of operations for both the board and the client, specify the parameters on the client side, and implement the corresponding operation handlers on the board side.

The example implementation provided here allows turning the built-in LED on and off on the board.

A better example of using this template is the [`EEPROM Programmer`](https://github.com/inn-goose/eeprom-programmer) project.


## Serial Connection and Board Reset

Note that each time a serial connection is established, the Arduino board performs a full reset. [Details](https://forum.arduino.cc/t/arduino-auto-resets-after-opening-serial-monitor/850915).

This has two inconvenient consequences:
(a) after each connection, you must wait about 2 seconds (on the UNO R3), and
(b) the microcontroller’s internal state is lost between sessions.

In practice, this means that the provided example does not simply turn the LED on and off, but **resets** → **turns on** → **resets again** → **turns off** in case of the "on → off" cycle. During each reset, the LED enters an undefined state (in my case, it glows dimly).


## Board

Board side uses the [ArduinoJson 7](https://arduinojson.org/)

> Note: use C++ `array` for `params` due to Arduino's limitations with `HashMap`

### `SerialJsonRpcBoard` class

Check the `board.ino` for the actual usecase

```cpp
#import "serial_json_rpc.h"

using namespace SerialJsonRpcLibrary;

// define the board
static SerialJsonRpcBoard rpc_board(rpc_processor);

// define a callback to execute the RPC
void rpc_processor(int request_id, const String &method, const String params[], int params_size) {
    ...

    // send result
    rpc_board.send_result_string(0, "Success");

    // send errro
    rpc_board.send_error(0, -1, "Error Title", "Error Details");
}

// read the data from the Serial
void loop() {
  rpc_board.loop();
}
```

### usage

Serial Monitor test / `115200` baud
```json
{"jsonrpc":"2.0","id":0,"method": "set_builtin_led", "params": [1]}

{"jsonrpc":"2.0","id":0,"method": "set_builtin_led", "params": [0]}
```


## Client

Client side uses the [pySerial library](https://pyserial.readthedocs.io/en/latest/pyserial.html)

> Note: use python `List` for `params` due to Arduino's limitations with `HashMap`

### `SerialJsonRpcClient` class

check the `cli.py` for the actual usecase

```py
from serial_json_rpc import client

# prefer singleton
json_rpc_client = client.SerialJsonRpcClient(...)

# run once, it restarts the board
json_rpc_client.init()

# send RPC request
result = json_rpc_client.send_request("set_builtin_led", [0])
```

### init

```bash
pip3 install virtualenv

PATH=${PATH}:~/Library/Python/3.9/bin/ ./env/init.sh

source venv/bin/activate

deactivate
```

### usage

> Note: arduino restarts on every serial session: [discussion](https://forum.arduino.cc/t/arduino-auto-resets-after-opening-serial-monitor/850915), so it require `~2 sec` to init before processing requests. use `--init-timeout` to configure

```bash
source venv/bin/activate

python -m serial.tools.list_ports

PYTHONPATH=./:$PYTHONPATH python3 ./py-cli/cli.py /dev/cu.usbmodem2101 led_on

PYTHONPATH=./:$PYTHONPATH python3 ./py-cli/cli.py /dev/cu.usbmodem2101 led_off
```
