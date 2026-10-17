# Instancia local de Vita3K para Vagante

La instancia de pruebas de Vagante vive bajo `_local/vita3k-vagante-dedicated/`.
Contiene su propio ejecutable y bibliotecas en `runtime/`, perfil Vita en
`filesystem/`, instalaciones en `filesystem/ux0/app/` y resultados en `runs/`.
Su `pref-path` se fija a esa carpeta; no utiliza el perfil global de Vita3K ni
el perfil de Touhou. La partición `sa0` se mantiene vacía para no copiar datos
de aplicaciones del perfil fuente. Solo se copian las particiones `vs0` y
`os0`, necesarias para el firmware HLE del emulador.

Abre `runtime/Vita3K.exe` dentro de esa carpeta desde el Explorador de archivos.
La biblioteca ya contiene VAGV00006 0.85; se puede seleccionar Vagante y pulsar
Play para una prueba visual. El replay externo está en
`filesystem/ux0/data/Vagante2Vita/autotest.vts`. Para regenerar el perfil de
prueba se puede usar `tools/test_vita3k.ps1` con `-TestRoot
_local/vita3k-vagante-dedicated`; el arnés instala el VPK y copia los recursos
validados sin copiar `sa0`. La carpeta `_local/` y los recursos externos
permanecen ignorados por Git.
