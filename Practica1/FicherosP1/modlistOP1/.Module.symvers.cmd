cmd_/home/kernel/Escritorio/LIN_2021/Practica1/FicherosP1/modlistOP1/Module.symvers := sed 's/ko$$/o/' /home/kernel/Escritorio/LIN_2021/Practica1/FicherosP1/modlistOP1/modules.order | scripts/mod/modpost -m    -o /home/kernel/Escritorio/LIN_2021/Practica1/FicherosP1/modlistOP1/Module.symvers -e -i Module.symvers   -T -
