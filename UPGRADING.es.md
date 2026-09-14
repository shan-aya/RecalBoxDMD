# Actualizar desde una versión anterior (v13 → v2.0)

[🇬🇧 English](UPGRADING.md) · [🇫🇷 Français](UPGRADING.fr.md) · 🇪🇸 **Español**

¿Ya usas RecalBoxDMD (firmware v13 "Raw565 Edition", cualquier versión de la caja de herramientas de PC)? Esto es lo que realmente cambia y lo que hay que hacer — la mayor parte es opcional, y nada de tu tarjeta SD ni de tu configuración de Recalbox necesita cambiar.

## 1. Actualiza la caja de herramientas de PC

Descarga la última versión desde la [página de Releases](https://github.com/shan-aya/RecalBoxDMD/releases) e instálala sobre la anterior (o reemplaza el `.exe` portable). Tus ajustes (tema, idioma, IP de Recalbox, imagen de repliegue guardada...) se conservan automáticamente — viven en un `RecalBoxDMD_prefs.json` aparte, que la actualización no toca.

## 2. Flashea el nuevo firmware

Como siempre — el [Web Installer de un clic](https://shan-aya.github.io/RecalBoxDMD/) (Chrome/Edge) flashea la v2.0 por USB en cerca de un minuto. No hace falta marcar "Borrar dispositivo" — un reflasheo normal conserva tu `config.ini` y todo lo que ya está en la tarjeta SD, incluido el campo `recalbox_ip`, que ahora identifica al par UDP en lugar del broker MQTT; no hay nada que cambiar ahí.

## 3. Reinstala los scripts de Recalbox — obligatorio

El enlace en tiempo real entre Recalbox y el DMD pasa por debajo de **MQTT a UDP** (ver el [Changelog](CHANGELOG.md) para saber por qué). Los scripts del lado de Recalbox (`marquee[...].sh`, `dmd_score[...].sh`, y el resto de `dmd_helpers/`) se actualizaron para hablar UDP en lugar de publicar en un broker MQTT — **ejecuta el Modo 9** una vez (o un Modo 1 completo) desde la caja de herramientas de PC para reinstalarlos; también limpia automáticamente las versiones antiguas de los scripts de la era MQTT. Nada que configurar: sin broker, sin puerto, sin credenciales que introducir — el DMD escucha en el mismo `recalbox_ip` que ya usaba.

## 4. ¿Tenías algo conectado a los antiguos topics MQTT?

El soporte de MQTT se ha eliminado por completo del firmware desde la v209 (2026-09-14) — no solo desactivado por un indicador, como decía una versión anterior de esta página. `MQTT_ENABLED=true` ya no tiene ningún efecto: el propio código de conexión/tarea ha desaparecido del código fuente, no solo está desactivado. Si tenías algo externo conectado a los antiguos topics MQTT del DMD, tendrías que recuperar ese código del historial de git anterior a la v209 y recompilar desde ahí. UDP es ahora el único camino en tiempo real compatible.

## Lo que *no* necesitas hacer

- Volver a escanear tus juegos, reconstruir tu tarjeta SD, regenerar ningún caché, ni tocar tus playlists — nada de eso ha cambiado.
- Reconfigurar la IP de Recalbox, el WiFi, o cualquier otra cosa en la página de configuración web — mismos campos, mismos valores.
- Hacer nada respecto al broker MQTT (Mosquitto) que sigue corriendo en Recalbox — el DMD simplemente ya no le habla; déjalo funcionando o quítalo, como prefieras.
