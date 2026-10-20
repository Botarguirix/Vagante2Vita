# Iteración 200 — combate cercano del Worm y salud encadenada

## Cambio del probe

El probe host deja de priorizar el arco cuando Rogue tiene una amenaza cercana
durante el encuentro con el Worm. En la geometría actual del piso 1, la mayoría
de esas flechas impactaban terreno (14 impactos de terreno por 17 disparos) y no
dañaban al jefe. La política cercana usa el ataque melee existente del jugador;
no cambia la física, el daño ni la salud del juego.

La corrida repetible está en
`_local/route-act-one-199-close-melee-fullchain/`. Los proofs individuales de
los pisos 0, 1 y 2 pasan con Rogue y su estado inicial normal. En el piso 1, el
Worm cae y el jugador alcanza la salida en 1,558 ticks con 4 HP; el único
disparo registrado impacta al jefe y el resto del daño viene del combate
cercano. Las 14 suites C host y 226 pruebas Python pasan.

## Límite de la cadena

Los tres proofs individuales no prueban una campaña continua porque cada piso
empieza con 80 HP y la carga inicial del Rogue. El replay del piso 0 puede
terminar con 10 HP en la ejecución actual; una ruta anterior terminó con 20 HP.
Al iniciar el piso 1 con ese mejor estado conocido (20 HP y 26 flechas), Rogue
muere en el tick 934, con 5 HP en el tick 900 y el Worm aún vivo. El resultado
está en `_local/route-act-one-199-close-melee-carry20/`.

La cadena ejecutada con salud persistente entra al piso 1 con 10 HP y muere en
el tick 850; el Worm conserva 94.8 HP. El chain verifier también marca que los
proofs de piso 1 y 2 empiezan con carga completa y, por ello, no equivalen al
estado heredado. Una prueba de salto cada 12 ticks terminó un tick antes que la
política base (933 frente a 934) y no se conservó.

No se añadió curación entre niveles sin evidencia del juego original. La
campaña persistente y la validación de gameplay en Vita3K siguen pendientes;
Vita3K continúa fallando antes de cargar el VPK, como documenta
`DEMO_ACT_ONE_199.md`.
