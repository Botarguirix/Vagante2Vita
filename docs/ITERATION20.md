# Iteración 020: entidades del mundo inicial original

## Evidencia física 019

La sesión aportada confirma carga desde ux0, recorridos/agarres de Knight,
reinicio y salida normal. Alterna los tres perfiles de menú; no contiene
mediciones externas de FPS/CPU. Sigue faltando knight/hang.png y usa climb.

## Recuperación e integración

El room INTRO original contiene 22 registros: 2 antorchas (DoodadType=3),
4 flores (13), 10 Firefly, 1 marcador de victoria (11), 4 de salto (12) y
1 trofeo (15). En Doodad::setTypeVars 0x0059a6e0, los marcadores no crean
Sprite y el trofeo consulta Stats::unlocks. No se inventan sprites ni desbloqueos.
Firefly::setLevel 0x004676b0 confirma little_firefly.png con cuadros de 16×16.

El nuevo paquete privado intro-entities.vwe conserva todos los registros y
coordenadas. El cargador valida tipos, posiciones, límite 64 y longitud exacta,
y solo aplica los datos al validar todo el archivo. Dibuja antorchas, flores y
luciérnagas en sus posiciones originales, con animaciones de atlas para flores
y luciérnagas. Solo recorre sus rectángulos visibles; sin lights/shadows ni
recomposición de capas. No se habilita iluminación que no esté reconstruida.

Alcance parcial de entidades: luciérnagas aún permanecen en su posición de
origen (falta trayectoria/RNG/tinte/iluminación exacta); antorchas no emiten
brasas del ParticlePool; flores no tienen interacción/pickup; marcadores se
preservan pero sus acciones no se ejecutan. Trofeo permanece oculto mientras
falta lectura de desbloqueos. El objetivo del juego completo no está terminado.

## Instalación

Instalar Vagante2Vita-iteration020.vpk sobre la misma burbuja VAGV00006.
**Copiar world completo de esta entrega a ux0:data/Vagante2Vita/world/**,
conservando las carpetas de clases y la nueva carpeta entities. Esto añade
intro-entities.vwe, torch.png, flower_1.png, little_firefly.png y los hang.png
que faltaban. Los recursos van aparte, nunca dentro del VPK o GitHub.
Mantener menu-only.vra/data.vra externos como antes.

NEW GAME → clase → X. Controles de movimiento/bordes y pausa anteriores.
Perfil ligero predeterminado; R alterna perfiles del menú. Sin métricas internas.
Logs nuevos: ux0:data/Vagante2Vita/logs/iteration020/.
WORLD_ENTITIES ready=1 count=22 confirma carga. Si falta el nuevo archivo,
la geometría/movimiento anteriores siguen disponibles y se registra el error.

## Validación

Seis pruebas nativas incluyen paquetes de entidades inválidos/truncados/
excesivos y preservación del estado anterior. Seis pruebas Python e integridad
comprimida. Recorridos de las tres clases, cambio de imagen al habilitar entidades,
animación con jugador fijo y equivalencia entre resoluciones. Imagen local
inspeccionada; falta validar 020 y coste de decorado en la consola.

Próximos sistemas: interacción real con entrada, generación original del piso,
combate, equipo, estadísticas derivadas, audio y guardado. Véase ROADMAP.md.
