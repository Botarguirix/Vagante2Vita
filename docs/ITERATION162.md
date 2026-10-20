# Iteración 01.62: ajuste provisional de la pantalla de clase

Esta entrega quitó temporalmente el retorno con Círculo desde la pantalla de
clase. El ajuste se basó en una hipótesis incompleta sobre el síntoma; la
iteración 01.63 restauró el comportamiento de menú. La captura que motivó el
reporte corresponde al VPK 01.55 y muestra `O RESET`; ese flujo antiguo sí
reiniciaba el mapa inicial al tocar su salida. La corrección de entrada a
ACT_ONE ya está en la línea desde 01.59. La inferencia visual de 01.62 sobre
caminar hacia la izquierda no estaba sustentada; la traza reproducible desde
el spawn avanza a la derecha.

Se agregó cobertura provisional al test de menú. El test se
compiló para Vita como `menu-test-vita.elf`; no se ejecutó en host porque esta
sesión no tiene un compilador C host disponible. Pasan 22 pruebas Python para
los replays y el checker de Vita3K. El VPK se compiló con VitaSDK, y
`check_vpk.py` valida `TITLE_ID=VAGV00006`, `APP_VER=01.62`, cabecera SELF y
siete entradas, sin errores ni advertencias. SHA-256:
`66D4D5DA60E26667FC4F14AA2CA0658407C8DF71F0D0DEFD67E9372729293807`.

La reproducción visual sigue pendiente: Vita3K local ha fallado antes de
crear sesión nativa incluso en un perfil vacío sin VPK. Por ello aún no está
confirmado si la cueva cambia al piso 0 en runtime. Para entrar al primer piso
se requieren los datos de campaña bajo
`ux0:data/Vagante2Vita/world/act-one/`; si no están o no cargan, la app debe
mostrar el aviso de bloqueo y registrar `WORLD_TRANSITION complete=0`, en vez
de reiniciar el intro.

VPK: `_local/build-act-one-162/Vagante2Vita-iteration162.vpk` (843,365 bytes).
