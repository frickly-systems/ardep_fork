import logging
import sys
import json
import time
import argparse
from logging import Logger
from typing import Any
from serial_communication import (
    SerialCommunication,
    decode_response,
    encode_request,
)
from logger_util import APP_PREFIX, get_logger
from data_types import DeviceRole, Request, RequestType, Response

log: Logger = get_logger("main")


def setup_logger(level=logging.INFO):
    for h in logging.root.handlers[:]:
        logging.root.removeHandler(h)
    logging.basicConfig(
        level=logging.WARNING,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    logging.getLogger(APP_PREFIX).setLevel(level)


results: dict[str, Any] = {}


def run(device1: str, device2: str, delimiter: int):
    com1 = SerialCommunication(
        device=device1,
        delimiter=delimiter,
    )

    com2 = SerialCommunication(
        device=device2,
        delimiter=delimiter,
    )

    while True:
        # Device 1 operations
        log.info("Communicating with device 1: %s", device1)
        response_raw: bytes = com1.transmit(
            encode_request(Request(type=RequestType.GET_DEVICE_INFO))
        )
        response1: Response = decode_response(response_raw)
        log.info("Device 1 - Received response: %s", response1)

        # Device 2 operations
        log.info("Communicating with device 2: %s", device2)
        response_raw: bytes = com2.transmit(
            encode_request(Request(type=RequestType.GET_DEVICE_INFO))
        )
        response2: Response = decode_response(response_raw)
        log.info("Device 2 - Received response: %s", response2)

        results["device"] = {
            "sut": (
                response1.to_dict()
                if response1.role == DeviceRole.SUT
                else response2.to_dict()
            ),
            "tester": (
                response1.to_dict()
                if response1.role == DeviceRole.TESTER
                else response2.to_dict()
            ),
        }

        sut = com1 if response1.role == DeviceRole.SUT else com2
        tester = com1 if response1.role == DeviceRole.TESTER else com2

        log.info("Setting up UART test on SUT")
        setup_response_raw: bytes = sut.transmit(
            encode_request(Request(type=RequestType.SETUP_UART_TEST))
        )
        setup_response_sut: Response = decode_response(setup_response_raw)
        log.info("SUT - UART Setup Response: %s", setup_response_sut)

        log.info("Setting up UART test on Tester")
        setup_response_raw: bytes = tester.transmit(
            encode_request(Request(type=RequestType.SETUP_UART_TEST))
        )
        setup_response_tester: Response = decode_response(setup_response_raw)
        log.info("Tester - UART Setup Response: %s", setup_response_tester)

        results["uart_setup"] = {
            "sut": setup_response_sut.to_dict(),
            "tester": setup_response_tester.to_dict(),
        }

        log.info("Execute UART test on SUT")
        setup_response_raw: bytes = sut.transmit(
            encode_request(Request(type=RequestType.EXECUTE_UART_TEST))
        )
        setup_response_sut: Response = decode_response(setup_response_raw)
        log.info("SUT - UART Execute Response: %s", setup_response_sut)

        log.info("Execute UART test on Tester")
        setup_response_raw: bytes = tester.transmit(
            encode_request(Request(type=RequestType.EXECUTE_UART_TEST))
        )
        setup_response_tester: Response = decode_response(setup_response_raw)
        log.info("Tester - UART Execute Response: %s", setup_response_tester)

        results["uart_execute"] = {
            "sut": setup_response_sut.to_dict(),
            "tester": setup_response_tester.to_dict(),
        }

        log.info("Stop UART test on SUT")
        setup_response_raw: bytes = sut.transmit(
            encode_request(Request(type=RequestType.STOP_UART_TEST))
        )
        setup_response_sut: Response = decode_response(setup_response_raw)
        log.info("SUT - UART Stop Response: %s", setup_response_sut)

        log.info("Stop UART test on Tester")
        setup_response_raw: bytes = tester.transmit(
            encode_request(Request(type=RequestType.STOP_UART_TEST))
        )
        setup_response_tester: Response = decode_response(setup_response_raw)
        log.info("Tester - UART Stop Response: %s", setup_response_tester)

        results["uart_stop"] = {
            "sut": setup_response_sut.to_dict(),
            "tester": setup_response_tester.to_dict(),
        }

        log.info("Current Results: %s", results)

        log.info(json.dumps(results, indent=2))

        time.sleep(5)


def main():
    parser = argparse.ArgumentParser(
        description="Data display for multi-sensor Zephyr RTOS project"
    )
    parser.add_argument(
        "-d1",
        "--device1",
        default="/dev/ttyACM1",
        help="First serial device path (default: /dev/ttyACM1)",
    )
    parser.add_argument(
        "-d2",
        "--device2",
        default="/dev/ttyACM3",
        help="Second serial device path (default: /dev/ttyACM3)",
    )
    parser.add_argument(
        "-e",
        "--delimiter",
        type=lambda x: int(x, 16),
        default=0x00,
        help="Delimiter as hex number (default: 0x00)",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        help="Set the logging level (default: INFO)",
    )

    args = parser.parse_args()

    setup_logger(level=args.log_level)

    delimiter = args.delimiter
    if delimiter < 0 or delimiter > 255:
        logging.warning("Delimiter must be a byte value (0-255).")
        sys.exit(1)

    run(args.device1, args.device2, args.delimiter)


if __name__ == "__main__":
    main()
