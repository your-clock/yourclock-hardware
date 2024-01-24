#include <SimpleTimer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <time.h>
#include <sys/time.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <ThingerESP32.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <TelnetStream.h>
#include "Debug.h"
#include "data.h"

typedef enum {

  buzzer_on_alarm,
  buzzer_on_hour,
  buzzer_off,
  buzzer_stb,
  buzzer_stb_alarm

} ST_buzzer;

typedef enum {

  print_wheater = 0,
  print_hour,
  print_alarm,
  print_error,
  print_stb,
  print_error_wifi

} ST_print;

typedef enum {

  request,
  checking,
  stb

} ST_request;

typedef enum {

  p_push,
  p_check,
  p_time

} ST_boton;

typedef enum {

  menu_stb,
  menu_hora,
  menu_alarma

} ST_menu;

typedef enum {

  conect,
  check_reconect,
  reconect,
  check

} ST_wifi;

enum {

  T_PrintHour = 0,
  T_PrintWeather,
  T_B1,
  T_B2,
  T_RequestWeather,
  T_RequestHour,
  T_RequestTemperature,
  T_Reconect,
  T_buzzer,
  T_buzzer_alarm,
  TIMERS,

};

ST_request hour = request, weather = stb, ambiente = stb;
ST_print clime = print_stb;
ST_boton P1 = p_check, P2 = p_check;
ST_wifi wifi = check;
ST_buzzer buzzer = buzzer_stb;
ST_menu menu = menu_stb;
Debug debug;

char ssid[50];
char password[50];
const char *ssidConf = "YOUR-CLOCK";
const char *passConf = "yourclock0114";
#define USERNAME "DavidG97"
#define DEVICE_ID "ESP32CLOCK"
#define DEVICE_CREDENTIAL "3SPE2CL0CK"
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 0;
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 2
#define CLK_PIN 18
#define DATA_PIN 23
#define B_1 21
#define B_2 19
#define BUZZER_PIN 15
#define ADC 35
#define SENSOR 22
#define FULL_CHARGE 4
#define CHARGING 5

double tem_amb;
int tem_amb_int, temperature = 0;
char tem[200];
char alar[200];
char hora[20];
char buffer[512];
uint16_t speedDisplay = 50, index_timer;
struct tm timeinfo;
int h = 0, m = 0, s = 0, mo = 0;
uint8_t espera_wifi = 0;
int h_alarm = 100, m_alarm = 100, ind_alarma = 0, flag_alarm = 0, switch_alarm = 0;
bool enabled_sound = true;
uint16_t triger_alarm = 0, triger_hour = 0;
volatile uint32_t timers[TIMERS];
const char *w;
const char *al;
const char *hora_aux;
uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 };         //caracter °c
uint8_t alarm_yes[] = { 5, 0, 60, 20, 60, 0 };        // caracter alarma on
uint8_t alarm_no[] = { 5, 255, 195, 235, 195, 255 };  // caracter alarma off

TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;

WebServer server(80);
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
SimpleTimer timer, timer2;
MD_Parola matrixLed = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
OneWire OneWireObject(SENSOR);
DallasTemperature sensorDS18B20(&OneWireObject);
String serie2;
HTTPClient http3;
StaticJsonDocument<32> doc3;

void loop1(void *parameter);
void loop2(void *parameter);
void loop3(void *parameter);
void conectarWifi();
void digitalClockDisplay();
void Tiempo_timer();
void process_buzzer();
void process_requestHour();
void process_requestWeather();
void process_print();
void process_boton1();
void process_boton2();
void process_sendDataTemperature();
void process_reconnect();
int controlAlarma(String hora_alarma);
void modoconf();
String leer(int addr);
void grabar(int addr, String a);
void mainPage();
void saveWifi();
void scanWifi();
void restart();

