> Plan previo a la iteración 2. Estado actual y siguientes hitos: [ROADMAP.md](ROADMAP.md).

# Siguiente iteración: recursos y menú

La captura aportada por el usuario el 10 de octubre de 2026 muestra el diagnóstico en pantalla y `DATA VRA ZLIB HEADER FOUND`. Esto confirma que la aplicación presenta su framebuffer y encuentra un encabezado compatible con zlib en la ruta esperada. No confirma contenido íntegro del archivo, gameplay, audio ni funcionamiento de START. Los 30 FPS del overlay no miden rendimiento de Vagante: la aplicación dibuja una pantalla estática y espera vblank.

## Diferencia respecto a Shotgun King

El proyecto local ShotKVita ejecuta los scripts Lua recuperados del APK mediante una implementación propia de la API SUGAR. La lógica del juego estaba disponible como scripts. El usuario reportó 1–3 FPS en todas las pantallas; esa cifra corresponde a aquel runtime y no predice el rendimiento de Vagante.

Vagante conserva su lógica en código C++ compilado. EXE + PDB + Ghidra proporcionan nombres, layouts y pseudocódigo, pero hay que reconstruir las funciones y verificar su comportamiento. El runtime Lua/SUGAR no sustituye ese trabajo. Sí pueden orientar la adaptación de mando, archivos, presentación y medición de rendimiento, evaluando licencias y dependencias antes de incorporar código.

## Objetivo verificable

1. Validar el archivo completo y leer sus registros, con límites de memoria y errores registrados.
2. Mostrar una textura original aportada por el usuario, usando una ruta gráfica que pueda ampliarse a aceleración GPU; medir carga y presentación.
3. Reconstruir progresivamente MainMenu y sus dependencias de entrada, configuración, texto y recursos. Están localizadas MainMenu::MainMenu, step, draw, up, down y action. Mostrar una imagen del título no equivale a recuperar el menú funcional.
4. Implementar navegación y selección verificables; después avanzar a una habitación y al jugador.

El próximo VPK útil debe registrar etapas, errores, memoria y tiempos. No se promete que la primera iteración alcance el menú completo. Primero se recupera comportamiento medible; las optimizaciones se apoyan en sus cuellos de botella.

## Colaboración del usuario

Ya se dispone de EXE, PDB y recursos necesarios para comenzar. Para cada VPK: probar en la Vita, indicar última pantalla alcanzada, comprobar botones solicitados y entregar el log de ejecución que produzca esa versión. Un vídeo corto del menú original será útil al contrastar navegación y animaciones; puede solicitarse cuando la reconstrucción llegue a esa etapa. No es necesario volver a enviar la instalación.
