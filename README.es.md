# RecalBoxDMD — RawEdition <img alt="Firmware: v3.50" src="https://img.shields.io/badge/firmware-v3.50-blueviolet.svg"> <img alt="Herramienta PC: v11469" src="https://img.shields.io/badge/herramienta%20PC-v11469-blueviolet.svg">

**Un verdadero marquee LED para tu mueble arcade Recalbox — visualización instantánea, incluso con un fullset MAME de 30 000 juegos.**

[🇬🇧 English](README.md) · [🇫🇷 Français](README.fr.md) · 🇪🇸 **Español**

> ℹ️ ¿Ya usas una versión anterior? Consulta **[UPGRADING.es.md](UPGRADING.es.md)** — qué cambia en la v13 y el único paso opcional (regenerar la caché) para sacarle el máximo partido.

<p align="center">
  <img src="medias/hero/hero_es.jpg" alt="RecalBoxDMD funcionando en vivo en un mueble Recalbox JAMMA real" width="420">
</p>

<p align="center">
  <img src="medias/dmd_in_action.gif" alt="Imágenes reales: el marquee cambiando en vivo al navegar por los juegos" width="260">
</p>
<p align="center"><sub>📹 Imágenes reales, no un montaje — el marquee se actualiza en vivo al navegar · <a href="medias/dmd_in_action.mp4">ver el clip completo (MP4)</a></sub></p>

<p align="center"><a href="https://youtu.be/xF8O88XXSwU"><img src="https://img.youtube.com/vi/xF8O88XXSwU/hqdefault.jpg" alt="Vídeo: presentación, montaje e instalación de RecalBoxDMD" width="420"></a><br><sub>🎬 <b>Vídeo</b> — presentación, montaje e instalación, por Jamyz Gaming · <a href="https://youtu.be/xF8O88XXSwU">YouTube</a></sub></p>

<p align="center">
  <img src="medias/clock_themes/00_super_mario.gif" width="140" alt="Tema de reloj Super Mario">
  <img src="medias/clock_themes/02_pac_man.gif" width="140" alt="Tema de reloj Pac-Man">
  <img src="medias/clock_themes/03_space_invaders.gif" width="140" alt="Tema de reloj Space Invaders">
  <img src="medias/clock_themes/07_fire.gif" width="140" alt="Tema de reloj Fire">
</p>

<p align="center">
  <img src="medias/fallback_images/default_RB.png" width="140" alt="Imagen de respaldo — Recalbox">
  <img src="medias/fallback_images/default_jamma.png" width="140" alt="Imagen de respaldo — Recalbox JAMMA">
  <img src="medias/fallback_images/default_rgbdual.png" width="140" alt="Imagen de respaldo — Recalbox RGB Dual">
  <img src="medias/fallback_images/default_rgbdual2.png" width="140" alt="Imagen de respaldo — Recalbox RGB Dual 2">
</p>

<p align="center">
  <a href="LICENSE"><img alt="Licencia: MIT" src="https://img.shields.io/badge/licencia-MIT-green.svg"></a>
  <img alt="Plataforma: ESP32" src="https://img.shields.io/badge/plataforma-ESP32-blue.svg">
  <img alt="Panel: HUB75 128x32" src="https://img.shields.io/badge/panel-HUB75%20128x32-blue.svg">
  <img alt="Recalbox: 10.x / 9.x / legacy" src="https://img.shields.io/badge/recalbox-10.x%20%7C%209.x%20%7C%20legacy-orange.svg">
  <img alt="Idiomas: FR EN ES" src="https://img.shields.io/badge/UI-FR%20%7C%20EN%20%7C%20ES-purple.svg">
  <a href="https://buymeacoffee.com/shan_aya"><img alt="Apoya el proyecto: Buy Me a Coffee" src="https://img.shields.io/badge/Buy%20Me%20a%20Coffee-support-FFDD00?logo=buymeacoffee&logoColor=black"></a>
</p>

---

## 🎬 Novedades desde RawEdition v2.64

