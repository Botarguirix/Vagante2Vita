# Iteración 022: ejecución nativa del ataque base

## Datos originales recuperados

Item::getWeapon (0x00626920) asigna player-stab a Sword/Dagger; nombres
confirmados en las cadenas del PE y llamadas 0x00626f21 / 0x00626f66.
Rod base usa la propiedad rod-magic. player-swing no es el ataque inicial:
Weapon::setSwordSwingAttack 0x007f45f0 lo vincula a una habilidad.

El nuevo paquete privado intro-melee.vwa se prepara desde attacks.json.
Contiene ocho keyframes de player-stab, propiedades de Sword/Dagger/Rod,
cuadros de sprite, duración relativa, origen/rotación y polígonos originales.
No se incluyen esos datos comerciales en código ni dentro del VPK/GitHub.

Weapon::startSwing (0x007f25f0) y stepSwing (0x007ef970) confirman duración
por cuadro max(1, trunc(perFrameDuration * durationFactor * (1 - IAS))).
keyFrameToWorldTransform (0x0040f4f0) confirma traslación, rotación y reflejo.

## Ejecución y alcance

**Cuadrado** inicia la estocada base. Secuencia activa, animación del personaje
 y arma rotada usan esos keyframes. Los cuadros de impacto activan los polígonos
nativos; las pruebas geométricas incluyen dirección/reflejo, rotación y ausencia
de impacto en preparación. El render solo recorre el rectángulo pequeño del arma.
Mantener Cuadrado con Rod retiene el cuadro de carga; soltar continúa la secuencia.
Sin recursos nuevos se conserva movimiento, pero se registra MELEE_BLOCKED.

Todavía no se aplica daño a enemigos/objetos, no hay sonido/effect-cues y la
carga de Rod no emite su efecto mágico. Faltan proyectiles/munición, ataques
especiales, reglas completas de cancelación/restricción y equivalencia del
orden de pasos con PC. No se añade un enemigo inventado a INTRO para simular
combate. La secuencia es avance funcional de las armas, no juego completo.

## Instalación y controles

Instalar Vagante2Vita-iteration022.vpk sobre VAGV00006. **Copiar world completo
a ux0:data/Vagante2Vita/world/**. Añade intro-melee.vwa, items-weapons.png y
stab.png de las tres clases; conserva datos de entidades y hang.png.
Mantener menu-only.vra/data.vra externos. No hay recursos del juego en el VPK.

Cuadrado: ataque base; X salto; movimiento y bordes anteriores conservados.
SELECT: ficha consultable. START pausa; Círculo reinicia; Triángulo clases.
La pausa/ficha detiene también la secuencia. START+SELECT sale.
Logs nuevos: ux0:data/Vagante2Vita/logs/iteration022/. MELEE_DATA confirma carga,
MELEE_EVENT registra cuadros/activación/carga; no se reintroducen métricas.

## Validación

Ocho pruebas CTest; lector rechaza truncado, datos no finitos, capacidad excesiva
 y basura final, conservando el estado anterior. Secuencia, carga/liberación,
no reinicio mientras activa e intersección de polígonos probados. Recorridos y
 ataques de las tres clases terminan con cuadros activos y render equivalente
entre resoluciones. Seis pruebas Python/archivo e integridad comprimida pasan.
Render local del ataque inspeccionado; VitaSDK compila. Falta prueba física 022.

## Próximo paso

Aplicación de impactos al sistema real de entidades, propiedades/efectos y
proyectiles; interacción de entrada y generación de pisos permanecen pendientes.
La auditoría del objetivo sigue mostrando alcance incompleto, según ROADMAP.md.
