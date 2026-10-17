# Iteración 026: ACT_ONE exportado al runtime

El arranque intenta cargar `ux0:data/Vagante2Vita/world/act-one/act-one-world.vwi`
y `intro-terrain.png`; si no están disponibles, conserva la intro anterior.
Así el nuevo ensamblado puede probarse sin sustituir la ruta establecida.
El VWI se genera localmente desde el informe de `floor_layout.py` y el atlas
del usuario; los gráficos y datos originales no se incluyen en el VPK ni en
Git.

La recuperación 032 exportó un mapa de 80×40 con entrada provisional en
(30,4) y meta en (36,28). El parser host lo cargó correctamente y confirmó las
seis propiedades de tile y que las entidades aún están pendientes. Además, el
exportador exige que la geometría de spawn del runtime (cuerpo 8×12 px con
inset de 4 px) esté libre y apoyada en un tile sólido/plataforma; el spawn de
esta semilla lo cumple con soporte tipo 1. Esto solo certifica la aparición.
El mapa
visual privado incluye una superposición del campo de agua calculado. Una
auditoría posterior comprobó toda la caja de spawn con la fórmula exacta del
runtime (`x=tile_x*16+4`, `y=tile_y*16+4`, 8×12 px), incluidas todas las celdas
atravesadas y las de apoyo; esta semilla ocupa (30,4) y apoya en (30,5), tipo 1.
El VWI y el PNG regenerados conservan sus hashes. La
selección de extremos usa candidatos y ordenación recuperados con Ghidra, pero
todavía no reproduce el barajado nativo, el estado del pool de partículas ni
el filtrado final contra entidades instanciadas. Por tanto, no es paridad exacta
ni certifica que el recorrido sea jugable. El informe posterior 034 además
confirma que entrada y salida pasan el A* topológico original: 58 pasos, 291
tiles visitados; este A* no modela gravedad, salto ni tamaño del jugador.

## Verificación

- Compilación del VPK ARM con WSL y VitaSDK: correcta.
- `tools/check_vpk.py`: sin errores/avisos; `VAGV00006`, versión `00.26`.
- Parser host del VWI2: `ready=1`, 80×40, extremos presentes.
- CTest: 11/11 correctos.
- Suite Python: 92 pruebas correctas.
- `player-test` host: mundo VWI cargado, jugador creado y 300 frames sin
  colisión sólida. Esta secuencia no intenta alcanzar la meta.
- Smoke Vita3K: pendiente. La copia dedicada vive en
  `_local/vita3k-vagante-clone`; el inicio termina con `0xC0000005` antes de
  escribir cualquier log. El VPK de control 025 produce el mismo resultado en
  el mismo runtime/perfil aislado, así que el fallo no se atribuye al cambio
  ACT_ONE de 026. Además, el ejecutable 0.2.1.4183 aislado falla con `--help`
  y `--version`, sin cargar VPK. No hay otro proceso Vita3K visible durante
  esa repetición.

El VPK privado está en `_local/build-iteration026-wsl/`. La ruta de mundo
opcional es `world/act-one/act-one-world.vwi`; para probarlo, copiar también
el paquete generado y `intro-terrain.png` al directorio de datos de la app.
El análisis/exportación privada actual está en `_local/generation036/`.
La prueba host confirma carga, spawn y estabilidad de colisiones, no render en
Vita ni recorrido hasta la meta.
Quedan por implementar la creación/comportamiento de entidades, la prueba de
recorrido físico hasta la salida, su activación y cambio de piso, regeneración
y transición desde INTRO. El primer mundo completo sigue en desarrollo.
