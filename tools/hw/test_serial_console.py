import pytest
import serial

from serial_console import SerialConsole, resolve_port


def test_send_writes_newline_terminated_command_and_returns_echo():
    ser = serial.serial_for_url("loop://", timeout=0.05)
    console = SerialConsole(ser)

    lines = console.send("touch 100 200", settle=0.05)

    assert lines == ["touch 100 200"]


def test_tail_collects_multiple_pending_lines():
    ser = serial.serial_for_url("loop://", timeout=0.05)
    console = SerialConsole(ser)
    ser.write(b"line one\nline two\n")
    ser.flush()

    lines = console.tail(0.05)

    assert lines == ["line one", "line two"]


def test_context_manager_closes_underlying_serial():
    ser = serial.serial_for_url("loop://", timeout=0.05)
    with SerialConsole(ser) as console:
        console.send("ping", settle=0.02)

    assert not ser.is_open


def test_resolve_port_uses_exact_port_when_it_exists():
    resolved = resolve_port(
        "/dev/cu.usbmodem21201",
        exists_fn=lambda p: p == "/dev/cu.usbmodem21201",
        glob_fn=lambda pattern: (_ for _ in ()).throw(AssertionError("should not glob")),
    )

    assert resolved == "/dev/cu.usbmodem21201"


def test_resolve_port_falls_back_to_single_glob_match():
    resolved = resolve_port(
        "/dev/cu.usbmodem21201",
        exists_fn=lambda p: False,
        glob_fn=lambda pattern: ["/dev/cu.usbmodem14301"],
    )

    assert resolved == "/dev/cu.usbmodem14301"


def test_resolve_port_picks_first_sorted_match_when_multiple_found():
    resolved = resolve_port(
        "/dev/cu.usbmodem21201",
        exists_fn=lambda p: False,
        glob_fn=lambda pattern: ["/dev/cu.usbmodem99999", "/dev/cu.usbmodem14301"],
    )

    assert resolved == "/dev/cu.usbmodem14301"


def test_resolve_port_raises_when_no_device_found():
    with pytest.raises(RuntimeError, match="no device at"):
        resolve_port(
            "/dev/cu.usbmodem21201",
            exists_fn=lambda p: False,
            glob_fn=lambda pattern: [],
        )
