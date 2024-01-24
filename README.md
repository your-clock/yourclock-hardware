# yourclock-hardware

## Configuración inicial:

 1. instalar la ultima versión del IDE de Arduino ([Software |
    Arduino](https://www.arduino.cc/en/software)) 
 2. En el IDE de arduino configurar:
	- Ir a File > Preferences > Additional boards manager URLs > copiar y pegar el siguiente link: https://espressif.github.io/arduino-esp32/package_esp32_index.json y dar clic en OK.
	- Ir a Tools > Board > Boards Manager > buscar **esp32 by Espressif Systems** e instalar
	- Ir a Tools > Board > esp32 > seleccionar **ESP32 Dev Module**
	- Ir a Tools > Manage libraries > buscar e instalar las siguientes librerías:
		 - ArduinoJson *by Benoit Blanchon* **(Versión 7.0.0)**
		 - DallasTemperature *by Miles Burton* **(Versión 3.9.0)**
		 - MD_MAX72XX *by majicDesigns* **(Versión 3.5.1)**
		 - MD_Parola *by majicDesigns* **(Versión 3.7.1)**
		 - NetApiHelpers *by Juraj Andrassy* **(Versión 1.0.1)**
		 - OneWire *by Jim Studt, Tom Pollard* **(Versión 2.3.7)**
		 - PubSubClient *by Nick O'Leary* **(Versión 2.8)**
		 - SimpleTimer *by Alexander Kiryanenko* **(Versión 1.0.0)**
		 - TelnetStream *by Juraj Andrassy* **(Versión 1.3.0)**
		 - thinger.io by *Alvaro Luis Bustamante* **(Versión 2.26.0)**

## Para subir nuevo codigo al dispositivo

### Por cable USB:
Conecte el cable USB al microcontrolador y a uno de los puertos USB del computador, luego en el IDE de Arduino, en el menú superior vaya a Tools > Port > y en la sección ***Serial ports***, seleccione el puerto en el que conecto el USB, luego use el boton de ***upload*** en el IDE de Arduino, cuando compile el programa aparecerá el siguiente mensaje en la consola de salida

    esptool.py v4.5.1
    Serial port COM3
    Connecting....

Entonces deberá con una punta pequeña mantener oprimido el botón de reset el cual esta ubicado en la parte inferior del reloj y será el botón mas cercano a la matriz LED, una vez observe que en la consola se muestra un estado de carga puede soltar el botón de reset

**NOTA:** Si le toma mucho tiempo oprimir el botón de reset para subir el código, se le mostrara un error y deberá volver a iniciar el proceso

### Por Wifi:
Asegúrese de que su PC y el microcontrolador están conectados a la misma red Wifi, de lo contrario no podrá llevar a cabo el proceso, luego en el menú superior vaya a Tools > Port > y en la sección ***Network ports*** seleccione el dispositivo según el nombre y la IP que tenga asignada, luego use el botón de ***upload*** en el IDE de Arduino, es probable que la primera vez le solicite una contraseña la cual será la misma de la red WiFi a la que esta conectado.