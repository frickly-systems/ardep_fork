# pylint: disable=missing-module-docstring, missing-class-docstring, missing-function-docstring, logging-fstring-interpolation, broad-except

import serial
import logger_util


log = logger_util.get_logger(__name__)


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
        self.uart.reset_input_buffer()
        self.uart.write(message)
        self.uart.flush()

        return self.receive()

    def receive(self) -> bytes:
        return self.uart.read_until(expected=bytes([self.message_delimiter]))
