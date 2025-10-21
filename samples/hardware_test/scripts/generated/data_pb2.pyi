from google.protobuf.internal import enum_type_wrapper as _enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from collections.abc import Mapping as _Mapping
from typing import ClassVar as _ClassVar, Optional as _Optional, Union as _Union

DESCRIPTOR: _descriptor.FileDescriptor

class State(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = ()
    IDLE: _ClassVar[State]

class DeviceRole(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = ()
    TESTER: _ClassVar[DeviceRole]
    SUT: _ClassVar[DeviceRole]

class RequestType(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = ()
    GET_DEVICE_INFO: _ClassVar[RequestType]
IDLE: State
TESTER: DeviceRole
SUT: DeviceRole
GET_DEVICE_INFO: RequestType

class DeviceInfo(_message.Message):
    __slots__ = ("device_id", "role")
    DEVICE_ID_FIELD_NUMBER: _ClassVar[int]
    ROLE_FIELD_NUMBER: _ClassVar[int]
    device_id: bytes
    role: DeviceRole
    def __init__(self, device_id: _Optional[bytes] = ..., role: _Optional[_Union[DeviceRole, str]] = ...) -> None: ...

class Response(_message.Message):
    __slots__ = ("result_code", "device_info")
    RESULT_CODE_FIELD_NUMBER: _ClassVar[int]
    DEVICE_INFO_FIELD_NUMBER: _ClassVar[int]
    result_code: int
    device_info: DeviceInfo
    def __init__(self, result_code: _Optional[int] = ..., device_info: _Optional[_Union[DeviceInfo, _Mapping]] = ...) -> None: ...

class Request(_message.Message):
    __slots__ = ("type",)
    TYPE_FIELD_NUMBER: _ClassVar[int]
    type: RequestType
    def __init__(self, type: _Optional[_Union[RequestType, str]] = ...) -> None: ...
