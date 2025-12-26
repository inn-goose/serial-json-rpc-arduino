#ifndef __serial_json_rpc_lib_h__
#define __serial_json_rpc_lib_h__

#include <limits.h>
#include <ArduinoJson.h>

namespace SerialJsonRpcLibrary {

enum JsonRpcErrorCode : short {
  SUCCESS = 0,
  // https://www.jsonrpc.org/specification#error_object
  PARSE_ERROR = -32700,
  INVALID_REQUEST = -32600,
  METHOD_NOT_FOUND = -32601,
  INVALID_PARAMS = -32602,
  INTERNAL_ERROR = -32603,
  // server error range: -32000 to -32099
  SERVER_ERROR = -32000,
  // unknown
  UNKNOWN_ERROR = SHRT_MAX
};


class SerialJsonRpcBoard {

  // request_id, method, params[], params_size
  using RpcProcessor = void (*)(int, const String&, const String[], int);

public:
  SerialJsonRpcBoard(RpcProcessor rpc_processor);

  void init();
  void loop();

  void send_result_string(int id, const char* string);
  void send_result_bytes(int id, uint8_t* buffer, size_t buffer_size);
  void send_result_longs(int id, long* buffer, size_t buffer_size);
  void send_error(int id, int error_code, const char* error_message, const char* error_data);

  // helpers
  static size_t json_array_to_byte_array(const String& raw_json, uint8_t* byte_array, size_t array_size);

private:
  // default baudrate
  static const unsigned long _DEFAULT_BAUDRATE = 115200;

  // balance between the protocol throughput and the board memory limit
  // works fine for UNO R3
  static const int _JSON_RPC_BUFFER_SIZE = 350;

  // use \n for simiplicity to use both py-client and Arduino Serial Monitor
  static const char _END_OF_JSON_RPC_MESSAGE = '\n';

  void _process_request(JsonDocument& request);

  DynamicJsonDocument _get_response(int id, int data_size);
  void _send_response(DynamicJsonDocument &response);

  int baudrate;

  RpcProcessor rpc_processor_callback;

  char serial_read_buffer[_JSON_RPC_BUFFER_SIZE];
  int serial_read_buffer_pos;
};

SerialJsonRpcBoard::SerialJsonRpcBoard(RpcProcessor rpc_processor)
  : rpc_processor_callback(rpc_processor), serial_read_buffer_pos(0) {}

void SerialJsonRpcBoard::init() {
  Serial.begin(_DEFAULT_BAUDRATE);
}

void SerialJsonRpcBoard::loop() {
  // read data by char if any
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == _END_OF_JSON_RPC_MESSAGE) {
      DynamicJsonDocument request(serial_read_buffer_pos);
      DeserializationError deserialization_error = deserializeJson(request, serial_read_buffer, serial_read_buffer_pos);
      if (deserialization_error) {
        const char* error_data = deserialization_error.c_str();
        send_error(0, JsonRpcErrorCode::PARSE_ERROR, "Parse error", error_data);
      } else {
        _process_request(request);
      }
      request.clear();
      request.garbageCollect();
      serial_read_buffer_pos = 0;
      return;
    }

    // buffer overflow
    if (serial_read_buffer_pos >= _JSON_RPC_BUFFER_SIZE) {
      send_error(0, JsonRpcErrorCode::INVALID_REQUEST, "Invalid Request", "JSON RPC message is to large");
      serial_read_buffer_pos = 0;
      return;
    }

    // read next char
    serial_read_buffer[serial_read_buffer_pos++] = c;
  }
}

void SerialJsonRpcBoard::send_result_string(int id, const char* string) {
  // >"result":< == 9
  // string len + "" (2)
  int data_size = 9 + strlen(string) + 2;
  DynamicJsonDocument response = _get_response(id, data_size);

  DynamicJsonDocument result(data_size);
  JsonVariant result_data = result.to<JsonVariant>();
  result_data.set(String(string));
  response["result"] = result_data;

  _send_response(response);
}

