# Demo local ACT_ONE 01.98

La entrega local está en `_local/demo-act-one-198/`:

- `Vagante2Vita-iteration198.vpk` es el homebrew instalable, ID `VAGV00006`, versión `01.98`.
- `ux0-data/Vagante2Vita/` contiene el paquete de recursos local para copiar a `ux0:data/Vagante2Vita/` en Vita3K o en una Vita.
- `demo-manifest.json` registra los SHA-256 de la VPK y de cada archivo de recursos.

Los recursos permanecen fuera del VPK. El paquete local usa datos extraídos de la copia de Vagante disponible en esta máquina; no se deben publicar ni subir al repositorio.

## Instalar y jugar

Instala `Vagante2Vita-iteration198.vpk` en Vita3K o Vita y copia el contenido de `ux0-data/Vagante2Vita/` a `ux0:data/Vagante2Vita/`. Inicia Vagante2Vita, pulsa X en el menú y elige una clase. En el mundo inicial, camina a la derecha y entra en la cueva para cargar ACT_ONE; el botón Círculo no abre la cueva y no reinicia el mundo durante el juego.

Controles de juego: cruceta o stick para caminar, X para saltar, abajo+X para bajar de plataformas, Cuadrado para melee, R para arco cuando la clase lo tiene, SELECT para consultar el personaje y Triángulo para volver. Tras derrotar al jefe de un piso, toca su salida para intentar cargar el siguiente.

## Qué contiene y qué se verificó

El paquete privado incluye el mapa introductorio y su transición al primer piso ACT_ONE; tres pisos ACT_ONE, entidades, trampas, equipo, enemigos y jefes; las reglas de transición y el replay automatizado de la entrada por la cueva. El paquete de campaña usa pruebas V4 enlazadas a los hashes de mapas, entidades y enemigos.

Verificación local: `check_vpk.py` acepta la VPK con siete entradas; pasan las 13 suites C host, 222 pruebas Python y 70 subpruebas. El generador prepara un replay intro→piso 0 de 224 segmentos (37 ticks de intro y una ruta host del piso 0 de 1,268 ticks); cada uno de los tres pisos supera su ruta host independiente.

## Límites conocidos

La campaña no está terminada ni validada en hardware. El replay integrado de Rogue completa el piso 0 con 10/80 HP y muere en el piso 1, antes del primer daño al Worm. Por ello, las rutas independientes de tres pisos no prueban que una misma partida pueda completar la campaña. La generación de niveles, colocación y comportamiento de varios jefes y recompensas todavía son provisionales; no hay paridad exacta demostrada con el juego original.

Vita3K 0.2.1.4194 no ejecutó esta VPK: con OpenGL y el replay de entrada por la cueva agotó 45 segundos antes de crear el log nativo. `vita3k.log`, stdout y stderr quedaron vacíos. La prueba equivalente 1.97 con Vulkan en una copia completa del runtime y perfil agotó 120 segundos antes de la sesión; OpenGL terminó con `0xC0000005`. Es un fallo de arranque del emulador en este entorno y no cuenta como resultado del juego.

La evidencia y los reportes reproducibles están bajo `_local/route-worm-first-current-20261019/`, `_local/host-chain-act-one-198-demo/` y `_local/vita3k-c197-fullcopy-test/runs/iteration198/run-000001/`.
