.. _debugging:

Debugging
#########

.. contents::
   :local:
   :depth: 2

.. tabs:: 

   .. tab:: Ardep v1.0.0
   
      The following section describes how to use an on chip debugger with ardep.

      Connect a debug probe
      =====================

      Connect a debug probe to the SWD pins of the ARDEP board.

      .. image:: swd_pinout.png
         :width: 600
         :alt: measurement

      Pin one should only be connected to a voltage measuring pin (if required) not to power the board.

      For example for an ST-Link V2 you can connect the following pins:

      * SWDIO to pin 2
      * SWCLK to pin 4
      * GND to pin 5
      * RST to pin 3

      Flash using openocd
      -------------------

      To flash the board using openocd you can use the following command:

      .. code-block:: bash

         west flash --runner openocd

      Debug using openocd
      -------------------

      To debug the board using openocd you can use the following command:

      .. code-block:: bash

         west debug --runner openocd

      Flash and debug using JLINK
      ---------------------------

      Use the sample commands as above, but replace `openocd` with `jlink`.

   .. tab:: Ardep v2.0.0 and later
   
      Ardep v2.0.0 and later come with an on-board debugger (OBD) based on the Black Magic Probe (BMP) project.

      This allows for easy debugging and flashing without the need for an external debug probe.
      
      After you connected the debugger to the host via USB-C, start the debugger session with ``arm-none-eabi-gdb /path/to/build/zephyr/zephyr.elf`` and run the following instructions to correctly configure the debugger:
      
      
      .. code-block:: sh

         target extended-remote /dev/ttyACM0

         monitor auto_scan

         attach 1

         load

         break main

         continue
         

      Replace ``/dev/ttyACM0`` with the correct device node for your system.

      A gdbinit script file is included in the root of the repository. You can use it to automate the startup process.

      .. note::

         We recommend using the official arm gdb build or a package from your distribution as the gdb version provided by the zephyr-sdk does not work with the BMP.