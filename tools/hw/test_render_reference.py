import os
import subprocess
import time

import render_reference as rr


def test_eye_pupil_is_black_when_not_blinking(tmp_path):
    out = tmp_path / "eye_open.png"

    rr.render("eye", str(out))

    img = rr.Image.open(out)
    # Matches tests/test_eye_scene.cpp EyeSceneTest.IrisVisibleWhenNotBlinking:
    # the pupil at dead center (233, 233) is COLOR_BG_BLACK (0x0000).
    assert img.getpixel((233, 233)) == (0, 0, 0)


def test_eye_blink_closes_eyelid_after_touch_and_150ms(tmp_path):
    out = tmp_path / "eye_blink.png"

    rr.render("eye", str(out), touch=True, update_ms=160)

    img = rr.Image.open(out)
    # Matches tests/test_eye_scene.cpp EyeSceneTest.BlinkFullyClosesEyelid:
    # past blink_duration (150ms), (263, 233) is inside the eyelid bar
    # (COLOR_BG_BLACK), not iris color.
    assert img.getpixel((263, 233)) == (0, 0, 0)


def test_home_sclera_matches_mono_neutral_rgb565_conversion(tmp_path):
    out = tmp_path / "home.png"

    rr.render("home", str(out))

    img = rr.Image.open(out)
    # (170, 155) is within HomeUI's left-eye sclera (center 170,190 radius 40)
    # but outside the iris (radius 18), so it's pure COLOR_MONO_NEUTRAL
    # (0xE73C). Converting RGB565->RGB888 by hand: R5=28->230, G6=57->231,
    # B5=28->230. A wrong bit-width split (e.g. swapping the 5/6-bit
    # channels) would produce a different triple here.
    assert img.getpixel((170, 155)) == (230, 231, 230)


def test_header_only_change_invalidates_build_cache(monkeypatch):
    # Regression for Finding 1: a header-only edit (e.g. eye_scene.h's
    # `blink_duration = 150` inline member initializer) carries real
    # behavior in this codebase but was previously invisible to the
    # mtime-based cache check, which only globbed *.cpp files.
    header = os.path.join(rr.REPO_ROOT, "src", "apps", "scenes", "eye_scene.h")
    original_mtime = os.path.getmtime(header)
    try:
        rr.build(force=True)
        mtime_before_rebuild = os.path.getmtime(rr.BIN_PATH)

        # Push the header's mtime past the just-built binary's mtime,
        # simulating a header-only edit made after the last build.
        future = time.time() + 100
        os.utime(header, (future, future))

        calls = []
        real_run = subprocess.run

        def spy_run(cmd, **kwargs):
            calls.append(cmd)
            return real_run(cmd, **kwargs)

        monkeypatch.setattr(rr.subprocess, "run", spy_run)

        rr.build(force=False)

        assert calls, (
            "expected build() to invoke g++ again after a header-only "
            "change made the cached binary stale"
        )
        assert os.path.getmtime(rr.BIN_PATH) > mtime_before_rebuild
    finally:
        # Restore the header's real mtime; touching mtime doesn't affect
        # git's tracked content, but keep the repo tidy regardless.
        os.utime(header, (original_mtime, original_mtime))


def test_touch_coordinates_are_forwarded_to_subprocess(monkeypatch, tmp_path):
    out = tmp_path / "eye_touch.png"
    captured = {}
    real_run = subprocess.run

    def spy_run(cmd, **kwargs):
        if cmd[0] == rr.BIN_PATH:
            captured["cmd"] = cmd
        return real_run(cmd, **kwargs)

    monkeypatch.setattr(rr.subprocess, "run", spy_run)

    rr.render("eye", str(out), touch=True, touch_x=120, touch_y=340)

    cmd = captured["cmd"]
    assert cmd[cmd.index("--touch-x") + 1] == "120"
    assert cmd[cmd.index("--touch-y") + 1] == "340"


def test_touch_without_coordinates_still_matches_prior_blink_behavior(tmp_path):
    # Regression for Finding 3: adding touch_x/touch_y params must not
    # change behavior for existing callers that omit them.
    out = tmp_path / "eye_blink_regression.png"

    rr.render("eye", str(out), touch=True, update_ms=160)

    img = rr.Image.open(out)
    assert img.getpixel((263, 233)) == (0, 0, 0)
