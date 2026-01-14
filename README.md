# Suspectre (ESP32)

Proof-of-concept ESP32 firmware that scans nearby BLE devices, lets you manually flag devices that feel relevant in the moment, and provides a local-only web UI for reviewing and managing those flags. The system is designed to keep identifiers local to the device and support privacy-friendly sharing of lists without tying them to people.

## Current Status
- BLE scanning with a live “Current Scan” table (ordered by strongest RSSI first)
- One-button “flag all in range” plus per-device “flag” actions in the UI
- Suspect tracking with counts, notes, and a confirmed flag
- Suspect list is sorted by last seen (RSSI as a tie breaker)
- Hidden/visible Wi‑Fi AP configurable in settings, served on-device only
- CSV export/import of suspect lists for offline sharing
- Onboard LED alerts: suspects blink, confirmed devices triple-flash (stronger signal = faster)

## Web UI Tabs
- Current Scan: live devices, inline flagging, pause/resume updates
- Suspects: confirmed/suspect lists with notes, whitelist, and CSV tools
- Settings: hotspot configuration and LED output settings

## Build (PlatformIO)
1. Install PlatformIO.
2. Connect an ESP32-C3 board (SuperMini target config).
3. Build and upload:
   ```bash
   pio run -t upload -t monitor
   ```

## Defaults
- Hotspot SSID: `Suspectre-<last 4 hex of MAC>` (visible)
- Hotspot password: `suspectre` (min 8 chars enforced)
- Button: GPIO 9 (BOOT button on ESP32-C3)

## Notes
- BLE names may be absent; only advertised local names are shown.
- Whitelisted devices are stored in NVS and excluded from future suspect flags.
