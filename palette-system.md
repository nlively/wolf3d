# The Palette System

How color works in this port: what the original VGA palette machinery did, and how this
project replaces it. **The short version: assets are decoded from indexed → RGB *once,
offline*, at extraction time. The runtime is (almost) truecolor and the live palette is
gone.** This is a deliberate departure from the original engine and from the earlier
"keep everything indexed" design.

---

## 1. VGA background: why a palette existed at all

Wolf3D renders in VGA 320×200 with **one byte per pixel**. That byte is not a color —
it's an **index** into the VGA DAC, a hardware table of 256 colors. Each DAC entry is
three 6-bit values (R, G, B, each **0–63**, not 0–255), written through I/O ports
(`PEL_WRITE_ADR` 0x3C8 / `PEL_DATA` 0x3C9 — see `VL_SetColor` in
[ID_VL.C](WOLFSRC/ID_VL.C#L114)).

Two consequences shaped the original engine:

1. **All art was stored as indices.** Walls, sprites, pics, fonts — every asset was 8-bit
   indexed and meaningless without a palette to decode it.
2. **Rewriting the DAC recolored the whole screen instantly**, without touching a pixel.
   On a 1992 CPU that made palette manipulation the *only* affordable full-screen effect:
   fades, damage flashes, and screen tints were all done by rewriting the table, never the
   pixels. (DAC writes had to happen during vertical blank — `VL_WaitVBL` — or the screen
   showed "snow.")

This port keeps neither consequence at runtime. Consequence #1 is resolved **offline**
(assets are pre-decoded to RGB). Consequence #2 moves to **RGB blending at present time**.

---

## 2. The decision this project made

The assets in [WOLFSRC/assets/extracted](WOLFSRC/assets/extracted) were extracted as
**RGB / RGBA PNGs**, with `gamepal` already applied:

| Asset kind | Format | Notes |
|---|---|---|
| Walls, pics | `RGB` | Palette baked in; no alpha needed |
| Sprites | `RGBA` | Magenta transparency (index 255) already converted to **alpha 0**; alpha is binary `{0,255}` |
| Fonts | `RGBA` | Mask baked into alpha |
| `palette.json` | 256 × rgb888 | A copy of `gamepal`, already scaled to 8-bit — see the caveat below |

Because the palette is baked into every PNG, the classic indexed pipeline is gone:

- There is **no index→RGB lookup per pixel** at runtime. The art is already color.
- There is **no live/mutable palette** to swap. `gamepal` as runtime state does not exist.
- The old doc's "framebuffer stays 8-bit indices, decode at the last moment" model is
  **not** what this project does. (That model is described in §5 only as the road not
  taken.)

The original engine's own source acknowledges this fork: converting everything to RGB
"forfeits the palette-swap machinery." For Wolf3D that machinery buys almost nothing
(see §4), so trading it for a simpler truecolor pipeline is the right call — but it *is* a
trade, and this document exists so the trade is understood rather than stumbled into.

---

## 3. Where RGB conversion happens now

### The big one: offline, at extraction (per asset, once)

256 × 3 six-bit VGA bytes → 8-bit RGB, baked into every PNG. This is the modern echo of
the original "decode with `gamepal`," done ahead of time instead of per frame. Nothing in
the running game repeats it.

### The residual: runtime index→RGB for code that names colors by number

A handful of drawing calls still specify a color as a **raw palette index**, not RGB —
these are inline color constants in the engine, not asset data. They're the last thing
tying the runtime to a palette, and each needs to be refactored to either resolve the
index → ARGB at the boundary (via a 256-entry table built from `palette.json`) or take an
ARGB value directly. The full call-site inventory is in **§3.5** below.

This index-resolution table is the **only** justification for keeping any palette in the
running game, and it's small and cold — nothing on the per-pixel path.

### The renderer is now truecolor (in progress)

The framebuffer has been converted to ARGB. Done so far:

- `framebuffer` / `shown` are now `uint32_t` ARGB.
- `R_PutPixel`, `R_DrawColumn`, `R_CaptureBackbuffer` take `uint32_t`. The `memcpy` sizes
  stay correct since every buffer is now the same width.
- `R_Present` blits straight through (`out[i] = vid.framebuffer[i]`) — the old per-pixel
  palette lookup is gone.
- **New primitives** carry the index→ARGB boundary and RGB drawing:
  - `R_SetPalette(const uint8_t *vga_pal)` — builds `vid.palette[256]` from a 768-byte
    6-bit VGA palette, with exact `*255/63` scaling (the §6 dim-white fix). *Implemented.*
  - `R_MapColor(uint8_t index)` — the sole index→ARGB resolver; the only surviving use of
    `vid.palette`.
  - `R_FillRect(x,y,w,h,argb)` — clamped rectangle fill; every `VWB_Bar`/`Plot`/`Hlin`/`Vlin`
    reduces to this.
  - `R_GetPixel(x,y)` — reads ARGB back out of the framebuffer; the modern replacement for
    the old `peekb(0xa000,...)` "sample a color off screen" trick.

Still to do inside render.c:

- **`R_DrawImage`/`R_DrawPic` are stubs** ([render.c:92-102](WOLFSRC/render.c#L92)). These
  are the consumers of the RGBA assets — they need to blit RGB with alpha. `R_DrawColumn`
  is likewise a stub for the raycaster.
- `R_SetPalette` is implemented but has **no data yet**: `gamepal`'s 768 bytes still need
  re-sourcing (see §3.5-C). Until then `vid.palette` is all zeros and `R_MapColor` returns
  black, so index-named fills draw black.
- Add `#include <assert.h>` — `R_PutPixel`/`R_GetPixel` use `assert` but only compile via a
  transitive SDL include.

```
   RGB/RGBA assets (pre-decoded PNGs)      index-named colors (VWB_Bar etc.)
          │  blit (with alpha)                    │  R_MapColor: index → ARGB
          ▼                                       ▼
   vid.framebuffer[320*200]   ← now uint32 ARGB, one color per pixel
          │
          │  R_Present(), once per frame:
          │    color = blend(color, fade/tint)    ← effects live here, in RGB
          ▼
   ARGB8888 SDL texture ──► GPU ──► screen
```

---

## 3.5 Remaining index-dependent call sites (the refactor checklist)

Everything below still traffics in palette **indices** and must be converted before the
runtime is fully index-free. Grouped by kind, in rough dependency order.

### A. Draw primitives that take a color *index* argument

**The chosen pattern (established by the Bar family):** keep the `int color` argument as a
palette *index* and resolve it to ARGB *inside* the primitive via `R_MapColor`, then draw
with `R_FillRect`. Callers stay untouched — they all speak indices (literals like `127`,
named macros like `TEXTCOLOR`=`0x17`, `WHITE`=`15`, or arithmetic like `MAINCOLOR-i`), and
`R_MapColor` is the single boundary. All four primitives collapse to one-liners:
`Plot`=1×1, `Hlin`=N×1, `Vlin`=1×N, `Bar`=N×M.

| Primitive | Status | Notes |
|---|---|---|
| `VWB_Bar` | ✅ **done** | `VWB_Bar` and `VW_Bar` **consolidated into one function** ([ID_VH.C:256](WOLFSRC/ID_VH.C#L256)); body is `R_FillRect(x,y,w,h,R_MapColor(color))`. 47 call sites, all unchanged. `VL_Bar`/`VW_Bar` removed. |
| `VL_Plot` | ✅ **done** | `R_PutPixel(x, y, R_MapColor(color))` ([ID_VL.C](WOLFSRC/ID_VL.C)). |
| `VL_Hlin` | ✅ **done** | `R_FillRect(x, y, width, 1, R_MapColor(color))`. |
| `VL_Vlin` | ✅ **done** | `R_FillRect(x, y, 1, height, R_MapColor(color))`. |
| `VL_ColorBorder` | ⬜ todo | Letterbox/border; cosmetic, safe to stub or drop. |

The planar-VGA mask tables (`pixmasks`/`leftmasks`/`rightmasks`) died with `VL_Hlin`/`VL_Bar`
and have been removed. `VW_Plot`/`VW_Hlin`/`VW_Vlin` are still macros over the `VL_*` forms
([ID_VH.H:87-89](WOLFSRC/ID_VH.H#L87)), and the `VWB_*` wrappers sit on top of those —
callers of all three layers now resolve to ARGB automatically.

**Bonus done — the `peekb` case (was `†`):** [WL_MAIN.C:657](WOLFSRC/WL_MAIN.C#L657) & [677](WOLFSRC/WL_MAIN.C#L677)
`VW_Bar(...,peekb(0xa000,0))` are now `R_FillRect(0,189,300,11,R_GetPixel(0,0))`. This one
legitimately bypasses `R_MapColor` — it samples a live on-screen color (ARGB) rather than
naming an index, exactly as the original did. (Only shows the right color once the signon
asset is sourced; harmless until then.)

### B. Font / text color

Text color is a global byte index, not RGB:

- `fontcolor` / `backcolor` globals ([ID_VH.C:30](WOLFSRC/ID_VH.C#L30)), set via
  `SETFONTCOLOR(f,b)` ([ID_HEADS.H:117](WOLFSRC/ID_HEADS.H#L117)) — e.g.
  [WL_GAME.C:971](WOLFSRC/WL_GAME.C#L971) `SETFONTCOLOR(0,15)`.
- Also set from in-string color codes while parsing help/end text
  ([WL_TEXT.C:210-219](WOLFSRC/WL_TEXT.C#L210)).
- The glyph blit itself was inline VGA asm using `fontcolor` as the pixel value
  ([ID_VH.C:62](WOLFSRC/ID_VH.C#L62), [118](WOLFSRC/ID_VH.C#L118)) — that has to be
  rewritten to blit the RGBA font PNG, tinting the glyph mask by an ARGB color derived from
  `fontcolor`/`backcolor`.

### C. Renderer entry points still pending (from §3)

- **`gamepal` data.** `R_SetPalette` is implemented and called at
  [WL_MAIN.C:1113](WOLFSRC/WL_MAIN.C#L1113) `R_SetPalette(gamepal)`, but `gamepal`'s 768
  bytes still need re-sourcing (they came from [OBJ/GAMEPAL.OBJ](WOLFSRC/OBJ/GAMEPAL.OBJ);
  `palette.json` is the extracted copy). Until this lands, `R_MapColor` returns black — so
  the whole Category-A path is wired but colorless. **This is the highest-leverage next
  step.**
- `R_DrawImage` / `R_DrawPic` / `R_DrawColumn` stubs — the actual RGBA asset blitters.

### D. Deletable once the above lands

- `redshifts` / `whiteshifts` tables + `InitRedShifts` remnants
  ([WL_PLAY.C:1033-1075](WOLFSRC/WL_PLAY.C#L1033)) — dead under RGB tinting (§4).
- Direct DAC I/O `VL_SetColor`/`VL_GetColor` ([ID_VL.C:114-138](WOLFSRC/ID_VL.C#L114)) —
  dead once nothing sets hardware palette entries.
- `screenfaded` global — superseded by `R_IsScreenFadedOut()`.

---

## 4. Effects, and why losing the live palette barely costs anything

The original did fades and flashes by rewriting the DAC. Here they become RGB-space
blends applied in `R_Present`. The math is equivalent (every pixel's color came from the
palette anyway) and it looks *better* — the VGA DAC was 6-bit, so old fades banded; 8-bit
blending doesn't.

| Effect | Old mechanism | New mechanism | Status |
|---|---|---|---|
| Fade in/out | Lerp DAC over VBLs (`VL_FadeIn/Out`) | `fade_level` in render.c, blended in `R_Present`, loop paced by present | Level plumbing exists; blend + pacing not wired |
| "Screen is faded" state | `screenfaded` global | `R_IsScreenFadedOut()` (level == 0) | Call sites converted ([WL_PLAY.C:622](WOLFSRC/WL_PLAY.C#L622), [WL_PLAY.C:1267](WOLFSRC/WL_PLAY.C#L1267)); dead global still declared |
| Damage/bonus flash | `redshifts`/`whiteshifts` → DAC | Full-screen red/white tint in `R_Present`, driven by `damagecount`/`bonuscount` | Old machinery removed; replacement not started; the `redshifts`/`whiteshifts` tables in [WL_PLAY.C:1033-1075](WOLFSRC/WL_PLAY.C#L1033) are now dead weight and can be deleted |
| Border color | `VL_ColorBorder` (overscan register) | Letterbox clear color (index→RGB via palette.json) or drop | Cosmetic |
| Direct DAC I/O | `VL_SetColor`/`VL_GetColor` | — | Dead once nothing calls it |

**What the truecolor move actually forfeits, and why it's cheap here:**

- **Palette cycling / animated color tricks** — Wolf3D essentially doesn't use these
  (no animated fire/water like Doom). Negligible loss.
- **Per-screen custom palettes** — `TITLEPALETTE`, `END1/END3PALETTE`, `IDGUYSPALETTE`
  ([GFXV_SOD.H](WOLFSRC/GFXV_SOD.H#L176)) exist to give a few screens colors outside
  `gamepal`. These are **Spear of Destiny** assets; this build is base Wolf3D (the maps
  are Wolf1–6, the six episodes), so they're effectively moot. If any base-Wolf3D screen
  (e.g. the id-guys easter egg) genuinely needs off-`gamepal` colors, the fix is trivial
  under this model: **extract that specific PNG under its own palette**, offline. No
  runtime palette swap required.

So the two things the indexed model protected are, for this game, nearly free to give up.

---

## 5. The road not taken (indexed-at-runtime)

For the record, the alternative design — the one an earlier version of this doc
described — kept the framebuffer as 8-bit indices and did index→RGB per pixel in
`R_Present` against a live `vid.palette[256]`, with a `R_SetPalette()` converting VGA
bytes on each palette change. It's more faithful to the original and preserves palette-
swap tricks.

We are **not** doing that, because:

- The assets are already RGB PNGs; reverting means re-extracting everything as indexed
  art and shipping `gamepal` as runtime data.
- RGBA sprites (partial-index → alpha) don't fit a single-byte index framebuffer cleanly.
- The tricks it preserves (§4) are ones Wolf3D doesn't meaningfully use.

If you ever *do* revert, the two conversion points would be `R_SetPalette` (per palette
change) and the per-pixel lookup in `R_Present` — and `gamepal` would need re-sourcing,
since its 768 bytes historically came from `OBJ/GAMEPAL.OBJ`
([WOLFSRC/OBJ/GAMEPAL.OBJ](WOLFSRC/OBJ/GAMEPAL.OBJ)) and the modern build can't link that.

---

## 6. Caveats baked into the current extraction

- **6→8-bit scaling is `<<2`, not exact.** `palette.json` maxes at **252**, not 255
  (`63<<2 = 252`). Pure white comes out slightly gray and the whole game reads a touch
  dim. This is baked into all ~676 PNGs. To fix, re-extract using `(v*255)/63`
  (equivalently `(v<<2)|(v>>4)`), which maps 63→255 exactly.
- **Transparency is binary.** Sprite alpha is only `{0, 255}` — the magenta key color
  (palette index 255 = `[152,0,136]`) became alpha 0. Fine for Wolf3D; there was never
  partial transparency to preserve.
- **Single palette.** Only one `palette.json` (a copy of `gamepal`) was produced, so any
  screen needing off-`gamepal` colors is currently baked with the wrong colors. See §4 —
  not expected to matter for base Wolf3D.

---

## 7. Quick reference

**Original 768-byte VGA palette layout** (`gamepal`, VGAGRAPH palette chunks):

```
byte 0   1   2   3   4   5   ...  765 766 767
     R0  G0  B0  R1  G1  B1  ...  R255 G255 B255      each value 0–63
```

| What | Where |
|---|---|
| Extracted RGB/RGBA assets | [WOLFSRC/assets/extracted](WOLFSRC/assets/extracted) |
| Runtime index→RGB table (gamepal copy, rgb888) | [WOLFSRC/assets/extracted/palette.json](WOLFSRC/assets/extracted/palette.json) |
| Modern video state (uint32 ARGB framebuffer + palette) | [render.c:9-20](WOLFSRC/render.c#L9-L20) |
| Present loop (where effects belong) | [render.c:61-76](WOLFSRC/render.c#L61-L76) |
| Index→ARGB boundary + RGB primitives | `R_SetPalette` / `R_MapColor` / `R_FillRect` / `R_GetPixel` in [render.c](WOLFSRC/render.c) |
| Fade API | [render.h](WOLFSRC/render.h), [ID_VL.C:151](WOLFSRC/ID_VL.C#L151) |
| Bar (done, consolidated) | [ID_VH.C:256](WOLFSRC/ID_VH.C#L256) `VWB_Bar` |
| Primitives still naming colors by index | `VL_Plot`/`VL_Hlin`/`VL_Vlin` [ID_VL.C:231-306](WOLFSRC/ID_VL.C#L231), fonts [ID_VH.C](WOLFSRC/ID_VH.C) |
| Dead flash tables / counters (deletable) | [WL_PLAY.C:1033-1075](WOLFSRC/WL_PLAY.C#L1033) |
| Old DAC I/O (reference/dead) | [ID_VL.C:114-138](WOLFSRC/ID_VL.C#L114) |
| Original gamepal binary (can't link in modern build) | [WOLFSRC/OBJ/GAMEPAL.OBJ](WOLFSRC/OBJ/GAMEPAL.OBJ) |
