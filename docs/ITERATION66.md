# Iteración 066: simulación de agua consume el RNG del generador

La generación privada ACT_ONE ahora simula los 60 ticks del agua usando el MT19937 activo. El pool nuevo recuperado desde Ghidra tiene 1024 slots; la simulación consume draws para celdas activas y para burbujas, incluso al probar la posición cuando no habría slots disponibles. El log del export incluye draw count y fingerprint del RNG al finalizar el agua.

La semilla 12345, piso 0 se exportó en `_local/generation066-final/map-export`: entrada `[30,4]`, salida `[36,28]`, ruta topológica de 58 pasos y ocho enemigos provisionales. En el warmup, 3975 celdas activas consumieron 3979 draws enteros; dentro de ese total, las dos burbujas aportaron dos draws flotantes de posición y cuatro enteros de inicialización. El pool acabó con 1022 slots disponibles.

El barrido de veinte seeds por cada uno de los tres pisos pasa, incluyendo que el contador del RNG coincida con los draws registrados por el agua, que el mapa se mantenga en 80×40, que el spawn esté despejado y apoyado, y que exista ruta topológica a la salida.

La recuperación completa llega hasta `water_particle_warmup`. El ledger ahora también consume el draw de constructor `Item::Item` para inicializar `randIndex` por cada pedestal sin objeto, pero marca explícitamente que falta el resto del estado de creación de entidades. Un spellbook hace cinco iteraciones condicionadas por estado de Skill/piso y `Item::setSkill` puede añadir un draw cuando el tipo difiere de `randIndex & SUMMON_MONSTER`; por ello, el conteo posterior depende del camino. Falta recuperar el campo Skill+0x70c, intercalar correctamente varios pedestales, el mapa de entidades bloqueantes G/H y los shuffles de endpoints. Los enemigos continúan provisionales y no hay prueba física de recorrido ni replay en Vita3K. No se modificó ni recompiló el VPK en esta iteración.

Validación: `python -m unittest discover -s tests -v` — 137 pruebas pasan. Export privado regenerado desde `_local/assets/rooms.json` y assets locales.
