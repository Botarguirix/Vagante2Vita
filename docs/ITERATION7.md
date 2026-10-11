# Iteración 007: una sola aplicación

Se comparó la configuración con el proyecto local Touhou75Vita: éste mantiene TITLE_ID entre iteraciones. Vagante2Vita adopta el mismo criterio.

**Identidad fija: VAGV00006, nombre Vagante2Vita.** Versión nueva 00.07. Instalar el VPK actualiza la aplicación 006 que ya contiene el arte de LiveArea; no crea otra burbuja. Conserva icono, fondo, caché, render y controles de 006. No hay nuevo gameplay en esta entrega.

Las burbujas históricas VAGV00001, VAGV00004 y VAGV00005 ya instaladas no desaparecen con este cambio. No se borran ni migran automáticamente. La identidad estándar de futuras entregas es la burbuja VAGV00006.

## Iteraciones separadas en PC

Se conserva artifacts/iteration007 junto a 001–006. El código se guarda en un commit/snapshot distinto y build-iteration007 no modifica compilaciones anteriores. Los VPK anteriores quedan intactos; instalar uno con la identidad estándar puede sustituir esa misma aplicación y volver a una versión previa, sujeto al instalador.

## Configuración y logs

vita/iteration.cmake concentra identidad, número y versión. Para la siguiente iteración cambiar sólo VAGANTE_ITERATION y VITA_VERSION; mantener VITA_TITLEID y VITA_APP_NAME. El nombre del VPK, BOOT y carpeta de logs derivan automáticamente de esos valores.

Logs nuevos en ux0:data/Vagante2Vita/logs/iteration007/session-NNNNNN.log. Los anteriores y recursos en ux0:data/Vagante2Vita se conservan. El manifest de entrega registra la identidad leída del SFO dentro del VPK.

Instalar artifacts/iteration007/Vagante2Vita-iteration007.vpk con VitaShell. El proceso de entrega abre la carpeta en el Explorador de Windows al finalizar, según la preferencia del usuario. Compilación/paquete/identidad comprobados en PC; actualización física pendiente de confirmar.

Esta política sustituye la indicación anterior de usar un TITLE_ID distinto por iteración en ITERATION4.md. Separar entregas no requiere duplicar aplicaciones instaladas.
