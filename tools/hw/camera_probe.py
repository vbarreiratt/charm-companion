"""Synchronized touch-injection + camera burst capture.

Solves the timing problem from the 2026-09-06 session: the MCP camera
tool's network round-trip (>150ms) was too slow to catch EyeScene's
150ms blink. This opens the camera directly via OpenCV (confirmed
locally to work, bypassing the MCP tool entirely) and holds the serial
connection open at the same time, so the touch command and the first
capture happen back-to-back in one process, not across separate tool
calls.

Before treating a captured frame as evidence of a code bug, check
docs/hardware/camera-quirks.md for known non-code artifacts (mirroring,
the camera's own reflected indicator LED, exposure extremes).
"""
import os
import time

import cv2


def capture_schedule(duration_s, interval_s):
    """Relative timestamps (seconds from t=0) at which to grab a frame."""
    schedule = []
    t = 0.0
    while t < duration_s:
        schedule.append(round(t, 4))
        t += interval_s
    return schedule


def frame_filename(out_dir, index, t):
    return os.path.join(out_dir, f"frame_{index:03d}_t{t:.3f}.png")


def open_camera(device_index=0):
    cam = cv2.VideoCapture(device_index)
    if not cam.isOpened():
        raise RuntimeError(f"could not open camera at index {device_index}")
    return cam


def run_touch_capture(console, cam, command, out_dir, duration_s=1.5, interval_s=0.03):
    """Send `command` over `console` and burst-capture frames from `cam`
    for `duration_s`, roughly every `interval_s`. Returns the saved frame
    paths, in order."""
    os.makedirs(out_dir, exist_ok=True)
    schedule = capture_schedule(duration_s, interval_s)

    start = time.monotonic()
    console._ser.write((command + "\n").encode())
    console._ser.flush()

    paths = []
    for i, target_t in enumerate(schedule):
        while (time.monotonic() - start) < target_t:
            pass
        ok, frame = cam.read()
        if not ok:
            continue
        path = frame_filename(out_dir, i, time.monotonic() - start)
        cv2.imwrite(path, frame)
        paths.append(path)
    return paths
