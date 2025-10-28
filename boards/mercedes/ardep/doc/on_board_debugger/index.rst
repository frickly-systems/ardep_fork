.. _on_board_debugger:

On-Board Debugger (OBD)
########################

.. contents::
   :local:
   :depth: 2

.. note::

    The on-board debugger is available on ARDEP board revision 2.0 and later.

Overview
========

The ARDEP mainboard integrates a `Black Magic Probe <https://black-magic.org/index.html>`_, exposing the following features:

- Second USB-C connector labeled ``DEBUG`` for host connectivity

- Two USB CDC ACM interfaces provided to the host system

  - ``uart-a`` bridge for a bidirectional serial console

  - Integrated GDB server for firmware debugging

- Full support for Zephyr ``west debug`` and manual ``arm-none-eabi-gdb`` sessions
  
.. note::

  Under linux, the gdb server usually has the lower numbered device node (e.g. ``/dev/ttyACM0``) while the UART console is assigned to the higher numbered one (e.g. ``/dev/ttyACM1``). Verify this by checking the device logs with ``dmesg`` after connecting the debugger.

Interfaces
==========

UART Console
------------

- Appears on the host as a USB CDC ACM serial device (typically ``/dev/ttyACM*`` under Linux and ``COM*`` under Windows)

- Provides access to the ARDEP ``uart-a`` console for logging and shell interaction

- Compatible with standard serial tools such as ``picocom`` and ``minicom``

GDB Server
----------

- Exposes a native GDB server on the second CDC ACM interface

- `Black Magic GDB command reference <https://black-magic.org/usage/gdb-commands.html>`_ documents the supported commands

- Works with ``west debug`` or direct ``arm-none-eabi-gdb`` connections

Debugging Workflow
==================

Start a debug session with ``west debug`` or by invoking ``arm-none-eabi-gdb`` manually against the GDB interface while the debugger is connected to the host.

During debugging it is possible that the gdb server may spawn new threads or processes lables as ``inferiors``. If that happens, you may require to reload the debug symbols for this inferior using the ``file`` command (e.g. ``file buil/zephyr/zephyr.elf``).

Currently, when startig a debug session, there is a problem with two threads/inferiors are spawned. To work around this problem issue the following commands after starting the debug session with ``west debug``:

  .. code-block:: gdb

     step
     file {path/to/build/zephyr/zephyr.elf}
     break main
     continue
     clear
     

Notice that when you hit a breakpoint, you have to remove it with ``clear`` so the program can continue execution.
Use temporary breakpoints ``tbreak`` to avoid this issue.


Firmware Updates
================

The Black Magic Probe firmware can be refreshed using the dedicated ``bmputil-cli`` utility or an external programmer.

bmputil-cli
-----------

- Install the utility following the `official upgrade guide <https://black-magic.org/upgrade.html>`_.

- With the debugger connected, inspect the current firmware revision:

  .. code-block:: sh

      bmputil-cli probe info

- Upgrade to the latest release:

  .. code-block:: sh

      bmputil-cli probe update

  Select ``Black Magic Debug for BMP (ST and ST-clones targets)`` when prompted for the firmware variant and ``Flash to probe`` for the action.

- Repeat ``bmputil-cli probe info`` to confirm that the new firmware version is active.

- To flash a specific release or custom build, pass the firmware binary path to ``bmputil-cli probe update``.

Manual Update via External Debug Probe
--------------------------------------

.. note::
  
  This method also supports flashing the bootloader.

A Tag-Connect footprint labeled ``DEBUGGER`` is located on the back of the board for in-circuit programming with an external debugger.
Use any SWD-compatible debugger to program the binaries at the following offsets:

  - Firmware image at ``0x08002000``

  - Bootloader image at ``0x08000000``


Obtaining Firmware Images
=========================

Pre-built Binaries
------------------

Download the latest release from the `Black Magic Probe GitHub repository <https://github.com/blackmagic-debug/blackmagic/releases>`_.

This board requires the release named ``blackmagic-native-st-clones``.

Build from Source
-----------------

Follow the upstream `getting started guide <https://github.com/blackmagic-debug/blackmagic/blob/main/README.md#getting-started>`_ to build the probe firmware with the following changes:

  - Configure the build with the ``stlink.ini`` cross-file
  - Add ``-Dbmd_bootloader=true`` to the build setup step to enable building of the bootloader

  .. code-block:: sh

      git clone https://github.com/blackmagic-debug/blackmagic.git
      cd blackmagic
      meson setup build --cross-file=cross-file/stlink.ini -Dbmd_bootloader=true
      meson compile -C build
      meson compile -C build boot-bin
