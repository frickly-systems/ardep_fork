# pylint: disable=missing-module-docstring, missing-class-docstring, missing-function-docstring, logging-fstring-interpolation, broad-except

from logging import Logger
import serial
import logger_util

from data_types import Request, Response
from cobs import cobs
from generated import data_pb2


log: Logger = logger_util.get_logger(__name__)


def cobs_encode(data: bytes) -> bytes:
    encoded = cobs.encode(data)
    if not encoded or encoded[-1] != 0x00:
        encoded += b"\x00"
    return encoded


def cobs_decode(data: bytes) -> bytes:
    if data and data[-1] == 0x00:
        data = data[:-1]
    return cobs.decode(data)


def protobuf_encode_request(request: Request) -> bytes:
    return request.to_protobuf().SerializeToString()


def protobuf_decode_response(data: bytes) -> Response:
    pb_response = data_pb2.Response()
    pb_response.ParseFromString(data)
    return Response.from_protobuf(pb_response)


def encode_request(request: Request) -> bytes:
    return cobs_encode(protobuf_encode_request(request))


def decode_response(data: bytes) -> Response:
    return protobuf_decode_response(cobs_decode(data))


class SerialCommunication:
    uart: serial.Serial
    device_str: str
    message_delimiter: int

    def __init__(
        self,
        device: str,
        delimiter: int,
    ):
        self.device_str = device
        self.uart = serial.Serial(device, baudrate=115200, timeout=2)
        self.message_delimiter = delimiter

        self.uart.reset_input_buffer()
        self.uart.reset_output_buffer()

    def transmit(self, message: bytes) -> bytes:

        log.debug(
            f"Transmitting message: {" ".join(format(x, "02x") for x in message)}"
        )

        self.uart.reset_input_buffer()
        self.uart.write(message)
        self.uart.flush()

        return self.receive()

    def receive(self) -> bytes:
        msg = self.uart.read_until(expected=bytes([self.message_delimiter]))
        log.debug(f"Received message: {" ".join(format(x, "02x") for x in msg)}")
        return msg
