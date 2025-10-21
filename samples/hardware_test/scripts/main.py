import logging
import sys
import time
import argparse
from typing import Any
from generated import data_pb2
from serial_communication import SerialCommunication
from logger_util import APP_PREFIX, get_logger
from data_types import Request, Response, RequestType


def setup_logger(level=logging.INFO):
    for h in logging.root.handlers[:]:
        logging.root.removeHandler(h)
    logging.basicConfig(
        level=logging.WARNING,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    logging.getLogger(APP_PREFIX).setLevel(level)  # controls all "myapp.*"


def response_callback(response: Response):
    log = get_logger(APP_PREFIX)
    log.info(f"Received Response: {response}")


def deserialize_response(data: bytes) -> Response:
    return Response.from_protobuf(data_pb2.Response.FromString(data))


def serialize_request(request: Request) -> bytes:
    log = get_logger(APP_PREFIX)
    log.debug(f"Serializing request: {request}")

    a: data_pb2.Request = request.to_protobuf()
    log.debug(f"Protobuf request type: {a.type}")
    log.debug(f"Protobuf request: {a}")

    b: bytes = a.SerializeToString()
    log.debug(
        f"Serialized bytes length: {len(b)}, content: {b.hex() if b else 'empty'}"
    )

    return b


def run(device: str, delimiter: int):

    com = SerialCommunication(
        device=device,
        delimiter=delimiter,
        deserialize_callback=deserialize_response,
        serialize_callback=serialize_request,
        response_callback=response_callback,
    )
    com.start()

    try:
        while True:
            logging.info("Heartbeat")
            com.send_data(Request(type=RequestType.GET_DEVICE_INFO))
            time.sleep(1)
    except KeyboardInterrupt:
        logging.info("Shutting down...")


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
