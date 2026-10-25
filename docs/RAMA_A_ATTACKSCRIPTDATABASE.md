# Rama A — AttackScriptDatabase

Inventario completo de `pseudo_index.py ... --unit AttackScriptDatabase`. Contrato PDB intacto; implementación en AttackScriptDatabase.cpp y AttackScriptDatabase_load.cpp. Evidencia local, no versionada, en `_local/gui-db/`.

| Función indexada | Estado |
|---|---|
| AttackScript::AttackScript | traducida: constructor con nombre @0040f160; copia @00411060 implícita de C++/STL |
| AttackScript::aimToWorldTransform | traducida @0040f680 |
| AttackScript::keyFrameToWorldTransform | traducida @0040f4f0 |
| AttackScript::operator= | asignación miembro a miembro implícita de C++/STL, sin redefinir el contrato |
| AttackScript::worldAimDegrees | traducida @0040f420, incluida conversión a grados omitida por Ghidra |
| AttackScript::worldAimOrigin | traducida @0040f2d0 |
| AttackScript::worldKeyFrameDegrees | traducida @0040f350, incluida conversión a grados omitida por Ghidra |
| AttackScript::worldKeyFrameOrigin | traducida @0040f250 |
| AttackScript::~AttackScript | destructor miembro a miembro implícito de C++/STL |
| AttackScriptKeyframe::AttackScriptKeyframe | traducida: constructor por defecto @0040f0d0; copia @00414cc0 implícita |
| AttackScriptKeyframe::~AttackScriptKeyframe | destructor implícito de C++/STL |
| AttackShape::containsPoint | traducida; orientación, bordes y polígono vacío conservados |
| AttackShape::vertexIndicesCloseToPoint | traducida; distancia inclusiva 0.5 y orden de índices |
| AttackShape::~AttackShape | marcada «−»; destructor STL implícito |
| EffectCue::EffectCue | copia @00414df0 implícita de C++/STL |
| EffectCue::~EffectCue | destructor implícito de C++/STL |
| WeaponProperties::WeaponProperties | traducida: constructor con nombre @0040f000; copia @004112a0 implícita |
| WeaponProperties::operator= | asignación miembro a miembro implícita de C++/STL |
| WeaponProperties::~WeaponProperties | destructor implícito de C++/STL |
| attackScriptExists | traducida; consulta a la base global y VLOG(DEBUG) al faltar |
| getAttack | traducida; copia por valor desde mapa global, out_of_range al faltar |
| getWeapon | traducida; copia por valor desde mapa global, out_of_range al faltar |
| loadScripts | traducida @004114d0; ataques, keyframes, formas, efectos y armas |
| weaponExists | traducida; consulta a la base global y VLOG(DEBUG) al faltar |
| saveScripts | excluida: serialización del editor; cuerpo vacío con comentario |
| lambda_4eb003509a6976945747410a26579477::operator() | traducida como readShapes @00412af0 |
| lambda_78e4bb2a254cfbb04c66bc5c53452977::operator() | traducida como readEffects @00412680 |
| lambda_00a69df82417f841a093ea7ab33a1c36::operator() | excluida: writer de formas del editor, sin llamador portado |
| lambda_c54a067eddf4c0b506e9ad2057c3cf11::operator() | excluida: writer de keyframes del editor, sin llamador portado |
| lambda_ff40d5093b97582909a5612ab0a78e0f::operator() | excluida: writer de efectos del editor, sin llamador portado |
| magVector2f | traducida @004196e0; multiplicaciones, suma y raíz en float |
| atan2f | wrapper de ABI/CRT @00419670; sustituido por std::atan2(float,float), sin redefinir el CRT |
| powf | wrapper de ABI/CRT; std::pow del runtime, no necesita implementación del juego |
| sqrtf | wrapper de ABI/CRT @004196a0; sustituido por std::sqrt(float) |

También se recuperó initialize del inline en ResourceManager::runTasks @0074f707ff: guarda global, carga única y marca initialized incluso si falla la lectura. Constructor de la base con initialized=false. Los constructores inline de AttackShape/EffectCue usados en los lectores construyen sus contenedores; EffectCue fija además shakeAmount=5, shakeTime=5, particleType=0, particleCount=1 y particleBits=0. No se inventan valores para escalares que el lector siempre sobrescribe. Otros helpers sin entrada de pseudocódigo en esta unidad (consultas/listas, mutaciones del editor y collidingShapes) siguen pendientes como stubs.

Las sobrecargas de construcción/copia están contabilizadas: --show revela más entradas que el resumen por nombre de --unit. Las copias, asignaciones y destructores generados por MSVC se expresan mediante los miembros STL normales; no se escriben sobrecargas inexistentes en la cabecera. Se revisó inline_accessors.cpp: no contiene accessors de esta unidad y no se duplicó ni añadió ninguno. No se tocaron Client, NetMenu, GuiCursor ni ConfigGui.

