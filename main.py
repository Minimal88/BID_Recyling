#! /usr/bin/env python2

import RPi.GPIO as GPIO
import time
import sys
import subprocess
from hx711 import HX711

upload_command = "/home/pi/BID_Recyling/upload_data.py"


#Funcion para salir del programa
def cleanAndExit():
    print "Cleaning..."
    GPIO.cleanup()
    print "Bye!"
    sys.exit()

GPIO.setwarnings(False)
GPIO.cleanup()

#Definicion pines de sensores
#TODO: manejar timeout y error
hx1 = HX711(27,22)
hx2 = HX711(9,11)
hx3 = HX711(5,6)
hx4 = HX711(13,19)

print('Sensores inicializados correctamente!')

#Definicion pines de botones
BTN_RESET = 2
BTN_TARE = 3


#Configuracion y referencia de sensores
hx1.set_reading_format("LSB", "MSB")
hx1.set_reference_unit(210)
hx2.set_reading_format("LSB", "MSB")
hx2.set_reference_unit(217)
hx3.set_reading_format("LSB", "MSB")
hx3.set_reference_unit(215)
hx4.set_reading_format("LSB", "MSB")
hx4.set_reference_unit(217) #217

#Configuracion de botones
#GPIO.setmode(GPIO.BOARD)
GPIO.setup(BTN_RESET,GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # asigna el pin como entrada
GPIO.setup(BTN_TARE,GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # asigna el pin como entrada

#Resetea y Tarea todos los sensores
hx1.reset()
hx1.tare()
hx2.reset()
hx2.tare()
hx3.reset()
hx3.tare()
hx4.reset()
hx4.tare()


print('Iniciando programa principal...')
#Loop principal
while True:        
        #Tare a todos los sensores
        
        #Reinicia todo el sistema
        
        #Lee los datos de los sensores    
        try:                    
                val1 = abs(hx1.get_weight(5))
                val2 = abs(hx2.get_weight(5))
                val3 = abs(hx3.get_weight(5))
                val4 = abs(hx4.get_weight(5))
                
                print('Sensor1: '+str(val1))
                print('Sensor2: '+str(val2))
                print('Sensor3: '+str(val3))
                print('Sensor4: '+str(val4)+'\n')
                
                #val1 = max(0,int(hx1.get_weight(5)))                              
                
                #Intenta subir los datos a la nube
                try:
                        x = subprocess.check_output(["python",upload_command,str(val1),str(val2),str(val3),str(val4)],stderr=subprocess.STDOUT)
                        returned = x.decode("utf-8")
                        print(returned)
                        
                except subprocess.CalledProcessError as e:
                        raise RuntimeError("command '{}' return with error (code{}): {}".format(e.cmd,e.returncode,e.output))

                
                
                
                #Apaga y prende los sensores TODO: cambiar esto
                hx1.power_down()
                hx1.power_up()
                hx2.power_down()
                hx2.power_up()
                hx3.power_down()
                hx3.power_up()
                hx4.power_down()
                hx4.power_up()
                
                #cada 3 minutos hace una medicion
                time.sleep(180) 
        except (KeyboardInterrupt, SystemExit):
                cleanAndExit()
