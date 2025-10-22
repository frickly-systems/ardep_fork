from typing import Any, List, Optional, Dict
from dataclasses import dataclass
from enum import IntEnum
from generated import data_pb2


class Serializable:
    """Base class for JSON serializable dataclasses"""

    def to_dict(self) -> Dict[str, Any]:
        raise NotImplementedError(
            f"{self.__class__.__name__} must implement to_dict method"
        )


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
    SETUP_UART_TEST = 4
    EXECUTE_UART_TEST = 5
    STOP_UART_TEST = 6


@dataclass
class GPIOResponse(Serializable):
    """GPIO test response payload"""

    @classmethod
    def from_protobuf(cls, pb_gpio_response: Any) -> "GPIOResponse":
        return cls()

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "GPIOResponse":
        """Convert dictionary to GPIOResponse instance"""
        return cls()

    def to_dict(self) -> Dict[str, Any]:
        """Convert GPIOResponse to dictionary"""
        return {}

    def __str__(self) -> str:
        """Human-readable string representation"""
        return "GPIOResponse()"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return "GPIOResponse()"


@dataclass
class UARTResponse(Serializable):
    """UART test response payload"""

    # Add fields here as needed based on UART test requirements
    # Currently empty as per protobuf definition

    @classmethod
    def from_protobuf(cls, pb_uart_response: Any) -> "UARTResponse":
        """Convert protobuf UARTResponse to dataclass"""
        return cls()

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "UARTResponse":
        """Convert dictionary to UARTResponse instance"""
        return cls()

    def to_dict(self) -> Dict[str, Any]:
        """Convert UARTResponse to dictionary"""
        return {}

    def __str__(self) -> str:
        """Human-readable string representation"""
        return "UARTResponse()"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return "UARTResponse()"


@dataclass
class DeviceInfo(Serializable):
    """Device information"""

    device_id: bytes

    @classmethod
    def from_protobuf(cls, pb_device_info: Any) -> "DeviceInfo":
        """Convert protobuf DeviceInfo to dataclass"""
        return cls(
            device_id=pb_device_info.device_id,
        )

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> "DeviceInfo":
        """Convert dictionary to DeviceInfo instance"""
        device_id_hex = data.get("device_id", "")
        device_id = bytes.fromhex(device_id_hex) if device_id_hex else b""
        return cls(device_id=device_id)

    def to_dict(self) -> Dict[str, Any]:
        """Convert DeviceInfo to dictionary"""
        return {"device_id": self.device_id.hex()}

    def __str__(self) -> str:
        """Human-readable string representation"""
        device_id_hex = self.device_id.hex()
        return f"DeviceInfo(device_id=0x{device_id_hex})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return f"DeviceInfo(device_id={self.device_id!r})"


@dataclass
class Response(Serializable):
    """Generic response data matching protobuf definition"""

    result_code: int
    role: DeviceRole
    device_info: Optional[DeviceInfo] = None
    gpio_response: Optional[GPIOResponse] = None
    uart_response: Optional[UARTResponse] = None

    @classmethod
    def from_protobuf(cls, pb_response: Any) -> "Response":
        """Convert protobuf Response to dataclass"""
        device_info = None
        if pb_response.HasField("device_info"):
            device_info = DeviceInfo.from_protobuf(pb_response.device_info)

        gpio_response = None
        if pb_response.HasField("gpio_response"):
            gpio_response = GPIOResponse.from_protobuf(pb_response.gpio_response)

        uart_response = None
        if pb_response.HasField("uart_response"):
            uart_response = UARTResponse.from_protobuf(pb_response.uart_response)

        return cls(
            result_code=pb_response.result_code,
            role=DeviceRole(pb_response.role),
            device_info=device_info,
            gpio_response=gpio_response,
            uart_response=uart_response,
        )

    def to_dict(self) -> Dict[str, Any]:
        """Convert Response to dictionary"""
        result: Dict[str, Any] = {
            "result_code": self.result_code,
            "role": self.role.name,
        }
        if self.device_info:
            result["device_info"] = self.device_info.to_dict()
        if self.gpio_response:
            result["gpio"] = self.gpio_response.to_dict()
        if self.uart_response:
            result["uart"] = self.uart_response.to_dict()
        return result

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
        if self.uart_response:
            parts.append(f"uart_response={self.uart_response}")
        return f"Response({', '.join(parts)})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return (
            "Response("
            f"result_code={self.result_code!r}, "
            f"role={self.role!r}, "
            f"device_info={self.device_info!r}, "
            f"gpio_response={self.gpio_response!r}, "
            f"uart_response={self.uart_response!r}"
            ")"
        )


@dataclass
class Request(Serializable):
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

    def to_dict(self) -> Dict[str, Any]:
        """Convert Request to dictionary"""
        return {"type": self.type.value}

    def __str__(self) -> str:
        """Human-readable string representation"""
        return f"Request(type={self.type.name})"

    def __repr__(self) -> str:
        """Detailed string representation for debugging"""
        return f"Request(type={self.type!r})"
