import serial

from serial_console import SerialConsole


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
