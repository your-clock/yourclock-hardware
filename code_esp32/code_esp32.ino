#include <SimpleTimer.h>
#include <WiFi.h>
#include <WiFiClient.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
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

typedef enum{

  buzzer_on_alarm=0,
  buzzer_on_hour,
  buzzer_off,
  buzzer_stb,
  buzzer_check
  
}ST_buzzer;

typedef enum{

  printing=0,
  print_error,
  print_stb, 
  print_error_wifi

}ST_print;

typedef enum{

  request,
  checking,
  stb

}ST_request;

typedef enum{

  p_push,
  p_check,
  p_time

}ST_boton;

typedef enum{

  menu_stb,
  menu_hora,
  menu_alarma
  
}ST_menu;

typedef enum{

  conect,
  check_reconect,
  reconect,
  check

}ST_wifi;

enum{

  T_PrintHour=0,
  T_PrintWeather,
  T_B1,
  T_B2,
  T_RequestWeather,
  T_RequestHour,
  T_RequestUbidotsTemperatura,
  T_Reconect,
  T_ControlCharge,
  T_buzzer,
  TIMERS,

};

ST_request hour=request, weather=stb, ambiente=stb;
ST_print tiempo=printing, clime=print_stb;
ST_boton P1=p_check, P2=p_check;
ST_wifi wifi=check;
ST_buzzer buzzer=buzzer_check;
ST_menu menu = menu_stb;

char ssid[50];
char password[50];
const char *ssidConf = "YOUR-CLOCK";
const char *passConf = "yourclock0114";
#define USERNAME "DavidG97"
#define DEVICE_ID "ESP32CLOCK"
#define DEVICE_CREDENTIAL "3SPE2CL0CK"
const char *ntpServer = "horalegal.inm.gov.co";
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 0;
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 2
#define CLK_PIN 18
#define DATA_PIN 23
#define B_1 21
#define B_2 19
#define buzzer_pin 15
#define ADC 35
#define SENSOR 22
#define FULL_CHARGE 4
#define CHARGING 5

double tem_amb;
bool trigger_hora = true;
int tem_amb_int,temperature = 0;
char tem[200];
char alar[200];
char hora[20];
int channel=0;
uint16_t v=50;
struct tm timeinfo;
int h=0,m=0,s=0,mo=0;
uint8_t espera_wifi = 0; 
int h_alarm=100,m_alarm=100, ind_alarma = 0, stb_alarma = 0;
uint16_t triger_alarm = 0,triger_hour = 0;
volatile uint32_t timers[TIMERS];
volatile String data_clima;
const char* w;
const char* al;
const char* ho;
uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 };   //caracter °c
uint8_t alarm_yes[] = { 5, 0, 60, 20, 60, 0};  // caracter alarma on
uint8_t alarm_no[] = { 5, 255, 195, 235, 195, 255};  // caracter alarma off

String mensaje = "";

//-----------CODIGO HTML PAGINA DE CONFIGURACION---------------
String pagina = "<!DOCTYPE html>"
"<html>"
"<head>"
"<title>Tutorial Eeprom</title>"
"<meta charset='UTF-8'>"
"</head>"
"<body>"
"</form>"
"<form action='guardar_conf' method='get'>"
"SSID:<br><br>"
"<input class='input1' name='ssid' type='text'><br>"
"PASSWORD:<br><br>"
"<input class='input1' name='pass' type='password'><br><br>"
"<input class='boton' type='submit' value='GUARDAR'/><br><br>"
"</form>"
"<a href='escanear'><button class='boton'>ESCANEAR</button></a><br><br>";

String paginafin = "</body>"
"</html>";

TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;

WebServer server(80);
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);
SimpleTimer timer, timer2;
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
OneWire OneWireObject(SENSOR);
DallasTemperature sensorDS18B20(&OneWireObject);
String serie2;
HTTPClient http3;
StaticJsonDocument<1000> doc3;
String serie;
HTTPClient http2;
StaticJsonDocument<1000> doc2;
String serie3;
HTTPClient http4;
StaticJsonDocument<1000> doc4;

