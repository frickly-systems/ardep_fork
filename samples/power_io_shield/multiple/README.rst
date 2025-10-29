.. _power_io_shield_multiple_sample:


HV Shield
#########

This firmware demonstrates how to use multiple Power IO Shields with the Ardep platform.
It configures 2 Power IO Shields, one set to addres 0 and the other to address 1.

Flash and run
=============

Build the firmware with:

.. code-block:: bash

  west build -b ardep samples/power_io_shield/multiple/


Flash it using dfu-util:

.. code-block:: bash

  west flash


Expected behavior
=================

todo