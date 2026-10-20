# Iteración 103: carga aérea del Bat y VPK 01.03

Ghidra `Bat::step @ 004fc7a0` recupera la aceleración dirigida de 0.05 por
tick, aviso de carga de `0x1e` (30) ticks, impulso de magnitud 4, duración de
`0x14` (20) ticks y enfriamiento `0x3c` (60) ticks. El runtime integra un
ciclo de carga con esos tiempos, conserva el contacto de 1–6 de daño y barre
el movimiento contra tiles sólidos. El movimiento directo hacia el jugador y
el tope de rapidez de 3 son aproximaciones; el original delega el ruteo al
PathManager y utiliza estados/animaciones que no están en el paquete de salas.

El test `world-entity-packet` comprueba aviso, impulso, duración y enfriamiento.
Build host en WSL y CTest pasaron 12/12. VitaSDK compiló el VPK
`_local/build-vita-103/Vagante2Vita-iteration103.vpk` (832,980 bytes),
title ID `VAGV00006`, versión `01.03`, SHA-256
`A022D8981D31FD0C57A721E0F0587A1D55F101FDA180C1A246FFB0F0BEA61086`.

Las trazas nativas de ruta se regeneraron con el fingerprint 103: pisos 0–2
alcanzan la salida en 1,741, 736 y 375 ticks. La entrega privada
`_local/delivery-act-one-campaign103` valida 113 archivos y prepara una replay
de 545 segmentos/3,184 ticks con las tres rutas `player_step`.

El smoke de campaña con Vita3K 0.2.1 terminó con `0xC0000005` antes de crear
sesión nativa con OpenGL y Vulkan en SDL. La build histórica 4170, arrancada
con sus DLL completas, terminó con el mismo código. Tampoco lo corrigieron
crear las carpetas vacías de filesystem Vita, cambiar comillas de `pref-path`
o iniciar con el modo consola/VPK directo. Como control, el VPK 00.25 también
terminó con `0xC0000005` en el Vita3K actual, con SDL y Qt visible; esto apunta
al entorno de arranque/perfil y no demuestra un defecto exclusivo del VPK
01.03. Los registros están bajo `_local/vita3k-vagante-test103*` y
`_local/vita3k-control025-latest`; no contienen sesión nativa del juego.
Ninguna corrida valida la replay ni el comportamiento del Bat en Vita3K.

La metodología de [Decompedia](https://decomp.wiki/) se aplica como
reconstrucción funcional trazable por dirección, función y evidencia del
listing, no como matching del binario x86: el objetivo de este port es el
comportamiento ARM en Vita. Las fuentes del sitio explican la diferencia entre
matching decompilation, que busca reproducir exactamente el binario con la
toolchain original, y las etapas/restricciones de los proyectos por plataforma
([descripción del matching](https://github.com/decompals/decompedia/blob/main/home.md)).

La paridad exacta de PathManager, sprites y estados de carga sigue pendiente,
al igual que la transición jugable y la verificación en hardware. El primer
mundo ACT_ONE continúa incompleto.
