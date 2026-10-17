# Iteración 042: rutas físicas verificadas por piso en ACT_ONE

La campaña privada 048 incorpora pruebas de recorrido nativo para los tres
paquetes ACT_ONE. Los logs usan `world_load`, `world_load_entities` y
`player_step`; el generador de campaña solo acepta cada prueba cuando los
SHA-256 del mapa y de las entidades coinciden con los paquetes de ese piso.
La prueba cubre navegación del spawn a la salida en el runtime nativo. La
transición de salida al siguiente piso está integrada, pero el smoke de Vita3K
no la verificó; tampoco está demostrada la equivalencia de generación con
Vagante.

Para la semilla 12345 se alcanzó la salida del piso 0 en 664 ticks y 3 saltos,
del piso 1 en 809 ticks y 7 saltos, y del piso 2 en 401 ticks sin saltos.
Las tres pruebas quedaron registradas en
`_local/act-one-campaign048/route-proofs/`; la entrega privada valida su
manifiesto de 68 archivos. El contenido comercial permanece local.

La referencia de Repentogxm confirma una ruta posible de traducción estática
para su binario x86 específico; D2Vita ejemplifica la separación entre runtime,
adaptadores Vita y datos que aporta el usuario. Ningún código de esos proyectos
se integra como traductor genérico: Vagante usa su extracción `.vra`, Ghidra/PDB
y el runtime nativo ya implementado. La revisión comparativa está en
`PORTING_REFERENCE_REVIEW.md`.

## Verificación

- `python -m unittest discover -s tests -p 'test_*.py' -v`: 119 pruebas OK.
- `tools/prepare_world_bundle.py --validate _local/act-one-campaign048`:
  manifiesto válido, 68 archivos.
- Compilación VitaSDK Windows terminada: VPK de 7 entradas, 808,688 bytes,
  Title ID `VAGV00006`, versión `00.42`; SHA-256
  `C8B3938B9A099B4F3BA011500A0283D04D09B1C3788507F96DF14FF9F98819CE`.
- Smoke Vita3K aislado con el VPK 042, OpenGL, argumento de contenido y
  campaña 048: timeout de 60 segundos, proceso terminó con `-1`, sin
  `vita3k.log` ni sesión nativa. No valida ni invalida el gameplay porque el
  título no alcanzó a iniciar.

## Alcance pendiente

Los pisos 1–2 aún usan reparaciones geométricas provisionales; los enemigos
conservan IA/daño de contacto provisionales. Faltan loot e inventario, paridad
exacta de RNG/generación, activación fiel de entidades y trampas, y un replay
de campaña aprobado en Vita3K que verifique la transición ya integrada. La
campaña y todos sus pisos continúan explícitamente marcados como incompletos.
