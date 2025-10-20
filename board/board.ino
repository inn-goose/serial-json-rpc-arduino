#import "serial_json_rpc.h"


static const unsigned long SERIAL_BAUD = 115200;


static SerialJsonRpcBoard rpc_board(rpc_processor);


void rpc_processor(int request_id, const String &method, const String params[], int params_size) {
  if (method == "set_builtin_led") {
    if (params_size != 1) {
      rpc_board.send_error(request_id, -32602, "Invalid params", "params_size != 1");
      return;
    }

    int status = atoi(params[0].c_str());

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, status ? HIGH : LOW);

    rpc_board.send_result(request_id, status ? "OK: builtin LED is ON" : "OK: builtin LED is OFF");

  } else {
    rpc_board.send_error(request_id, -32601, "Method not found", method.c_str());
  }
}


void setup() {
  Serial.begin(SERIAL_BAUD);
}


void loop() {
  rpc_board.loop();
}
