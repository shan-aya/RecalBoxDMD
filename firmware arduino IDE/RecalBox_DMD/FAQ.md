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

## General troubleshooting

| Symptom | Likely cause | Try this |
|---|---|---|
| Animation freezes on one frame, sometimes with visual glitches | USB power spike on a bright image (see above) | Shorter cable, powered hub/adapter, lower brightness |
| DMD vanishes from the PC (COM port errors, "device not functioning") while testing over USB | Same as above — severe enough to disrupt the USB link itself, not just the display | Same as above |
| Screen shows "RecalBox connectée" even though Recalbox is off | Cosmetic display bug, fixed in firmware v210+ | Update the firmware |

<!-- TODO (utilisateur) : ajouter d'autres entrees FAQ au fur et a mesure des retours -->
