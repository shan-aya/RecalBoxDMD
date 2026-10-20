# RecalBoxDMD en Raspberry Pi

[🇬🇧 English](RASPBERRY-PI.md) · [🇫🇷 Français](RASPBERRY-PI.fr.md) · 🇪🇸 **Español**

El mismo DMD, con una **Raspberry Pi** en lugar del ESP32: mismos scripts de Recalbox, mismo contenido de la tarjeta SD, mismas páginas de configuración web, widgets y temas. La Pi maneja dos paneles P5 de 64×32 (128×32 en total) mediante una placa de interfaz HUB75.

> ✅ **Validado: Raspberry Pi 3B** (Raspberry Pi OS Lite de 32 bits « Bookworm »).
> ⚠️ **Previsto pero sin probar: Pi 4 / 400 / CM4, Pi 2, Zero 2.** El software detecta el modelo y ajusta `gpio_slowdown` en consecuencia (Pi 4: 4; Pi 2/3: 2; Pi 1/Zero: 1), pero solo la Pi 3B se ha probado con paneles reales. Si tu Pi parpadea, ajusta `gpio_slowdown=1..4` en `config.ini`.

## Instalar (lo más fácil: la caja de herramientas del PC)

1. Descarga la **caja de herramientas del PC** (página *Releases*) y ejecútala. En la parte superior de la ventana elige el destino **Raspberry Pi**.
2. **Modo 1**: el asistente encadena todo, desde una **tarjeta SD virgen** hasta una tarjeta lista para insertar en la Pi — formateo, Raspberry Pi OS Lite, tu Wi-Fi / usuario / SSH, el software del DMD y tus logos convertidos en una partición `RBPIDMD`. Windows pide permisos de administrador (el disco se escribe directamente); la tarjeta se borra tras confirmación.
3. Inserta la tarjeta en la Pi, enciéndela y espera 2–3 minutos. El DMD muestra **Página de configuración** y su dirección (o abre `http://rbpidmd.local/`). Sin Wi-Fi válido, la Pi abre la red **RecalBox-DMD-Config** (dirección 192.168.4.1) para configurarla desde un teléfono.
4. Introduce la IP de tu Recalbox en los ajustes Wi-Fi y ejecuta el **Modo 9** de la caja de herramientas: los scripts de Recalbox son **los mismos** que para el ESP32.

Los modos separados siguen disponibles: **Modo 16** (preparar una tarjeta, sustituye a Raspberry Pi Imager) y **Modo 17** (instalar o actualizar el software en una Pi ya arrancada y copiar el contenido de la tarjeta SD del DMD). El software del DMD para la Pi se **descarga de GitHub** (release `pi-v…`, huella SHA-256 verificada); sin red se usa la copia ya presente en el PC.

## Hardware

- Raspberry Pi 3B (validada) y una placa de interfaz HUB75 (placas tipo RPI2DMD: mapeo GPIO « regular », RTC DS1307 opcional).
- Dos paneles P5 de 64×32 en cadena (128×32), barrido 1/16: ajustes por defecto `--led-cols=64 --led-rows=32 --led-chain=2 --led-pwm-bits=11 --led-slowdown-gpio=<según modelo>`.
- Un botón físico opcional entre GPIO 21 y GND.
- El audio de la Pi se desactiva y se reserva un núcleo para la pantalla (`isolcpus=3`): lo hace la instalación.

## Actualizar

- **Desde la página web del DMD (v3.59 y posteriores)**: la página **Ajustes** muestra un aviso cuando se publica una nueva versión del demonio, con un botón **Actualizar ahora**. La Pi descarga la última release `pi-v…` de GitHub, verifica su huella SHA-256, instala, reinicia el servicio y **vuelve sola a la versión anterior** si no arranca. El DMD muestra « Actualizando… » durante la operación (aproximadamente 1 minuto). Una Pi que siga en 3.58 pasa una primera vez a 3.59 con el Modo 17.
- **Modo 17** de la caja de herramientas, o
- a mano: descarga `RecalBoxDMD_pi_payload.zip` de la última release `pi-v…` y ejecuta `sudo bash install.sh` desde la carpeta descomprimida en la Pi.

`config.ini` y el contenido de la tarjeta nunca se sobrescriben.

## Compilarlo uno mismo

El demonio está en [`raspberry-pi/rpi_daemon/`](raspberry-pi/rpi_daemon/) (C++17, CMake). Se enlaza con [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) de Henner Zeller:

```
git clone https://github.com/hzeller/rpi-rgb-led-matrix ~/rpi-rgb-led-matrix && make -C ~/rpi-rgb-led-matrix/lib
sudo apt install cmake g++ libpng-dev libgif-dev zlib1g-dev libcpp-httplib-dev
cd raspberry-pi/rpi_daemon && cmake -B build && cmake --build build -j3
```

Cuenta entre 10 y 25 minutos en una Pi 3B. `deploy/install.sh --build` hace todo esto e instala el servicio.

## Licencia

La carpeta [`raspberry-pi/rpi_daemon/`](raspberry-pi/rpi_daemon/) está bajo **GPL-2.0-or-later** (el demonio se enlaza con rpi-rgb-led-matrix, GPL-2.0+): texto en `raspberry-pi/rpi_daemon/LICENSE`, detalles en `LICENSE.md`. El binario distribuido se construye a partir de las fuentes de este repositorio. El resto del repositorio (firmware ESP32, caja de herramientas) sigue bajo licencia MIT.

## Límites conocidos

- Algunos LED defectuosos de los paneles siguen visibles sea cual sea el ajuste (hardware).
- Modo nativo de Recalbox / Visual Pinball: validado con un simulador y a ojo en los paneles; sin campaña de pruebas prolongada.
