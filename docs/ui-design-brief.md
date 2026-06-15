# Device UI Design Brief — Voice-Note Recorder

You are designing the UI for a small, battery-powered handheld hardware device.
Design within its capabilities and constraints exactly — this is an embedded
e-paper device, not a phone, web, or desktop app.

## Display
- **Resolution:** 200 × 200 pixels, square (~1.5" panel).
- **Color:** 1-bit **monochrome only** — pure BLACK (#000000) and WHITE (#FFFFFF).
  No grayscale, color, gradients, drop shadows, or glows. Every pixel is on or off.
  Simulate "shading" only with 1-bit dither/halftone patterns.
- **Technology:** e-paper / e-ink. This means:
  - The image **persists with zero power** and looks crisp when static.
  - Refreshes are **slow and visibly flashy** — design **static frames**, not motion.
    No animations, transitions, scrolling motion, or spinners.
  - Treat it like a **printed page**: high contrast, bold shapes, generous whitespace.

## Input
- **Two physical buttons only.** No touchscreen, scroll wheel, or keyboard.
- Each button can detect **single-press, long-press, and double-press**, plus **hold**.
- Conventionally one button **cycles/advances a selection** and the other
  **confirms/activates** it — so the UI must be navigable as single-column,
  one-item-at-a-time selection with no pointer.
- Because there is no pointer, every screen should make clear **what is selected**
  and **what each button does right now** (on-screen button hints).

## Audio
- **Microphone input** — can record voice/audio.
- **Speaker output** (codec + amplifier) — can play audio back and emit UI sounds/tones.

## Storage
- **SD card** — large local storage for many audio files and associated text/metadata.

## Connectivity
- **WiFi** (and Bluetooth LE capable) — can connect to the internet to upload audio,
  receive transcribed text, sync/export data, and host a simple local web interface
  reachable from a phone or computer.

## Timekeeping, Power & Feedback
- **Real-time clock** — knows the current date/time; can timestamp items.
- **Battery powered** with battery-level sensing — battery life is a core goal, so the
  device sleeps aggressively when idle; show charge state and low-battery warnings.
- **Audio feedback** — can play short confirmation/navigation sounds.

## Compute
- Dual-core 240 MHz MCU with 8 MB RAM and 8 MB storage — capable of on-device audio
  buffering, file management, and running a lightweight web server, but **not** heavy
  graphics; keep rendering simple and 1-bit.

## What to Design For
- A cohesive 1-bit UI system: type scale, a consistent header/footer pattern, a clear
  selected/active treatment (e.g. inverted — white text on a solid black block), and a
  simple icon set that reads at tiny sizes.
- Static, high-contrast screens that swap cleanly (no effects needing grayscale or motion).
- Every interaction expressible with two buttons (cycle + confirm, plus long/double/hold).
