"""Persistent serial connection to the ESP32-S3's debug command console
(see src/main.cpp's handle_serial_command: "touch <x> <y>", "app <name>",
"next", "prev").

One connection is opened and reused for a whole session instead of being
reopened per command. This board is configured with
ARDUINO_USB_CDC_ON_BOOT=1; reopening the serial port risks toggling
DTR/RTS and resetting the board mid-session, which happened during manual
testing on 2026-09-06. connect() below opens with dsrdtr=False to avoid
that, matching what was confirmed to work that session.
"""
import time


class SerialConsole:
    def __init__(self, serial_obj, read_timeout=0.05):
        self._ser = serial_obj
        self._ser.timeout = read_timeout

    def send(self, command, settle=0.3):
        self._ser.write((command + "\n").encode())
        self._ser.flush()
        return self.tail(settle)

    def tail(self, seconds):
        end = time.time() + seconds
        lines = []
        while time.time() < end:
            line = self._ser.readline()
            if line:
                lines.append(line.decode(errors="replace").rstrip())
        return lines

    def close(self):
        self._ser.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()


def connect(port="/dev/cu.usbmodem21201", baud=115200):
    import serial

    ser = serial.Serial(port, baud, timeout=0.05, dsrdtr=False)
    return SerialConsole(ser)
