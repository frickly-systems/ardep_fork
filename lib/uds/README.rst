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

The UDS library uses several Macros to allow you to statically define event handlers for specific groups of events.

Usually, every event is associated with a ``check`` and an ``action`` function.
The ``check``function is to check, whether the event with its arguments can be handled in the current state of the ECU.
If it is the case, the associated ``action`` function is executed.



      
TODO:
***** 

 - Actor Model

Internals
*********

Internally, the UDS library uses an `Iterable Section <https://docs.zephyrproject.org/4.2.0/kernel/iterable_sections/index.html>`_ to hold statically defined event handlers and a single linked list for dynamically defined event handlers.

When an event is received from the underlying :ref:`iso14229-lib`, the UDS library iterates over the iterable section to find an event handler that can handle the event. Then, the check and action function are called as described in **TODO! REFERENCE**.
If the event is consumed, the iteration stops, if not, the iteration continues on.
When the event was not consumed by any event handlers in the iterable seciton, the dynamically defined event handlers are iterated over next.
Finally, when no event handler could be found, a negative default response is send back to the client.