## Evidencia y constantes

call_args.py y desensamblado verifican que el primer float es aimDegrees y el segundo parentDegrees: rotate(parent), scale(reflejo horizontal), translate(aimOrigin), rotate(aim), y finalmente translate(origin)/rotate(originRotationDegrees) para keyframes con useKeyframeAimOrigin. Las funciones worldOrigin transforman (0,0); worldDegrees transforman (0,0) y (1,0), restan y aplican atan2. Las instrucciones mulss de 0040f402/0040f4d2 usan el float de 00b20598: **57.2957763671875**, perdido en el retorno del pseudocódigo. No se devolvieron radianes por seguir esa salida incompleta.

field_writes.py y offset_name.py se usaron para contrastar campos. La herramienta interpreta mal los tipos anidados (los trata como la clase externa, por ejemplo imprime attackScripts+0x88 en lugar de AttackScript+0x94), por lo que prevalecen los offsets de cabecera y el asm: +94/+95 son useAfterImages/useKeyframeAimOrigin; +98 es defaultProjectileType=-1; movl 0 a +9C limpia cuatro flags; movl 1 a +A0 deja maintainProjectileVelocity=true y los tres siguientes=false. Los inmediatos 3f153f7d/3f0ac083 fijan aimArmAnchor=(.583f,.542f), y el ancla del portador es (.5f,1.f). El arma usa atlas items/items-weapons.png, frame1, dimensiones20×20, ancla(0,.5), duración5, daño1–2, knockback1, velocidad−1 y proyectil−1. Keyframe inicial: frames1, enabled=true, escala(1,1), duración1 y otros flags=false.

No se consume RNG. containsPoint prueba cada semiplano y no lee enabled: acepta bordes y polígonos vacíos y conserva el efecto del winding al reflejar. Selección de vértices usa sqrt(x*x+y*y)<=.5, sin cambiarla por distancia al cuadrado o hypot.

## Carga y límites

Carga attacks.json local o ResourceManager::getAttackData si existe config y está cargado el archive, con el getter privado existente recuperado por instanciación explícita como en PaletteDatabase. Mantiene el orden JSON, campos completos, isChargeFrame opcional (false al faltar), borrado de ataques antes de armas, y duplicados con última entrada ganadora. getAttack/getWeapon/existence y la inicialización conservan la especialización del ejecutable al objeto global incluso si el receptor es otro objeto.

Las guardas para JSON inválido/ausente evitan asserts/accesos nulos; parse fallido conserva mapas anteriores. Si falta o tiene tipo erróneo un campo tras empezar la carga se diagnostica y se conservan los cambios parciales ya hechos. Para keyframes vacíos se diagnostica y devuelve identidad en lugar del módulo por cero original. Estas diferencias solo afectan entradas inválidas. Las tablas estáticas ATTACK_SOUND_TO_STRING y EffectCue::TYPE_TO_STRING no figuran en este inventario y siguen como TODO dinámico; no se usan para leer sus enums numéricos. No se portó el editor.

Fue necesario ajustar **tools/pdb_stubs.py**: su detección solo reconocía métodos y main, por lo que volvía a emitir magVector2f y fallaba el enlace con definición duplicada. El cambio reconoce definiciones de funciones libres con retorno escalar y cuerpo, no declaraciones. Se comprobó con un fixture temporal de definiciones, declaraciones, main y retorno auto: pasó; la regeneración elimina exactamente ese stub adicional (free_stub 26→25). No se modificó ninguna cabecera generada.

## Verificación

`pwsh tools/build_recon_pc.ps1` terminó en **OK:**. `pwsh tools/run_recon_tests.ps1 -Unit AttackScriptDatabase` pasó con asserts habilitados. Casos: defaults, orden de transformaciones/reflejo, vuelta de índice, grados, winding/bordes/vacío, umbral de distancia, carga disco/archive, todos los grupos de campos, enum/bit31, isChargeFrame ausente, duplicados, copia profunda, mapa global, excepciones, parse fallido, ausencia de attackData, inicialización única, ausencia de RNG y editor sin escrituras.

Prueba adicional de datos reales: extracción selectiva mediante tools/vra.py, solo a `_local/gui-db/original-attacks.json`; SHA-256 del JSON **67a33ce84afb5f30689568e5c6623875bbea0760567aa132957f7d3ac1991c04**. El loader C++ cargó **31 ataques y 48 armas**, con comprobación de cada consulta y valores finitos en las formas/efectos. La variable opcional VAGANTE_ATTACK_FIXTURE permite repetirla sin versionar recursos comerciales. Esto acredita el parser, no la ejecución de ataques durante gameplay.

Logs: `_local/gui-db/AttackScriptDatabase_build.log` y `AttackScriptDatabase_test.log`. Se repitieron además los tests GameGui y RoomDatabase sobre el enlace final. Sin push; sin validación de gameplay en Vita/Vita3K.
