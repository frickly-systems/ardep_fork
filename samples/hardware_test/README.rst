.. _hardware_test_sample:

Hardware Test Sample
####################

A sample used to ensure that the hardware is working correctly.

This requires 2 ARDEP boards to be connected on each pin of the board.

One board must run the SUT (System Under Test) firmware and the other the Tester firmware.

To build each firmware, see below.

Each firmware provides 2 CDC-ACM devices.
One is used for logging to be visible by the user and the other is used to drive the tests with the host script.

Build the Example
=========================

Build the application in the different modes with:

.. code-block:: bash

    # From the root of the ardep repository

    # Build SUT mode
    west build --board ardep samples/hardware_test -- -DEXTRA_CONF_FILE=sut.conf

    # Build Tester mode
    west build --board ardep samples/hardware_test -- -DEXTRA_CONF_FILE=tester.conf


Execute tests
=============


To texecute the tests, run the host script and pass the cd

.. code-block:: bash

    TODO
    

.. warning::

    TODO: THIS README!