# FAQ & Troubleshooting

🇬🇧 **English** · [🇫🇷 Français](FAQ.fr.md) · [🇪🇸 Español](FAQ.es.md)

## ⚠️ Power supply — please read if your DMD freezes or glitches

The DMD draws **all** of its power from the USB port/cable — there's no separate power input for the LED panel. A bright image lights up far more LEDs at full intensity than a dark one, which means the panel's current draw spikes noticeably every time the display changes to something light-colored. On a USB port/cable that can't quite keep up with that spike, the symptoms can look like a firmware bug: the animation freezes on one frame, the image glitches/shows corrupted pixels, or in more severe cases the DMD disappears from the PC entirely (serial/COM port drops with a Windows device error) for a moment.

**If you're seeing freezes, corruption, or random disconnects — especially ones that seem to happen on lighter/brighter GIFs — try, in order:**

1. **Use a shorter/better-quality USB cable.** A thin or long cable is the single most common cause of USB power sag.
2. **Power it from a powered USB hub** instead of a direct PC/laptop port, or a phone-charger-style 5V USB power adapter (data-only isn't needed once it's running, just for flashing/setup). Some PC ports simply can't supply enough current under a sudden spike.
3. **Lower the brightness** in the web configuration page — a lower `brightness` setting reduces the current draw per LED directly, which reduces the size of the spike on bright content. You don't need to go all the way to 10%; the goal is just headroom over whatever your specific USB source can reliably supply.

This isn't a firmware bug and reflashing won't fix it — it's the physical power budget of USB. Once you've found a cable/power source/brightness combination that's stable for your setup, it stays stable — this is a one-time thing to sort out per DMD, not something that comes back later.

## ⚠️ ESP32 chip revision — some boards may be more sensitive than others

<!-- TODO (utilisateur) : completer avec les infos precises sur les revisions ESP32-D0WD-V3 concernees, les references produit ou marchands a eviter/preferer, et tout autre retour d'experience -->

Not every ESP32 module is identical under the hood — Espressif has shipped several **silicon revisions** of the chip inside the common ESP32-D0WD-V3 module (e.g. v3.0 vs v3.1) over the years, each with its own set of fixed/introduced hardware quirks. In practice, some revisions appear to be **more sensitive to marginal power conditions** than others — the freezing/corruption described above can show up much sooner (or not at all) purely depending on which revision happens to be inside the specific board you got, even with the exact same firmware and the exact same USB setup as someone else who's never seen the issue.

You can check which revision your board reports during flashing — the Web Installer / `esptool` output shows a line like `Chip is ESP32-D0WD-V3 (revision vX.X)`. There's currently no way to change this after the fact (it's baked into the physical chip), so if your specific board turns out to be a more sensitive revision, the power-supply advice above becomes more important for you than it might be for someone else's DMD.

## microSD card quality matters

The firmware reads from the SD card constantly — every GIF, the playlist, and its own configuration file all live there — so a marginal or failing card can cause a surprisingly wide range of symptoms that have nothing to do with the firmware itself: looping back to the WiFi setup screen instead of connecting, settings that don't actually save, the display getting stuck on one image, and more. Use a genuine, reasonably modern microSD card from a known brand — very cheap/no-name cards are by far the most common source of these issues.

**If you suspect an SD card error:**

- Power off the DMD, remove the SD card, and reinsert it firmly. This alone resolves most transient card-read glitches.
- Reboot the DMD afterward if the problem persists.

## The web configuration page loads slowly, or not at all

The DMD's WiFi/network stack can be genuinely busy at any given moment (talking to Recalbox, rendering, handling other requests) — a page in the web configuration interface loading slowly, or a save/action taking a while to respond, is expected from time to time, not necessarily a sign of a problem.

**Good habits when this happens:**

- **Don't repeatedly hit refresh/retry.** Each attempt is a new request competing with whatever the DMD is already doing — spamming reload tends to make things pile up rather than speed anything up.
- **Wait for your browser's own error** (a timeout, "can't reach this page", etc.) before trying again. A single retry after that point is usually enough.
- **If it still won't load after that, reboot the DMD.** A fresh reboot generally restores immediate access to the web configuration page.

## Brief WiFi drop / DMD seems out of sync with Recalbox

