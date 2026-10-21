# Tareas para Codex (rama A)

Cada tarea es independiente y se puede pegar tal cual en Codex. Todas trabajan en
`C:\Users\Andy PC\Documents\Chatgpt\Vagante2Vita`, rama `rama-a`, y **solo** crean
o editan los archivos que la tarea indica (Claude trabaja en el resto).

## Reglas comunes (incluir siempre)

```
Proyecto: reconstrucción en C++ de Vagante para un port a PS Vita (rama A).
Carpeta: C:\Users\Andy PC\Documents\Chatgpt\Vagante2Vita, rama git rama-a. No hagas push.
Git: usa C:\Users\Andy PC\AppData\Local\GitHubDesktop\app-3.6.7\resources\app\git\cmd\git.exe

Fuentes de verdad, en este orden:
1. recon/include/*.h: cabeceras generadas desde vagante.pdb. Son el contrato: layout,
   nombres, firmas y vtables exactos. NO las edites (se regeneran). Si una firma parece
   mal, anótalo en el informe en vez de cambiarla.
2. Pseudocódigo de Ghidra, una función por archivo en
   C:\Users\Andy PC\Documents\Chatgpt\VaganteVita\_local\decompiled
   Búscalo por nombre con:
   python -I tools/pseudo_index.py "C:\Users\Andy PC\Documents\Chatgpt\VaganteVita\_local\decompiled" --show Clase::metodo
   y lista las funciones de una unidad con --unit <Unidad>.
3. recon/stubs/statics.cpp: constantes leídas del ejecutable. Las marcadas
   "TODO value" no estaban en la imagen: búscalas como valores inmediatos en el pseudocódigo.

Cómo traducir:
- Escribe C++ legible y fiel al comportamiento, no una copia literal del pseudocódigo:
  quita los artefactos de MSVC (std::_Tree, _Mypair, refcounts manuales de shared_ptr,
  __security_cookie, destructores inline de std::string, etc.) y usa la STL y SFML 2.5 normales.
- Conserva el orden de las operaciones, las constantes y las llamadas al RNG exactamente:
  la generación de niveles depende de ello.
- Todas las sobrecargas de un método van juntas (el generador deja de crear stubs
  para un nombre en cuanto ve su definición en recon/src).
- No uses APIs de Windows ni de Steam. Red/Steam: deja el cuerpo vacío con un comentario.

Verificación (obligatoria antes de terminar), desde PowerShell 7 en la carpeta del repo:
  pwsh tools/build_recon_pc.ps1 -Run
Regenera los stubs, compila con MSYS2 y ejecuta vagante.exe. Debe compilar sin errores,
terminar con "exit code: 0" y recon.log lista las funciones aún sin traducir que se alcanzaron.
Los errores de compilación están en _local/build-recon-pc.log. Haz commit solo de tus archivos, con un mensaje
"rama-a - traducir <Unidad>". Informe final: funciones traducidas, dudas y constantes
recuperadas.
```

## Tareas

| # | Unidad | Archivo a crear | Notas |
|---|---|---|---|
| 1 | `Log` | `recon/src/Log.cpp` | Pequeña; sirve de prueba del flujo. Guardar logs en la carpeta de datos del juego. |
| 2 | `Sprite` | `recon/src/Sprite.cpp` | Animaciones por atlas; la usan casi todas las entidades. |
| 3 | `VFont` | `recon/src/VFont.cpp` | Fuente bitmap del juego. |
| 4 | `Input` | `recon/src/Input.cpp` | Sin DirectInput: teclado y joystick por SFML. |
| 5 | `ResourceManager` | `recon/src/ResourceManager.cpp` | Grande (~26 000 líneas de pseudocódigo). Carga de `data.vra`; reutiliza el formato documentado en `tools/vra.py`. |
| 6 | Constantes | `recon/src/constants_<unidad>.cpp` | Recuperar los valores "TODO value" de `statics.cpp` de una unidad, buscándolos en su pseudocódigo. |

Claude se encarga de: `main`, `GameEngine`, el sistema de build, la capa SFML para Vita y la revisión.
