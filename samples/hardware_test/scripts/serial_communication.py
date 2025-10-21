# pylint: disable=missing-module-docstring, missing-class-docstring, missing-function-docstring, logging-fstring-interpolation, broad-except

import time
from typing import Any, Optional, Callable
import threading

import serial
from cobs import cobs
import logger_util


log = logger_util.get_logger(__name__)


class SerialCommunication:
    device_str: str
    uart: serial.Serial
    cobs_delimiter: int
    send_bytes: list[bytes]

    response_callback: Callable[[Any], None]
    deserialize_callback: Callable[[bytes], Any]
    serialize_callback: Callable[[Any], bytes]

    send_thread: Optional[threading.Thread] = None
    receive_thread: Optional[threading.Thread] = None

    def __init__(
        self,
        device: str,
        delimiter: int,
        response_callback: Callable[[Any], None],
        deserialize_callback: Callable[[bytes], Any],
        serialize_callback: Callable[[Any], bytes],
    ):
        # Ensure logger is set up
        self.device_str = device
        self.cobs_delimiter = delimiter
        self.deserialize_callback = deserialize_callback
        self.response_callback = response_callback
        self.serialize_callback = serialize_callback
        self.uart = serial.Serial(device, baudrate=115200, timeout=2)
        self.send_bytes = []

    def receive_data_thread_cb(self) -> None:
        self.uart.reset_input_buffer()
        self.uart.read_until(expected=bytes([self.cobs_delimiter]))

        while True:
            try:
                raw_data: bytes = self.uart.read_until(
                    expected=bytes([self.cobs_delimiter])
                )

                log.debug(f"Received raw:\t{' '.join(f'{b:02x}' for b in raw_data)}")
                if raw_data and raw_data[-1] == self.cobs_delimiter:
                    raw_data = raw_data[:-1]

                if not raw_data:
                    continue

                decoded: bytes = cobs.decode(raw_data)
                log.debug(f"Decoded packet:\t{' '.join(f'{b:02x}' for b in decoded)}")

                # Parse as SensorMessage directly
                try:
                    self.response_callback(self.deserialize_callback(decoded))

                except Exception as e:
                    log.error(f"Error parsing Response: {e}")

            except Exception as e:
                log.error(f"Error in receive thread: {e}")

    def send_data(self, data: Any) -> None:
        if not self.uart.is_open:
            log.warning(f"Serial port {self.device_str} is not open.")
            return

        log.debug(f"Sending data: {data}")
        log.info(f"Sending data: {data}")

        serialized_data = self.serialize_callback(data)

        encoded_data = cobs.encode(serialized_data)

        self.send_bytes.append(encoded_data)

    def send_data_thread_cb(self) -> None:
        self.uart.reset_output_buffer()

        while True:
            if not self.send_bytes:
                time.sleep(0.1)
                continue

            data_to_send = self.send_bytes.pop(0)
            data_to_send += bytes([self.cobs_delimiter])
            log.info(f"Sending data: {' '.join(f'{b:02x}' for b in data_to_send)}")

            self.uart.write(data_to_send)
            log.debug(f"Sent {len(data_to_send)} bytes + delimiter")

            # time.sleep(0.1)

    def start(self) -> None:
        self.stop()

        self.receive_thread = threading.Thread(
            target=self.receive_data_thread_cb, daemon=True
        )

        self.send_thread = threading.Thread(
            target=self.send_data_thread_cb, daemon=True
        )

        # Actually start the threads
        self.receive_thread.start()
        self.send_thread.start()
        log.info("Threads started")

    def stop(self) -> None:
        if self.receive_thread:
            self.receive_thread.join(timeout=1)
        if self.send_thread:
            self.send_thread.join(timeout=1)
