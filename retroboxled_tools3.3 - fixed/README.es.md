# 🎮 RetroBoxLED Toolkit GUI (Recalbox) — Ayuda 


---

## ✅ Objetivo

Preparar la tarjeta SD para el ESP32 **marquee HUB75 / DMD 128x32** generando:

- `systems/<system>/...` (imágenes por juego/sistema)
- `systems/_defaults/` (archivos de reemplazo)
- `games_cache*.bin` (índice de imágenes de juegos)
- `systems_cache.dat` (índice de sistemas ESP32)
- (y luego copiar a la tarjeta SD)

---

## 📁 Dónde trabaja el script

En Windows, el script crea una carpeta temporal de trabajo (tipo “sd_card”) en:

**%LOCALAPPDATA%/Temp/RetroBoxLED**

Al final, el modo “copiar” sincroniza ese contenido con la **raíz** de tu tarjeta SD.

---

## 🎛️ Modos (opción 1 a 8)

### Modo 1 — Extracción + Conversión + Build cache (TODO)
- lee `gamelist.xml` desde tu carpeta ROMs
- extrae imágenes (PNG/GIF)
- convierte **PNG → 128x32** (los GIF se conservan tal cual)
- construye `games_cache*.bin` + `systems_cache.dat`
- también descarga `systems/_defaults` (GitHub)

### Modo 2 — Descargar `systems/_defaults` vía GitHub (solo)
- **descarga solo** `systems/_defaults/`
- **no** realiza extracción, conversión ni genera caches

> Ideal si solo quieres actualizar los archivos de fallback `_defaults`.

### Modo 3 — Extraer gamelist solamente
- extrae imágenes desde `gamelist.xml`
- no genera caches

### Modo 4 — Convertir solamente
- convierte imágenes PNG a **128x32**
- conserva los GIF
- no genera caches

### Modo 5 — Build `games_cache.bin` solo
- genera el índice/cache de imágenes de juegos a partir de imágenes ya presentes

### Modo 6 — Generar `systems_cache.dat` solo
- construye el índice de sistemas ESP32
- se apoya en `systems/_defaults` y/o sistemas ya presentes

### Modo 7 — Copiar a la tarjeta SD (robocopy)
- copia la carpeta de trabajo (`sd_card/`) **a la raíz** de la tarjeta SD
- usa `robocopy` (rápido)

---

## 🧠 Reemplazo `_defaults` (fallback)

El toolkit utiliza:

- `systems/_defaults/`

Si falta una imagen de sistema/juego, el ESP32 usará el archivo dentro de `_defaults`.

---

## 🖼️ Conversión 128x32 (PNG → 128x32)

La conversión requiere **Pillow**.
El script intenta instalarlo automáticamente si es necesario.
Si Pillow no está disponible, la conversión se desactiva (pero los GIF siguen funcionando).

---

## 🧪 Requisitos (tarjeta SD & ROMs)

- Tarjeta SD: **FAT32**
- ROMs: subcarpetas con un `gamelist.xml` (estructura Recalbox estándar)

---

## ▶️ Cómo usar

### Vía la interfaz Tkinter
1. Ejecuta `RetroBoxLED_gui.py`
2. Selecciona la carpeta ROMs
3. Elige un **modo**
4. Pulsa **Start**
5. (Modo 7) Elige la tarjeta SD y empieza la copia

### Vía el script en consola (opcional)
Sigue el menú de texto del script: la opción `2` corresponde a **descargar `_defaults` únicamente**.

---

## ℹ️ Notas útiles
- El script puede descargar desde GitHub para el flujo de `_defaults` (se requiere internet).
- El Modo 2 es “ligero”: **solo** `systems/_defaults`.

---

## 🧾 Qué comprobar después de copiar a la SD
En la **raíz** de la tarjeta SD, deberías ver:

- `systems/` (incluyendo `_defaults/`)
- `systems_cache.dat`
- `games_cache*.bin` (o `games_cache.bin` según las imágenes/las carpetas)