void SerialJsonRpcBoard::send_result_bytes(int id, uint8_t* buffer, size_t buffer_size) {
  // >"result":< == 9
  // max byte = 255 + comma separator + space == 4
  // array braces = [] == 2
  int data_size = 9 + 2 + 4 * buffer_size;
  DynamicJsonDocument response = _get_response(id, data_size);

  DynamicJsonDocument result(data_size);
  JsonArray arr = result.to<JsonArray>();
  for (int i = 0; i < buffer_size; i ++) {
    arr.add(buffer[i]);
  }
  response["result"] = result;

  _send_response(response);
}

void SerialJsonRpcBoard::send_result_longs(int id, long* buffer, size_t buffer_size) {
  // >"result":< == 9
  // max byte = minus + 10 digits + comma separator + space == 13
  // array braces = [] == 2
  int data_size = 9 + 2 + 13 * buffer_size;
  DynamicJsonDocument response = _get_response(id, data_size);

  DynamicJsonDocument result(data_size);
  JsonArray arr = result.to<JsonArray>();
  for (int i = 0; i < buffer_size; i ++) {
    arr.add(buffer[i]);
  }
  response["result"] = result;

  _send_response(response);
}

size_t SerialJsonRpcBoard::json_array_to_byte_array(const String& raw_json, uint8_t* byte_array, size_t array_size) {
  DynamicJsonDocument json_doc(raw_json.length());
  deserializeJson(json_doc, raw_json);
  JsonArray json_array = json_doc.as<JsonArray>();
  if (json_array.size() > array_size) {
    return -1;
  }
  for (size_t i = 0; i < json_array.size(); i++) {
    byte_array[i] = json_array[i].as<uint8_t>();
  }
  return json_array.size();
}

void SerialJsonRpcBoard::send_error(int id, int error_code, const char* error_message, const char* error_data) {
  // {"jsonrpc":"2.0","id":-,"error":{"code":-,"message":"","data":""}}
  // base lenght is 66
  // +10 for ID (max signed 32 len)
  // +10 for error_code
  // 86 in total
  DynamicJsonDocument response(86 + strlen(error_message) + strlen(error_data));
  response["jsonrpc"] = "2.0";
  response["id"] = id;

  JsonObject error = response.createNestedObject("error");
  error["code"] = error_code;
  error["message"] = error_message;
  if (error_data != 0) {
    error["data"] = error_data;
  }

  serializeJson(response, Serial);
  response.clear();
  response.garbageCollect();

  Serial.write(_END_OF_JSON_RPC_MESSAGE);
  Serial.flush();
}

void SerialJsonRpcBoard::_process_request(JsonDocument& request) {
  // validata JSON RPC format
  if (!request.containsKey("jsonrpc") || strcmp(request["jsonrpc"], "2.0") != 0) {
    send_error(0, JsonRpcErrorCode::INVALID_REQUEST, "Invalid Request", "Invalid protocol version");
    return;
  }

  int request_id = request.containsKey("id") ? request["id"].as<int>() : 0;

  const String method = request["method"] | "";
  JsonVariant params = request["params"];

  if (!params.is<JsonArray>()) {
    send_error(request_id, JsonRpcErrorCode::INVALID_PARAMS, "Invalid params", "Array expected");
    return;
  }

  // convert JsonArray to const String[]
  JsonArray params_json_array = params.as<JsonArray>();
  const size_t params_size = params_json_array.size();
  String params_array[params_size];
  for (size_t i = 0; i < params_size; i++) {
    params_array[i] = params_json_array[i].as<String>();
  }

  rpc_processor_callback(request_id, method, params_array, params_size);
}

DynamicJsonDocument SerialJsonRpcBoard::_get_response(int id, int data_size) {
  // {"jsonrpc":"2.0","id":}
  // base lenght is 24
  // +10 for ID (max signed 32 len)
  // 34 in total
  // +10 buffer
  DynamicJsonDocument response(34 + data_size + 10);
  response["jsonrpc"] = "2.0";
  response["id"] = id;
  return response;
}

void SerialJsonRpcBoard::_send_response(DynamicJsonDocument &response) {
  serializeJson(response, Serial);
  response.clear();
  response.garbageCollect();

  Serial.write(_END_OF_JSON_RPC_MESSAGE);
  Serial.flush();
}

}

#endif  // !__serial_json_rpc_lib_h__
