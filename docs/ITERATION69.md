# Iteración 069: shuffles de endpoints ACT_ONE

Ghidra confirma que `Level::findRandomAboveGroundTiles` baraja el grupo de
candidatos preferidos (o el grupo próximo al agua cuando no hay preferidos)
con el `std::shuffle` de MSVC de este ejecutable. La meta primero baraja los
candidatos de `findRandomReachableAboveGroundTiles`; cuando hay menos de ocho,
`addStartAndGoal` vuelve a buscar y barajar candidatos con el método de entrada.
El comparador recuperado ordena por más vecinos horizontales abiertos, soporte
sólido, tile que no sea plataforma y, finalmente, coordenada Y descendente.

`floor_layout.py` enlaza esas operaciones al MT19937 y registra los draws de
cada shuffle. Los 60 casos del barrido (20 seeds por cada piso ACT_ONE) cubren
también la rama de fallback de meta, que se activa en 11 combinaciones. La
semilla 12345/floor0 ahora da entrada `[34,6]`, meta `[38,29]` y ruta topológica
de 55 pasos. El export de prueba está en
`_local/generation071/map-export`; el VWI2 tiene SHA-256
`b410f43eb5b429c339643715f28d1ebb3f1d1f60803aaf77803d90bb6d9edcbe`.

La suite Python completa pasa 139 pruebas. La ruta con la física C no pudo
ejecutarse: no hay compilador host disponible y WSL devuelve `E_ACCESSDENIED`.
No se modificó el runtime Vita ni se compiló un VPK; 00.65 sigue siendo el último
paquete. La semilla post-entidades sigue incompleta por hooks de entidades aún
sin modelar y el mapa bloqueante G/H no está aplicado. El algoritmo de shuffle
y el comparador están recuperados, pero el desempate de `std::sort` no está
contrastado con MSVC. Por ello los endpoints y el mapa continúan sin paridad
exacta, y ACT_ONE no se declara completo ni probado en Vita3K.