void loop1(void *parameter);
void loop2(void *parameter);
void loop3(void *parameter);
void conectarWifi();
void digitalClockDisplay();
void Tiempo_timer();
void process_buzzer();
void process_requestHour();
void process_requestWeather();
void process_printHour();
void process_printWeather();
void process_boton1();
void process_boton2();
void process_SendUbidotsTemperatura();
void process_reconnect();
int controlAlarma(String hora_alarma);
void modoconf();
String leer(int addr);
void grabar(int addr, String a);
void paginaconf();
void escanear();



void setup() {

  Serial.begin(115200); 
  pinMode(B_1, INPUT);
  pinMode(B_2, INPUT);
  pinMode(FULL_CHARGE, INPUT);
  pinMode(CHARGING, INPUT);
  pinMode(2, OUTPUT);

  P.begin();
  P.setIntensity(0);
  P.addChar('#', alarm_yes);
  P.addChar('!', alarm_no);
  
  EEPROM.begin(512);
  if (digitalRead(B_1) == 1) {
    modoconf();
  }
  
  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(password, 50);

  sensorDS18B20.begin();

  timer.setInterval(1);
  timer2.setInterval(1000);

  ledcSetup(0,3500,8);
  ledcAttachPin(buzzer_pin,0);

  conectarWifi();

  thing.add_wifi(ssid, password);
  thing["temperatura"] >> outputValue(sensorDS18B20.getTempCByIndex(0));
  thing["alarma"] << [](pson& in){
    String alarma = in;
    Serial.println(alarma);
    controlAlarma(alarma);
  };

  xTaskCreatePinnedToCore(loop1,"Task_1",5120,NULL,1,&Task1,0);
  xTaskCreatePinnedToCore(loop2,"Task_2",6144,NULL,1,&Task2,1);
  xTaskCreatePinnedToCore(loop3,"Task_3",5120,NULL,1,&Task3,0);

}

void loop() {
  Tiempo_timer();                   //timers

  digitalClockDisplay();            //conteo reloj interno
  process_printHour();
  process_boton1();
  process_boton2();
  process_printWeather();
}

void loop1(void *parameter){
  for(;;){
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;

    thing.handle();
    process_requestHour();
    process_requestWeather();
    process_SendUbidotsTemperatura();
  }
  vTaskDelay(10);
}

void loop2(void *parameter){
  for(;;){
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;

    process_reconnect();
  }
  vTaskDelay(10);
}

void loop3(void *parameter){
  for(;;){
    TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdt_feed = 1;
    TIMERG0.wdt_wprotect = 0;

    process_buzzer();
  }
  vTaskDelay(10);
}

void Tiempo_timer(){
  uint16_t i;
  if(timer.isReady()){
    for(i=0;i<=TIMERS;i++){
      if(timers[i]!=0){
        timers[i]--;
      }
    }
    timer.reset();
  }
}

