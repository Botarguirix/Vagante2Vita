# Iteración 123: transporte de prueba del Goblin King

Se conectó un registro VWE3 de tipo 49 con la carga y el dibujo del atlas
`bosses/goblin_king/stand.png`. El loader valida coordenadas, variante y tipo;
el cuerpo de colisión usa 14×12 píxeles del constructor nativo. El renderer
usa la celda 32×16 y el offset observados en `BossGoblinKing::setLevel`
(`0x00515c70`). Esto deja una ruta de datos comprobable desde el paquete de
entidad hasta el draw call.

El soporte sigue siendo únicamente de renderizado: no se crea ningún registro
de jefe desde el exportador ACT_ONE, las campañas no llevan ese registro y el
runtime no tiene IA, ataques, daño, muerte ni transición. El test de paquete
demuestra aceptación/rechazo y preservación de estado ante un subtipo inválido;
no demuestra un encuentro jugable.

## Validación

- Suite host Windows: 12/12 pruebas, incluidas las de `world_entities_test`.
- VPK VitaSDK iteración 112 compilado: `_local/build-act-one-boss-render/Vagante2Vita-iteration112.vpk`, 836 665 bytes, SHA-256 `47E2F5CC034D7D9571A462C2B2440F479B48021DA34D70AA9E9AC2EDD2B02130`.
- Bundle privado validado: 137 archivos; la replay se generó con 545 pasos para los tres pisos.
- Vita3K aislado expiró a los 90 segundos antes de crear una sesión nativa; stdout, stderr y log del emulador vacíos. No es una prueba de arranque ni de campaña. Evidencia: `_local/vita3k-boss-render-dedicated/runs/iteration112/run-000002/`.
- No se probó el render del Goblin King en Vita3K: el bundle usado no contiene un registro de jefe. El lanzador omitía además `bosses/` al copiar un paquete raíz; ya quedó corregido para que las siguientes pruebas entreguen esos atlas al runtime.

## Siguiente trabajo

Recuperar una ruta ejecutable de jefe con la evidencia Ghidra y reproducir el
contrato completo del spawn. En paralelo, diagnosticar el bloqueo de Vita3K
antes del frontend/título; el intento de `--help` en el clon también quedó
esperando sin salida. ACT_ONE continúa incompleto.
