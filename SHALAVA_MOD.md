# Shalava Mod for UzbekGram Desktop

This mod adds a configurable visual overlay to the application.

## Features
- **3 Intensity Levels:**
  1. **SHALAVA:** Light particles (stars), subtle shake.
  2. **SUPER SHALAVA:** More particles, checkmarks, stronger shake.
  3. **ULTRA SHALAVA:** Maximum chaos, text overlays, intense shake.
- **Visuals:** Custom icons (`ghost`, `star`, `checkmark`, `uzbekchan`).
- **Safety:** Epilepsy warning before enabling ULTRA mode. Safe Mode option to reduce intensity.
- **Settings:** Configurable in `Settings -> Appearance -> Shalava Mod`.

## Controls
- **Drawer:** 3 Ghost buttons near the top of the main menu allow quick toggling between modes.
- **Settings:**
  - Enable/Disable
  - Safe Mode
  - Particle Limit
  - Text Overlays
  - Mouse Capture (Overlay blocks mouse events if enabled)

## Implementation Details
- **Overlay:** `Ayu::Ui::ShalavaOverlay` (QWidget)
- **Settings:** `AyuSettings` updated with new fields.
- **Assets:** `Telegram/Resources/art/ayu/shalava/`
- **Platform:** Linux (Cross-platform compatible Qt code)
