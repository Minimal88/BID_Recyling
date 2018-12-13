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

#Definicion pines de sensores
hx1 = HX711(9, 11)
#hx2 = HX711(5, 6)
#hx3 = HX711(5, 6)
#hx4 = HX711(5, 6)

#Definicion pines de botones
BTN_RESET = 2
BTN_TARE = 3



#Configuracion y referencia de sensores
hx1.set_reading_format("LSB", "MSB")
hx1.set_reference_unit(240)

#Configuracion de botones
#GPIO.setmode(GPIO.BOARD)
GPIO.setup(BTN_RESET,GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # asigna el pin como entrada
GPIO.setup(BTN_TARE,GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # asigna el pin como entrada


hx1.reset()
hx1.tare()

#Loop principal
while True:        
        #Tare a todos los sensores
        
        #Reinicia todo el sistema
        
        #Lee los datos de los sensores    
        try:                    
                #val = hx.get_weight(5)
                val1 = max(0,int(hx1.get_weight(5)))              
                #print(upload_command)

                try:
                        x = subprocess.check_output(["python",upload_command,str(val1),str(val1),str(val1),str(val1),str(val1)],stderr=subprocess.STDOUT)
                except subprocess.CalledProcessError as e:
                        raise RuntimeError("command '{}' return with error (code{}): {}".format(e.cmd,e.returncode,e.output))

                returned = x.decode("utf-8")
                print(returned)

                hx1.power_down()
                hx1.power_up()
                time.sleep(10) #cada 3 minutos hace una medicion
        except (KeyboardInterrupt, SystemExit):
                cleanAndExit()
