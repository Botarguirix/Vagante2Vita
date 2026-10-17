# Iteración 063: botín durante transiciones de ACT_ONE

La carga de pisos 1 y 2 ahora carga también `item-loot.vlt` en el mundo
candidato y fija `loot_level` al índice del piso antes de cargar los atlas y
transferir el mundo al jugador. Antes, la asignación del candidato sustituía
todo `World` y dejaba sin tabla de loot a los cofres tras salir del piso 0.
La preparación de campañas conserva el archivo en el manifiesto.

Se preparó `_local/world-data063/act-one` con los tres pisos y route proofs
asociados por SHA-256 a cada par mundo/entidades. El manifiesto valida 78
archivos. Esto prueba las rutas físicas del runtime reconstruido sobre esos
paquetes provisionales; los informes siguen declarando `floor_complete=false`
y `exact_generation_parity=false`, así que la campaña no se considera una
reconstrucción exacta ni un primer mundo terminado.

El VPK `_local/build-act-one-063/Vagante2Vita-iteration063.vpk` se compiló con
VitaSDK y pasa `check_vpk.py`: 7 entradas, 0 errores y 0 advertencias. Tiene
825.067 bytes y SHA-256
`E4B109F17F9FAFF82FBF26F93B922932984885C2BB7CB0E600AA4B8812E0D637`.
La suite Python pasa 136 pruebas; el conjunto C nativo no se pudo ejecutar por
falta de compilador host/CMake.

El smoke visible de Vita3K en sandbox aislado, con replay de salida ACT_ONE,
terminó al límite de 60 segundos sin crear sesión nativa, logs debug, stdout ni
stderr. Los intentos anteriores con INTRO y ambos renderers tuvieron el mismo
resultado. El VPK compila, pero el arranque y la transición aún no se validan
en el emulador.
