# Iteración 021: estadísticas derivadas y ficha de personaje

## Evidencia e implementación

PlayerCustomizer::setStartingItems (0x0070ac30) y los enums PDB confirman
Sword=49, Dagger=45, Bow=93, Rod=79 y Arrow=3. La construcción del lote de
Rogue confirma 30 flechas también en ensamblador (0x0070b2af–0x0070b2e7).
InitializePlayer (0x00709ac0) añade Frost Nova=11 y Elec Lance=10 a Mage.
No se aplican antecedentes especiales ni equipo aleatorio al perfil base.

Player::updateStats (0x006cb380) proporciona fórmulas base de salud, velocidad,
daño melee/ranged intrínseco, velocidad de ataque, multiplicador de daño recibido,
evasión, crítico y aire. Se implementan en player_stats.c y se integran al spawn.
La velocidad de movimiento conserva 1,5 unidades/paso; la salud sigue 100/80/60.
No se inventa una barra de maná: el subsistema de hechizos aún falta.

Equipo y habilidades son estado inicial recuperado, **no ataques ni hechizos
ya ejecutables**. La ficha es de consulta, no permite equipar/consumir/castear.
Los números de daño son la base intrínseca del personaje, no el resultado de
un golpe con arma. Bonificaciones de equipo, afinidades subidas, antecedentes,
estados y cálculo completo de combate permanecen pendientes.

## Controles e instalación

Instalar VPK 021 sobre la misma aplicación VAGV00006. No hay recursos del juego
dentro del VPK; los externos world y menu-only.vra/data.vra de 020 no cambian.
Si ya copiaste world de 020 no necesitas volver a copiarlo.

En el mundo, **SELECT abre/cierra la ficha**; izquierda/derecha recorre páginas
Estadísticas base / Equipo inicial / Habilidades iniciales. Círculo cierra sin
reiniciar. La física se detiene al consultar; al cerrar se sincroniza la entrada
para evitar convertir una pulsación antigua en un salto. Si se abrió estando
pausado, la pausa se conserva. START pausa fuera de la ficha, Triángulo vuelve
a clases y START+SELECT sale. Círculo fuera de ficha reinicia como antes.

Sin métricas integradas de FPS/CPU. Las estadísticas de la ficha pertenecen al
personaje. Logs nuevos: ux0:data/Vagante2Vita/logs/iteration021/.
PLAYER_BASE/START_ITEM/START_SKILL registran inicialización y CHARACTER_PANEL
registra apertura/página/cierre. Los logs anteriores se preservan.

## Validación

Siete pruebas CTest, seis pruebas Python e integridad comprimida correctas.
Cobertura: valores de las tres clases, cantidades/IDs/skills, límites de evasión
/crítico, clases inválidas sin modificar estado y conservación del movimiento.
Recorrido/render de tres clases y entidades/resoluciones validado. Nueve
páginas generadas por el mismo render que usa Vita; páginas Rogue de estadísticas
/equipo y Mage de skills inspeccionadas, sin recortes ni decimales omitidos.
VitaSDK compila. Falta prueba física de SELECT, pausa/cierre y páginas.

## Pendiente para completar

Inventario funcional, ejecución de armas/hechizos, efectos de equipo,
afinidades/antecedentes, interacción de entrada y generación de niveles,
audio/guardado y movimientos condicionados por habilidades. Se conserva el
objetivo completo activo. Véase GOAL_AUDIT21.md y ROADMAP.md.
