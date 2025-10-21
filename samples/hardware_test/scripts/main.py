import logging
import sys
import time
import argparse
from logging import Logger
from serial_communication import (
    SerialCommunication,
    decode_response,
    encode_request,
)
from logger_util import APP_PREFIX, get_logger
from data_types import Request, RequestType

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


def run(device: str, delimiter: int):
    com = SerialCommunication(
        device=device,
        delimiter=delimiter,
    )

    while True:
        response_raw = com.transmit(
            encode_request(Request(type=RequestType.GET_DEVICE_INFO))
        )
        response = decode_response(response_raw)

        log.info(f"Received response: {response}")

        response_raw = com.transmit(
            encode_request(Request(type=RequestType.SETUP_GPIO_TEST))
        )
        response = decode_response(response_raw)
        log.info(f"Received response: {response}")

        response_raw = com.transmit(
            encode_request(Request(type=RequestType.EXECUTE_GPIO_TEST))
        )
        response = decode_response(response_raw)
        log.info(f"Received response: {response}")

        response_raw = com.transmit(
            encode_request(Request(type=RequestType.STOP_GPIO_TEST))
        )
        response = decode_response(response_raw)
        log.info(f"Received response: {response}")

        time.sleep(1)


def main():
    parser = argparse.ArgumentParser(
        description="Data display for multi-sensor Zephyr RTOS project"
    )
    parser.add_argument(
        "-d",
        "--device",
        default="/dev/ttyACM1",
        help="Serial device path (default: /dev/ttyACM1)",
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

    run(args.device, args.delimiter)


if __name__ == "__main__":
    main()