void conectarWifi(){

  WiFi.begin(ssid, password);
  Serial.print("Conecting to: ");
  Serial.println(ssid);

  while(WiFi.status() != WL_CONNECTED){
    
    Serial.print(".");
    delay(1000);
    espera_wifi++;
    Serial.println(espera_wifi);
    if(espera_wifi == 60){
      break;
    }
    
  }
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void digitalClockDisplay() {
  if(menu == menu_stb){
    if(timer2.isReady()){
      s = s+1;
      if(s==60){
        m=m+1;
        s=0;
      }
      if(m==60){
        h=h+1;
        m=0;
      }
      if(h==24){
        h=0;
      }
      timer2.reset();
    }
  }
}

void process_requestHour(){
  switch(hour){
    case stb:
      //do nothing
    break;
    case checking:
      timers[T_RequestHour]=300000;
      hour=request;
    break;
    case request:
      if(timers[T_RequestHour]==0){
        if(!getLocalTime(&timeinfo)){
          //error al sincronizar la hora
          return;
        }
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        h=timeinfo.tm_hour;
        m=timeinfo.tm_min;
        s=timeinfo.tm_sec;
        mo=timeinfo.tm_mon;
        hour=stb;
      }
    break;
  }
}

void process_requestWeather(){
  switch(weather){
    case stb:
      timers[T_RequestWeather]=60000;
      weather=request;
    break;
    case request:
      if(timers[T_RequestWeather]==0){
        if(WiFi.status() == WL_CONNECTED){
          HTTPClient http;
          http.begin("http://api.openweathermap.org/data/2.5/weather?q=Bogota&units=metric&lang=es&appid=d212d43340e59e9c201119a4fdae37c6");
          int httpCode = http.GET();
          if(httpCode > 0){
            String payloads = http.getString();
            StaticJsonDocument<1000> doc;
            deserializeJson(doc, payloads);
            const char* clima = doc["weather"][0]["main"];
            temperature = doc["main"]["temp"];
            sensorDS18B20.requestTemperatures();
            tem_amb_int = sensorDS18B20.getTempCByIndex(0);
            P.addChar('$', degC);
            sprintf(tem," T:%d$ C:%s TA:%d$ ",temperature,clima,tem_amb_int);
            w = tem;
          }
          http.end();
        }else if(WiFi.status() == WL_DISCONNECTED){
          tiempo=print_stb;
          clime=print_error_wifi;
        }
      weather=stb;
      }
    break;
  }
}

void process_printHour(){
  switch(tiempo){
    case printing:
      if(P.displayAnimate()){

        if((m==0 && s==0) && (h!=22 && h!=23 && h!=0 && h!=1 && h!=2 && h!=3 && h!=4 && h!=5) && triger_alarm==0){
          triger_hour = 1;
        }else{
          triger_hour = 0;
        }

        if(h == h_alarm && m == m_alarm && s==0 && stb_alarma == 0){
          stb_alarma = 1;
        }

        if(h == h_alarm && m == m_alarm && s>=0 && triger_alarm==0 && stb_alarma == 1){
          triger_alarm = 1;
        }else if(m != m_alarm){
          stb_alarma = 0;
          triger_alarm = 0;
        }
        
        if(h<10 && m<10){
          if(ind_alarma == 1){
            sprintf(hora, "0%d:0%d!", h, m);
          }else if(ind_alarma == 0){
            sprintf(hora, "0%d:0%d#", h, m);
          }
        }else if(h>9 && m>9){
          if(ind_alarma == 1){
            sprintf(hora, "%d:%d!", h, m);
          }else if(ind_alarma == 0){
            sprintf(hora, "%d:%d#", h, m);
          }
        }else if(h<10 && m>9){
          if(ind_alarma == 1){
            sprintf(hora, "0%d:%d!", h, m);
          }else if(ind_alarma == 0){
            sprintf(hora, "0%d:%d#", h, m);
          }
        }else if(h>9 && m<10){
          if(ind_alarma == 1){
            sprintf(hora, "%d:0%d!", h, m);
          }else if(ind_alarma == 0){
            sprintf(hora, "%d:0%d#", h, m);
          }
        }
        
        ho=hora;

        P.setTextAlignment(PA_CENTER);
        P.write(ho);
        
      }
    break;
    case print_stb:
      tiempo=printing;
    break;
    case print_error:
      //no necesario
    break;
  }
}

void process_printWeather(){
  switch(clime){
    case printing:
      P.displayText(w, PA_CENTER, v, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      P.displayAnimate();
      clime=print_stb;
    break;
    case print_error:
      if(h_alarm == 100 && m_alarm == 100){
        sprintf(alar," Alarma desactivada ");
        al = alar;
      }else{
        sprintf(alar," Alarma activa: %d:%d",h_alarm,m_alarm);
        al = alar;
      }
      P.displayText(al, PA_CENTER, v, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      P.displayAnimate();
      clime=print_stb;
    break;
    case print_error_wifi:
      P.displayText("No hay conexion wifi", PA_CENTER, v, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      P.displayAnimate();
      clime=print_stb;
    case print_stb:
      //do nothing
    break;
  }
}

void process_boton1(){
  int val = digitalRead(B_1);
  if(menu == menu_stb){
     switch(P1){
      case p_check:
        if(val == 1){
          timers[T_B1] = 2500;
          P1 = p_push;
          Serial.println("1 pulsado");
        }
      break;
      case p_time:
        if(timers[T_B1] == 0){
          P1 = p_check;
        }
      break;
      case p_push:
        if(timers[T_B1] >= 2400){
          //antirrebote
          Serial.print("*");
        }else if(timers[T_B1] < 2400 && timers[T_B1] > 0){
          Serial.print("_");
          if(val == 1){
            //do nothing
          }else if(val == 0){
            Serial.println("soltado");
            if(stb_alarma == 1){
              ledcWriteTone(0,0);
              triger_alarm = 0;
              stb_alarma = 0;
              timers[T_B1]=200;
              P1=p_time;
            }else{
              tiempo=print_stb;
              clime=printing;
              timers[T_B1]=9000;
              P1=p_time;
              timers[T_B2]=9000;
              P2=p_time;
            }
          }
        }else if(timers[T_B1] == 0 ){
          hour = stb;
          menu = menu_hora;
          timers[T_B1] == 1500;
          P1 = p_time;
          ledcWriteTone(0,4000);
          Serial.println("modo hora");
        }
      break;
    }
  }else{
    ESP.restart();
  }
}

void process_boton2(){
  int val2 = digitalRead(B_2);
  switch(P2){
    case p_check:
      if(val2 == 1){
        timers[T_B2] = 2500;
        P2 = p_push;
        Serial.println("2 pulsado");
      }
    break;
    case p_time:
      if(timers[T_B2] == 0){
        P2 = p_check;
      }
    break;
    case p_push:
      if(timers[T_B2] >= 2400){
        //antirrebote
      }else if(timers[T_B2] < 2400 && timers[T_B2] > 0){
        if(val2 == 1){
          //do nothing
        }else if(val2 == 0){
          if(stb_alarma == 1){
            ledcWriteTone(0,0);
            triger_alarm = 0;
            stb_alarma = 0;
            timers[T_B2]=200;
            P2=p_time;
          }else{
            tiempo=print_stb;
            clime=print_error;
            timers[T_B2]=7000;
            P2=p_time;
            timers[T_B1]=7000;
            P1=p_time;
          }
        }
      }else if(timers[T_B2] == 0 ){
        menu = menu_alarma;
        
        Serial.println("modo alarma");
      }
    break;
  }
}

void process_reconnect() {
  switch(wifi){
    case conect:
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      wifi=check;
    break;
    case check_reconect:
      if(timers[T_Reconect] == 0){
        if(WiFi.status() == WL_CONNECTED){
          wifi=conect;
          Serial.println("Wifi conectado");
        }else{
          wifi=reconect;
        }
      }
    break;
    case check:
      if(WiFi.status() == WL_DISCONNECTED){
        wifi=reconect;
        Serial.println("WiFi desconectado");
      }
    break;
    case reconect:
      WiFi.begin(ssid, password);
      Serial.println("Reconectado...");
      timers[T_Reconect]=10000;
      wifi=check_reconect;
    break;
  }
}

void process_SendUbidotsTemperatura(){
  switch(ambiente){
    case request:
      if(timers[T_RequestUbidotsTemperatura]==0){
        if(WiFi.status() == WL_CONNECTED){
          sensorDS18B20.requestTemperatures();
          tem_amb = sensorDS18B20.getTempCByIndex(0);

          http3.begin("https://your-clock.herokuapp.com/api/datos");
          http3.addHeader("Content-Type", "application/json");
          serie2="";
          doc3["temp_amb"]=tem_amb;
          doc3["temp_local"]=temperature;

          serializeJson(doc3, serie2);
          Serial.println(serie2);
          int code = http3.POST(serie2);
          if(code > 0){
            Serial.println("OK");   
          }else{
            Serial.println("Error en HTTP request");
          }
          http3.end();    
        }
        ambiente=stb;
      }
    break;
    case stb:
      timers[T_RequestUbidotsTemperatura]=20000;
      ambiente=request;
    break;
  }
}

void process_buzzer(){
  switch(buzzer){
    case buzzer_on_alarm:
    
      if(timers[T_buzzer]>=1000 && timers[T_buzzer]<=2000){
        ledcWriteTone(0,4000);
      }else if(timers[T_buzzer]>0 && timers[T_buzzer]<1000){
        ledcWriteTone(0,0);
      }else if(timers[T_buzzer]==0){
        buzzer = buzzer_check;
      }
      
    break;
    case buzzer_on_hour:

      ledcWriteTone(0,4000);
      buzzer = buzzer_check;
      
    break;
    case buzzer_off:

      ledcWriteTone(0,0);
      buzzer = buzzer_check;
      
    break;
    case buzzer_stb:
      
    break;
    case buzzer_check:

      if(triger_alarm == 0 && triger_hour == 0){
        buzzer = buzzer_off;
      }else if(triger_alarm == 1 && triger_hour == 0){
        buzzer = buzzer_on_alarm;
        timers[T_buzzer]=2000;
      }else if(triger_alarm == 0 && triger_hour == 1){
        buzzer = buzzer_on_hour;
      }else if(triger_alarm == 1 && triger_hour == 1){
        buzzer = buzzer_on_alarm;
        timers[T_buzzer]=2000;
      }
      
    break;
  }
}

int controlAlarma(String hora_alarma){

  int aux_alarma = hora_alarma.toInt();
  if(aux_alarma == 0){
    h_alarm = 0;
    m_alarm = 0;
    ind_alarma = 1;
  }else if(aux_alarma >= 1 && aux_alarma <=59){
    h_alarm = 0;
    m_alarm = aux_alarma;
    ind_alarma = 1;
  }else if(aux_alarma >=100 && aux_alarma <=2359){
    h_alarm = (aux_alarma / 100);
    m_alarm = aux_alarma - (h_alarm * 100);
    ind_alarma = 1;
  }else{
    h_alarm = 100;
    m_alarm = 100;
    ind_alarma = 0;
  }
  return 1;
}

void modoconf() {

  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP(); 
  Serial.print("IP del access point: ");
  Serial.println(myIP);
  Serial.println("WebServer iniciado...");
  
  P.setTextAlignment(PA_CENTER);
  P.write("Hola :)");
  delay(1000);
  P.displayText("http://your-clock.local", PA_CENTER, v, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  P.displayAnimate();
  delay(4000);
  server.on("/", paginaconf); //esta es la pagina de configuracion
  server.on("/guardar_conf", guardar_conf); //Graba en la eeprom la configuracion
  server.on("/escanear", escanear); //Escanean las redes wifi disponibles
  server.begin();
  while (true) {
    server.handleClient();
  }
}

String leer(int addr) {
   byte lectura;
   String strlectura;
   for (int i = addr; i < addr+50; i++) {
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
  a.toCharArray(inchar, tamano+1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr+i, inchar[i]);
  }
  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr+i, 255);
  }
  EEPROM.commit();
}

void escanear() {  
  int n = WiFi.scanNetworks(); //devuelve el número de redes encontradas
  Serial.println("escaneo terminado");
  if (n == 0) { //si no encuentra ninguna red
    Serial.println("no se encontraron redes");
    mensaje = "no se encontraron redes";
  }  
  else
  {
    Serial.print(n);
    Serial.println(" redes encontradas");
    mensaje = "";
    for (int i = 0; i < n; ++i)
    {
      // agrega al STRING "mensaje" la información de las redes encontradas 
      mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + " </p>\r\n";
      //WiFi.encryptionType 5:WEP 2:WPA/PSK 4:WPA2/PSK 7:open network 8:WPA/WPA2/PSK
      delay(10);
    }
    Serial.println(mensaje);
    paginaconf();
  }
}

void paginaconf() {
  server.send(200, "text/html", pagina + mensaje + paginafin); 
}

void guardar_conf() {
  
  Serial.println(server.arg("ssid"));//Recibimos los valores que envia por GET el formulario web
  grabar(0,server.arg("ssid"));
  Serial.println(server.arg("pass"));
  grabar(50,server.arg("pass"));

  mensaje = "Configuracion Guardada...";
  paginaconf();
}
