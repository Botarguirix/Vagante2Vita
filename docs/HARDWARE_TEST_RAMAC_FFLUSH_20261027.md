# Prueba en hardware — rama C, fflush del log (2026-10-27)

## Paquete identificado

- Commit de código: be032c6 (`rama-c - fflush del log`).
- VPK: `artifacts/recomp/9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c/vita/Vagante-ramaC.vpk`.
- SHA-256: `c77592597c665d79614a092a63561138ea824a9a885309bb3c808943ba9b73c5`.
- TITLE_ID: VAGA00002; APP_VER: 00.01. La versión SFO no distingue esta compilación: usar el hash.
- GCC ARM 15.2.0, -Os, ventana guest 288 MiB, pila principal 8 MiB.
- .text ARM: 23.118.634 bytes. eboot.bin: 15.993.763 bytes.
- Snapshot generado: 168 unidades; SHA-256 `7fb23c2072cad5c6f4febd2eacac37d4bd32e8b7690354c5eb04048553341ceb`.

## Preparación

Instalar el VPK con VitaShell. Conservar una copia de los logs anteriores antes
de iniciar, porque el juego puede sobrescribirlos. El VPK no incluye datos comerciales.
Los archivos de la copia del usuario van en:

```text
ux0:data/Vagante/vagante.exe
ux0:data/Vagante/data.vra
```

El ejecutable esperado es Steam 1.10.1, SHA-256
`9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c`.
No sustituirlo por otro ejecutable. Conservar las mismas opciones y datos al
comparar tiempos. No borrar partidas/configuración para esta prueba.

## Qué probar y registrar

1. Arrancar y medir desde pulsar la burbuja hasta terminar la carga. Anotar si se
   alcanza el menú, o el último texto visible y el tiempo si se detiene.
2. Si permite jugar, comprobar intro → cueva, entrada al piso 0, jefe y transición
   al piso 1. Registrar qué pasos fueron realmente alcanzados, FPS percibidos,
   cuelgues y HP al llegar al piso 1. No se presume que esta rama ya los complete.
3. Tras una carga, cerrar la aplicación desde el sistema y copiar inmediatamente
   los logs. Comprobar si la última línea del log del juego queda completa.
4. Si falla espontáneamente, recoger los logs antes de volver a abrirlo; no hace
   falta provocar un fallo. Anotar el último mensaje y si faltó algún archivo.
5. Para comparar rendimiento con una compilación previa, identificar ambos VPK por
   hash y alternar al menos siete cargas con las mismas opciones. Separar las
   cargas de calentamiento. Comparar medianas; el resultado PC no garantiza el
   límite del 2 % en la consola.

Recoger, si existen:

```text
ux0:data/Vagante/logs/log.log
ux0:data/Vagante/recomp-trace.log
ux0:data/Vagante/recomp-result.txt
ux0:data/Vagante/recomp-screen.bmp
```

`recomp-result.txt` y la captura se escriben al pasar por platform_finish;
pueden faltar en un cierre forzado. El log de juego y el trace son distintos:
la tarea 10 vacía los WriteFile de archivos guest abiertos para escritura.
fflush entrega datos al sistema; no garantiza supervivencia a corte de energía.

## Evidencia disponible

Build completo y comprobación VPK: siete entradas, cero errores y advertencias.
Pruebas PC fileio repetidas el 2026-10-27: OK, incluida terminación forzada de
hijos de prueba. Medición PC previa: mediana 8,566249 → 7,834515 s, siete pares;
no se atribuye la diferencia a una aceleración por fflush.

Pendiente: ejecución en Vita física, tiempos de carga y gameplay. No se ejecutó
Vita3K ni se afirma validación de gameplay por compilar o pasar pruebas host.