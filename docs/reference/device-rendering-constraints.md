# Scribr — Device Rendering Constraints

Hardware constraints for the Scribr UI, derived from actually rendering the
screens on the Waveshare ESP32-S3 1.54" e-Paper panel (not theory). Intended to be passed back to
Claude Design so mockups stay faithful to what the device can render.

**Panel:** 200×200 px, 1-bit. Pure black or pure white only; "gray" is
impossible (1-bit dither at best).

## Fonts — the big one

The device renders **Adafruit GFX fonts**, which behave very differently from
web fonts:

1. **Only a fixed set of sizes exists** — FreeSans & FreeSansBold at **9pt,
   12pt, 18pt, 24pt**, plus a built-in **5×7 pixel** font. There is **no
   arbitrary px sizing** (no 10px, 11px, 13px). Design the type scale from those
   discrete sizes, not a CSS px slider.
2. **There's a hard gap between 5×7 and 9pt.** The 5×7 font (~7px tall) is the
   only thing smaller than FreeSans9pt (~13px tall). Nothing fills the "11px
   caption" slot — a caption is either tiny (5×7) or full-size (9pt). We hit
   this exactly with the footer note.
3. **ASCII only.** No `×`, `—`, `•`, curly quotes, accents. Use `x`, `-`, `*`,
   straight quotes.
4. **No letter-spacing / tracking.** GFX has fixed per-glyph advance;
   `letter-spacing: .14em` on the stat labels can't be reproduced. Don't rely on
   tracking for the look.
5. **FreeSans is proportional and *wide*** — much wider than the IBM Plex Mono /
   Archivo used in the mockup. Text that fits in the prototype will overflow on
   device.

## Real line-width budget

Content width = **192px** (see padding below). Average characters that fit on
one line per font:

| Font              | UPPERCASE  | lowercase/mixed |
| ----------------- | ---------- | --------------- |
| FreeSans 9pt      | ~15 chars  | ~21 chars       |
| FreeSansBold 9pt  | ~15 chars  | ~20 chars       |
| FreeSans 12pt     | ~12 chars  | ~17 chars       |
| FreeSansBold 12pt | ~11 chars  | ~15 chars       |
| Built-in 5×7      | 32 chars (fixed) | 32 chars  |

**Concrete example that bit us:** "DOUBLE-PRESS TO WAKE" = 226px in FreeSans9pt
uppercase → overflows. Mixed-case "Double-press to wake" = 174px → fits.
**Implication:** prefer mixed case for longer strings, or keep all-caps labels
short (≤~15 chars), or wrap to two lines.

## Layout

- **Padding** settled at **3px top/bottom, 4px sides** → usable content area is
  **192×194px**. Keep all rules/dividers/text within that.
- Rules and hairline dividers should span the content width (x 4→196), not
  full-bleed to the panel edge.

## Refresh behavior

- e-paper **full refresh flashes black→white (~1–2s)**. Use it only on screen
  changes.
- Frequently-updating elements (REC/PLAY timers, progress bar) must use
  **partial refresh** — design them as small, isolated regions that can update
  without a full-screen flash. Keep live-updating fields in their own zone.

## Type-scale mapping in use (working reference)

| Role                  | Font              |
| --------------------- | ----------------- |
| Title wordmark        | FreeSansBold 12pt |
| Section / stat labels | FreeSansBold 9pt  |
| Values / body / hints | FreeSans 9pt      |
| Tiny captions         | 5×7 (reads small) |
