from argparse import ArgumentParser, Namespace
from os import path
import os
import subprocess
import tempfile
from west import log


class CreateUdevRule:
    command: str = "create-udev-rule"
    _command_name: str = None
    _udev_file_name: str = None
    _board_name: str = None
    _vid: str = None
    _pid: str = None
    _pid_dfu: str = None

    def __init__(
        self, command_name: str, board_name: str, vid: str, pid: str, pid_dfu: str
    ):
        self._command_name = command_name
        self._board_name = board_name
        self._udev_file_name: str = f"99-{self._command_name}.rules"
        self._vid = vid
        self._pid = pid
        self._pid_dfu = pid_dfu

    def add_args(self, parser: ArgumentParser):
        subcommand_parser: ArgumentParser = parser.add_parser(
            self.command,
            help=f"creates a udev rule to allow dfu-util access to {self._command_name} devices (may require `sudo`)",
        )

        subcommand_parser.add_argument(
            "-d",
            "--directory",
            help="destination directory for the udev rule (default: /etc/udev/rules.d)",
            default="/etc/udev/rules.d",
            metavar="DIRECTORY",
        )

        subcommand_parser.add_argument(
            "-f",
            "--force",
            help="forces an overwrite, if the file rule file already exists",
            action="store_true",
        )

    def run(self, args: Namespace):
        destination_directory: str = path.realpath(args.directory)
        rule_path: str = f"{destination_directory}/{self._udev_file_name}"

        if not path.exists(destination_directory):
            log.die(
                f"Destination directory does not exist: {destination_directory}",
                exit_code=1,
            )

        if (not args.force) and path.exists(rule_path):
            log.die(
                f"File already exists at: {rule_path}\nuse --force to overwrite",
                exit_code=2,
            )

        (fd, tmpfile) = tempfile.mkstemp()
        with open(fd, "w", encoding="utf-8") as f:
            f.write(
                f"""\
# {self._board_name}: Allow dfu-util/libusb access to ZEPHYR {self._board_name.upper()} devices
#
# Group selection:
# - First rule sets ASSIGN_GROUP to a suitable system group present on the host:
#     * "uucp" (common on Arch-like distros), otherwise
#     * "plugdev" (common on Ubuntu/Debian)
# - Following rules use GROUP="%E{{ASSIGN_GROUP}}" plus TAG+="uaccess".
#
# Action/match keys used below:
#   ACTION=="add"               → run when the device is added (hotplug event)
#   SUBSYSTEM=="usb"            → act only on devices in the USB subsystem
#   ENV{{DEVTYPE}}=="usb_device"  → target the USB *device* node (/dev/bus/usb/BUS/DEV), not interfaces
#   ATTR{{idVendor}}/idProduct    → match the specific board VID/PIDs
#   MODE:="0660"                → set permissions (rw for owner+group)
#   GROUP:="%E{{ASSIGN_GROUP}}"   → set the node's group to the selected group from the first rule
#   TAG+="uaccess"              → grant an ACL to the active logged-in user via systemd-logind

ACTION=="add", SUBSYSTEM=="usb", ENV{{DEVTYPE}}=="usb_device", \\
  IMPORT{{program}}="/bin/sh -c 'if getent group plugdev >/dev/null && [ $(getent group plugdev | cut -d: -f3) -lt 1000 ]; then echo ASSIGN_GROUP=plugdev; elif getent group uucp >/dev/null && [ $(getent group uucp | cut -d: -f3) -lt 1000 ]; then echo ASSIGN_GROUP=uucp; else echo ASSIGN_GROUP=plugdev; fi'"

ACTION=="add", SUBSYSTEM=="usb", ENV{{DEVTYPE}}=="usb_device", ATTR{{idVendor}}=="{self._vid}", ATTR{{idProduct}}=="{self._pid}", \\
  MODE:="0660", GROUP:="%E{{ASSIGN_GROUP}}", TAG+="uaccess"

ACTION=="add", SUBSYSTEM=="usb", ENV{{DEVTYPE}}=="usb_device", ATTR{{idVendor}}=="{self._vid}", ATTR{{idProduct}}=="{self._pid_dfu}", \\
  MODE:="0660", GROUP:="%E{{ASSIGN_GROUP}}", TAG+="uaccess"
"""
            )

        cmd = ["mv", tmpfile, rule_path]
        if not os.access(destination_directory, os.W_OK):
            cmd = ["sudo"] + cmd

        rc = subprocess.call(cmd)

        if rc != 0:
            log.die(f"Failed to create udev rule at {rule_path}", exit_code=3)

        log.inf("New rule was successfully created")

        if destination_directory.startswith("/etc/udev/rules.d"):
            log.inf(
                f"To activate the new rule, unplug the {self._board_name} and run:\n    sudo udevadm control --reload-rules && sudo udevadm trigger"
            )
