# Iteración 01.61: replay automática de la cueva al primer piso

VTS1 ahora reserva el bit 0 de su encabezado para solicitar un inicio en el
mapa introductorio. El arranque normal también prioriza ese mapa cuando está
disponible; las replays de ACT_ONE mantienen el inicio directo en el piso 0.
Al terminar la entrada por la cueva y la salida del primer piso, la replay
especial deja una marca explícita para el verificador.

El smoke nuevo encadena la traza nativa del intro (38 ticks) con la traza
`player_step` del piso 0 (424 ticks) y espera 60 ticks tras entrar al mundo.
Genera 77 segmentos / 674 ticks. El checker requiere la carga del intro, el
contacto con la cueva, la transición `source=intro target=act-one`, la carga
del piso 0 y su transición de salida al siguiente piso. Las pruebas comprueban
el bit de inicio en intro, rechazan flags desconocidos y validan las
afirmaciones del checker.

Como cambió el formato admitido del encabezado VTS1, se regeneraron las tres
pruebas hash-bound de la campaña con la fuente actual antes de preparar el
replay. El paquete privado de campaña valida 142 archivos. Pasan 13 grupos C
host y 206 pruebas Python. El VPK se compiló y se verificaron su identidad
(`VAGV00006`, `01.61`), cabecera SELF y siete entradas permitidas.

Vita3K 0.2.1.4190 recibió el VPK y el replay desde un clon aislado completo de
2,018 archivos. Terminó con `0xC0000005` antes de crear sesión nativa; `vita3k.log`
quedó vacío y no se ejecutó el juego. El perfil de origen y la instancia
compartida no se modificaron. La validación real del paso por la cueva sigue
pendiente.

Diagnóstico posterior: se encontró el runtime 0.2.1.4170 con sus DLL de Qt y se
repitió desde un clon completo de 1,754 archivos, usando Qt/OpenGL. También
terminó con `0xC0000005` antes de la sesión nativa, con los logs del emulador,
stdout y stderr vacíos. Un intento Qt con la otra copia 0.2.1.4170, que carecía
de DLL de Qt, falló con `0xC0000135` y no se considera una prueba válida. Por
último se repitió la ejecución Qt desde un clon bajo `C:\Users\ANDYPC~1\AppData\Local\Temp`
para eliminar los espacios del path; Vita3K volvió a salir con `0xC0000005` sin
crear sesión. Esto descarta el path largo como causa única, pero aún no revela
el módulo que origina la excepción.

VPK: `_local/build-act-one-161/Vagante2Vita-iteration161.vpk` (843,131 bytes,
SHA-256 `A33D8F73C799221A54D868E871F252D546AC5C80AE953D4124A45CF5BC6DEBE4`).
Replay de entrada: `_local/intro-entry-161.vts`. La igualdad exacta de
generación y el contenido completo de ACT_ONE siguen pendientes según la hoja
de ruta.
