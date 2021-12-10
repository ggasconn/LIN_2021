# Dudas
* Hay que rodear los accesos a emergency_threshold, code_format... con un mutex?
* Se planifica el envio de valores antes o después del kfifo_in?
* Sacar datos del buffer circular con strlen(code_format)? Si me cambian el formato se rompe esta función...
* Cuando se hace read de la entrada /proc/codetimer, debe de estar en bucle infinito hasta que se pulse ctrl-c o espera a que haya algo en la lista y simplemente lo devuelve?