- 👉 **[Vista general visual (página interactiva)](https://shan-aya.github.io/RecalBoxDMD/promo/rawedition_infographic_es.html)**

RawEdition v2.0 era un marquee: un logo o un GIF por juego. **Ahora son dos mundos en el mismo panel — y los mezclas a tu gusto:**

- 🎬 **Lista de GIF** — tus marquees, tu modo attract de 600 GIF, los overlays en juego (todo lo que ya conoces).
- 🧩 **Widgets** — una nueva familia de pantallas vivas y animadas: **reloj, fecha, efemérides, cuenta atrás, ambientes, tiempo, estadísticas, citas y quiz, y tres widgets Recalbox** (desafío, recompensas, salud), todos sobre un cielo que sigue el sol real y el tiempo.

### 🔀 Mézclalos a tu gusto

- Cada widget está **desactivado**, **en lugar del reloj** o **además de él**, con su propia duración — todo desde la página web *Widgets*.
- Los widgets aparecen **cada N GIF** o **cada N minutos**, en orden fijo o aleatorio; incluso eliges cuántos widgets «adicionales» se sortean.
- **Modo solo widgets**: ningún GIF — el panel encadena tus widgets sin parar (un reloj de escritorio vivo). ¿Solo lista? Desactiva todos los widgets. Todo lo intermedio funciona.

---

## ¿Qué es esto?

**RecalBoxDMD** convierte un pequeño **panel LED 128×32** (2 módulos HUB75 64×32 encadenados) en un verdadero marquee de arcade para tu mueble **Recalbox**: lanzas un juego y su logo/marquee se enciende en el panel en pocos milisegundos — además de un conjunto de 10 **temas de reloj** en pixel-art (Mario, Pac-Man, Tetris, Space Invaders, Pong...) y un pack incluido de unos **600 GIFs retro** para el modo de espera/atracción.

Es un fork de [RetroBoxLED de Jamyz](https://github.com/Jamyz/RetroBoxLED), reconstruido alrededor de un formato de píxeles propio, **raw565**, y una **caja de herramientas de PC (GUI de Windows)** para resolver un problema concreto: en colecciones grandes (fullset MAME, FBNeo...), el firmware original en PNG/GIF terminaba congelándose o mostrando pantalla negra varios segundos entre cada juego. Esta edición, no.

|                          | PNG/GIF original | **RecalBoxDMD RawEdition** |
|--------------------------|--------------------|----------------------------------|
| Tiempo de visualización por juego | 500 ms – 3 s+     | **5 – 15 ms**                    |
| RAM necesaria en el ESP32 | 50-100 KB         | **8 KB**                         |
| Fullset MAME (30 000 juegos) | congela 5-10 s      | **sin congelación, sin pantalla negra** |
| Puesta en marcha           | manual, imagen por imagen | **caja de herramientas de PC, un clic** |

> ### 🚀 Lo esencial: un clic construye toda la tarjeta SD
>
> Apunta la **caja de herramientas de PC** a tu carpeta de ROMs y pulsa **Iniciar** (**Modo 1 — AUTO**). Encadena todo por sí sola — detección de la versión de Recalbox, extracción del gamelist, conversión raw565, caché de bigramas, imágenes por defecto, scripts de Recalbox — hasta obtener una tarjeta SD lista para usar, y luego se ofrece a copiarla en tu tarjeta. **Inserta esa tarjeta SD en el DMD, enciéndelo, y ya está.** Sin configuración manual archivo por archivo, nunca.

---

## Índice

1. [¿Qué es esto?](#qué-es-esto)
2. [Características clave](#características-clave)
3. [Cómo funciona](#cómo-funciona)
4. [Capturas de pantalla](#capturas-de-pantalla--la-caja-de-herramientas-de-pc)
5. [Inicio rápido](#inicio-rápido)
6. [Hardware](#hardware)
7. [Caja de herramientas de PC — referencia de modos](#caja-de-herramientas-de-pc--referencia-de-modos)
8. [10 temas de reloj retro](#10-temas-de-reloj-retro)
9. [Widgets, ambientes y nuevos relojes](#widgets-ambientes-y-nuevos-relojes)
10. [El pack de 600 GIFs](#el-pack-de-600-gifs)
11. [Firmware — compilar y flashear](#firmware--compilar-y-flashear)
12. [Configuración (`config.ini`)](#configuración-configini)
13. [Configuración web — en vivo, en el navegador](#configuración-web--en-vivo-en-el-navegador)
14. [UDP y Telnet](#udp-y-telnet)
15. [Superposiciones en juego — Hi-Score, Info del juego, Logros y Challenge RB](#superposiciones-en-juego--hi-score-info-del-juego-logros-y-challenge-rb)
16. [Mesas de Visual Pinball (VPX) en el DMD](#-mesas-de-visual-pinball-vpx-en-el-dmd)
17. [El formato raw565 en detalle](#el-formato-raw565-en-detalle)
18. [Estructura de la tarjeta SD](#estructura-de-la-tarjeta-sd)
19. [Estructura del repositorio](#estructura-del-repositorio)
20. [Solución de problemas](#solución-de-problemas)
21. [Créditos y licencia](#créditos-y-licencia)

---

## Características clave

<table>
<tr><th width="42%" align="left">🎬 Lista de GIF y marquee</th><th width="58%" align="left">🧩 Widgets y pantallas vivas</th></tr>
<tr><td valign="top">

- ⚡ **Motor raw565** — 5–15 ms por juego, 8 KB de RAM, incluso con un fullset MAME de 30 000 juegos (sin bloqueos ni pantalla negra).
- 🖼️ **Marquees fijos *y* animados**, por juego o por sistema; imágenes de reserva (4 incluidas, o la tuya).
- 🎯 **Sistema de máscara** para colecciones enormes: una imagen por defecto en caché se muestra mientras se decodifica la real.
- 🧮 **Caché bigrama** — búsquedas casi instantáneas, sin listar carpetas en ejecución.
- 📦 **~600 GIF retro gratis** y una **pestaña Lista** para crear tus propias rotaciones attract.
- 🏆 **Overlays en juego** — Hi-Score, Info del juego, RetroAchievements y el Desafío mensual de Recalbox, alternando con el marquee mientras juegas.
- 🎨 **El DMD sigue tu tema de Recalbox** — los logos de sistemas cambian con el tema, el idioma y la región, al vuelo.
- 🕹️ **10 temas de reloj pixel-art** (Mario, Pac-Man, Tetris, Space Invaders…).
- 🖥️ **Caja de herramientas de PC en un clic** (FR/EN/ES) que construye toda la tarjeta SD; **Web Installer** para flashear el ESP32 desde el navegador.

</td><td valign="top">

Cada pantalla se dibuja sobre un **cielo que sigue el amanecer/atardecer real de tu lugar y el tiempo en directo** (sol, luna, nubes, lluvia, nieve, tormenta).

- 📅 **Fecha** — una tarjeta de calendario con el día de la semana; domingos y festivos en color.
- 🌦️ **Tiempo** — temperatura, flechas mín/máx, los nombres largos de lugar se desplazan de ida y vuelta.
- 🌅 **Efemérides** — salida, puesta, duración del día y **fase de la luna**.
- ⏳ **Cuenta atrás** — días que faltan para tus eventos (Navidad, cumpleaños…).
- 📊 **Estadísticas Recalbox** — partidas de hoy / totales, juego y sistema más jugados.
- 💬 **Citas y quiz** — los temas que elijas, **fondos ilustrados** para los quiz, la respuesta correcta parpadea y se queda en pantalla.

<p><img src="medias/widgets/es/date.png" width="215" alt="date"> <img src="medias/widgets/es/weather.png" width="215" alt="weather"> <img src="medias/widgets/es/ephemeris.png" width="215" alt="ephemeris"> <img src="medias/widgets/es/countdown.png" width="215" alt="countdown"> <img src="medias/widgets/es/stats.png" width="215" alt="stats"> <img src="medias/widgets/es/quote.png" width="215" alt="quote"></p>

- 🌀 **8 ambientes** — plasma, lluvia, estrellas, juego de la vida, líquido, arena, **arena 2** (capas de dos colores) y **lámpara de lava**; los colores los eliges tú.

<p><img src="medias/widgets/amb_plasma.png" width="105" alt="plasma"> <img src="medias/widgets/amb_rain.png" width="105" alt="rain"> <img src="medias/widgets/amb_stars.png" width="105" alt="stars"> <img src="medias/widgets/amb_life.png" width="105" alt="life"> <img src="medias/widgets/amb_liquid.png" width="105" alt="liquid"> <img src="medias/widgets/amb_sand1.png" width="105" alt="sand1"> <img src="medias/widgets/amb_sand2.png" width="105" alt="sand2"> <img src="medias/widgets/amb_lava.png" width="105" alt="lava"></p>

- 🕰️ **5 nuevos relojes animados** — la hora sobre **Plasma**, **Life**, **Stars**, **Rain** o **Lava**, además de los 10 temas pixel-art.

<p><img src="medias/widgets/clk_plasma.png" width="105" alt="plasma"> <img src="medias/widgets/clk_life.png" width="105" alt="life"> <img src="medias/widgets/clk_stars.png" width="105" alt="stars"> <img src="medias/widgets/clk_rain.png" width="105" alt="rain"> <img src="medias/widgets/clk_lava.png" width="105" alt="lava"></p>

🕹️ **Tres widgets Recalbox**, alimentados por un pequeño script instalado en tu Recalbox:

  - **Desafío** — desafío en curso, días restantes, **top 3 con movimientos ▲▼**, luego tu puesto y tus GG.
  - **Recompensas** — lo que puedes ganar (avatar, insignia, título) y cómo.
  - **Salud** — luces de temperatura, alimentación, reloj, red, disco, memoria + cifras clave.

<p><img src="medias/widgets/es/rb_challenge_top.png" width="215" alt="challenge"> <img src="medias/widgets/es/rb_challenge_me.png" width="215" alt="rank"> <img src="medias/widgets/es/rb_rewards_1.png" width="215" alt="rewards"> <img src="medias/widgets/es/rb_health.png" width="215" alt="health"></p>

</td></tr>
</table>

### ✨ Nuevo para todo

- 🔁 **Varias Recalbox, un solo DMD** — lista hasta 4 IP: cuando la Recalbox activa se apaga, el DMD **cambia solo** a la siguiente que responde (o elige un destino en la página web). La caja de herramientas de PC también acepta varias IP.
- 🔘 **Botón físico** (GPIO 21) — pulsación corta, doble, larga y muy larga, cada una asociada a la acción que elijas (GIF siguiente, reloj, pausa, brillo, pantalla de info, reinicio…) desde una página web.
- 🎮 **Modo DMD nativo de Recalbox y Visual Pinball** — EmulationStation puede manejar el panel directamente; las mesas de pinball muestran su DMD.
- ⬆️ **Actualizaciones desde GitHub** directamente en la caja de herramientas de PC, y un **`config.ini` más seguro** (escrituras atómicas, protección contra guardar una página vacía).

<sub>Las imágenes de widgets son **renders simulados** del panel 128×32, dibujados con las propias rutinas de dibujo del firmware.</sub>

<p align="center"><img src="medias/marketing/plaquette_6_themes_recalbox_es.png" alt="Anuncio: el DMD sigue el tema de tu Recalbox" width="720"></p>
<p align="center"><img src="medias/marketing/plaquette_7_recalbox11_natif_es.png" alt="Anuncio: Recalbox 11, modo nativo" width="720"></p>
<p align="center"><img src="medias/marketing/plaquette_8_widgets_es.png" alt="Anuncio: widgets, ambientes y nuevos relojes" width="720"></p>

---

## Cómo funciona

```
┌─────────────────────────────────────────────────────────────┐
│                          RECALBOX                             │
│   Lanza un juego → marquee[...].sh envía "mame/kof98"          │
│                           vía UDP                           │
└──────────────────────────────┬────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                 ESP32 + Panel LED HUB75 128×32                   │
│                                                                 │
│  Recibe "mame/kof98":                                            │
│   1. /systems/mame/kof98.raw565 (o .raw565pack)  → instantáneo  │
│   2. ¿no está? busca en games_cache.bin (índice de bigramas)    │
│   3. ¿sigue sin estar? muestra /systems/_defaults/mame.raw565   │
│   4. ¿sigue sin estar? muestra /systems/_defaults/default.raw565│
│                                                                 │
│   ⏱️  5-15 ms en total, sea cual sea el tamaño de la colección  │
└─────────────────────────────────────────────────────────────┘

           ┌───────────────────────────────────────────────┐
           │   RecalBoxDMD Toolkit  (prepara la tarjeta SD)  │
           │  Extrae las marquees desde gamelist.xml          │
           │  PNG → .raw565   /   GIF → .raw565pack + .meta   │
           │  Construye la caché de juegos por bigramas        │
           │  Marca los sistemas lentos ("L") para la máscara  │
           │  Descarga recursos gratuitos (_defaults + 600 GIFs)│
           │  Copia todo a la tarjeta SD (reanudable)          │
           └───────────────────────────────────────────────┘
```

---

## Capturas de pantalla — la caja de herramientas de PC

La herramienta incluye 9 temas visuales (SNES, Mega Drive, Dreamcast, PlayStation, N64, Neo Geo, Game Boy, Atari 2600, Aleatorio) además de su interfaz FR/EN/ES — algunos ejemplos:

| Pestaña Main (inglés · tema SNES) | Configuración — idioma y tema (inglés · tema Dreamcast) |
|---|---|
| ![Pestaña Main, inglés, tema SNES](medias/screenshots/gui_en_main_snes.png) | ![Pestaña Configuración, inglés, tema Dreamcast](medias/screenshots/gui_en_settings_dreamcast.png) |

| Pestaña Main (francés · tema Mega Drive) | Pestaña Playlist (francés · tema Neo Geo) |
|---|---|
| ![Pestaña Main, francés, tema Megadrive](medias/screenshots/gui_fr_main_megadrive.png) | ![Pestaña Playlist, francés, tema Neo Geo](medias/screenshots/gui_fr_playlist_neogeo.png) |

| Pestaña Main (español · tema PlayStation) | Pestaña Avanzado (español · tema Atari 2600) |
|---|---|
| ![Pestaña Main, español, tema PlayStation](medias/screenshots/gui_es_main_playstation.png) | ![Pestaña Avanzado, español, tema Atari 2600 — Modo 11, pack 600 GIFs](medias/screenshots/gui_es_advanced_atari2600.png) |

---

## Inicio rápido

*(También disponible como página independiente: [QUICK-START.md](QUICK-START.es.md))*

<p align="center"><b>🚀 De cero a un marquee funcionando en 4 pasos 🚀</b></p>

🎬 *¿Prefieres verlo? El [vídeo](https://youtu.be/xF8O88XXSwU) cubre la presentación, el montaje y la instalación.*

<table align="center">
<tr>
<td align="center" width="70"><h2>1️⃣</h2></td>
<td>

**[Instala la caja de herramientas de PC](#instala-la-caja-de-herramientas-de-pc) + primer arranque**
Haz el scrape de tus juegos en Recalbox, apunta la herramienta a tu carpeta de ROMs, pulsa **Iniciar**.

</td>
</tr>
<tr>
<td align="center"><h2>2️⃣</h2></td>
<td>

**[Monta el DMD](#hardware)**
Une los dos paneles, coloca la placa DMDos, cablea — **~5 minutos, sin soldadura**.

</td>
</tr>
<tr>
<td align="center"><h2>3️⃣</h2></td>
<td>

**[Flashea el firmware](#firmware--compilar-y-flashear)**
Instalador web en un clic — **sin necesidad de Arduino IDE**.

</td>
</tr>
<tr>
<td align="center"><h2>4️⃣</h2></td>
<td>

**Inserta la tarjeta SD, enciende**
El primer arranque te guía por la configuración WiFi, y luego la **[página de configuración web](#configuración-web--en-vivo-en-el-navegador)** se encarga de todo lo demás (brillo, playlists, temas de reloj...).

</td>
</tr>
</table>

### Instala la caja de herramientas de PC

Se descarga de 4 formas — elige la que prefieras en la **[página de Releases](https://github.com/shan-aya/RecalBoxDMD/releases)** (los archivos `.exe`/`.msi` compilados no están en el propio repositorio, solo publicados ahí):

**Opción A — Instalador de Windows (recomendado)**

```
1. Descarga RecalBoxDMD_Toolkit_Setup.exe desde la página de Releases
2. Ejecútalo — acceso directo en el menú Inicio, icono de escritorio opcional, desinstalador real
3. Abre «RecalBoxDMD Toolkit» desde el menú Inicio
```

**Opción B — Ejecutable portable (sin instalación)**

```
1. Descarga RecalBoxDMD_GUI.exe desde la página de Releases
2. Ejecútalo directamente — sin instalación, sin necesidad de Python, archivo único
```

**Opción C — .msi (para despliegue mediante script/GPO)**

```
1. Descarga el .msi desde la página de Releases
2. msiexec /i "RecalBoxDMD Toolkit-1.0.0-win64.msi"   (o doble clic)
```

**Opción D — Desde el código fuente Python**

```
1. Descarga la carpeta tools/
2. Haz doble clic en install_and_run.bat — instala Python (vía winget,
   si falta), Pillow y Markdown, y luego abre la GUI
   (o manualmente: pip install Pillow Markdown && python run_gui.py)
```

### Primer arranque

```
1. Haz el scrape de tus juegos en Recalbox (ver «¿Cómo hacer el scrape?» en la
   herramienta, según tu versión de Recalbox — logo, marquee o logo recortado)
2. Abre la caja de herramientas → pestaña Main
3. Elige tu versión de Recalbox (10.x / 9.x / legacy)
4. Elige tu carpeta de ROMs (ej.: D:\Recalbox\share\roms)
5. Haz clic en Iniciar — el MODO 1 encadena todo el proceso automáticamente
6. Inserta la tarjeta SD → el botón parpadeante se ofrece a copiarla por ti
```

A continuación: [monta el hardware](#hardware) y [flashea el firmware](#firmware--compilar-y-flashear) — luego inserta esa tarjeta SD y enciende.

---

## Hardware

| Componente | Referencia | Precio aprox. |
|-----------|-----------|----------------|
| 🧠 Microcontrolador | ESP32 DevKit V1 USB-C (38 pines) | ~5 € |
| 🖥️ Panel LED | 2× paneles HUB75 RGB **P4, 64×32, 256×128 mm**, unidos uno junto al otro (→ 128×32) | ~15-25 €/panel |
| 🔌 Placa de conexión | **DMDos Board V3** (recomendada — incluye el lector SD, sin soldadura) | ~15 € |
| 💾 Lector SD | Módulo adaptador Micro SD SPI (integrado en la DMDos Board) | ~2 € |
| ⚡ Fuente de alimentación | 5V 4A+ | ~10 € |

<p align="center">
  <img src="medias/marketing/plaquette_5_materiel_montage.png" alt="Hardware y montaje" width="720">
</p>

El montaje físico (paneles + placa DMDos + ESP32 + microSD) es idéntico al descrito en el sitio oficial **[dmdos.net](https://www.dmdos.net/)** de Mortaca — realmente rápido, sin soldadura, sin más herramienta que un destornillador:

1. **Une los dos paneles.** Usa las piezas de unión que vienen con la placa DMDos. Los tornillos no están incluidos — cualquier tornillo M3 que tengas en casa sirve (por ejemplo, recuperado de una regleta).
2. **Coloca la placa DMDos.** Una vez unidos, mantén la orientación de los componentes traseros igual en ambos lados. Verás dos conectores idénticos: uno de **entrada**, otro de **salida**. La placa solo funciona en el lado de **entrada** — elige la orientación que despeje fácilmente el soporte de plástico.
3. **Cablea la alimentación.** Antes de colocar el ESP32 encima, conecta los cables rojo/negro de alimentación de cada panel a los bornes de la placa según la serigrafía (rojo↔rojo, negro↔negro) — puedes conservar el conector suministrado y atornillar solo un pin, o pelar/recortar el cable para que encaje directamente en el borne. Une los dos paneles entre sí con el cable plano incluido.
4. **Tarjeta SD, ESP32, alimentación.** Inserta la tarjeta SD que preparaste con la caja de herramientas de PC (ver [Inicio rápido](#inicio-rápido)), conecta el ESP32 ya flasheado con el firmware RecalBoxDMD (ver [Firmware](#firmware--compilar-y-flashear)) encima de la placa, y alimenta todo por el puerto USB-C del ESP32.

<p align="center">
  <a href="https://www.dmdos.net/#montaje" title="Guía ilustrada completa en dmdos.net"><img src="medias/assembly/1_union.png" width="220" alt="Paso 1 — unión de los dos paneles"></a>
  <a href="https://www.dmdos.net/#montaje" title="Guía ilustrada completa en dmdos.net"><img src="medias/assembly/2_posicion.png" width="220" alt="Paso 2 — placa DMDos colocada en el conector de entrada"></a>
  <a href="https://www.dmdos.net/#montaje" title="Guía ilustrada completa en dmdos.net"><img src="medias/assembly/3_cableado.png" width="220" alt="Paso 3 — cableado de alimentación y cable plano"></a>
  <a href="https://www.dmdos.net/#montaje" title="Guía ilustrada completa en dmdos.net"><img src="medias/assembly/4_final.png" width="220" alt="Paso 4 — ESP32 montado, listo para alimentar"></a>
</p>
<p align="center"><sub>Las miniaturas enlazan a la guía oficial paso a paso en dmdos.net</sub></p>

📖 **Guía oficial ilustrada**: [dmdos.net → Hardware](https://www.dmdos.net/#hardware) · [dmdos.net → Montaje/Assembly](https://www.dmdos.net/#montaje) · [dmdos.net → Mueble/Frame](https://www.dmdos.net/#mueble)

> ⚠️ El sitio web de DMDos ofrece su propio firmware/sistema operativo, independiente. **No flashees el firmware de DMDos** si quieres usar RecalBoxDMD — solo se reutilizan el **hardware** (paneles, placa, marco) y la **guía de montaje**; el firmware y el contenido de la tarjeta SD provienen de este repositorio.

Marco imprimible en 3D por **Janibol** ([Retromojones](https://www.youtube.com/@retromojones)) en [Thingiverse](https://www.thingiverse.com/thing:6704880). Enlaces de compra actualizados: 🇫🇷 **Francia**: [Benfactory.fr](https://benfactory.fr/produit/dmdos/) — placa DMDos + ESP32, código promocional **`retrobat10`** (−10 %) · 🇪🇸 **España**: [dmdos.net](https://www.dmdos.net/).

---

## Caja de herramientas de PC — referencia de modos

La pestaña **Avanzado** de la GUI agrupa cada operación en 5 categorías plegables; el **Modo 1** de la pestaña **Main** las encadena todas por ti.

| Modo | Categoría | Nombre | Qué hace |
|------|-----------|--------|-----------|
| **1** | *(pestaña Main)* | **AUTO — todo** | Detección de versión Recalbox → extracción de gamelist → conversión raw565 → caché de bigramas → descarga de `_defaults` → instalación de scripts Recalbox → copia a la SD |
| 2 | 📥 GitHub | Descargar `_defaults` | Obtiene las imágenes de respaldo por defecto de cada sistema conocido |
| 11 | 📥 GitHub | **Pack 600 GIFs** | Descarga en un clic de la colección gratuita de GIFs (Arcade, Consolas, Ordenadores, Pinball, Halloween, Navidad, Logo y más) |
| 3 | 🗂️ Gamelist | Solo extracción | Lee `gamelist.xml`, copia la marquee/logo correcta según tu perfil de versión de Recalbox |
| 8 | 🗂️ Gamelist | Verificación de imágenes faltantes | Informa, por sistema/juego, si la imagen esperada realmente existe (ROMs / carpeta de trabajo / tarjeta SD) |
| 4 | 🖼️ Imágenes | Conversión raw565 | PNG → `.raw565`, GIF → `.raw565pack` + `.meta` |
| 5 | 🖼️ Imágenes | Redimensionado 128×32 | Redimensiona los PNG a la resolución del panel (formato imagen, sin conversión raw565) |
| 10 | 🖼️ Imágenes | Imagen de respaldo | Define/genera la imagen por defecto global que se muestra cuando nada más coincide |
| 6 | 🧮 Cachés | Caché de juegos | Construye `games_cache.bin` (índice de bigramas, 703 entradas) |
| 7 | 🧮 Cachés | Caché de sistemas | Construye `systems_cache.dat` (índice de sistemas + flags lento/rápido **«L»/«N»**) |
| 9 | 📜 Scripts | Instalar scripts de Recalbox | Copia los scripts de marquee, recuperación WiFi, config web, brillo y [Hi-Score/Info del juego/Logros/Challenge](#superposiciones-en-juego--hi-score-info-del-juego-logros-y-challenge-rb) directamente al recurso compartido de red de la Recalbox, limpiando de paso los nombres de scripts antiguos de una instalación anterior |

Herramientas adicionales disponibles desde cada modo correspondiente: **«¿Cómo hacer el scrape?»** (capturas anotadas y específicas de tu versión de la pestaña Scraper de Recalbox), **«Limpiar carpetas antes del scrape»**, una **pestaña Playlist** para construir playlists de GIFs desde una tarjeta SD o carpetas del PC, un **umbral de sistema lento** ajustable (pestaña Configuración, 5000 archivos convertidos por defecto), y una **copia a la SD reanudable** que resiste una desconexión/fallo y puede reintentar solo los archivos fallidos.

---

## 10 temas de reloj retro

Se muestran periódicamente entre juegos (intervalo/duración configurables) o a tiempo completo; cada tema es una escena pixel-art hecha a mano, seleccionable desde la web, con una **vista previa en vivo instantánea enviada al panel físico** en cuanto eliges una.

<p align="center">
  <img src="medias/clock_themes/00_super_mario.gif" width="180" alt="Super Mario"> <img src="medias/clock_themes/01_tetris.gif" width="180" alt="Tetris">
  <img src="medias/clock_themes/02_pac_man.gif" width="180" alt="Pac-Man"> <img src="medias/clock_themes/03_space_invaders.gif" width="180" alt="Space Invaders">
</p>
<p align="center">
  <img src="medias/clock_themes/04_pong.gif" width="180" alt="Pong"> <img src="medias/clock_themes/05_neon.gif" width="180" alt="Neon">
  <img src="medias/clock_themes/06_matrix.gif" width="180" alt="Matrix"> <img src="medias/clock_themes/07_fire.gif" width="180" alt="Fire">
</p>
<p align="center">
  <img src="medias/clock_themes/08_rainbow.gif" width="180" alt="Rainbow"> <img src="medias/clock_themes/09_level_1_1.gif" width="180" alt="Level 1-1">
</p>

Super Mario · Tetris · Pac-Man · Space Invaders · Pong · Neon · Matrix · Fire · Rainbow · Level 1-1 (con scroll).

### Imágenes de respaldo

Se muestran cuando un juego/sistema no tiene marquee propia. Se incluyen 4 de serie — o aporta la tuya desde el selector de imagen de respaldo de la caja de herramientas de PC.

<p align="center">
  <img src="medias/fallback_images/default_RB.png" width="160" alt="Imagen de respaldo — Recalbox">
  <img src="medias/fallback_images/default_jamma.png" width="160" alt="Imagen de respaldo — Recalbox JAMMA">
  <img src="medias/fallback_images/default_rgbdual.png" width="160" alt="Imagen de respaldo — Recalbox RGB Dual">
  <img src="medias/fallback_images/default_rgbdual2.png" width="160" alt="Imagen de respaldo — Recalbox RGB Dual 2">
</p>

---

## Widgets, ambientes y nuevos relojes

Desde el firmware **3.28**, el panel son dos mundos en uno: la **lista de GIF** que ya conoces y los **widgets** — pantallas vivas y animadas, dibujadas sobre un **cielo que sigue el amanecer/atardecer real de tu lugar y el tiempo en directo**. Los mezclas desde la página web **Widgets** (`/config/widgets`).

### Mezclar los dos
- Cada widget está **desactivado**, **en lugar del reloj** (se turna con él en la franja principal) o **además del reloj** (se reproduce después), con su duración en segundos.
- **Disparador**: cada **N GIF** o cada **N minutos**. Franja principal: **rotación** o **aleatorio**. Extras: **todos** o **N al azar**.
- **Fondo**: *lista de GIF* o **solo widgets** (no se lee ningún GIF: los widgets se encadenan sin parar). Un juego, una página web o un comando recupera la pantalla en cualquier momento.
- Cada widget tiene un botón **Vista previa** que lo reproduce ya en el panel; lanzar otra vista previa interrumpe la actual.

### Los widgets
| Widget | Lo que muestra | Necesita |
|---|---|---|
| 🕰️ **Reloj** | los 10 temas pixel-art + **5 nuevos relojes animados** (Plasma, Life, Stars, Rain, Lava), colores ajustables | — |
| 📅 **Fecha** | una tarjeta de calendario con el día de la semana; domingos y festivos en color | hora (NTP) |
| 🌅 **Efemérides** | salida, puesta, duración del día, **fase de la luna** | lugar |
| ⏳ **Cuenta atrás** | días que faltan para tus eventos (botón *Eventos*; anual o único) | hora |
| 🌀 **Ambiente** | 8 animaciones (abajo), 2 colores a elegir cuando tiene sentido | — |
| 🌦️ **Tiempo** | temperatura, ▲▼ mín/máx, condiciones; los nombres largos de lugar se desplazan de ida y vuelta | lugar + Internet (open-meteo.com, cada 30 min) |
| 💬 **Citas y quiz** | temas de citas, chistes, adivinanzas y **quiz de 3 opciones**, con fondos ilustrados | — |
| 🏆 **Recalbox: desafío** | desafío en curso, días restantes, **top 3 con movimientos ▲▼**, tu puesto y tus GG | script Recalbox |
| 🎁 **Recalbox: recompensas** | lo que puedes ganar (avatar, insignia, título) y cómo | script Recalbox |
| 🩺 **Recalbox: salud** | luces de temperatura, alimentación, reloj, red, disco, memoria + cifras clave | script Recalbox |

**Cielo**: el fondo de los widgets fecha, tiempo, efemérides y cuenta atrás sigue el **amanecer y atardecer reales** de tu lugar (página *Lugar*) y el tiempo en directo: sol animado, luna y estrellas, nubes, lluvia, nieve, tormenta. Los tres widgets Recalbox llevan en su lugar una **ilustración** de fondo.

### 8 ambientes
Plasma · lluvia · estrellas · juego de la vida · líquido (olas que se mecen) · arena · **arena 2** (capas de dos colores que fluyen como en el agua) · **lámpara de lava** (cera que se funde, sube, se estira, se pellizca y se fusiona). Los colores se ajustan para plasma, lluvia, estrellas, vida, líquido, arena 2 y lámpara de lava (aceite / cera).

### 5 nuevos relojes animados
La hora en grande sobre **Plasma**, **Life**, **Stars**, **Rain** (verde) o **Lava** — se suman a los 10 temas pixel-art en la página *Reloj* y en el sorteo aleatorio.

### Citas y quiz en detalle

**Temas listos para usar.** El **Modo 15** de la caja de herramientas (que también ofrece el **Modo 1**) descarga desde GitHub 15 temas de 50 entradas — citas célebres, galleta de la fortuna, chistes, adivinanzas y quiz de retrogaming, cada uno en francés, inglés y español — a la tarjeta SD (`widgets/quotes/<idioma>/`). Indica cuáles faltan, están al día, desactualizados o modificados por usted; un archivo modificado se guarda antes de ser reemplazado y sus propios temas nunca se tocan. La caja de herramientas avisa además al abrirse cuando se publica una nueva revisión.
Un **tema = un archivo de texto, una cita por línea**. Cuatro formatos de línea:

| Línea | Lo que muestra el panel |
|---|---|
| `Texto de la galleta` | el texto, centrado y ajustado |
| `Texto \| Autor` | la cita, luego el autor |
| `Pregunta ? \| Respuesta` | una adivinanza o un chiste: la respuesta se revela tras una pausa |
| `Pregunta ? \| A / B / C \| 3` | un **quiz de 3 opciones**: opciones en rojo / blanco / verde, una barra de cuenta atrás con los segundos, y la respuesta correcta parpadea y **se queda en pantalla** |

- **Temas integrados** por idioma (galletas de la fortuna, citas célebres, quiz retro-gaming; chistes en francés) — el botón *Restaurar* los recupera.
- **Crea los tuyos**: en la página *Widgets → Citas*, escribe un tema, edita o borra uno, o **importa un archivo `.txt`** (64 KB máx. por tema, acentos convertidos solos); o copia `.txt` en `/widgets/quotes/<idioma>/` de la tarjeta SD.
- **Fondos ilustrados**: una imagen 128×32 raw565 con el nombre del tema (`/widgets/quotes/bg/<tema>.raw565`, 8 192 bytes) pasa a ser su fondo; un archivo opcional `<tema>.col` ajusta los colores del texto, la sombra o el contorno, el brillo y la zona de texto. Los de origen están **integrados en el firmware** y se escriben en la SD si faltan.
- La **duración del widget es un número de citas** seguidas.

### Widgets Recalbox (desafío, recompensas, salud)
Un pequeño script en la Recalbox lee sus propios archivos (desafío en curso, clasificación, movimientos, líneas `[Health]` del registro, temperatura, memoria, disco) y envía un resumen compacto al DMD por UDP (`CMD=rbw`) **cada 2 minutos**. Se instala con los scripts de Recalbox (**Modo 9** de la caja de herramientas de PC): `dmd_rbwidgets[watch](permanent).sh` + `dmd_helpers/dmd_rbwidgets.py`. Mientras no lleguen datos, los widgets se omiten sin ruido. Duraciones por defecto: desafío 15 s, **recompensas 30 s** (8 s mínimo por recompensa, los textos largos se desplazan), salud 15 s.

### Varias Recalbox, un solo DMD
Lista hasta **4 IP de Recalbox** (página Wi-Fi: campo separado por comas, o `recalbox_ips=` en `config.ini`) y elige **Destino = Automático** o una Recalbox fija. El DMD se queda en la Recalbox que responde; cuando deja de responder (apagada), **sondea las demás y cambia solo** a la primera que responde — sin reconfigurar ni reiniciar. Los paquetes de la otra Recalbox de la lista se ignoran para que dos consolas no se mezclen. La caja de herramientas de PC (Modos 1 y 9, incluida la oferta de reinicio) también acepta varias IP.

### Botón físico
Cablea un pulsador entre **GPIO 21 y GND** (pull-up interno). La página **Botón** asigna una acción a cada gesto: pulsación **corta**, **doble**, **larga** (~1,5 s) y **muy larga** (~5 s) — GIF siguiente, pausa/reanudar, reloj ahora, brillo, pantalla apagada, pantalla de info, mostrar la URL de config, reiniciar… Las acciones de mantenimiento solo se ofrecen en la pulsación muy larga.

### Configuración más segura
`config.ini` se escribe ahora de forma **atómica** (archivo completo en `config.tmp`, luego intercambio; un corte de corriente deja intacto el archivo anterior y se repara en el siguiente arranque), se **rechaza** un guardado si una página envía campos Wi-Fi/lista vacíos, los ajustes Wi-Fi solo los aplica una página *Wi-Fi* cargada, y el guardado se **verifica** antes de responder *OK*.

<p align="center"><img src="medias/widgets/es/date.png" width="150" alt="date"> <img src="medias/widgets/es/weather.png" width="150" alt="weather"> <img src="medias/widgets/es/ephemeris.png" width="150" alt="ephemeris"> <img src="medias/widgets/es/rb_challenge_top.png" width="150" alt="rb_challenge_top"> <img src="medias/widgets/es/rb_rewards_1.png" width="150" alt="rb_rewards_1"> <img src="medias/widgets/es/rb_health.png" width="150" alt="rb_health"> <img src="medias/widgets/es/qcm_question.png" width="150" alt="qcm_question"></p>

---

## El pack de 600 GIFs

El **Modo 11** (o el botón «Pack 600 GIFs» de la pestaña Playlist) descarga una colección lista para reproducir de unos **600 GIFs retro**, organizada por categorías, directamente desde este repositorio (`carte SD/gifs/`) — sin sitios externos, sin cuentas:

| Categoría | Categoría | Categoría |
|---|---|---|
| Arcade | Consolas | Ordenadores |
| Pinball (corto) | Pinball (historia) | Logo |
| Halloween | Navidad | Otros / Suite de pruebas |

Apunta una playlist a cualquier subconjunto de estas carpetas (pestaña Playlist) para construir tu propia rotación de modo de espera — los GIFs animados se reproducen por la misma vía rápida `.raw565pack` que las marquees de juegos, así que la reproducción se mantiene fluida incluso en el ESP32.

> ℹ️ Una categoría (`XXX_Mature`) contiene pixel-art de temática adulta para quien lo quiera en su propio mueble — totalmente opcional y nunca seleccionada por defecto.

### ¿De dónde viene este pack?

Estos 600 GIFs son la **muestra gratuita** de la colección de animaciones «pixel perfect» para relojes DMD de **eLLuiGi** (RpiTeaM) — más de 4 años de trabajo de curación, redistribuida aquí con permiso para instalarla en un clic, sin sitios de terceros ni cuentas.

La colección completa va mucho más allá: el **«ULTIMATE GIFS DLC»** reúne unas **11 000 animaciones pixel perfect** (1441 Arcade, 3601 Consolas, 849 Ordenadores, más Pinball/Halloween/Navidad/Logo...). No está alojada en este repositorio — es el pack de pago del propio creador, consíguelo directamente aquí:

- 🔗 **Portal de RpiTeaM**: [rpiteam.carrd.co](https://rpiteam.carrd.co/)
- 🔗 **Hilo del foro (detalles y acceso)**: [neo-arcadia.com — «ULTIMATE GIFS DLC»](https://www.neo-arcadia.com/forum/viewtopic.php?t=67065)

Cualquier GIF funciona igual sea cual sea su origen — pero añade siempre los packs adicionales a través de la **pestaña Playlist de la caja de herramientas de PC** (apuntando a la carpeta en tu PC) o de la **página Medios de la configuración web** (subida), nunca copiando archivos directamente en la tarjeta SD: eso es lo que reconstruye la playlist y la caché de GIFs que el firmware realmente lee. Los archivos copiados directamente en la SD fuera de esas dos vías no aparecerán hasta que lo hagas.

---

## Firmware — compilar y flashear

### 🌐 Opción A — Flashear desde el navegador (lo más sencillo, sin instalar nada)

> [👉 **Abrir el instalador web de RecalBoxDMD**](https://shan-aya.github.io/RecalBoxDMD/install/)

Con **Chrome o Edge**, conecta el ESP32 por USB, haz clic en **Instalar**, elige el puerto COM, y listo en un minuto aproximadamente — nada que instalar en tu PC, sin Arduino IDE. Flashea el último firmware precompilado directamente desde [`binaries/`](binaries/) usando [ESP Web Tools](https://esphome.github.io/esp-web-tools/). Marca **«Erase device»** en una primera instalación (o al venir de otro firmware, p. ej. DMDos) para borrar por completo la memoria flash antes.

### 🛠️ Opción B — Arduino IDE (para compilar desde el código fuente / personalizar)

1. Abre `RecalBox_DMD.ino` en el **Arduino IDE**.
2. Instala estas bibliotecas (Programa → Incluir Librería → Gestionar Bibliotecas):

| Biblioteca | Utilidad |
|---|---|
| [ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) | Control DMA del panel LED |
| [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF) | Decodificación de GIF (ruta de respaldo) |
| [pngle](https://github.com/kikuchan/pngle) | Decodificación de PNG (ruta de respaldo, incluida en el sketch) |
| [WiFiManager](https://github.com/tzapu/WiFiManager) | Configuración WiFi |
| [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) | Renderizado de texto/formas |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | (De)serialización de la configuración y la web |

3. Herramientas → Placa: **ESP32 Dev Module**, Tamaño de flash **4 MB**, Esquema de particiones **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** (dos ranuras de firmware, necesarias para las actualizaciones por Wi-Fi).
4. Selecciona el puerto COM correcto y pulsa **Subir**.

### ⌨️ Opción C — `esptool.py` (línea de comandos)

Los mismos binarios precompilados que usa el instalador web (bootloader/particiones/app/imagen fusionada) están en [`binaries/`](binaries/):

```bash
esptool.py --chip esp32 --port COM3 --baud 921600 write_flash -z 0x10000 RecalBox_DMD.ino.bin
# o, flasheo en un solo archivo:
esptool.py --chip esp32 --port COM3 write_flash 0x0 RecalBox_DMD.ino.merged.bin
```

### Asignación de pines (por defecto)

| Tarjeta SD (SPI) | GPIO |  | HUB75 | GPIO |  | HUB75 | GPIO |
|---|---|---|---|---|---|---|---|
| CS | 5 | | CLK | 16 | | R1 / R2 | 25 / 14 |
| MOSI | 23 | | OE | 15 | | G1 / G2 | 26 / 12 |
| MISO | 19 | | LAT | 4 | | B1 / B2 | 27 / 13 |
| SCLK | 18 | | A/B/C/D | 33 / 32 / 22 / 17 | | E | -1 |

---

## Configuración (`config.ini`)

No hace falta escribir ni copiar este archivo a mano: se crea automáticamente — bien por la **caja de herramientas de PC** (el Modo 1 lo escribe al final del proceso), bien por el propio **ESP32**, que ofrece su propia página de configuración WiFi en el primer arranque o cuando no consigue conectarse. A partir de ahí, cada valor de abajo se edita en vivo desde la **página de configuración web** (siguiente sección) — sin tener que tocar la tarjeta SD. Como referencia, esto es lo que contiene:

```ini
# Info
info=1                        # 0 = sin info al arrancar, 1 = mostrar info al arrancar

# Pantalla
brightness=40                 # brillo del panel 0-100 %

# Playlist
playlist=RecalBox_intros.txt  # se reproduce desde /playlist
random=1                      # 0 = secuencial, 1 = aleatorio

# Wi-Fi
wifi_enabled=1
wifi_ssid=mi_wifi
wifi_password=mi_contraseña
wifi_static_enabled=1
wifi_static_ip=192.168.1.240
wifi_gateway=192.168.1.1
wifi_subnet=255.255.255.0

# Enlace con Recalbox (UDP)
recalbox_ip=192.168.1.104     # la Recalbox ACTIVA (elegida por el DMD entre recalbox_ips)
recalbox_ips=192.168.1.104,192.168.1.105   # hasta 4 Recalbox; el DMD cambia a la primera que responde
recalbox_target=0             # 0 = automático, 1..4 = fuerza esta Recalbox

# Pinball (VPX) y reposo de Recalbox
feat_vpinball_dmd=0           # 1 = mostrar las mesas de Visual Pinball (VPX) en directo (se aplica al próximo reinicio del DMD)
feat_demo_follow=1            # 1 = seguir el logo del juego durante el reposo «demos de juegos», 0 = playlist simple
feat_clip_follow=1            # ídem para el reposo «clips de vídeo de juegos»

# Reloj (temas de reloj retro)
[CLOCK]
CLOCK_ENABLED=1
CLOCK_THEME=-1                # -1=aleatorio, 0=Mario ... 9=Level 1-1
CLOCK_INTERVAL=5              # número de GIFs antes de mostrar el reloj
CLOCK_DURATION=60             # segundos que el reloj permanece en pantalla
TZ=CET-1CEST,M3.5.0,M10.5.0/3

# Botón físico (página /config/button)
[BUTTON]
BTN_ENABLED=1                 # GPIO 21 a GND
BTN_ACT_SHORT=11              # números de acción: ver la página Botón
BTN_ACT_DOUBLE=6
BTN_ACT_LONG=2
BTN_ACT_VLONG=10

# Widgets (página /config/widgets) -- WIDGET_MODE_n: 0=off 1=en lugar del reloj 2=además ; WIDGET_DUR_n en segundos
[WIDGETS]
WIDGET_BG=0                   # 0 = lista de GIF, 1 = solo widgets
WIDGET_TRIGGER=0              # 0 = cada N GIF, 1 = cada N minutos
WIDGET_AMB=8                  # estilo de ambiente (1 plasma ... 8 lámpara de lava)
WIDGET_LAT=43.3852
WIDGET_LON=5.7081
WIDGET_PLACE=Casa
WIDGET_MODE_8=2               # 8 desafío, 9 recompensas, 10 salud (Recalbox)
WIDGET_DUR_9=30
```

---

## Configuración web — en vivo, en el navegador

Escribe la IP del ESP32 (mostrada al arrancar, o visible en el propio panel) en el navegador de un móvil o PC: obtienes un sitio de configuración completo, dividido en páginas de carga rápida, trilingüe (FR/EN/ES), con ayuda integrada — sin apps, sin recompilar.

**🏠 Página de inicio** — el menú principal: un botón por página, **⚙ Ajustes** primero. Nada que configurar aquí.

**⚙ Ajustes** — brillo del panel con una **vista previa en vivo enviada directamente al panel físico** mientras mueves el control deslizante, arranque silencioso o normal, el interruptor **Modo Pinball (VPX)**, las alertas RetroAchievements, el **modo Recalbox nativo** y la **versión del firmware** con su botón *Actualizar*. Su botón Guardar confirma en la línea 2 del DMD. En cualquier página, un **banner en la parte superior** anuncia un firmware o un lote de scripts de Recalbox más reciente (para los scripts: ejecuta la caja de herramientas de PC, Modo 9); la comprobación la hace tu navegador, el DMD no necesita acceso a Internet.

**💡 Pantalla y listas** — opciones de las superposiciones en juego, **reposo de Recalbox** (durante los salvapantallas «demos de juegos» / «clips de vídeo de juegos», o bien seguir el logo del juego como antes, o bien mantener la playlist simple como en los demás reposos), el ajuste **tema de Recalbox** (seguir el tema activo para los logos de sistema, o mostrar siempre los logos por defecto), playlist por defecto + reproducción aleatoria, y gestión de playlists (crear una nueva playlist directamente desde las carpetas de GIFs ya presentes en la SD, editar o borrar las existentes — para carpetas con muchos archivos, usa mejor la caja de herramientas de PC, pensada para eso).

<p align="center"><img src="medias/screenshots/webconfig_display_playlists.png" alt="Configuración web — página Pantalla y listas" width="420"></p>

**📶 Wi-Fi** — escaneo y selección de red, contraseña, IP estática (puerta de enlace/máscara/DNS), y la IP de Recalbox usada para el enlace UDP.

<p align="center"><img src="medias/screenshots/webconfig_wifi_bluetooth.png" alt="Configuración web — página Wi-Fi y Bluetooth" width="420"></p>

**⏰ Reloj** — activar/desactivar, selector de tema con una **vista previa en vivo instantánea enviada al panel físico** mientras la página permanece abierta, color neón personalizado, intervalo en número de GIFs o en minutos, duración en pantalla, y zona horaria.

<p align="center"><img src="medias/screenshots/webconfig_clock.png" alt="Configuración web — página Reloj" width="420"></p>

**✨ Widgets y Reloj** — la mezcla (estado y duración de cada widget, disparador, orden, fondo), estilo y colores de ambiente, tema de citas, lugar del cielo, eventos de la cuenta atrás, **Vista previa** de cada widget en el panel.

**💬 Citas** — crear, editar, importar o borrar los temas de citas/quiz (uno por idioma).

**🔘 Botón** — la acción de cada gesto del botón físico.

**📶 Wi-Fi** — el campo *IP de Recalbox* acepta ahora **varias IP** (separadas por comas) y un selector de **Recalbox destino** (*Automático* o 1–4); se muestra la Recalbox activa.

**💿 Medios** — explora y borra carpetas de GIFs directamente en la tarjeta SD, y sube GIFs uno a uno desde el navegador (cómodo para pocos archivos; para transferencias masivas, usa la caja de herramientas de PC).

<p align="center"><img src="medias/screenshots/webconfig_media.png" alt="Configuración web — página Medios" width="420"></p>

---

## UDP y Telnet

```
Recalbox → marquee[rungame,endgame,...].sh → UDP → ESP32 → Panel LED

1. Lanzas "King of Fighters '98"
2. El script bash de usuario detecta el evento → publica "mame/kof98"
3. El ESP32 busca, en orden:
   a. /systems/mame/kof98.raw565 (o .raw565pack)   ← instantáneo
   b. índice de bigramas de games_cache.bin          ← acelerado
   c. /systems/_defaults/mame.raw565                 ← respaldo de sistema
   d. /systems/_defaults/default.raw565               ← respaldo global
4. Se muestra en menos de 15 ms
```

Instala el script de usuario con el **Modo 9** de la caja de herramientas, o copia `marquee[...].sh` manualmente a `/recalbox/share/userscripts/`.

### 🎮 Control manual desde el menú de Recalbox

El **Modo 9** también instala scripts que puedes lanzar a mano desde Recalbox (**START → Configuración avanzada → Scripts de usuario**), sin tocar la tarjeta SD:

| Script | Efecto |
|---|---|
| **WiFi Recovery DMD** | Vuelve a poner el DMD en modo punto de acceso (AP) de emergencia para reconfigurar el WiFi. |
| **Config Web DMD** | Abre la página de configuración web del DMD y muestra su IP en Recalbox para acceder directamente. |
| **Reboot DMD** | Reinicia el DMD de forma remota. |
| **Luminosité DMD +10% / -10%** | Ajusta el brillo de la pantalla en pasos de 10 puntos porcentuales (limitado 0-100%), aplicado al instante y guardado en `config.ini`. |

Todos usan el mismo canal UDP que el puente marquee, sin interrumpir nunca lo que se muestra en pantalla.

Incluye una consola **Telnet** para depuración en el dispositivo:
```
telnet <ip-esp32>
> help
```

---

## Superposiciones en juego — Hi-Score, Info del juego, Logros y Challenge RB

<p align="center">
  <img src="medias/social/hiscore.png" alt="Superposición Hi-Score de MAME/FBNeo en RecalBoxDMD" width="720">
</p>

Mientras un juego está realmente en marcha (nunca en modo de espera/playlist), el panel puede alternar automáticamente el marquee con hasta **cuatro** superposiciones impulsadas por la comunidad — a pantalla completa, sin bloquear nada, y siempre autolimitadas: cada una vuelve al marquee por sí sola tras una breve duración, con un temporizador que vive **enteramente en el propio DMD**, para que un script de Recalbox lento o fallido nunca pueda dejar el panel bloqueado.

- 🏆 **Hi-Score (MAME / FBNeo)** — decodifica el propio archivo de puntuación guardado por el emulador (`.hi`) contra un manifiesto comunitario convertido desde **hi2txt-xml** (4087 juegos cubiertos) y muestra la clasificación real de esa máquina, tal como la mostraría el propio MAME/FBNeo. Sin lectura de RAM en vivo, sin receta individual por juego que mantener: si un juego no está en el manifiesto, o su archivo de guardado falta o es inesperado, no se muestra nada — silencioso por diseño, nunca una puntuación equivocada.
- ℹ️ **Info del juego** — descripción, género, desarrollador y año de lanzamiento, extraídos directamente de tu `gamelist.xml` existente (el mismo scrape que ya hiciste para el propio marquee).
- 🎖️ **RetroAchievements** — aparece en cuanto desbloqueas un logro, en plena partida.
- 📅 **Challenge RB** — lee la clasificación oficial del Challenge comunitario mensual de Recalbox (un juego elegido por Recalbox cada mes, un solo crédito, sin continuar) directamente desde el recurso compartido de Recalbox — mismo panel, mismo estilo, sin configuración aparte.

**Cero configuración en el lado del DMD.** Instala los scripts de Recalbox una vez — **Modo 9** de la caja de herramientas de PC (o la instalación automática integrada en el **Modo 1**) — y cada una de estas superposiciones empieza a funcionar sola para cualquier juego/sistema que tenga datos que mostrar; el DMD sigue siendo una pantalla «tonta» de principio a fin, toda la lógica (qué enviar, cuándo, durante cuánto tiempo) vive en los scripts del lado de Recalbox, nunca en el propio firmware.

---

## 🎯 Mesas de Visual Pinball (VPX) en el DMD

El DMD puede comportarse como un display **ZeDMD-WiFi**: habla el mismo protocolo de red que el ZeDMD (un pequeño saludo HTTP y después las imágenes en flujo **UDP por el puerto 3333**), el que usa el plugin **DMDUtil** de Visual Pinball para manejar un ZeDMD por WiFi. Lanza una mesa VPX desde Recalbox y el DMD muestra en directo la imagen DMD propia de la mesa. **Desactivado por defecto**: con la opción sin marcar, nada cambia.

### Qué obtienes

- **Sin reinicio** al lanzar una mesa: la imagen aparece directamente.
- Mientras corre la mesa, la **playlist y las pantallas de Recalbox** («RecalBox conectada»…) quedan suspendidas para no dibujar nada encima.
- **Vuelta a la normalidad** en cuanto termina la partida (los scripts de Recalbox avisan al DMD), o unos 5 segundos después de que la mesa deje de enviar imagen. Se restaura el brillo configurado.
- Los DMD grandes (p. ej. 192×64) los reduce VPX al panel de 128×32; las mesas coloreadas se ven en color.

### Puesta en marcha — una sola vez

1. **En el DMD**: marca **Modo Pinball (VPX)** en el marco *Pantalla* de la página de inicio de la configuración web y pulsa **Guardar y reiniciar** (la opción solo se lee al arrancar).
2. **En la Recalbox (obligatorio)**: hay que indicar a VPX que envíe su DMD a este panel por WiFi. En `/recalbox/share/system/configs/vpinball/VPinballX-configgen.ini`, sección `[Plugin.DMDUtil]`:

   ```ini
   [Plugin.DMDUtil]
   Enable = 1
   ZeDMDWiFiEnabled = 1
   ZeDMDWiFiAddr = 192.168.1.240
   ```

   Sustituye `192.168.1.240` por la **IP de este DMD** (sin comentarios al final de línea: un archivo `.ini` los leería como parte del valor).

   Comprobación: tras lanzar una mesa, `vpinball.log` (misma carpeta) debe contener `ZeDMD WiFi enabled, connected to <IP del DMD>`. Si no, revisa la IP, que el DMD esté en la misma red y que el Modo Pinball se activó *y el DMD se reinició* después.

   **Alternativa automática (firmware del DMD v230+, scripts instalados con el Modo 9)**: con **Modo Pinball (VPX)** marcado en el DMD, el script de Recalbox `dmd_vpx_config` comprueba estos ajustes (y el de AlphaDMD más abajo) en cada arranque de la Recalbox y después de cada partida, y escribe lo que falte — nunca mientras corre una mesa, y sin sobrescribir nunca una IP que hayas escrito tú (si no, usa la que anuncia el DMD). **Marcar la opción es, por tanto, lo que autoriza la modificación del archivo de configuración de la Recalbox**: sin marcar, el script no hace nada. Para desactivarlo definitivamente, crea el archivo `/recalbox/share/userscripts/dmd_helpers/vpx_autoconfig.disabled`. Su registro: `/recalbox/share/system/logs/dmd_vpx_config.log`.

3. **Scripts de Recalbox**: instálalos con el **Modo 9** del PC Toolkit (necesario de todos modos para el resto del proyecto) — permiten al DMD salir del modo Pinball al instante al terminar una partida.

> ⚠️ Edita `VPinballX-configgen.ini` **solo cuando no hay ninguna mesa en marcha**: VPX reescribe ese archivo al cerrarse y borraría tu cambio. Una actualización de Recalbox también puede reiniciarlo.

### Ajustes opcionales, según el tipo de mesa

- **Mesas antiguas con display alfanumérico (segmentos)** — mesas PinMAME de la época Bally/Williams que solo muestran puntuaciones: activa el plugin **AlphaDMD** (`[Plugin.AlphaDMD]` → `Enable = 1`, mismo archivo). Sin él, VPX no envía nada al DMD para estas mesas y el marquee simplemente se queda en pantalla.
- **Mesas coloreadas (Serum)** — una mesa que trae una carpeta `altcolor/<rom>/<rom>.cRZ` solo muestra colores (e imágenes de fondo) si al plugin **Serum** de VPX se le indica dónde buscar: añade `[Plugin.Serum]` / `SerumPath = <carpeta de la mesa>/altcolor` en el archivo `.ini` de la mesa (junto al `.vpx`). Sin eso, el log dice `Serum: No colorization file found for <rom>` y la mesa se queda en monocromo.

### Compatibilidad

- **Probado** con mesas DMD 128×32 (Batman), Big Bang Bar (DMD 192×64), 6 de 8 mesas alfanuméricas (Black Hole y Farfalla usan disposiciones de display que el plugin AlphaDMD no soporta) y una mesa coloreada (Diner).
- **Nada que mostrar**: las mesas puramente electromecánicas y las que dibujan su DMD con PinUP Player (p. ej. Batman 66) no envían nada al DMD.
- **Probado solo con Recalbox.** El DMD solo depende del protocolo ZeDMD-WiFi: un Visual Pinball Standalone que use DMDUtil con los mismos ajustes `ZeDMDWiFi*` debería poder manejarlo también — sin probar. Específico de Recalbox: las rutas de archivos anteriores y la salida instantánea al terminar la partida (sin los scripts, el DMD vuelve al marquee unos 5 segundos después de la última imagen).

---

## El formato raw565 en detalle

**`.raw565`** — imagen fija (desde un PNG): exactamente `128 × 32 × 2 = 8192 bytes`, RGB565 en bruto (5-6-5 bits), leída en una sola operación SD y enviada directamente (`drawRGBBitmap`).

**`.raw565pack` + `.meta`** — imagen animada (desde un GIF): todos los fotogramas concatenados como bloques raw565 en `.raw565pack`; los retardos por fotograma (`uint16`, ms) en `.meta`, cargados una sola vez en RAM. Una apertura SD + un seek por fotograma, cero decodificación de GIF en el dispositivo.

**Caché de juegos por bigramas** (`games_cache.bin`) — un índice de 703 entradas por sistema (una entrada por prefijo de 2 letras, ej. `KO` para `kof98`) evita tener que listar jamás una carpeta con decenas de miles de archivos; una búsqueda salta directamente al fragmento correcto de la caché.

**Sistema de máscara** — cualquier sistema marcado como **«L»** (por encima del umbral configurable, 5000 archivos convertidos por defecto — MAME, FBNeo...) muestra *de inmediato* su imagen por defecto en caché mientras una tarea en segundo plano localiza y decodifica la real, así que el panel **nunca** se queda en blanco.

---

## Estructura de la tarjeta SD

```
📁 TARJETA SD (FAT32)
├── config.ini
├── systems/
│   ├── <sistema>/
│   │   ├── <juego>.raw565            ← marquee fija
│   │   ├── <juego>.raw565pack        ← marquee animada (fotogramas)
│   │   └── <juego>.meta               ← marquee animada (tiempos)
│   └── _defaults/
│       ├── default.raw565             ← respaldo global
│       └── <sistema>.raw565           ← respaldo por sistema, en el idioma
│                                         elegido (inglés por defecto; las
│                                         versiones FR/ES sobrescriben este
│                                         mismo archivo si se seleccionan
│                                         en la herramienta de PC)
├── gifs/                              ← playlists de modo de espera (aquí llega el pack de 600 GIFs)
│   ├── Arcade/  Consoles/  Computers/  Pinball_Short/  Pinball_Story/
│   └── Halloween/  XMAS/  Logo/  Other/ ...
├── widgets/
│   └── quotes/
│       ├── <fr|en|es>/<tema>.txt        ← temas de citas / quiz (una cita por línea)
│       └── bg/<tema>.raw565 (+ .col)    ← fondos ilustrados (citas + widgets Recalbox)
├── playlists/
│   └── <nombre_playlist>.txt
├── games_cache.bin                    ← índice de bigramas
└── systems_cache.dat                  ← índice de sistemas + flags L/N
```

---

## Estructura del repositorio

```
RecalBox_DMD.ino / *.h        ← código fuente del firmware ESP32 (proyecto Arduino IDE)
binaries/                     ← imágenes de firmware precompiladas (bootloader/app/fusionada)
tools/                        ← caja de herramientas de PC (GUI Python, FR/EN/ES) + build de Windows
carte SD/                     ← contenido de la tarjeta SD listo para copiar (gifs, defaults del sistema, scripts)
medias/                       ← capturas de pantalla, GIFs de demostración de los temas de reloj, kit de prensa
docs/                          ← GitHub Pages: página de presentación (shan-aya.github.io/RecalBoxDMD) + instalador web (…/install/)
```

---

## Solución de problemas

Para congelamientos, corrupción de pantalla, bucles en la configuración WiFi, o un DMD que parece desincronizado de Recalbox, consulta primero la página dedicada **[FAQ y solución de problemas](FAQ.es.md)** — cubre las causas más comunes (alimentación USB, calidad de la tarjeta microSD, revisión del chip ESP32) con más detalle del que cabe aquí.

| Problema | Solución |
|---|---|
| «Pillow no está instalado» | Se instala automáticamente en el primer arranque; si falla: `pip install Pillow` |
| «API de GitHub inaccesible» | Las descargas de `_defaults`/pack de 600 GIFs necesitan conexión a internet; reinténtalo más tarde (puede haber límite de peticiones) |
| No se detecta ninguna unidad extraíble | Inserta/comprueba que la tarjeta SD sea visible en el Explorador de Windows |
| El ESP32 no muestra nada | Comprueba la alimentación (5V 4A mín.), que `config.ini` esté en la raíz de la SD, el cableado HUB75; prueba Telnet `help` |
| ESP32 no detectado (sin puerto COM) | Instala los drivers USB: [CP2102 (Silicon Labs)](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) o [CH340/CH341](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all) |
| Visualización lenta / pantalla negra entre juegos | Confirma que has usado el **Modo 1**; comprueba que el sistema esté marcado como `L` en `systems_cache.dat`; sube el umbral de flag lento (pestaña Configuración) si tu SD es rápida |
| Aparece la imagen equivocada (carátula en vez del logo) | Revisa el perfil de **versión de Recalbox** y usa **«¿Cómo hacer el scrape?»**; ejecuta el **Modo 8** para verificar qué hay realmente presente |

---

## Créditos y licencia

- **Proyecto original RetroBoxLED**: [Jamyz](https://github.com/Jamyz/RetroBoxLED) — la base del firmware ESP32 y la idea original
- **RawEdition**: **Shan_ayA** — formato raw565, caché de bigramas, sistema de máscara, caja de herramientas de PC, temas de reloj, gestión de versiones de Recalbox, vista previa web en vivo
- **Inspiración**: [RetroPixelLED](https://github.com/fjgordillo86/RetroPixelLED) de fjgordillo86
- **Pack de 600 GIFs**: **eLLuiGi** / [RpiTeaM](https://rpiteam.carrd.co/) — muestra gratuita de su colección de GIFs retro
- **Manifiesto Hi-Score**: convertido desde el formato comunitario **hi2txt-xml**, construido en torno al proyecto `hiscore.dat` de MAME — 4087 juegos cubiertos
- **Hardware y guía de montaje**: [Mortaca — DMDos Board](https://www.mortaca.com/) / [dmdos.net](https://www.dmdos.net/)
- **Marco 3D**: Janibol — [Retromojones](https://www.youtube.com/@retromojones)
- **Comunidad**: [Recalbox](https://www.recalbox.com/)
- **Desarrollo**: escrito con la asistencia de [Claude](https://www.anthropic.com/claude) (Anthropic) — código asistido por IA en todo el firmware y la caja de herramientas de PC

📜 Historial completo de versiones: [CHANGELOG.es.md](CHANGELOG.es.md)

Bajo licencia [MIT](LICENSE).

🔔 **Entérate de las nuevas versiones**: en GitHub pulsa **Watch → Custom → Releases** (o suscríbete al [feed de releases](https://github.com/shan-aya/RecalBoxDMD/releases.atom)). La caja de herramientas de PC también te avisa al arrancar cuando hay una versión más reciente, y el [Web Installer](https://shan-aya.github.io/RecalBoxDMD/install/) muestra la versión del firmware que instala.

☕ Si este proyecto te resulta útil: [invítame a un café](https://buymeacoffee.com/shan_aya) o [dona vía PayPal](https://www.paypal.com/paypalme/felysaya)

<p align="center"><i>RecalBoxDMD RawEdition — Recalbox + un verdadero marquee LED, instantáneo incluso con 30 000 juegos de MAME.</i> 🎮⚡</p>
