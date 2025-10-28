.. _on_board_debugger:

On-Board Debugger (OBD)
######################

.. contents::
   :local:
   :depth: 2

.. note::

    This only applies to ARDEP board revision 2.0 and later.

- black-magic probe debugger
- website: https://black-magic.org/index.html
- The debug probe is connected to the host via the second USB-C connector (labeled "DEBUG").
- The debugger provides two USB-CDC-ACM interfaces to the host
    - The Probe is connected to uart-a of the board and provides a two way CDC-ACM for the UART
    - The probe provides a GDB server that can be connected to via a second USB CDC-ACM interface. Use `west debug` or connect manually. See [here](https://black-magic.org/usage/gdb-commands.html) for more information.

How to debug
=============

.. todo


Update the Debugger
===================

To update the debugger firmware, you have two choices:

bmputil-cli
----------------


- The Black Magic Probe has its own cli tool to update the firmware called `bmputil-cli`.
- It automatically flashes the latest firmware release from the github releases page.

- See https://black-magic.org/upgrade.html for more information on how to install it.

- Once installed, you can do the following:
  - With the Debugger connected to the host, see the current firmware version of the debugger:
    ```sh
    > bmputil-cli probe info
    Found: Black Magic Probe 2.0.0-rc1
    Serial: 7DC09797
    Port:  1-10
    ```
  - Then you can update the firmware to the latest version with:
    ```sh
    > bmputil-cli probe update
      Updating release metadata cache
      [2025-10-28T10:14:04Z INFO  bmputil::metadata] Validating v1 metadata with 18 releases present
      [2025-10-28T10:14:04Z INFO  bmputil_cli] Upgrading probe firmware from 2.0.0-rc1 to 2.0.0
    ✔ Which firmware variant would you like to run on your probe? · Black Magic Debug for BMP (ST and ST-clones targets)
    ✔ What action would you like to take with this firmware? · Flash to probe
      Downloading requested firmware
      Found: Black Magic Probe 2.0.0-rc1
        Serial: 7DC09797
        Port:  1-10
    Erasing flash...
    Flashing...
     100% |██████████████████████████████████████████████████| 91.93 KiB/91.93 KiB [10.87 KiB/s 8s]
    [2025-10-28T10:14:29Z INFO  bmputil::flasher] Flash complete!
    ```
  - During the process make sure to select  `Black Magic Debug for BMP (ST and ST-clones targets)` when prompted for the firmware variant and `Flash to probe` when prompted for the action.

  - You can then check the updated version with the info command above
    
  - Optionally, when you want to update to a specific version or custom build, you can pass the path to the bin file to the `bmputil-cli probe update` command. For building from source, see below.


Manually via external debugger
---------------------------------

- This also allows for flashing the bootloader, if needed.

- On the backside of the ARDEP board is a Tag-Connect plug of nails labeld `DEBUGGER` that allows for flashing the debugger firmware manually.
- Connect your debugger and flash the firmware binary.
  - Offset `0x8002000` for the firmware binary
  - Offset `0x8000000` for the bootloader binary


Optaining the debugger firmware
===============================


Pre-build binaries
------------------

- Download the latest "blackmagic-native-st-clones" firmware from https://github.com/blackmagic-debug/blackmagic/releases

Build from source
------------------

- Follow the steps of the [Getting started guide](https://github.com/blackmagic-debug/blackmagic/blob/main/README.md#getting-started) to build the firmware from source.
- Make sure you choose the `stlink.ini` cross-file when configuring
- When you want to build the bootloader as well, pass `-Dbmd_bootloader=true` when issuing the `meson setup build` command.
- Example of a full build:
  ```sh
  git clone https://github.com/blackmagic-debug/blackmagic.git
  cd blackmagic
  meson setup build --cross-file=cross-file/stlink.ini -Dbmd_bootloader=true
  meson compile -C build
  meson compile -C build boot-bin
  ```