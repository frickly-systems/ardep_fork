from typing import Any, List, Optional
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
    SETUP_GPIO_TEST = 1
    EXECUTE_GPIO_TEST = 2
    STOP_GPIO_TEST = 3


@dataclass
class GPIOResponse:
    """GPIO test response payload"""

    errors: List[str]

    @classmethod
    def from_protobuf(cls, pb_gpio_response: Any) -> "GPIOResponse":
        """Convert protobuf GPIOResponse to dataclass"""
        # Split newline-delimited error payload from protobuf into individual entries
        errors_payload = getattr(pb_gpio_response, "errors", "")
        errors = [entry for entry in errors_payload.splitlines() if entry]
        return cls(errors=errors)

    def __str__(self) -> str:
        """Human-readable string representation"""
        return f"GPIOResponse(errors={self.errors})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return f"GPIOResponse(errors={self.errors!r})"


@dataclass
class DeviceInfo:
    """Device information"""

    device_id: bytes

    @classmethod
    def from_protobuf(cls, pb_device_info: Any) -> "DeviceInfo":
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
    role: DeviceRole
    device_info: Optional[DeviceInfo] = None
    gpio_response: Optional[GPIOResponse] = None

    @classmethod
    def from_protobuf(cls, pb_response: Any) -> "Response":
        """Convert protobuf Response to dataclass"""
        device_info = None
        if pb_response.HasField("device_info"):
            device_info = DeviceInfo.from_protobuf(pb_response.device_info)

        gpio_response = None
        if pb_response.HasField("gpio_response"):
            gpio_response = GPIOResponse.from_protobuf(pb_response.gpio_response)

        return cls(
            result_code=pb_response.result_code,
            role=DeviceRole(pb_response.role),
            device_info=device_info,
            gpio_response=gpio_response,
        )

    def __str__(self) -> str:
        """Human-readable string representation"""
        parts = [
            f"result_code={self.result_code}",
            f"role={self.role.name}",
        ]
        if self.device_info:
            parts.append(f"device_info={self.device_info}")
        if self.gpio_response:
            parts.append(f"gpio_response={self.gpio_response}")
        return f"Response({', '.join(parts)})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return (
            "Response("
            f"result_code={self.result_code!r}, "
            f"role={self.role!r}, "
            f"device_info={self.device_info!r}, "
            f"gpio_response={self.gpio_response!r}"
            ")"
        )


@dataclass
class Request:
    """Request data matching protobuf definition"""

    type: RequestType

    @classmethod
    def from_protobuf(cls, pb_request: Any) -> "Request":
        """Convert protobuf Request to dataclass"""
        return cls(
            type=RequestType(pb_request.type),
        )

    def to_protobuf(self) -> Any:
        """Convert dataclass to protobuf Request"""
        request_cls = getattr(data_pb2, "Request")
        pb_request = request_cls()
        setattr(pb_request, "type", self.type.value)
        return pb_request

    def __str__(self) -> str:
        """Human-readable string representation"""
        return f"Request(type={self.type.name})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return f"Request(type={self.type!r})"
