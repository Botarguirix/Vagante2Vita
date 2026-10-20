# Iteración 01.50: encuentro de Dragon en el piso 0

El exportador coloca un jefe provisional de tipo Dragon (VWE3 subtype 50) en un ancla verificada del piso 0: tile [23,27], centrado según su hitbox de 13×13. El ancla mantiene libre el corredor final a la salida. `campaign_exit_action` ahora bloquea el avance de pisos 0 y 1 mientras haya un jefe cargado sin derrotar; piso 1 no tiene jefe y piso 2 conserva su requisito del Goblin King y la máscara de tres jefes.

La prueba automática de ruta ya reconoce Goblin King y Dragon, carga el arco de la carpeta del mundo y prueba combate desde ACT_ONE. El recorrido del Dragon sigue fallando: su IA queda bajo plataformas y las flechas chocan con terreno; el jugador también puede morir antes de derrotarlo. No existe todavía un route proof físico del piso 0 ni una traza de campaña aprobada con Dragon.

VPK: `_local/build-act-one-150/Vagante2Vita-iteration150.vpk`, ID `VAGV00006`, `APP_VER=01.50`. SHA-256: `8249AA136335E3813263E00402EE7288795C43CE888EB6A10B17F48D1B9A773B`.

Validación: 204 pruebas Python y 13 pruebas C host pasan. La VPK se instaló en `_local/vita3k-vagante-fullcopy-20261017-worldtest2`, una copia completa de Vita3K que contiene 2,126 archivos verificados por SHA-256 (358,185,148 bytes). Vita3K abre el proceso de frontend pero no crea `session-*.log` de la iteración 150; la sesión nativa, el arranque del juego y el recorrido de campaña no se consideran validados. WSL devuelve `E_ACCESSDENIED` al enumerar distribuciones; se compiló con VitaSDK nativo de Windows.

La campaña continúa incompleta: selección de jefe/RNG nativa, combate físico fiable del Dragon, Worm y sus 20 segmentos, recompensa nativa, generación exacta y transición de piso verificada en Vita3K siguen pendientes. Tras cambiar la huella del simulador para incluir el módulo de gates de campaña, los route proofs de iteraciones anteriores ya no se consideran válidos para estos fuentes; el proof nuevo del Dragon sigue sin generarse.
