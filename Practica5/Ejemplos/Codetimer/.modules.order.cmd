cmd_/home/kernel/Escritorio/FicherosP5/Ejemplos/Codetimer/modules.order := {   echo /home/kernel/Escritorio/FicherosP5/Ejemplos/Codetimer/codetimer.ko; :; } | awk '!x[$$0]++' - > /home/kernel/Escritorio/FicherosP5/Ejemplos/Codetimer/modules.order