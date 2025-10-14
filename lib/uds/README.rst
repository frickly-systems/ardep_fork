.. _uds-lib:

UDS Library
###########

This library encapsulates and extends the :ref:`iso14229-lib` and provides a higher level API to predefine UDS services via event handlers with associated data.

The following UDS services are currently implemented:

.. list-table::
    :header-rows: 1
    :widths: 50 50

    * - Service
      - SID
    * - Diagnostic Session Control
      - ``0x10``
    * - ECU Reset
      - ``0x11``
    * - Clear Diagnostic Information
      - ``0x14``
    * - Read DTC Information
      - ``0x19``
    * - Read Data By Identifier
      - ``0x22``
    * - Read Memory By Address
      - ``0x23``
    * - Security Access
      - ``0x27``
    * - Communication Control
      - ``0x28``
    * - Authentication
      - ``0x29``
    * - Dynamically Define Data Identifier
      - ``0x2C``
    * - Write Data By Identifier
      - ``0x2E``
    * - Input Output Control By Identifier
      - ``0x2F``
    * - Routine Control
      - ``0x31``
    * - Request Download
      - ``0x34``
    * - Request Upload
      - ``0x35``
    * - Transfer Data
      - ``0x36``
    * - Request Transfer Exit
      - ``0x37``
    * - Request File Transfer
      - ``0x38``
    * - Write Memory By Address
      - ``0x3D``
    * - Tester Present
      - ``0x3E``
    * - Control DTC Settings
      - ``0x85``
    * - Link Control
      - ``0x86``

Basic API
*********

Usually, every event is associated with a ``check`` and an ``action`` function.
The ``check`` function is supposed to check, whether the event with its arguments can be handled in the current state of the ECU.
If it is the case, the associated ``action`` function is executed to act on the event.

The UDS library uses several Macros to allow you to statically define event handlers for specific groups of events.



Macros and KConfig options per Event
====================================

The following macros are available to register event handlers for the specific event:

``UDS_EVT_ReadDTCInformation``
------------------------------

- ``UDS_REGISTER_READ_DTC_INFO_HANDLER``: Register a handler for a specific subfunction
- ``UDS_REGISTER_READ_DTC_INFO_HANDLER_MANY``: Register a handler for multiple subfunctions
- ``UDS_REGISTER_READ_DTC_INFO_HANDLER_ALL``: Register a handler for all subfunctions
  
Because the *Read DTC Information* service has so many subfunctions, it is possible to register a handler for every subfunction for more fine grained control.

``UDS_EVT_ReadMemByAddr``, ``UDS_EVT_WriteMemByAddr``
-----------------------------------------------------------------
- ``UDS_REGISTER_MEMORY_HANDLER``: Register a custom handler
- ``UDS_REGISTER_MEMORY_DEFAULT_HANDLER``: Register a default handler provided by the library

``UDS_EVT_EcuReset``, ``UDS_EVT_DoScheduledReset``
--------------------------------------------------

- ``UDS_REGISTER_ECU_RESET_HANDLER``: Register a custom handler
- ``UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER``: Register a default handler for a hard reset event

``UDS_EVT_ReadDataByIdent``, ``UDS_EVT_WriteDataByIdent``, ``UDS_EVT_IOControl``
--------------------------------------------------------------------------------

- ``UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER``: Register a custom handler

The *IO Control* and *Read/Write Data by Identifier* events because they share the Data Identifiers

``UDS_EVT_DiagSessCtrl``, ``UDS_EVT_SessionTimeout``
----------------------------------------------------

- ``UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER``: Register a custom handler
  
This handler is optional. Requests to this service will succeed even without an event handler.
The current session is stored internally and can be accessed as follows:

    .. code-block:: c

        struct uds_instance_t instance;
        
        instance.iso14229.server.sessionType
        
.. note::

    The default link control event handler also registers a handler for these events. Make sure you don't consume the events when using the default link control handler
        
``UDS_EVT_ClearDiagnosticInfo``
--------------------------------

- ``UDS_REGISTER_CLEAR_DIAG_INFO_HANDLER``: Register a custom handler
  
``UDS_EVT_RoutineCtrl``
-----------------------

- ``UDS_EVT_ClearDiagnosticInfo``: Register a custom handler

``UDS_EVT_SecAccessRequestSeed``, ``UDS_EVT_SecAccessValidateKey``
---------------------------------------------------------------------------

- ``UDS_REGISTER_SECURITY_ACCESS_HANDLER``: Register a custom handler

``UDS_EVT_CommCtrl``
---------------------
  
- ``UDS_REGISTER_COMMUNICATION_CONTROL_HANDLER``: Register a custom handler

``UDS_EVT_ControlDTCSetting``
-----------------------------

- ``UDS_REGISTER_CONTROL_DTC_SETTING_HANDLER``: Register a custom handler


``UDS_EVT_DynamicDefineDataId``
-------------------------------

- ``UDS_REGISTER_DYNAMICALLY_DEFINE_DATA_IDS_HANDLER``: Register a custom handler
- ``UDS_REGISTER_DYNAMICALLY_DEFINE_DATA_IDS_DEFAULT_HANDLER``: Register a default handler provided by the library

Since dynamically defining data identifiers is a complex task that requires accessing internal structures of the uds library, it is recommended to only use the provided default handler.

.. note::

    The registration / handling of these events requires enabling the KConfig option: ``UDS_USE_DYNAMIC_REGISTRATION``

``UDS_EVT_Auth``, ``UDS_EVT_AuthTimeout``
-----------------------------------------

- ``UDS_REGISTER_AUTHENTICATION_HANDLER``: Register a custom handler

``UDS_EVT_LinkControl``
-----------------------

- ``UDS_REGISTER_LINK_CONTROL_HANDLER``: Register a custom handler
- ``UDS_REGISTER_LINK_CONTROL_DEFAULT_HANDLER``: Register a default handler provided by the library

Because the *Link Control* service changes the communication settings of the underlying can interface, it is recommended to use the default handler to correctly restore the interface on a timeout.

The default handler also registers a handler for the session control events, so don't consume these events if you use the default handler.

.. note::

    The usage of this is gated behind the KConfig option which is disabled by default: ``UDS_USE_LINK_CONTROL``


``UDS_EVT_RequestDownload``, ``UDS_EVT_RequestUpload``, ``UDS_EVT_TransferData``, ``UDS_EVT_RequestTransferExit``, ``UDS_EVT_RequestFileTransfer``
--------------------------------------------------------------------------------------------------------------------------------------------------

These events don't allow registration of custom handlers. The data transfer can be enabled by setting the KConfig option: ``UDS_USE_DYNAMIC_REGISTRATION``.

To further allow file transfer, additionally enable ``UDS_FILE_TRANSFER``




Internals
*********

Internally, the UDS library uses an `Iterable Section <https://docs.zephyrproject.org/4.2.0/kernel/iterable_sections/index.html>`_ to hold statically defined event handlers and a single linked list for dynamically defined event handlers.

When an event is received from the underlying :ref:`iso14229-lib`, the UDS library iterates over the iterable section to find an event handler that can handle the event. Then, the check and action function are called as described in **TODO! REFERENCE**.
If the event is consumed, the iteration stops, if not, the iteration continues on.
When the event was not consumed by any event handlers in the iterable seciton, the dynamically defined event handlers are iterated over next.
Finally, when no event handler could be found, a negative default response is send back to the client.