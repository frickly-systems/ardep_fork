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

To handle events from the underlying :ref:`iso14229-lib`, the UDS library uses event handlers registered either statically or dynamically at runtime.
And event handler basically associates custom callbacks (``check`` and ``action``) and related data with one or more UDS events.

The ``check`` function is supposed to check, whether the event with its arguments can be handled in the current state of the ECU.
If it is the case, the associated ``action`` function is executed to act on the event.

One or more the these ``check`` / ``action`` pairs together with other associated data are registered as an event handler within an ``struct uds_registration_t`` for the ``struct uds_instance_t``.

These registrations can be statically defined via macros or dynamically at runtime (see below).

When an event is received from the underlying :ref:`iso14229-lib`, the UDS library iterates over all registered event handlers and calls the ``check`` function of each handler and the ``action`` function if applicable.


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

The current security level is stored internally and can be accessed as follows:

    .. code-block:: c

        struct uds_instance_t instance;
        
        instance.iso14229.server.securityLevel

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
  
Since the authentication process is more complex than a simple security access, no authentication data is stored internally.
It is up to the user to store and manage authentication data. To make it available in the context for every event handler, you can store the data e.g. in the user context when initializing the UDS instance:

    .. code-block:: c

        struct my_context {
            // your data
        };
        
        struct my_context context = {
            // initialize your data
        };
        
        struct uds_instance_t instance;
        
        uds_init(&instance, &iso_tp_config, &can_dev, &context)
        

        UDSErr_t my_custom_check_function(const struct uds_context *const context,
                                 bool *apply_action){
            struct my_context *context = (struct my_context *)context->instance->user_context;
            
            // ...
        }

``UDS_EVT_TesterPresent``

``UDS_EVT_LinkControl``
-----------------------

- ``UDS_REGISTER_LINK_CONTROL_HANDLER``: Register a custom handler
- ``UDS_REGISTER_LINK_CONTROL_DEFAULT_HANDLER``: Register a default handler provided by the library

Because the *Link Control* service changes the communication settings of the underlying can interface, it is recommended to use the default handler to correctly restore the interface on a timeout.

Due to restrictions on the can interface (it is not possible to read the current bitrate), the default bitrate must be set with `CONFIG_UDS_DEFAULT_CAN_BITRATE` in your project's prj.conf.

The default handler also registers a handler for the session control events, so don't consume these events if you use the default handler.

.. note::

    The usage of this is gated behind the KConfig option which is disabled by default: ``UDS_USE_LINK_CONTROL``


``UDS_EVT_RequestDownload``, ``UDS_EVT_RequestUpload``, ``UDS_EVT_TransferData``, ``UDS_EVT_RequestTransferExit``, ``UDS_EVT_RequestFileTransfer``
--------------------------------------------------------------------------------------------------------------------------------------------------

These events don't allow registration of custom handlers. The data transfer can be enabled by setting the KConfig option: ``UDS_USE_DYNAMIC_REGISTRATION``.

To further allow file transfer, additionally enable ``CONFIG_UDS_FILE_TRANSFER``


Dynamically register Event Handler
====================================

To dynamically register an event handler at runtime, do the following:

- ensure the ``CONFIG_UDS_USE_DYNAMIC_REGISTRATION`` KConfig option is enabled

- create an ``struct uds_registration_t`` object. Look at the macro implementations above for what data should be set to correctly configure the event handler registration.

- register the event handler by calling the ``instance.register_event_handler`` function. The ``dynamic_id_out`` parameter retuns the ID, the custom registration is registered under.

To unregister a dynamically registered event handler, you can use ``instance.unregister_event_handler`` and pass the dynamic id of the registration as an argument.


Internals
*********

Internally, the UDS library uses an `Iterable Section <https://docs.zephyrproject.org/4.2.0/kernel/iterable_sections/index.html>`_ to hold statically defined event handlers and a single linked list for dynamically defined event handlers.

When an event is received from the underlying :ref:`iso14229-lib`, the UDS library iterates over the iterable section to find an event handler that can handle the event. Then, the check and action function are called as described in **TODO! REFERENCE**.
If the event is consumed, the iteration stops, if not, the iteration continues on.
When the event was not consumed by any event handlers in the iterable seciton, the dynamically defined event handlers are iterated over next.
Finally, when no event handler could be found, a negative default response is send back to the client.