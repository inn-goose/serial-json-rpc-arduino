from typing import Any, Dict, List, Optional, Tuple

import json
import time

import serial


class SerialJsonRpcClientError(Exception):
    pass


class SerialJsonRpcClient:
    """
    https://pyserial.readthedocs.io/en/latest/pyserial.html
    """

    # https://www.jsonrpc.org/specification
    JSON_RPC_VERSION = "2.0"

    RESPONSE_READ_TIMEOUT_SEC = 2.0

    def __init__(self, port: str, baudrate: int, init_timeout: float, read_timeout: Optional[float] = None, write_timeout: Optional[float] = None):
        self.port = port
        self.baudrate = baudrate
        self.init_timeout = init_timeout
        self.read_timeout = read_timeout
        self.write_timeout = write_timeout
        #
        self.serial = None
        self.json_rpc_request_id = 0

    def init(self) -> str:
        if self.serial is not None:
            # already initialized
            return

        # initialize serial protocol
        try:
            self.serial = serial.Serial(
                port=self.port, baudrate=self.baudrate, timeout=self.read_timeout, write_timeout=self.write_timeout)
        except Exception as ex:
            raise SerialJsonRpcClientError(
                f"failed to open serial port with {str(ex)}")

        # init: read welcome message
        # Arduino auto-resets on every new serial session
        # so we need to wait for the full board initialization
        response, _ = self._read_response(self.init_timeout)

        # can be None
        return response

    def send_request(self, method: str, params: Optional[List[Any]]) -> str:
        if self.serial is None:
            raise SerialJsonRpcClientError("uninitialized serial protocol")

        request = self._build_request(method, params)

        # send request and read the amount of written bytes
        w_res = self.serial.write((json.dumps(request) + '\n').encode())
        if not w_res:
            raise SerialJsonRpcClientError(
                "failed to send request, 0 bytes written")

        # flush the data to the board
        self.serial.flush()

        response, resp_wait_sec = self._read_response(self.init_timeout)
        if response is None:
            raise SerialJsonRpcClientError(
                f"failed to read response for {method}, resp_wait_sec = {resp_wait_sec}")

        return response

    def _build_request(self, method: str, params: Optional[List[Any]] = None) -> Dict[str, Any]:
        request = {
            "jsonrpc": self.JSON_RPC_VERSION,
            "id": self.json_rpc_request_id,
            "method": method,
        }
        if params:
            request["params"] = params
        self.json_rpc_request_id += 1
        return request

    def _read_response(self, read_timeout_sec: float) -> Tuple[Optional[str], float]:
        if self.serial is None:
            raise SerialJsonRpcClientError("uninitialized serial protocol")

        start_ts = time.time()
        deadline_ts = start_ts + read_timeout_sec

        buffer = b""
        raw_response = None
        resp_wait_sec = read_timeout_sec

        # keep reading until full JSON is received
        while time.time() < deadline_ts:
            if self.serial.in_waiting > 0:
                buffer += self.serial.read(self.serial.in_waiting)
                try:
                    raw_response = json.loads(buffer.decode())
                    resp_wait_sec = time.time() - start_ts
                    break
                except json.JSONDecodeError:
                    continue
            time.sleep(0.05)

        return self._parse_response(raw_response), resp_wait_sec

    def _parse_response(self, response: Optional[Dict[str, Any]]) -> Optional[str]:
        if response is None:
            return None

        jsonrpc = response.get("jsonrpc", None)
        if jsonrpc != self.JSON_RPC_VERSION:
            raise SerialJsonRpcClientError(
                f"parse error: invalid `jsonrpc` = {jsonrpc}")

        error = response.get("error", None)
        if error:
            raise SerialJsonRpcClientError(f"error response: {error}")

        result = response.get("result", None)
        if result is None:
            raise SerialJsonRpcClientError(f"parse error: missing `result`")

        return result
