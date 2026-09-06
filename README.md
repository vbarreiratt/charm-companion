# Charm Companion

ESP32-S3 WiFi companion app for Waveshare AMOLED 1.75C display.

**Hardware:** Waveshare ESP32-S3-Touch-AMOLED-1.75C (CO5300 driver, 466×466px circular AMOLED, capacitive touch)

**Status:** Fresh start, hardware knowledge consolidated in HARDWARE.md

## Quick Start

See [HARDWARE.md](./HARDWARE.md) for driver setup, known issues, and build configuration.

## Development Workflow

For iterating on this hardware (build → flash → simulate touch → verify against a real or WASM-rendered digital twin), see the `hardware-iterate` Claude Code skill: [.claude/skills/hardware-iterate/SKILL.md](./.claude/skills/hardware-iterate/SKILL.md). Design rationale: [docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md](./docs/superpowers/specs/2026-09-06-hardware-iteration-harness-design.md).
