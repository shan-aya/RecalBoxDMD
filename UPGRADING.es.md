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

## 5. Logos del tema de Recalbox (firmware 2.33 y posteriores) — opcional, pero tres pasos si los quieres

Desde el firmware **2.33**, el DMD puede mostrar los logos de tus consolas con el estilo del tema activo en Recalbox (Midnight, CRT Color, Neoretro…). No pasa nada hasta que hagas los tres pasos — flashear solo el firmware **no basta**:

1. **Reinstala los scripts de Recalbox con el Modo 9** (o un Modo 1 completo): el `dmd_udp_resync.py` actualizado (**v6**) es el que le dice al DMD qué tema usa Recalbox. Con el script antiguo, el DMD nunca conoce el tema y mantiene sus logos por defecto.
2. **Copia los logos del tema a la tarjeta SD con el Modo 12 o el Modo 13** de la caja de herramientas de PC — los logos viven en `systems/_defaults/_themes/<tema>/` en la tarjeta SD y no están ahí tras una simple actualización del firmware:
   - **El Modo 13** descarga de GitHub el paquete de temas listo para usar (10 temas, incluido el tema por defecto de Recalbox) y copia a la tarjeta SD los que marques — lo más rápido;
   - **El Modo 12** («Gestión de temas de Recalbox») lista los temas de tu Recalbox, convierte los que faltan o están desactualizados y mantiene la tarjeta SD al día.

   El Modo 1 también pregunta por los temas (propone los que encuentra en tu Recalbox), así que un Modo 1 completo cubre también este paso. Los temas ya actualizados en la tarjeta SD no se reescriben.
3. **Comprueba la casilla**: sección *Tema de Recalbox* (página Pantalla) de la página de configuración web del DMD (`feat_theme_follow`, marcada por defecto). Desmárcala para conservar los logos por defecto.

Un tema o un logo ausente de la tarjeta SD es inofensivo: el DMD vuelve al logo por defecto. Pasar del firmware 2.33 al 2.35 no requiere más que volver a flashear.

**Desde el firmware 2.38 — el idioma y la región son automáticos.** La tarjeta SD contiene ahora todas las variantes (temas: subcarpetas `l_<idioma>/` y `r_<región>/`; imágenes por defecto: `_defaults/fr/` y `_defaults/es/`) y el DMD usa el idioma y la región del tema de tu Recalbox, incluso tras cambiarlos. Para tenerlo: flashea la **2.38**, reinstala los scripts de Recalbox con el **Modo 9** (`dmd_udp_resync` v8) y **reinstala una vez tus temas** (Modo 1, 12 o 13) y las imágenes por defecto (Modo 1 o 2) para que la tarjeta SD reciba las variantes — la caja de herramientas ya no pregunta el idioma. Hasta entonces, todo sigue funcionando con los logos en inglés / US.

**Desde el firmware 2.42 — se te avisa cuando los scripts no están al día.** La página de inicio del DMD y la caja de herramientas del PC (build 10466 y posteriores, al arrancar) comprueban también los scripts de Recalbox e indican qué hacer: ejecuta la caja de herramientas del PC y haz un **Modo 9**, luego reinicia la Recalbox. No se lee nada en la Recalbox (sin SSH): los scripts (`dmd_udp_resync` v11) anuncian su número al DMD, que lo muestra en su página web y se lo da a la caja de herramientas por Wi-Fi. Los scripts antiguos no anuncian nada — cuando la Recalbox lleva encendida un minuto y medio, el DMD lo detecta y pide el Modo 9. Regla simple tras cualquier actualización: **firmware → Modo 9 → reiniciar la Recalbox**.

## Lo que *no* necesitas hacer

- Volver a escanear tus juegos, reconstruir tu tarjeta SD, regenerar ningún caché, ni tocar tus playlists — nada de eso ha cambiado.
- Reconfigurar la IP de Recalbox, el WiFi, o cualquier otra cosa en la página de configuración web — mismos campos, mismos valores.
- Hacer nada respecto al broker MQTT (Mosquitto) que sigue corriendo en Recalbox — el DMD simplemente ya no le habla; déjalo funcionando o quítalo, como prefieras.
