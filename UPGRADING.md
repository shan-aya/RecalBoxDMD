# Upgrading from an earlier version (v13 → v2.0)

🇬🇧 **English** · [🇫🇷 Français](UPGRADING.fr.md) · [🇪🇸 Español](UPGRADING.es.md)

Already running RecalBoxDMD (firmware v13 "Raw565 Edition", any PC Toolkit)? Here's what actually changes and what you need to do — most of it is optional, and nothing about your SD card or Recalbox setup needs to change.

## 1. Update the PC Toolkit

Grab the latest release from the [Releases page](https://github.com/shan-aya/RecalBoxDMD/releases) and install it over the old one (or replace the portable `.exe`). Your settings (theme, language, Recalbox IP, saved fallback image...) carry over automatically — they live in a separate `RecalBoxDMD_prefs.json`, untouched by the update.

## 2. Flash the new firmware

Same as always — the [one-click Web Installer](https://shan-aya.github.io/RecalBoxDMD/) (Chrome/Edge) flashes v2.0 over USB in about a minute. No need to tick "Erase device" — a normal reflash preserves your `config.ini` and everything already on the SD card, including the `recalbox_ip` field, which now identifies the UDP peer instead of the MQTT broker; nothing to change there.

## 3. Reinstall the Recalbox userscripts — required

The real-time link between Recalbox and the DMD switched from **MQTT to UDP** under the hood (see the [Changelog](CHANGELOG.md) for why). The Recalbox-side scripts (`marquee[...].sh`, `dmd_score[...].sh`, and the rest of `dmd_helpers/`) were updated to speak UDP instead of publishing to an MQTT broker — **run Mode 9** once (or a fresh Mode 1) from the PC Toolkit to reinstall them; it also cleans up the old MQTT-era script versions automatically. Nothing to configure: no broker, no port, no credentials to enter — the DMD listens on the same `recalbox_ip` it already used.

## 4. Had something wired into the old MQTT topics?

MQTT support has been fully removed from the firmware as of v209 (2026-09-14) — not just turned off by a flag as earlier versions of this page said. `MQTT_ENABLED=true` no longer does anything: the connection/task code itself is gone from the source, not just disabled. If you had something external publishing to or subscribing from the DMD's old MQTT topics, you'd need to pull that code back from the git history predating v209 and rebuild from there. UDP is the only supported real-time path going forward.

## What you *don't* need to do

- Re-scrape your games, rebuild your SD card, regenerate any cache, or touch your playlists — none of that changed.
- Reconfigure the Recalbox IP, WiFi, or anything else on the web config page — same fields, same values.
- Do anything about the MQTT broker (Mosquitto) still running on Recalbox — the DMD simply no longer talks to it; leave it running or remove it, your call.
