cmd_/home/kernel/Escritorio/LIN_2021/Practica1/FicherosP1/Hello5/Module.symvers := sed 's/ko$$/o/' /home/kernel/Escritorio/LIN_2021/Practica1/FicherosP1/Hello5/modules.order | scripts/mod/modpost -m    -o /home/kernel/Escritorio/LIN_2021/Practica1/FicherosP1/Hello5/Module.symvers -e -i Module.symvers   -T -
