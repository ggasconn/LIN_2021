# Dudas
* Hay que rodear los accesos a emergency_threshold, code_format... con un mutex?
* Que entrada de /proc implementa los try_module(), solo hacen eso?
* Si no hay espacio en el buffer circular, hay que bloquearse hasta que haya? Si es así nunca se llegaría a ejecutar la llamada a la workqueue, no?
* Para la restricción de no volver a planificar el trabajo de la wq hay que llamar a flush después de crear el trabajo? O basta con volver a comprobar si se ha llegado al umbral?
* Se planifica el envio de valores antes o después del kfifo_in?
* Sacar datos del buffer circular con strlen(code_format)? Si me cambian el formato se rompe esta función...