# Viabilidad técnica

Evaluación del 10 de octubre de 2026. **Conclusión: un port nativo parece técnicamente plausible, pero no está demostrado. Un port directo del ejecutable Windows no es viable.** La ruta principal propuesta es reconstruir código portable y sustituir las dependencias de plataforma. Obtener el código fuente de los autores reduciría muchísimo el trabajo; no se dispone de él.

## Evidencia de la instalación local

| Dato | Resultado observado |
|---|---|
| Ejecutable | PE32, máquina 0x014c (Intel x86), 8 322 560 bytes |
| SHA-256 del EXE | `9af1d8cc204aca2be94f597af21ced50d77d09c1ff8f998def5f2920c3c4624c` |
| PDB | 64 974 848 bytes, MSF 7, 1 128 streams |
| Símbolos públicos | 24 487; incluye datos y bibliotecas, no es cantidad de funciones del juego |
| Correspondencia EXE/PDB | GUID y age coinciden; age 21 |
| Render/audio | Importaciones OpenGL32/OpenAL32 y símbolos `sf::` |
| Plataforma | Win32, DirectInput, XInput, Winsock y Steamworks |
| Recursos | `data.vra`, zlib, 99 768 748 bytes comprimidos; 124 879 064 descomprimidos |
| Entradas de recursos | 2 705 registros, 2 nombres repetidos; encabezado 2 702 y diferencia 3 |
| Contenido | 2 395 PNG, 272 WAV, 32 OGG, TTF, OTF, 3 JSON, 1 shader fragment |

Los símbolos incluyen clases Player, Level, Entity, Server, ResourceManager y tipos SFML. Esto confirma C++/SFML en esta compilación; no confirma la versión exacta de SFML. La versión comercial del juego tampoco se infiere a partir de la fecha del archivo o del contador del archivo de recursos. El PDB conserva información de tipos; algunas estructuras no se reconstruyen correctamente y deben verificarse.

## Rutas evaluadas

| Ruta | Evaluación |
|---|---|
| Recompilar fuente original | Mejor opción si los autores la proporcionan; requiere portar backend y dependencias |
| Recompilar el pseudocódigo de Ghidra sin intervención | No: conserva ABI, offsets, estructuras parciales, convenciones MSVC y llamadas de plataforma |
| Reconstruir C++ a partir de EXE + PDB | Candidata principal con los archivos disponibles; exige reconstrucción y validación de comportamiento |
| Cargar una biblioteca Android ARM | No se encontró una compilación Android ARM utilizable; la copia local es x86 Windows |
| YoYoLoader | No hay evidencia de GameMaker; la compilación examinada usa C++/SFML |
| Emular/traducir x86 Windows | No evaluada con prototipo; sobrecoste y Win32 la hacen una apuesta peor que el port nativo |

PS Vita requiere código ARM y sus propias APIs. El port de GTA SA demuestra la ruta de carga de una biblioteca Android ARM compatible, pero no transforma ejecutables x86 a ARM ([proyecto original](https://github.com/TheOfficialFloW/gtasa_vita)).

## Adaptaciones necesarias

- **Gráficos:** sustituir ventana/contexto Win32 de SFML; evaluar un subconjunto SFML sobre SDL2/vitaGL o reemplazar sus llamadas por un backend propio. El shader local usa semántica GLSL de escritorio y gamma; necesita conversión o equivalente. No se presupone compatibilidad completa de vitaGL ([vitaGL](https://github.com/Rinnegatamante/vitaGL)).
- **Audio:** adaptar OpenAL o implementar audio equivalente con bibliotecas Vita; transmitir música OGG y limitar canales/decodificación simultánea.
- **Entrada:** reemplazar XInput/DirectInput por controles Vita; definir mapeo sin teclado/ratón.
- **Archivos:** resolver rutas con `ux0:data/Vagante2Vita`; mantener datos y guardados independientes de la instalación Steam.
- **Steam/red:** separar Steamworks de lógica local; empezar con un jugador offline. El multijugador requiere trabajo posterior, no basta con sustituir sockets.
- **ABI:** rehacer convenciones `thiscall`, RTTI, excepciones, contenedores MSVC y layouts de clases; usar tipos explícitos cuando haga falta.
- **Memoria y rendimiento:** no cargar todos los PNG decodificados. Los 119 MiB del archivo descomprimido no representan el coste RAM/GPU. Medir atlas, texturas, buffers, audio y entidades en consola real.

La estética 2D por sí sola no demuestra rendimiento suficiente. No hay mediciones de FPS ni RAM de Vagante en Vita. El objetivo inicial de 30 FPS es un criterio de prueba, no una promesa; 60 FPS se evaluará después.

## Criterios para avanzar o detener la ruta

Continuar si podemos reconstruir un bucle determinista, representar una habitación con recursos del usuario y ejecutar jugador/colisiones en ARM con presupuesto de memoria y tiempo medidos. Revisar la estrategia si tipos críticos no se recuperan, dependencias de plataforma dominan la lógica o el primer segmento jugable incumple presupuesto aun tras optimizar. No se ha demostrado ninguna imposibilidad absoluta del juego completo.
