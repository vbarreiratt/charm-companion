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
