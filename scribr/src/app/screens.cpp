#include "screens.h"

#include <Adafruit_GFX.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Fonts are included in exactly ONE translation unit (Adafruit font headers
// define non-static globals). Discrete GFX sizes map to the design's px scale:
//   FreeSans 9pt  ~= 13px      FreeSansBold 12pt ~= 17px
//   FreeSansBold 24pt ~= 33px (the big timer)
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#include "display.h"

using app::Model;
using app::Screen;

namespace {

constexpr int L = 4;                    // left inset  (design padding 4px)
constexpr int R = display::WIDTH - 4;   // right inset (196)
constexpr int W = display::WIDTH;
constexpr int INK = 1;                  // canvas: 1 = ink
constexpr int BG  = 0;                  // canvas: 0 = paper
constexpr int HINT_GAP = 5;             // space between hint action and modifier
                                        // (a real space has no ink, so getTextBounds
                                        //  would measure it as 0px — use a fixed gap)

const GFXfont* F_REG   = &FreeSans9pt7b;       // body / values / hints
const GFXfont* F_BOLD  = &FreeSansBold9pt7b;   // labels / keycaps
const GFXfont* F_TITLE = &FreeSansBold12pt7b;  // titles / dates
const GFXfont* F_BIG   = &FreeSansBold24pt7b;  // big timer

// ── small text utilities ────────────────────────────────────────────
int width(GFXcanvas1& c, const char* s) {
  int16_t bx, by; uint16_t bw, bh;
  c.getTextBounds(s, 0, 0, &bx, &by, &bw, &bh);
  return bw;
}
void textLeft(GFXcanvas1& c, int x, int base, const char* s, const GFXfont* f, int color = INK) {
  c.setFont(f); c.setTextColor(color); c.setCursor(x, base); c.print(s);
}
void textRight(GFXcanvas1& c, int xr, int base, const char* s, const GFXfont* f, int color = INK) {
  c.setFont(f); c.setTextColor(color); c.setCursor(xr - width(c, s), base); c.print(s);
}
void textCenter(GFXcanvas1& c, int base, const char* s, const GFXfont* f, int color = INK) {
  c.setFont(f); c.setTextColor(color); c.setCursor((W - width(c, s)) / 2, base); c.print(s);
}

// ── time / format helpers ───────────────────────────────────────────
void fmtHMS(char* out, int s) {
  snprintf(out, 12, "%02d:%02d:%02d", s / 3600, (s % 3600) / 60, s % 60);
}
void fmtShort(char* out, int s) {                 // M:SS, or H:MM:SS past an hour
  if (s >= 3600) snprintf(out, 12, "%d:%02d:%02d", s / 3600, (s % 3600) / 60, s % 60);
  else           snprintf(out, 12, "%d:%02d", s / 60, s % 60);
}
void rowDate(char* out, const char* date) {       // "YYYY-MM-DD HH:MM" -> "MM-DD HH:MM"
  strncpy(out, date + 5, 11); out[11] = 0;
}

// ── shared chrome ───────────────────────────────────────────────────
// Title + 2px rule. recdot draws the recording dot; counter is the right meta.
void header(GFXcanvas1& c, const char* title, bool wordmark, const char* counter, bool recdot) {
  const int base = 21;
  int x = L;
  if (recdot) { c.fillCircle(L + 5, base - 5, 5, INK); x = L + 16; }
  textLeft(c, x, base, title, F_TITLE);
  if (counter) textRight(c, R, base, counter, F_REG);
  c.drawFastHLine(L, 28, R - L, INK);
  c.drawFastHLine(L, 29, R - L, INK);
  if (recdot) { c.drawFastHLine(L, 33, R - L, INK); c.drawFastHLine(L, 34, R - L, INK); }
}

struct Hint { const char* action; const char* mod; };  // action bold UPPERCASE, mod regular; {"-",0} = dash

void upperCopy(char* dst, const char* src, size_t n) {
  size_t i = 0;
  for (; src[i] && i < n - 1; i++) dst[i] = (char)toupper((unsigned char)src[i]);
  dst[i] = 0;
}

int hintWidth(GFXcanvas1& c, const Hint& h) {
  char up[16]; upperCopy(up, h.action, sizeof up);
  c.setFont(F_BOLD);
  int w = width(c, up);
  if (h.mod) { c.setFont(F_REG); w += HINT_GAP + width(c, h.mod); }
  return w;
}
void drawHint(GFXcanvas1& c, int x, int base, const Hint& h) {
  char up[16]; upperCopy(up, h.action, sizeof up);   // .hint b is uppercase
  textLeft(c, x, base, up, F_BOLD);
  if (h.mod) {
    c.setFont(F_BOLD); int wa = width(c, up);
    textLeft(c, x + wa + HINT_GAP, base, h.mod, F_REG);
  }
}
void footer(GFXcanvas1& c, const Hint& boot, const Hint& pwr) {
  for (int x = L; x < R; x += 3) c.drawPixel(x, 174, INK);  // dotted rule
  const int base = 190;
  drawHint(c, L, base, boot);
  drawHint(c, R - hintWidth(c, pwr), base, pwr);
}

// ── screens ─────────────────────────────────────────────────────────
void idle(GFXcanvas1& c, const Model& m) {
  header(c, "scribr", true, nullptr, false);

  char nrec[8]; snprintf(nrec, sizeof nrec, "%d", m.recCount);
  char bat[16];
  if (m.battery >= 0) snprintf(bat, sizeof bat, "%d%% %.2fV", m.battery, m.batteryV);
  else strncpy(bat, "--", sizeof bat);
  char sd[16];
  if (m.sdPresent) snprintf(sd, sizeof sd, "%.1f GB free", m.sdFreeGB);
  else strncpy(sd, "NO SD", sizeof sd);
  const char* labels[3] = {"BAT", "SD", "REC"};
  const char* values[3] = {bat, sd, nrec};

  const int top = 58, rowH = 30;                  // 3 rows, centered-ish in body
  for (int i = 0; i <= 3; i++) c.drawFastHLine(L, top + i * rowH, R - L, INK);
  for (int i = 0; i < 3; i++) {
    const int b = top + i * rowH + 20;
    textLeft(c, L, b, labels[i], F_BOLD);
    textRight(c, R, b, values[i], F_REG);
  }

  footer(c, {"list", "(x2)"}, {"rec", "(x2)"});
}

void recording(GFXcanvas1& c, const Model& m) {
  header(c, "RECORDING", false, nullptr, true);
  char t[12]; fmtHMS(t, m.recElapsed);
  textCenter(c, 122, t, F_BIG);
  footer(c, {"cancel", nullptr}, {"save", nullptr});
}

void list(GFXcanvas1& c, const Model& m) {
  if (m.recCount == 0) {
    header(c, "RECORDINGS", false, "0 / 0", false);
    textCenter(c, 110, "No recordings yet", F_REG);
    footer(c, {"exit", "(hold)"}, {"OK", nullptr});
    return;
  }

  char counter[12]; snprintf(counter, sizeof counter, "%d / %d", m.listIndex + 1, m.recCount);
  header(c, "RECORDINGS", false, counter, false);

  // 3-row window that keeps the selection visible.
  const int MAXROWS = 3;
  int start = 0;
  if (m.recCount > MAXROWS) {
    start = m.listIndex - 1;
    if (start < 0) start = 0;
    if (start > m.recCount - MAXROWS) start = m.recCount - MAXROWS;
  }

  const int top = 42, rowH = 30;
  c.drawFastHLine(L, top, R - L, INK);
  for (int i = 0; i < MAXROWS && start + i < m.recCount; i++) {
    const int idx = start + i;
    const int y   = top + i * rowH;
    const bool sel = (idx == m.listIndex);
    if (sel) c.fillRect(L, y + 1, R - L, rowH - 1, INK);  // inverted row
    const int color = sel ? BG : INK;
    const int b = y + 20;

    char num[4]; snprintf(num, sizeof num, "%02d", idx + 1);
    char when[12]; rowDate(when, m.recs[idx].date);
    char dur[12];  fmtShort(dur, m.recs[idx].dur);
    textLeft(c, L + 3, b, num, F_BOLD, color);
    textLeft(c, L + 28, b, when, F_REG, color);
    textRight(c, R - 3, b, dur, F_BOLD, color);
    c.drawFastHLine(L, y + rowH, R - L, INK);
  }

  footer(c, {"next", nullptr}, {"play", nullptr});
}

void playing(GFXcanvas1& c, const Model& m) {
  header(c, "PLAYING", false, nullptr, false);
  const app::Rec& r = m.recs[m.listIndex];

  textLeft(c, L, 56, r.date, F_TITLE);
  char t[12]; fmtHMS(t, m.playElapsed);
  textLeft(c, L, 96, t, F_BIG);
  char total[16]; total[0] = '/'; total[1] = ' '; fmtHMS(total + 2, r.dur);
  textLeft(c, L, 116, total, F_REG);

  // progress bar (160 x 9, inset border), fill by elapsed/total
  const int bx = (W - 160) / 2, by = 128, bw = 160, bh = 9;
  c.drawRect(bx, by, bw, bh, INK);
  if (r.dur > 0) {
    int fw = (int)((long)(bw - 4) * m.playElapsed / r.dur);
    if (fw > bw - 4) fw = bw - 4;
    if (fw > 0) c.fillRect(bx + 2, by + 2, fw, bh - 4, INK);
  }

  footer(c, {"-", nullptr}, {"stop", nullptr});
}

void deleteConfirm(GFXcanvas1& c, const Model& m) {
  header(c, "DELETE?", false, nullptr, false);
  const app::Rec& r = m.recs[m.listIndex];

  textLeft(c, L, 60, r.date, F_TITLE);

  char hms[12]; fmtHMS(hms, r.dur);
  textLeft(c, L, 92, "DURATION", F_REG);
  textRight(c, R, 92, hms, F_BOLD);

  char mb[16]; snprintf(mb, sizeof mb, "%.1f MB", r.bytes / (1024.0 * 1024.0));
  textLeft(c, L, 114, "SIZE", F_REG);
  textRight(c, R, 114, mb, F_BOLD);

  footer(c, {"cancel", nullptr}, {"OK", nullptr});
}

void conditionScreen(GFXcanvas1& c, const Model& m) {
  switch (m.condition) {
    case app::Condition::LowBattery:
      header(c, "LOW BATTERY", false, nullptr, false);
      textCenter(c, 92, "Battery low", F_TITLE);
      textCenter(c, 122, "Recording continues", F_REG);
      footer(c, {"-", nullptr}, {"dismiss", nullptr});
      break;
    case app::Condition::Charging:
      header(c, "CHARGING", false, nullptr, false);
      c.fillRect(30, 72, 140, 42, INK);
      textCenter(c, 101, "POWER", F_TITLE, BG);
      footer(c, {"-", nullptr}, {"dismiss", nullptr});
      break;
    case app::Condition::NoSd:
      header(c, "NO SD CARD", false, nullptr, false);
      textCenter(c, 96, "Insert SD card", F_TITLE);
      textCenter(c, 124, "Cannot record/list", F_REG);
      footer(c, {"exit", "(hold)"}, {"-", nullptr});
      break;
    case app::Condition::StorageFull:
      header(c, "STORAGE FULL", false, nullptr, false);
      textCenter(c, 96, "Free space needed", F_TITLE);
      footer(c, {"-", nullptr}, {"OK", nullptr});
      break;
    case app::Condition::TimeNotSet:
      header(c, "TIME NOT SET", false, nullptr, false);
      textCenter(c, 88, "Recording allowed", F_TITLE);
      textCenter(c, 118, "Timestamps unset", F_REG);
      footer(c, {"-", nullptr}, {"continue", nullptr});
      break;
    case app::Condition::Sleep:
      header(c, "scribr", true, nullptr, false);
      textCenter(c, 105, "Sleeping...", F_TITLE);
      break;
    case app::Condition::None:
      break;
  }
}

}  // namespace

void screens::render(const Model& m) {
  GFXcanvas1& c = display::canvas();
  c.fillScreen(BG);
  c.setTextWrap(false);

  if (m.condition != app::Condition::None) {
    conditionScreen(c, m);
    return;
  }

  switch (m.screen) {
    case Screen::IDLE:           idle(c, m);          break;
    case Screen::REC:            recording(c, m);     break;
    case Screen::LIST:           list(c, m);          break;
    case Screen::PLAY:           playing(c, m);       break;
    case Screen::DELETE_CONFIRM: deleteConfirm(c, m); break;
  }
}