void setup() {

  Serial.begin(115200);
  pinMode(B_1, INPUT);
  pinMode(B_2, INPUT);
  //pinMode(FULL_CHARGE, INPUT);
  //pinMode(CHARGING, INPUT);
  pinMode(2, OUTPUT);

  //disableCore0WDT();
  disableCore1WDT();

  matrixLed.begin();
  matrixLed.setIntensity(0);
  matrixLed.addChar('#', alarm_yes);
  matrixLed.addChar('!', alarm_no);
  matrixLed.addChar('$', degC);
  matrixLed.write("Hola :D");

  EEPROM.begin(512);

  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(password, 50);

  sensorDS18B20.begin();

  timer.setInterval(1);
  timer2.setInterval(1000);

  ledcSetup(0, 3500, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  conectarWifi();

  thing.add_wifi(ssid, password);
  thing["temperatura"] >> outputValue(sensorDS18B20.getTempCByIndex(0));
  thing["alarma"] << [](pson &in) {
    String alarma = in;
    controlAlarma(alarma);
  };

  xTaskCreatePinnedToCore(loop1, "Task_1", 5120, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(loop2, "Task_2", 6144, NULL, 1, &Task2, 1);
  xTaskCreatePinnedToCore(loop3, "Task_3", 5120, NULL, 1, &Task3, 0);

  ArduinoOTA.begin();
  TelnetStream.begin();
}

void loop() {
  Tiempo_timer();

  digitalClockDisplay();
  process_boton1();
  process_boton2();
  process_print();
  process_buzzer();
}

void loop1(void *parameter) {
  for (;;) {
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;

    thing.handle();
    process_requestHour();
    process_requestWeather();
  }
  vTaskDelay(10);
}

void loop2(void *parameter) {
  for (;;) {
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;

    ArduinoOTA.handle();
    process_reconnect();
  }
  vTaskDelay(10);
}

void loop3(void *parameter) {
  for (;;) {
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;

    process_sendDataTemperature();
  }
  vTaskDelay(10);
}

void Tiempo_timer() {
  if (timer.isReady()) {
    for (index_timer = 0; index_timer <= TIMERS; index_timer++) {
      if (timers[index_timer] != 0) {
        timers[index_timer]--;
      }
    }
    timer.reset();
  }
}

void conectarWifi() {

  WiFi.begin(ssid, password);
  debug.print("Conecting to: ");
  debug.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {

    debug.print(".");
    delay(1000);
    espera_wifi++;
    if (espera_wifi == 60) {
      break;
    }
  }
  debug.println(F("WiFi connected"));
  debug.print(F("IP address: "));
  debug.println(WiFi.localIP());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void digitalClockDisplay() {
  if (menu == menu_stb) {
    if (timer2.isReady()) {
      s = s + 1;
      if (s == 60) {
        m = m + 1;
        s = 0;
      }
      if (m == 60) {
        h = h + 1;
        m = 0;
      }
      if (h == 24) {
        h = 0;
      }
      timer2.reset();
    }
  }
}

void process_requestHour() {
  switch (hour) {
    case stb:
      //do nothing
      break;
    case checking:
      timers[T_RequestHour] = 300000;
      hour = request;
      break;
    case request:
      if (timers[T_RequestHour] == 0) {
        debug.println(getLocalTime(&timeinfo));
        if (!getLocalTime(&timeinfo)) {
          debug.println("Error al sincronizar la hora");
          return;
        }
        debug.print("Hora sincronizada exitosamente: ");
        debug.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        h = timeinfo.tm_hour;
        m = timeinfo.tm_min;
        s = timeinfo.tm_sec;
        mo = timeinfo.tm_mon;
        hour = stb;
      }
      break;
  }
}

void process_requestWeather() {
  switch (weather) {
    case stb:
      timers[T_RequestWeather] = 60000;
      weather = request;
      break;
    case request:
      if (timers[T_RequestWeather] == 0) {
        if (WiFi.status() == WL_CONNECTED) {
          HTTPClient http;
          http.begin("http://api.openweathermap.org/data/2.5/weather?q=Bogota&units=metric&lang=es&appid=d212d43340e59e9c201119a4fdae37c6");
          int httpCode = http.GET();
          if (httpCode > 0) {
            String payloads = http.getString();
            StaticJsonDocument<1024> doc;
            deserializeJson(doc, payloads);
            const char *clima = doc["weather"][0]["main"];
            temperature = doc["main"]["temp"];
            sensorDS18B20.requestTemperatures();
            tem_amb_int = sensorDS18B20.getTempCByIndex(0);
            sprintf(tem, " T:%d$ C:%s TA:%d$ ", temperature, clima, tem_amb_int);
            w = tem;
          }
          http.end();
        } else if (WiFi.status() == WL_DISCONNECTED) {
          clime = print_error_wifi;
        }
        weather = stb;
      }
      break;
  }
}

void process_print() {
  switch (clime) {
    case print_hour:
      if (matrixLed.displayAnimate()) {

        if ((m == 0 && s == 0) && (h != 21 && h != 22 && h != 23 && h != 0 && h != 1 && h != 2 && h != 3 && h != 4 && h != 5 && h != 6) && flag_alarm == 0) {
          buzzer = buzzer_on_hour;
        } else if (flag_alarm == 0) {
          buzzer = buzzer_off;
        }

        if (h == h_alarm && m == m_alarm && s == 0 && flag_alarm == 0) {
          buzzer = buzzer_on_alarm;
          flag_alarm = 1;
        }

        if (h < 10 && m < 10) {
          if (ind_alarma == 1) {
            sprintf(hora, "0%d:0%d!", h, m);
          } else if (ind_alarma == 0) {
            sprintf(hora, "0%d:0%d#", h, m);
          }
        } else if (h > 9 && m > 9) {
          if (ind_alarma == 1) {
            sprintf(hora, "%d:%d!", h, m);
          } else if (ind_alarma == 0) {
            sprintf(hora, "%d:%d#", h, m);
          }
        } else if (h < 10 && m > 9) {
          if (ind_alarma == 1) {
            sprintf(hora, "0%d:%d!", h, m);
          } else if (ind_alarma == 0) {
            sprintf(hora, "0%d:%d#", h, m);
          }
        } else if (h > 9 && m < 10) {
          if (ind_alarma == 1) {
            sprintf(hora, "%d:0%d!", h, m);
          } else if (ind_alarma == 0) {
            sprintf(hora, "%d:0%d#", h, m);
          }
        }

        hora_aux = hora;

        matrixLed.setTextAlignment(PA_CENTER);
        matrixLed.write(hora_aux);
      }
      break;
    case print_wheater:
      matrixLed.displayText(w, PA_CENTER, speedDisplay, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      matrixLed.displayAnimate();
      clime = print_stb;
      break;
    case print_alarm:
      if (h_alarm == 100 && m_alarm == 100) {
        sprintf(alar, " Alarma desactivada ");
        al = alar;
      } else {
        sprintf(alar, " Alarma activa: %d:%d", h_alarm, m_alarm);
        al = alar;
      }
      matrixLed.displayText(al, PA_CENTER, speedDisplay, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      matrixLed.displayAnimate();
      clime = print_stb;
      break;
    case print_error_wifi:
      matrixLed.displayText("No hay conexion wifi", PA_CENTER, speedDisplay, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      matrixLed.displayAnimate();
      clime = print_stb;
      break;
    case print_stb:
      clime = print_hour;
      break;
  }
}

void process_boton1() {
  int val = digitalRead(B_1);
  switch (P1) {
    case p_check:
      if (val == 1) {
        timers[T_B1] = 2500;
        P1 = p_push;
        debug.println("1 pulsado");
      }
      break;
    case p_time:
      if (timers[T_B1] == 0) {
        P1 = p_check;
      }
      break;
    case p_push:
      if (timers[T_B1] >= 2400) {
        //antirrebote
      } else if (timers[T_B1] < 2400 && timers[T_B1] > 0) {
        if (val == 1) {
          //do nothing
        } else if (val == 0) {
          if (flag_alarm == 1) {
            buzzer = buzzer_off;
            flag_alarm = 0;
            timers[T_B1] = 200;
            P1 = p_time;
          } else {
            clime = print_wheater;
            timers[T_B1] = 9000;
            P1 = p_time;
            timers[T_B2] = 9000;
            P2 = p_time;
          }
        }
      } else if (timers[T_B1] == 0) {
        hour = stb;
        timers[T_B1] == 1500;
        P1 = p_time;
        ledcWriteTone(0, 4000);
        delay(750);
        ledcWriteTone(0, 0);
        debug.println("Ingresando al modo de configuracion de red wifi");
        modoconf();
      }
      break;
  }
}

void process_boton2() {
  int val2 = digitalRead(B_2);
  switch (P2) {
    case p_check:
      if (val2 == 1) {
        timers[T_B2] = 2500;
        P2 = p_push;
        debug.println("2 pulsado");
      }
      break;
    case p_time:
      if (timers[T_B2] == 0) {
        P2 = p_check;
      }
      break;
    case p_push:
      if (timers[T_B2] >= 2400) {
        //antirrebote
      } else if (timers[T_B2] < 2400 && timers[T_B2] > 0) {
        if (val2 == 1) {
          //do nothing
        } else if (val2 == 0) {
          if (flag_alarm == 1) {
            buzzer = buzzer_off;
            flag_alarm = 0;
            timers[T_B2] = 200;
            P2 = p_time;
          } else {
            clime = print_alarm;
            timers[T_B2] = 7000;
            P2 = p_time;
            timers[T_B1] = 7000;
            P1 = p_time;
          }
        }
      } else if (timers[T_B2] == 0) {
        hour = stb;
        timers[T_B1] == 1500;
        P1 = p_time;
        ledcWriteTone(0, 4000);
        delay(750);
        ledcWriteTone(0, 0);
        debug.println("Ingresando al modo de configuracion de red wifi");
        modoconf();
      }
      break;
  }
}

void process_reconnect() {
  switch (wifi) {
    case conect:
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      wifi = check;
      break;
    case check_reconect:
      if (timers[T_Reconect] == 0) {
        if (WiFi.status() == WL_CONNECTED) {
          wifi = conect;
          debug.println("Wifi conectado");
        } else {
          wifi = reconect;
        }
      }
      break;
    case check:
      if (WiFi.status() == WL_DISCONNECTED) {
        wifi = reconect;
        debug.println("WiFi desconectado");
      }
      break;
    case reconect:
      WiFi.begin(ssid, password);
      debug.println("Reconectando...");
      timers[T_Reconect] = 10000;
      wifi = check_reconect;
      break;
  }
}

void process_sendDataTemperature() {
  switch (ambiente) {
    case request:
      if (timers[T_RequestTemperature] == 0) {
        if (WiFi.status() == WL_CONNECTED) {
          sensorDS18B20.requestTemperatures();
          tem_amb = sensorDS18B20.getTempCByIndex(0);

          http3.begin("https://your-clock.up.railway.app/api/reloj/datos");
          http3.addHeader("Content-Type", "application/json");
          http3.addHeader("device_id", "114");
          serie2 = "";
          doc3["temp_amb"] = tem_amb;
          doc3["temp_local"] = temperature;

          serializeJson(doc3, serie2);

          debug.println("Datos a enviar: " + serie2);
          int code_res = http3.POST(serie2);
          String payload = http3.getString();
          if (code_res > 0) {
            debug.print("Respuesta del servidor: ");
          } else {
            debug.print("Error al enviar la data: ");
          }
          debug.println(code_res+" - "+payload);
          http3.end();
        }
        ambiente = stb;
      }
      break;
    case stb:
      timers[T_RequestTemperature] = 20000;
      ambiente = request;
      break;
  }
}

void process_buzzer() {
  switch (buzzer) {
    case buzzer_on_alarm:
      timers[T_buzzer] = 60000 * 2;
      buzzer = buzzer_stb_alarm;

      break;
    case buzzer_on_hour:

      ledcWriteTone(0, 4000);
      buzzer = buzzer_stb;

      break;
    case buzzer_off:

      ledcWriteTone(0, 0);
      buzzer = buzzer_stb;

      break;
    case buzzer_stb:

      //do nothing

      break;
    case buzzer_stb_alarm:
      if (timers[T_buzzer] > 0) {
        if (timers[T_buzzer] % 1000 == 0) {
          if (switch_alarm == 0) {
            if (enabled_sound) {
              ledcWriteTone(0, 4000);
              enabled_sound = 1;
            } else {
              ledcWriteTone(0, 0);
              enabled_sound = 0;
            }
            enabled_sound = !enabled_sound;
            switch_alarm = 1;
          }
        } else {
          switch_alarm = 0;
        }
      } else {
        buzzer = buzzer_off;
      }

      break;
  }
}

int controlAlarma(String hora_alarma) {

  int aux_alarma = hora_alarma.toInt();
  if (aux_alarma == 0) {
    h_alarm = 0;
    m_alarm = 0;
    ind_alarma = 1;
  } else if (aux_alarma >= 1 && aux_alarma <= 59) {
    h_alarm = 0;
    m_alarm = aux_alarma;
    ind_alarma = 1;
  } else if (aux_alarma >= 100 && aux_alarma <= 2359) {
    h_alarm = (aux_alarma / 100);
    m_alarm = aux_alarma - (h_alarm * 100);
    ind_alarma = 1;
  } else {
    h_alarm = 100;
    m_alarm = 100;
    ind_alarma = 0;
  }
  return 1;
}

void modoconf() {

  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP();
  debug.print("IP del access point: ");
  debug.println(myIP);
  debug.println("WebServer iniciado");
  if (!MDNS.begin("yourclock")) {
    debug.println("Error iniciando mDNS");
  }
  debug.println("mDNS iniciado");
  //matrixLed.displayText("Ingresa a http://" + myIP, PA_CENTER, 30, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  //matrixLed.displayAnimate();
  server.on("/", mainPage);
  server.on("/save", saveWifi);
  server.on("/scan", scanWifi);
  server.on("/restart", restart);
  server.onNotFound(notFound);
  server.begin();
  debug.println("Server iniciado");
  Page_set_Wifi.replace("%ip", myIP.toString());
  while (true) {
    //matrixLed.displayAnimate();
    server.handleClient();
  }
}

String leer(int addr) {
  byte lectura;
  String strlectura;
  for (int i = addr; i < addr + 50; i++) {
    lectura = EEPROM.read(i);
    if (lectura != 255) {
      strlectura += (char)lectura;
    }
  }
  return strlectura;
}

void grabar(int addr, String a) {
  int tamano = a.length();
  char inchar[50];
  a.toCharArray(inchar, tamano + 1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr + i, inchar[i]);
  }
  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr + i, 255);
  }
  EEPROM.commit();
}

void scanWifi() {
  int n = WiFi.scanNetworks();
  debug.println("Escaneo terminado");
  if (n == 0) {
    debug.println("No se encontraron redes");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(204, "text/plain", "No se encontraron redes");
  } else {
    debug.print(n);
    debug.println(" redes encontradas");
    StaticJsonDocument<512> wifiNetworksDoc;
    JsonObject wifiNetworksJson = wifiNetworksDoc.to<JsonObject>();
    JsonArray wifiNetworksArray = wifiNetworksJson.createNestedArray("networks");
    for (int i = 0; i < n; ++i) {
      wifiNetworksArray.add(WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i));
      delay(10);
    }
    serializeJson(wifiNetworksJson, buffer);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", buffer);
  }
}

void mainPage() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", Page_set_Wifi);
}

void saveWifi() {
  debug.println(server.arg("ssid"));
  grabar(0, server.arg("ssid"));
  debug.println(server.arg("pass"));
  grabar(50, server.arg("pass"));

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Configuracion Guardada");

}

void restart() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Reiniciando el reloj");
  delay(500);
  ESP.restart();
}

void notFound() {
  String mensaje = "<h1>404</h1>";
  mensaje += "Pagina No encontrada</br>";
  mensaje += "Intenta otra pagina</br>";
  server.send(404, "text/html", mensaje);
}
