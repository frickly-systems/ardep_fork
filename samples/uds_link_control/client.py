#!/usr/bin/env -S uv run --script
# /// script
# dependencies = [
#    "can-isotp==2.0.3",
#    "udsoncan==1.21.2",
# ]
# ///
#
# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0


from argparse import ArgumentParser, Namespace
import struct
from typing import Any

import isotp
import udsoncan
from udsoncan.client import Client
from udsoncan.connections import IsoTPSocketConnection
from udsoncan import Baudrate, Request
from udsoncan.exceptions import (
    NegativeResponseException,
)
from udsoncan.services import DiagnosticSessionControl


def data_by_identifier(client: Client):
    data_id = 0x0050
    data = client.read_data_by_identifier_first([data_id])
    print(f"\tReading data from identifier\t0x{data_id:04X}:\t0x{data:04X}")

    print("Requesting baudrate")
    response = client.link_control(control_type=0x01, baudrate=Baudrate(250000))
    if not response.positive:
        print("Negative response when requesting baudrate transition")
        return

    print("Applying baudrate")

    with client.suppress_positive_response(wait_nrc=True):
        response = client.link_control(control_type=0x03)

    print(data)


def try_run(runnable):
    try:
        runnable()

    except NegativeResponseException as e:
        print(
            f"Server refused our request for service {e.response.service.get_name()} "
            f'with code "{e.response.code_name}" (0x{e.response.code:02X})'
        )


class CustomUint16Codec(udsoncan.DidCodec):
    def encode(self, *did_value: Any):
        return struct.pack(">H", *did_value)  # Big endian, 16 bit value

    def decode(self, did_payload: bytes):
        return struct.unpack(">H", did_payload)[0]  # decode the 16 bits value

    def __len__(self):
        return 2  # encoded payload is 2 byte long.


def main(args: Namespace):

    print("=== UDS Link Control Sample Client ===\n")

    can: str = args.can

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)
    conn = IsoTPSocketConnection(can, addr)

    config = dict(udsoncan.configs.default_client_config)
    config["data_identifiers"] = {
        "default": ">H",  # Default codec is a struct.pack/unpack string. 16bits little endian
        0x0050: CustomUint16Codec,
    }

    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: data_by_identifier(client))

        print("\n=== Demo finished ===\n")


if __name__ == "__main__":
    parser = ArgumentParser(description="UDS Link Control Sample Client")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    parsed_args = parser.parse_args()

    main(parsed_args)
