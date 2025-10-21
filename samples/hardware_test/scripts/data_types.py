from typing import Optional
from dataclasses import dataclass
from enum import IntEnum
from generated import data_pb2


class DeviceRole(IntEnum):
    """Device role enum matching protobuf definition"""

    TESTER = 0
    SUT = 1


class RequestType(IntEnum):
    """Request type enum matching protobuf definition"""

    GET_DEVICE_INFO = 0


@dataclass
class DeviceInfo:
    """Device information"""

    device_id: bytes

    @classmethod
    def from_protobuf(cls, pb_device_info: data_pb2.DeviceInfo) -> "DeviceInfo":
        """Convert protobuf DeviceInfo to dataclass"""
        return cls(
            device_id=pb_device_info.device_id,
        )

    def __str__(self) -> str:
        """Human-readable string representation"""
        device_id_hex = self.device_id.hex()
        return f"DeviceInfo(device_id=0x{device_id_hex})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return f"DeviceInfo(device_id={self.device_id!r})"


@dataclass
class Response:
    """Generic response data matching protobuf definition"""

    result_code: int
    device_info: Optional[DeviceInfo] = None

    @classmethod
    def from_protobuf(cls, pb_response: data_pb2.Response) -> "Response":
        """Convert protobuf Response to dataclass"""
        device_info = None
        if pb_response.HasField("device_info"):
            device_info = DeviceInfo.from_protobuf(pb_response.device_info)

        return cls(
            result_code=pb_response.result_code,
            device_info=device_info,
        )

    def __str__(self) -> str:
        """Human-readable string representation"""
        if self.device_info:
            return f"Response(result_code={self.result_code}, device_info={self.device_info})"
        return f"Response(result_code={self.result_code})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return f"Response(result_code={self.result_code}, device_info={self.device_info!r})"


@dataclass
class Request:
    """Request data matching protobuf definition"""

    type: RequestType

    @classmethod
    def from_protobuf(cls, pb_request: data_pb2.Request) -> "Request":
        """Convert protobuf Request to dataclass"""
        return cls(
            type=RequestType(pb_request.type),
        )

    def to_protobuf(self) -> data_pb2.Request:
        """Convert dataclass to protobuf Request"""
        pb_request = data_pb2.Request()
        pb_request.type = int(self.type)
        return pb_request

    def __str__(self) -> str:
        """Human-readable string representation"""
        return f"Request(type={self.type.name})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return f"Request(type={self.type!r})"