A short, temporary WiFi hiccup doesn't need any action on your part. The firmware detects the reconnection on its own, generally within a few seconds, and automatically resyncs the DMD's display with whatever Recalbox is actually doing at that moment (current game, browsing, demo mode...) — there's no need to restart anything, on the DMD or on Recalbox, for a brief drop like this to resolve itself.

## Stuck looping back to the DMD's own WiFi hotspot (AP mode)

If, after saving your WiFi network on the DMD's own captive-portal setup page, it keeps coming back to broadcasting its own hotspot (`RecalBox-DMD-Config`) instead of joining your network, the simplest fix is to avoid that screen entirely: use the **Wi-Fi step built into the PC Toolkit's Mode 1** instead. It scans your networks, verifies the password by actually testing the connection from your PC, and writes it straight into `config.ini` on the SD card before the card ever goes into the DMD — the DMD then joins your network on its very first boot, and the captive-portal screen never needs to appear at all.

If you do need to use the DMD's own setup page and it keeps looping back: double-check the password (typos are the most common cause), and see "microSD card quality" above — a failing card can silently prevent the WiFi settings from actually saving, even when the setup itself looked like it went through fine.

## Give your DMD a fixed IP address — pick ONE way to do it, never both

Some integrations — the Recalbox-side scripts that talk to the DMD, for example — reach it by a fixed IP address. If your DMD's address changes over time (most routers/boxes hand out addresses dynamically via DHCP, and can assign a different one after a reboot or reconnection), anything relying on the old address quietly stops working.

There are two ways to give it a fixed address:

- **A DHCP reservation on your router/box**, tying a fixed address to the DMD's WiFi MAC address — the DMD keeps requesting an address the normal way, your router just always hands it the same one.
- **A static IP configured directly on the DMD** (`wifi_static_enabled` / `wifi_static_ip` in `config.ini`, or the matching fields on the web configuration page) — the DMD assigns itself the address directly, without asking the router at all.

**Set up only one of the two, never both at the same time.** Configuring a DHCP reservation on the router *and* a static IP on the DMD can point them at different, conflicting addresses, or fail in ways that are confusing to diagnose. Pick whichever is easier for you — a router-side reservation is usually simpler and keeps all your network configuration in one place — and leave the other one untouched.

## Recalbox scripts don't seem to do anything after an update

If the DMD stops reacting to what's happening on Recalbox after updating the firmware or the PC Toolkit, the Recalbox-side scripts already installed on your Recalbox may be out of date. Reinstall them with **Mode 9** (or a fresh **Mode 1**) from the PC Toolkit — it also removes old script versions automatically. See [UPGRADING.md](UPGRADING.md) if you're coming from an older release: some updates change the underlying protocol the scripts and the DMD use to talk to each other, so the scripts genuinely need reinstalling, not just the firmware reflashing.

## General troubleshooting

| Symptom | Likely cause | Try this |
|---|---|---|
| Animation freezes on one frame, sometimes with visual glitches | USB power spike on a bright image (see above) | Shorter cable, powered hub/adapter, lower brightness |
| DMD vanishes from the PC (COM port errors, "device not functioning") while testing over USB | Same as above — severe enough to disrupt the USB link itself, not just the display | Same as above |
| Screen shows "RecalBox connectée" even though Recalbox is off | Cosmetic display bug, fixed in firmware v210+ | Update the firmware |
| Web config page slow to load or times out | DMD's WiFi/network stack busy at that moment | Wait for the browser's error, retry once; reboot if it persists |
| Display briefly out of sync after a WiFi drop | Normal — firmware resyncs automatically | Nothing to do, wait a few seconds |
| Loops back to WiFi setup, settings don't save, display gets stuck | Marginal/failing microSD card | Remove and reinsert the card firmly, reboot if needed; try a different card |
| Keeps looping back to the DMD's own WiFi hotspot | Password typo, or captive-portal save didn't stick | Use Mode 1's Wi-Fi step instead; check password; see microSD note above |
| Recalbox scripts stopped working after an update | Old script versions still on Recalbox | Reinstall via Mode 9 (or a fresh Mode 1) |

<!-- TODO (utilisateur) : ajouter d'autres entrees FAQ au fur et a mesure des retours -->
