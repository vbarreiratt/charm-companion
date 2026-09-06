# Camera Quirks — Not Code Bugs

Observed artifacts when photographing the ESP32-S3 board with the EMEET
SmartCam C960 4K via OpenCV/MCP videocapture, that are camera behavior,
not firmware behavior. Check this list before concluding a captured
frame shows a rendering bug.

## Mirrored image
The frame may appear horizontally flipped relative to how you're looking
at the board. This is the camera/capture pipeline's default orientation,
not a `TFT_ROTATION`/display bug. If left/right positions look swapped,
check this before touching display rotation config.

## Blue reflection point
A small blue point of light can appear reflected in the AMOLED glass.
This is the camera module's own indicator LED reflecting back, not a
pixel the firmware drew. It moves with the camera's position/angle, not
with anything the app is rendering.

## Too dark / too bright / blown-out colors
Auto-exposure varies a lot with ambient light. A color that looks
different from `render_reference`'s output can be an exposure artifact
rather than a wrong color constant — before suspecting `color_utils.h`,
try comparing hue/shape rather than absolute brightness, or take another
frame under more consistent lighting.

## Adding to this list
If a session hits a new camera-only artifact (something that reproduces
regardless of firmware state), add it here with what it looks like and
how to tell it apart from a real bug, so the next session doesn't
re-diagnose it from scratch.
