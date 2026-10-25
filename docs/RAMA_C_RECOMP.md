# Rama C: recompilación estática de vagante.exe

Fecha: 25 de octubre de 2026. Prueba de viabilidad, en paralelo a la rama A (que sigue en su carpeta).

## Idea

En vez de traducir el juego función por función a mano (rama A), un traductor convierte
**todas** las funciones de `vagante.exe` (x86) a C de una vez, y el trabajo se concentra en
una capa de plataforma (Win32, OpenGL, OpenAL, entrada). El traductor viene del proyecto
Touhou123Vita (`recomp/`), copiado y apuntado a `vagante.exe` Steam 1.10.1
(SHA-256 `9af1d8cc…624c`).

El C generado y los informes van a `artifacts/recomp/<sha>/` y **no se versionan**.

## Encaje de Vagante

`vagante.exe` (8 MB) trae SFML enlazado dentro e importa `OPENGL32` (lo cubre vitaGL),
`OpenAL32` (openal-soft ya funciona en Vita), Win32, DirectInput/XInput y `steam_api`.
Touhou tuvo que emular Direct3D 9; aquí los gráficos son OpenGL de función fija.

## Resultados

| Paso | Resultado |
|---|---|
| Descubrimiento de funciones | 34 392 funciones, 99,68 % del código ejecutable explicado, 586 tablas de switch |
| Traducción sin SSE (traductor de Touhou) | 148 088 instrucciones sin soporte (128 910 en código del juego), casi todas SSE |
| Traducción con SSE (`recomp/lift/sse.py`) | **241** sin soporte en el juego (MMX y bytes mal decodificados) y 2 265 en librerías (AVX/CRT) |
| Compilación PC (gcc -O1) | **las 34 392 funciones compilan y enlazan**; 173 unidades, 269 s, ejecutable de 43 MB |
| Test diferencial SSE (`recomp/tests/sse_diff.py`) | 176 instrucciones × 1 000 casos contra Unicorn: **0 diferencias** |

### Soporte SSE

- `Cpu` del runtime tiene `xmm[8]` (vista de bytes, palabras, enteros, floats y doubles) y `mxcsr`.
- `recomp/lift/sse.py`: movimientos, aritmética escalar y empaquetada, lógica, comparaciones,
  `comiss`/`ucomiss`, conversiones, shuffles/unpacks, enteros empaquetados, shifts.
- Semántica x86 donde C difiere: conversión float→int fuera de rango o NaN da `0x80000000`;
  `min`/`max` devuelven el segundo operando con NaN o ceros; en add/sub/mul/div con NaN se
  devuelve el primer NaN silenciado (macros `SSE_ARITH_*`; `-DSSE_FAST_NAN` las quita si
  cuestan rendimiento en la Vita).
- `movsd` con operandos es SSE; sin operandos sigue siendo la instrucción de cadenas.

### Excepción del oráculo

Con NaN en la misma posición de **ambos** operandos, Unicorn (QEMU) a veces devuelve el NaN del
segundo; el manual de Intel y el hardware devuelven el del primero. Esos casos no se comparan
(`qemu_nan_pair` en el test).

## Pendiente / riesgos

1. **Capa de plataforma**: cargar la imagen, Win32 (kernel32/user32/gdi32), OpenGL→vitaGL,
   OpenAL, DirectInput/XInput→controles de la Vita, `steam_api` anulado. Es el grueso del trabajo.
2. **Tamaño y rendimiento en la Vita**: compilar para ARM y medir el `.text` y el arranque.
   Touhou corre ~60× más lento que nativo; si Vagante no llega a ~30 fps en menús, se vuelve a la rama A.
3. MMX (unas decenas de instrucciones en el juego) si alguna ruta llega a ejecutarlas.
