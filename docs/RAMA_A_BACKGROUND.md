# Background — reconstrucción rama A

2026-10-24. Cabeceras PDB sin cambios; sin push.

| Función del índice | Estado |
|---|---|
| Background::Background @004f97b0 | Traducida: inicializa tipo, contenedores y actualiza textos. |
| Background::~Background @004f9860 | Traducida: destrucción STL automática. |
| Background::updateNameAndDescription @004f9890 | Traducida: 32 casos, limpieza de efectos, traducción en orden, fallback para tipos desconocidos. |
| lambda_fa45034264dd1bed06edf3926bde1a04 @004fbe90 | Traducida como good: añade a positive. |
| lambda_aef388607e34fa12e99bcf2430279079 @004fbf00 | Traducida como bad: añade a negative. |

Evidencia: pseudocódigo por función; argumentos perdidos recuperados con call_args.py y PE.cstring. La tabla de saltos @004fbe10 fija los 32 valores numéricos: los nombres de enums de Ghidra están desfasados entre 19 y 23; se usan las cabeceras PDB. No se añaden reglas de juego ni llamadas RNG.

Peculiaridades conservadas: Death tiene el mismo efecto en positive y negative; Chaos tiene el literal +??? en ambos, sin pasar por traducción. No son placeholders de esta reconstrucción. Los textos largos se conservan exactamente como en el ejecutable local.

checkRequirements no figura en --unit Background: permanece stub, sin inventar restricciones.

Verificación: pwsh tools/build_recon_pc.ps1 -Run, compilación correcta y cierre normal con exit code: 0; pwsh tools/run_recon_tests.ps1 -Unit Background pasó (32 casos, cantidades/orden de efectos, cambio de tipo, idempotencia, enums y tipo inválido). Evidencia local: _local/gui/Background_build.log y Background_test.log. No acredita gameplay Vita.
