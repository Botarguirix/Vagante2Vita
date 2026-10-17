# Iteración 082: VPK actual y replay automatizada de campaña

Compilé el código actual con VitaSDK para Windows mediante
`tools/build_vpk_windows.ps1`. El artefacto es
`_local/build-act-one-082/Vagante2Vita-iteration082.vpk`, versión 00.82,
830,147 bytes, SHA-256
`76fb58b0440b029ecb6ba84a9ceea9417f47b608b0f6fad69398b9690bf22bef`. El
contenedor tiene `eboot.bin`, `param.sfo` y los cinco recursos LiveArea
esperados. El paquete de campaña privado `_local/delivery-act-one-campaign082`
valida su manifiesto de 100 archivos.

El harness preparó una replay de campaña de 545 segmentos y 3,184 ticks. Las
tres rutas provienen del `player_step` nativo y usan las trazas selladas en el
bundle: `player_step:0,player_step:1,player_step:2`. Se ejecutó el smoke de
campaña en el perfil aislado `_local/vita3k-vagante-clone`, con OpenGL y el
frontend Qt. Vita3K permaneció sin log de emulador y sin sesión nativa durante
60 segundos; el harness detuvo únicamente el proceso que inició y registró
`failure_stage=before_native_session`, `process_timed_out=true` y
`route_sources=player_step:0,player_step:1,player_step:2` en
`_local/vita3k-vagante-clone/runs/iteration082/run-000001/result.json`.
Después abrí manualmente la instancia aislada desde su ejecutable de runtime.
Vita3K v0.2.1.4183 mostró `Vagante2Vita`, ID `VAGV00006`, versión `0.82` y
816 KB en App Library, leyendo su propio directorio `filesystem`. Esto confirma
que el paquete quedó registrado en el perfil aislado. La ventana no entregó
captura ni aceptó la selección automatizada del título, así que no se inició
una sesión nativa; no hay prueba de carga del ejecutable ni de gameplay.

Una sesión manual posterior sí generó el log nativo
`filesystem/ux0/data/Vagante2Vita/logs/iteration082/session-000001.log` con
`BOOT version=00.82`. Cargó ACT_ONE, recorrió el piso 0, completó su transición
al piso 1 y luego cargó el piso 2. El replay marcó ambos indicadores de campaña
como consumidos, pero el checker rechazó el resultado: no hubo una tercera
`WORLD_EXIT`, `ACT_ONE_DUNGEON cleared=1` ni salida a `CLASS_PREVIEW`. La causa
era que `act_one_exit_reached` seguía activo después de cambiar de piso; al
entrar al piso 2, la condición de fin detenía el replay antes de tocar su
salida. Corregí ese estado en la iteración 083. El run automatizado adicional
por argumento de VPK terminó con `0xC0000005` antes de crear una sesión, así
que no sustituye la comprobación desde la biblioteca.

El 00.82 demuestra carga nativa de recursos y dos cambios de piso en Vita3K,
pero no una campaña completa. La generación exacta por semilla y la paridad de
enemigos de pisos posteriores siguen pendientes; la nueva iteración debe
confirmar la salida real del piso 2 y el cierre de ACT_ONE.
