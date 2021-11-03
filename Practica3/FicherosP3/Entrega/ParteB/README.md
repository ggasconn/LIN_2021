# Blinkstick

## Ejecucion
`./blinkShow [police/bounce/guess/game]`

## Descripción de los modos
* `police`, imita las luces de la sirena de un coche de policia.
* `bounce`, elige un color del pool aleatorio y mueve una pelota a lo largo del blinkstick en un sentido y en otro, como si rebotase.
* `guess`, pide al usuario mediante la entrada estandar un número de 0-7 y en caso de acierto el blinkstick se vuelve verde, en caso de fallo rojo. Antes de mostrar el resultado, realiza una animación de espera.
* `game`, usa dos blinsticks. Se eligen dos números al azar que se van a comparar:
    * El ganador se pone en verde, el perdedor en rojo.
    * Si quedan empate, ambos se ponen en amarillo.
