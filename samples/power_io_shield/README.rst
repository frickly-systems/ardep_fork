.. _power_io_shield_sample:


HV Shield
#########

TODO: EDIT
This firmware demonstrates the HV Shield v2 driver using one HV (High voltage) GPIO and HV DAC.

Flash and run
=============

Build the firmware with:

.. code-block:: bash

  west build -b ardep samples/power_io_shield


Flash it using dfu-util:

.. code-block:: bash

  west flash


Expected behavior
=================

TODO
The HV Shield puts out a slow sawtooth on AO36 and toggles D1
