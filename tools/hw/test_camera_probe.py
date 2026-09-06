from camera_probe import capture_schedule, frame_filename


def test_capture_schedule_spans_duration_at_fixed_interval():
    schedule = capture_schedule(0.1, 0.03)

    assert schedule == [0.0, 0.03, 0.06, 0.09]


def test_frame_filename_encodes_index_and_timestamp():
    path = frame_filename("/tmp/out", 3, 0.075)

    assert path == "/tmp/out/frame_003_t0.075.png"
