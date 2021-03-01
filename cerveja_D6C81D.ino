
/*  Control Board - Temperature / Relay
    Objective: publish information in the web
               send and receive information from the mqtt
               read 4 temperature sensors,
               control 4 relays
*/

/*
   rev 0.1 Only Wifi works without any issue.
   rev 0.2 Time counter
   rev 0.3.0 mqtt pub
   rev 0.3.1 mqtt sub
   rev 0.4 relays
   rev 0.5 temperature
   rev 0.6 contador de tempo
   rev 0.7 ESP reset and mqtt buttons
   rev 0.8 atuador com buttons, alterado a funcao atuador

*/

/*
  Equivalencia das saidas Digitais entre NodeMCU e ESP8266 (na IDE do Arduino)
  NodeMCU - ESP8266
  D0 = GPIO16;
  D1 = GPIO5;
  D2 = GPIO4;
  D3 = GPIO0;
  D4 = GPIO2;
  D5 = GPIO14;
  D6 = GPIO12;
  D7 = GPIO13;
  D8 = GPIO15;
  RX = GPIO3;
  TX = GPIO1;
  S3 = GPIO10;
  S2 = GPIO9;
*/

/*
  Update the node mcu by FDTI
  NodeMCU pin - FDTI (pin out)
  Vin         - 3.3V
  En          - 3.3V
  GND         - GND
  D3 (GPIO0)  - GND
  TX          - RX
  RX          - TX
*/

/*
   ESP8266 (NODEMCU)
   Button 0 - GPIO0  - D3
   Button 1 - GPIO5  - D2
   Relay 1  - GPIO12 - D6
   Relay 2  - GPIO5  - D1


   Sonoff Dual R2
   Button 0 - GPIO0  - D3
   Button 1 - GPIO9  - S2
   Relay 1  - GPIO12 - D6
   Relay 2  - GPIO5  - D1

   Sonoff Single
   Button 0 - GPIO14 - D5
   Relay    - GPIO12 - D6

   Placa de Cerveja
   D1 = GPIO5 - sensores de temperatura
   D5 = GPIO14;
   D6 = GPIO12;
   D7 = GPIO13;
   D8 = GPIO15;



   Configuration
   board: Generic ESP8266 Module (sonoff)
   flash mode: DOUT
   size: 1M 64K SPIFFS
   Debug Port: Disable
   IwIP: 1.4 higher bandwidth
   Reset method: nodemcu
   cristal 26Mhz
   flash   40 Mhz
   cpu     80Mhz
   Upload speed 115200
   Erase Flash: only sketch

*/
///////////////////////////////////////////////
///////////////////////////////////////////////
//Libraries:

// WiFi
#include <FS.h>                   // It is the first library to access the esp memory
#include <ESP.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266mDNS.h>               //Biblioteca que permite chamar o seu modulo ESP8266 na sua rede pelo nome ao inves do IP.
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h> //Biblioteca que cria o servico de atualizacão via wifi (ou Over The Air - OTA)
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>              //MQTT
#include <ArduinoJson.h>                 //https://github.com/bblanchon/ArduinoJson

//Temperature control
#include <OneWire.h>              // biblioteca para acessar o sensor de temperatura (DS28B20)
#include <DallasTemperature.h>    // biblioteca para acessar o sensor de temperatura (DS28B20)


/* Class da funcao relay
*/
/*
  class Relay {
  private:
    int pino;
    String control;
    String estado;
    String mqtt_sub;
  public:
    Relay(int pino, String control, String estado, String mqtt_sub);
    void atua();
  };
*/


/* Constantes
*/

// HTML update
String Revisao = "0.9";
String Placa = "ESP_D6C81D";
String ControlName = "NodeMcu";
String MAC_Address = "68:C6:3A:D6:C8:1D";


//WiFi
#ifndef APSSID
#define APSSID "winterfell"
#endif
#ifndef AAPSK
#define APPSK  "BrieneTarth"
#endif
//DNS - nome do esp na rede
const char* wifi_host      = "Cerveja_ESP"; //Nome que seu ESP8266 (ou NodeMCU) tera na rede
//const char* ssid = "CasaFeliz"; //"dd-wrt_Ext";
//const char* passwd = "12345678"; //"casafeliz2018";
const char* ssid = APSSID;
const char* passwd = APPSK;


//Time
unsigned long second;
unsigned long minute;
unsigned long hours;
unsigned long days;
unsigned long lasttime;
String timemeasure;
/*
  unsigned long time1sec = 1000;
  unsigned long time_timeout = millis();
  unsigned long wifireconnect_timeout = millis();

*/

//MQTT
String mqtt_topico;         //Define o topico do mqtt
String mqtt_mensagem;       //Define a mensagem mqtt
boolean mqttButton = true;  // liga/desliga as logicas do mqtt

#define servidor_mqtt             "192.168.1.125"  //URL do servidor MQTT - Raspberry Pi
#define servidor_mqtt_porta       "1883"  //Porta do servidor (a mesma deve ser informada na variável abaixo)
#define servidor_mqtt_usuario     "pi1"  //Usuário
#define servidor_mqtt_senha       "12345"  //Senha

// MQTT - definicao de sub/pub
const char* mqtt_topico_pub1 = "cerveja/pin1";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* mqtt_topico_sub1 = "cerveja/pin1";    //Topico para ler o comando do ESP8266
const char* mqtt_topico_pub2 = "cerveja/pin2";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* mqtt_topico_sub2 = "cerveja/pin2";    //Topico para ler o comando do ESP8266
const char* mqtt_topico_pub3 = "cerveja/pin3";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* mqtt_topico_sub3 = "cerveja/pin3";    //Topico para ler o comando do ESP8266
const char* mqtt_topico_pub4 = "cerveja/pin4";    //Topico para publicar o comando de inverter o pino do outro ESP8266
const char* mqtt_topico_sub4 = "cerveja/pin4";    //Topico para ler o comando do ESP8266
const char* mqtt_topico_pubTemp1 =  "cerveja/temp1"; //topico para publicar a temperatura 1
const char* mqtt_topico_pubTemp2 =  "cerveja/temp2"; //topico para publicar a temperatura 2
const char* mqtt_topico_pubTemp3 =  "cerveja/temp3"; //topico para publicar a temperatura 3
const char* mqtt_topico_pubTemp4 =  "cerveja/temp4"; //topico para publicar a temperatura 4
const char* mqtt_topico_subTempoCont =  "cerveja/tempoContador"; //topico para publicar a temperatura 4

String pin1Estado = "off";
String pin2Estado = "off";
String pin3Estado = "off";
String pin4Estado = "off";

String pin1Cmd = "off";
String pin2Cmd = "off";
String pin3Cmd = "off";
String pin4Cmd = "off";


String pin1CmdEstado = "off";
String pin2CmdEstado = "off";
String pin3CmdEstado = "off";
String pin4CmdEstado = "off";


//Relay Pin
/*
  #define PIN1        14   // D5 (nodeMCU) / GPIO14 (ESP8266);
  #define PIN2        12   // D6 (nodeMCU) / GPIO12 (ESP8266)
  #define PIN3        13   // D7 (nodeMCU) / GPIO13 (ESP8266)
  #define PIN4        15   // D8 (nodeMCU) / GPIO15 (ESP8266)
*/
int PIN1 = 14;   // D5 (nodeMCU) / GPIO14 (ESP8266);
int PIN2 = 12;   // D6 (nodeMCU) / GPIO12 (ESP8266)
int PIN3 = 13;   // D7 (nodeMCU) / GPIO13 (ESP8266)
int PIN4 = 15;   // D8 (nodeMCU) / GPIO15 (ESP8266)


//Temperature Pin
#define ONE_WIRE_BUS 05  // GPIO5 (ESP9266) / D1 (nodeMCU)
float temp1;
float temp2;
float temp3;
float temp4;
int tempDeviceCount;      //Number of devices
unsigned long tempoTempSensor = millis();


///////////////////////////////////////////////
///////////////////////////////////////////////
/* General configuration
*/

///WiFi/WiFi Server
ESP8266HTTPUpdateServer atualizadorOTA; //Este e o objeto que permite atualizacao do programa via wifi (OTA)
ESP8266WebServer servidorWeb(80);       //Servidor Web na porta 80
WiFiClient espClient;                   //serve para o MQTT, create the WiFiClient
PubSubClient client(espClient);         //Serve para o MQTT publish/subscriber

//Temperature Sensor
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature temp_sensors(&oneWire);

///////////////////////////////////////////////
///////////////////////////////////////////////
/* OTA Configuration
*/
//Servico OTA
void InicializaServicoAtualizacao() {
  atualizadorOTA.setup(&servidorWeb);
  servidorWeb.begin();
  Serial.print("O servico de atualizacao remota (OTA) Foi iniciado com sucesso! Abra http://");
  Serial.print(wifi_host);
  Serial.print(".local/update no seu browser para iniciar a atualizacao\n");
}

///////////////////////////////////////////////
///////////////////////////////////////////////
/* MDS
*/
//Inicializa o servico de nome no DNS
void InicializaMDNS() {
  for (int imDNS = 0; imDNS <= 3; imDNS++) {
    if (!MDNS.begin(wifi_host)) {
      // se nao esta conectado tenta conectar, espera 1 segundo entre loops
      Serial.print("Erro ao iniciar o servico mDNS!");
      Serial.print("O servico mDNS foi iniciado com sucesso!");
      Serial.print("\n");
      MDNS.addService("http", "tcp", 80);
      delay(1000);
    }  else {
      //leave loop
      break;
    }
  }
}

///////////////////////////////////////////////
///////////////////////////////////////////////
/* MQTT - Sub - Indica os subscribes para monitorar
   adicionar todos os subscribes
*/
void mqttSubscribe() {
  client.subscribe(mqtt_topico_sub1, 1); //QoS 1
  client.subscribe(mqtt_topico_sub2, 1); //QoS 1
  client.subscribe(mqtt_topico_sub3, 1); //QoS 1
  client.subscribe(mqtt_topico_sub4, 1); //QoS 1
  client.subscribe(mqtt_topico_subTempoCont, 1); //QoS 1
}


///////////////////////////////////////////////
///////////////////////////////////////////////
/* MQTT - Reconectar
    Serve para reconectar o sistema de MQTT, tenta reconectar 3 vez por ciclo
*/
void reconnectMQTT() {
  String textretorno = "reconnect MQTT";
  //client.publish("cerveja/supervisorio", textretorno.c_str());
  //Try to reconect to MQTT without the while loop, try 3 times, before leave
  //for (int iI = 0; iI <= 3; iI++) {
  if (!client.connected()) {
    //Tentativa de conectar. Se o MQTT precisa de autenticação, será chamada a função com autenticação, caso contrário, chama a sem autenticação.
    bool conectado = strlen(servidor_mqtt_usuario) > 0 ?
                     client.connect("ESP8266Client", servidor_mqtt_usuario, servidor_mqtt_senha) :
                     client.connect("ESP8266Client");

    //Conectado com senha
    if (conectado) {
      Serial.println("connected");
      // Once Connected, read the subscribes
      mqttSubscribe();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 1 seconds before retrying
      delay(1000);
    }
  }
  /* else {
    //leave loop
    break;
    }
    } //for loop
  */
}

///////////////////////////////////////////////
///////////////////////////////////////////////
/* MQTT - Sub - monitora as mensagens do servidor
   Função que será chamada ao monitar as mensagens do servidor MQTT
   Retorna todas as mensagens lidas
*/
void retornoMQTTSub(char* topico, byte* mensagem, unsigned int tamanho) {
  //A mensagem vai ser On ou Off
  //Convertendo a mensagem recebida para string
  mensagem[tamanho] = '\0';
  String strMensagem = String((char*)mensagem);
  strMensagem.toLowerCase();
  //float f = s.toFloat();

  //informa o topico e mensagem
  mqtt_topico = String(topico);
  mqtt_mensagem = strMensagem;


  String textretorno = "Retorno: topico: ";
  textretorno += String(topico).c_str();
  textretorno += " in mqtt_topico ";
  textretorno += mqtt_topico.c_str();
  textretorno += " msg: ";
  textretorno += mqtt_mensagem.c_str();
  textretorno += " \n\n";
  client.publish("cerveja/supervisorio", textretorno.c_str());

}

///////////////////////////////////////////////
///////////////////////////////////////////////
/* Funcao de atuacao do atuador
 *  atuaPino(PIN1, mqtt_topico_sub1, mqtt_topico_pub1, &pin1Estado, pin1Cmd, &pin1CmdEstado);
*/
void atuaPino (int numPino, const char* subMqtt, const char* pubMqtt, String *estado, String cmd, String *cmdEstado) {
  /*
     Esta funcao tem o objetivo de atuar o pino
  */

  //Verifica a comunicao via MQTT
  if (strcmp(mqtt_topico.c_str(), subMqtt) == 0) {
    if (mqtt_mensagem != *estado) {
      //inverte o pino
      digitalWrite(numPino, !digitalRead(numPino));

      //publica o estado
      //client.publish(pubName, estadoVelho.c_str());

      *estado = mqtt_mensagem;
      //return estadoVelho ;
    }
  }
  // altera o estado do pino via web command apenas se o cmd alterar de estado (para o caso do mqtt atuar)
  // e o for inverter realmente
  if ((cmd != *estado) and (*cmdEstado != cmd)) {
    //inverte o pino
    digitalWrite(numPino, !digitalRead(numPino));

    //Atualiza o estado
    *estado = cmd;
    *cmdEstado = cmd;
    // precisa tambem publicar no mqtt
    client.publish(pubMqtt, cmd.c_str());
  }


}

///////////////////////////////////////////////
///////////////////////////////////////////////
/*  Calculo Tempo
*/
void updateTempo(const char* subMqtt) {
  //Tempo Calculo
  //Zera o tempo quando estiver em zero ou o contador for ativo
  if ((millis() == 0) or ((strcmp(mqtt_topico.c_str(), subMqtt) == 0) and (mqtt_mensagem == false))) {
    lasttime = millis();
    second = 0;
    minute = 0;
    hours = 0;
    days = 0;
  }
  //Conta os segundos
  if (millis() > lasttime + 1000) {
    //unsigned int diffsec = round((millis() - lasttime)/1000);
    //if (diffsec <= 1.5){
    second = ++second;
    //} else {
    //  second =++ diffsec;
    //}
    lasttime = millis();
  }
  if (second >= 60) {
    //if((second-60)>0){
    //  second = (second-60);
    //}else{
    second = 0;
    //}
    minute = ++minute;
  }
  if (minute == 60) {
    hours = ++hours;
    minute = 0;
  }
  if (hours == 24) {
    days = ++ days;
    hours = 0;
  }
}

///////////////////////////////////////////////
///////////////////////////////////////////////
/* Web Page
*/
void handleRootInicial() {
  // SW to create the html
  String html = "<html><head><title> Programa Cerveja Suzuki</title>";
  html += "<style> body ( background-color:#cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; ) </style> ";
  html += "</head><body> ";
  html += "<h1>suzuki_teste cerveja basic ESP_D6C81D</h1> ";
  /*
  html += "Module: ";
  html += Placa ;
  html += " Controller name: ";
  html += ControlName;
  html += " MAC: ";
  html += MAC_Address;
  html += "";
  
    html += "<p> wifi ssdi: ";
    html += ssid;
    html += "</p>";
    html += "<p> wifi password: ";
    html += passwd;
    html += "</p>";
  */
  html += "Time pass:";
  html += timemeasure.c_str();
  //html += "</p>";
  html += "Conectado no Wireless (0-desconectado, 1-conectado): ";
  html += (WiFi.status() == WL_CONNECTED);
  //html += "</p>";
  //html += "WiFi.status: ";
  html += (WiFi.status());
  //html += "</p>";
  //html += "Possible WiFi Status (3 connected): ";
  //html += "(WL_NO_SHIELD=255;WL_IDLE_STATUS=0;WL_NO_SSID_AVAIL=1;WL_SCAN_COMPLETED=2;";
  //html += "WL_CONNECTED=3;WL_CONNECT_FAILED=4;WL_CONNECTION_LOST=5;WL_DISCONNECTED = 6)";
  /*
    html += "<p> WiFi Reconnect count : ";
    html += (countWire);
    html += "</p>";
    html += "<p>marca o tempo se for desconectado: ";
    html += tempoReconnect;
    html += "</p>";
  */
  html += "<p> Conectado ao MQTT (0-desconectado, 1-conectado): ";
  html += client.connected();
  /*
  html += " <a href=\"MQTTButtonWeb\"><button>MQTTButton</button></a>";
  html += " Button status (1-on/0-off): ";
  html += mqttButton;
  html += "</p>";
  html += "<p> MQTT Reconnect count : ";
  //html += (countMQTT);
  html += "</p>";
  html += "";
  html += "<p> Pagina principal, revisao ";
  html += Revisao;
  html += "</p>";
  
  html += "<p>----------------------------------------------------------</p>";
  html += "<p>Numero de sensores: ";
  html += String(tempDeviceCount).c_str();
  html += "</p>";
  html += "<p>Temperature Sensor 1: ";
  html += String(temp1).c_str();
  html += "</p>";
  html += "<p>Temperature Sensor 2: ";
  html += String(temp2).c_str();
  html += "</p>";
  html += "<p>Temperature Sensor 3: ";
  html += String(temp3).c_str();
  html += "</p>";
  html += "<p>Temperature Sensor 4: ";
  html += String(temp4).c_str();
  html += "</p>";
  html += "<p>----------------------------------------------------------</p>";
  */
  html += "<p>----------------------------------------------------------</p>";
  html += " <a href=\"htmlControl\"><button>Control</button></a>";
  html += "<p>----------------------------------------------------------</p>";
  html += "<table style="width:100%">";
  html += "<tr>";
  html += "<th>Sensor</th>";
  html += "<th>Leitura</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>Temperature Sensor 1</th>";
  html += "<th>String(temp1).c_str()</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>Temperature Sensor 1</th>";
  html += "<th>String(temp2).c_str()</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>Temperature Sensor 1</th>";
  html += "<th>String(temp3).c_str()</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>Temperature Sensor 4</th>";
  html += "<th>String(temp4).c_str()</th>";
  html += "</tr>";
  html += "</table>";
  html += "<p>----------------------------------------------------------</p>";
  /*
  html += "<p> Status do Rele:</p>";
  html += "<p>Atuador 1: ";
  html += pin1Estado.c_str();
  html += " <a href=\"Pin1CmdOn\"><button>On</button></a>";
  html += " <a href=\"Pin1CmdOff\"><button>Off</button></a>";
  html += " Button status (1-on/0-off): ";
  html += pin1Cmd.c_str();
  html += "</p>";
  html += "<p>Atuador 2: ";
  html += pin2Estado.c_str();
  html += " <a href=\"Pin2CmdOn\"><button>On</button></a>";
  html += " <a href=\"Pin2CmdOff\"><button>Off</button></a>";
  html += " Button status (1-on/0-off): ";
  html += pin2Cmd.c_str();
  html += "</p>";
  html += "<p>Atuador 3: ";
  html += pin3Estado.c_str();
  html += " <a href=\"Pin3CmdOn\"><button>On</button></a>";
  html += " <a href=\"Pin3CmdOff\"><button>Off</button></a>";
  html += " Button status (1-on/0-off): ";
  html += pin3Cmd.c_str();
  html += "</p>";
  html += "<p>Atuador 4: ";
  html += pin4Estado.c_str();
  html += " <a href=\"Pin4CmdOn\"><button>On</button></a>";
  html += " <a href=\"Pin4CmdOff\"><button>Off</button></a>";
  html += " Button status (1-on/0-off): ";
  html += pin4Cmd.c_str();
  html += "</p>";
  */
  html += "<table style="width:100%">";
  html += "<tr>";
  html += "<th><Atuador 1/th>";
  html += "<thpin1Cmd.c_str()></th>";
  html += "<th><a href=\"Pin1CmdOn\"><button>On</button></a></th>";
  html += "<th><a href=\"Pin1CmdOff\"><button>Off</button></a></th>";
  html += "</tr>";
  
  html += "<tr>";
  html += "<th>Atuador 2</th>";
  html += "<th>pin2Cmd.c_str()</th>";
  html += "<th><a href=\"Pin2CmdOn\"><button>On</button></a></th>";
  html += "<th><a href=\"Pin3CmdOff\"><button>Off</button></a></th>";
  html += "</tr>";
  
  html += "<tr>";
  html += "<th>Atuador 3</th>";
  html += "<th>pin3Cmd.c_str()</th>";
  html += "<th><a href=\"Pin3CmdOn\"><button>On</button></a></th>";
  html += "<th><a href=\"Pin3CmdOff\"><button>Off</button></a></th>";
  html += "</tr>";
  
  html += "<tr>";
  html += "<th>Atuador 4</th>";
  html += "<th>pin4Cmd.c_str()</th>";
  html += "<th><a href=\"Pin4CmdOn\"><button>On</button></a></th>";
  html += "<th><a href=\"Pin4CmdOff\"><button>Off</button></a></th>";
  html += "</tr>";
  html += "</table>";
  html += "<p>----------------------------------------------------------</p>";
  
    /*
    <input type="text" class="form-control" placeholder="Start Minute?" id="startMinute">
    <input type="text" class="form-control" placeholder="Stop Hour?" id="stopHour">
    <input type="text" class="form-control" placeholder="Stop Minute?" id="stopMinute">
    */
    


  html += "</body></html>";

  servidorWeb.send(200, "text/html", html);

}
///////////////////////////////////////////////
void handleControl() {
  // SW to create the html
  String html = "<html><head><title> Programa Cerveja Suzuki</title>";
  html += "<style> body ( background-color:#cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; ) </style> ";
  html += "</head><body> ";
  html += "<h1>suzuki_teste cerveja basic ESP_D6C81D</h1> ";
  
  html += "<p>----------------------------------------------------------</p>";
  html += " <a href=\"htmlRootInicial\"><button>Pagina Principal</button></a>";
  html += "<p>----------------------------------------------------------</p>";
  /*
  html += "Module: ";
  html += Placa ;
  html += " Controller name: ";
  html += ControlName;
  html += " MAC: ";
  html += MAC_Address;
  html += "";
  html += "<p> Pagina principal, revisao ";
  html += Revisao;
  html += 
  
    html += "<p> wifi ssdi: ";
    html += ssid;
    html += "</p>";
    html += "<p> wifi password: ";
    html += passwd;
    html += "</p>";
  */

  html += "<table style="width:100%">";
  html += "<tr>";
  html += "<th>Module:</th>";
  html += "<th>Placa</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>Controller name:</th>";
  html += "<th>ControlName</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>MAC:</th>";
  html += "<th>MAC_Address</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th>MQTT Server:</th>";
  html += "<th>servidor_mqtt</th>";
  html += "</tr>";
  html += "</table>";
  
  html += "Time pass:";
  html += timemeasure.c_str();
  //html += "</p>";
  html += "Conectado no Wireless (0-desconectado, 1-conectado): ";
  html += (WiFi.status() == WL_CONNECTED);
  //html += "</p>";
  html += "WiFi.status: ";
  html += (WiFi.status());
  //html += "</p>";
  html += "Possible WiFi Status: ";
  html += "(WL_NO_SHIELD=255;WL_IDLE_STATUS=0;WL_NO_SSID_AVAIL=1;WL_SCAN_COMPLETED=2;";
  html += "WL_CONNECTED=3;WL_CONNECT_FAILED=4;WL_CONNECTION_LOST=5;WL_DISCONNECTED = 6)";
  /*
    html += "<p> WiFi Reconnect count : ";
    html += (countWire);
    html += "</p>";
    html += "<p>marca o tempo se for desconectado: ";
    html += tempoReconnect;
    html += "</p>";
  */
  /*
  html += "Conectado ao MQTT (0-desconectado, 1-conectado): ";
  html += client.connected();
  html += " <a href=\"MQTTButtonWeb\"><button>MQTTButton</button></a>";
  html += " Button status (1-on/0-off): ";
  html += mqttButton;
  html += "";
  html += " MQTT Reconnect count : ";
  //html += (countMQTT);
  html += "";
  html += "";
  */

  html += "<table style=\"width:100%\">";
  html += "<tr>";
  html += "<th>Conectado ao MQTT (0-desconectado, 1-conectado):</th>";
  html += "<th>client.connected()</th>";
  html += "<th>" "</th>";
  html += "</tr>";
  html += "<tr>";
  html += "<th><a href=\"MQTTButtonWeb\"><button>MQTTButton</button></a></th>";
  html += "<th>Button status (1-on/0-off): </th>";
  html += "<th>";
  html += mqttButton;
  html += "</th></tr>";
  html += "</table>";
  html += "<p>----------------------------------------------------------</p>";
  html += "<p><a href=\"/update\">Update by OTA</a></p>";
  html += "<p>----------------------------------------------------------</p>";
  html += "<p><a href=\"ESPReset\"><button>Reset</button></a></p>";
  html += "<p>----------------------------------------------------------</p>";
  html += "<ul>";
  html += "<li>rev 0.1 - Only Wifi works without any issue</li>";
  html += "<li>rev 0.2 - Time counter</li>";
  html += "<li>rev 0.3.0 - MQTT publish</li>";
  html += "<li>rev 0.3.1 - MQTT publish /subscribe</li>";
  html += "<li>rev 0.4 - logic with the relay and mqtt work</li>";
  html += "<li>rev 0.5 - temperature reading</li>";
  html += "<li>rev 0.6 - contador tempo</li>";
  html += "<li>rev 0.7 ESP reset and mqtt buttons</li>";
  html += "<li>rev 0.8 atuador com buttons, alterado a funcao atuador</li>";
  html += "<li>rev 0.9 cria a pagina de controle</li>";
  html += "</ul>";
  html += "<p>----------------------------------------------------------</p>";
  html += "<input type=\"text\" class=\"form-control\" placeholder=\"Start Hour?\" id=\"startHour\">";
  html += "<button type=\"button\" onclick=\"sendValues()\">Send values</button>";
    

  html += "</body></html>";

  servidorWeb.send(200, "text/html", html);

}

///////////////////////////////////////////////
/* Web Page RESET
*/
void WebReset() {
  // SW to create the html
  String html = "<html><head><title> Programa Cerveja Suzuki</title>";
  html += "<style> body ( background-color:#cccccc; ";
  html += "font-family: Arial, Helvetica, Sans-Serif; ";
  html += "Color: #000088; ) </style> ";
  html += "</head><body> ";
  html += "<h1>suzuki_teste cerveja basic ESP_D6C81D</h1> ";
  html += "<h1>RESET</h1> ";
  html += "</body></html>";

  servidorWeb.send(200, "text/html", html);

}
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
/* Setup

*/
void setup() {

  Serial.begin(115200);

  //nodemcu led pin GPIO16
  pinMode(LED_BUILTIN, OUTPUT);

  //pino de atuadores
  pinMode(PIN1, OUTPUT);
  pinMode(PIN2, OUTPUT);
  pinMode(PIN3, OUTPUT);
  pinMode(PIN4, OUTPUT);

  //Pino de temperatura -> nao necessario
  //pinMode(ONE_WIRE_BUS, INPUT);

  //Version without WifiManager
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //Default 180 seconds
  wifiManager.setTimeout(180);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP_ESP_D6C81D", "senha1234")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    //ESP.reset();
    //delay(5000);
    Serial.print("setup connect \n");
  }

  //Inicia o servidor web
  servidorWeb.on("/", handleRootInicial);
  //Reset Button
  servidorWeb.on("/ESPReset", []() {
    WebReset();
    delay(2000);
    ESP.reset();
  });
  // MQTT Button
  servidorWeb.on("/MQTTButtonWeb", []() {
    if (mqttButton == true) {
      mqttButton = false;
    } else {
      mqttButton = false;
    }
    //carrega a pagina anterior
    handleRootInicial();
  });
  //Pin1 Cmd
  servidorWeb.on("/Pin1CmdOn", []() {
    if (pin1Cmd == "off") {
      pin1Cmd = "on";
    }
    //carrega a pagina anterior
    //
    handleRootInicial();
  });

  servidorWeb.on("/Pin1CmdOff", []() {
    if (pin1Cmd == "on") {
      pin1Cmd = "off";
    }
    //carrega a pagina anterior
    //
    handleRootInicial();
  });
  //Pin2 Cmd
  servidorWeb.on("/Pin2CmdOn", []() {
    if (pin2Cmd == "off") {
      pin2Cmd = "on";
    }

    //carrega a pagina anterior
    handleRootInicial();
  });

  servidorWeb.on("/Pin2CmdOff", []() {
    if (pin2Cmd == "on") {
      pin2Cmd = "off";
    }

    //carrega a pagina anterior
    handleRootInicial();
  });
  //Pin3 Cmd
  servidorWeb.on("/Pin3CmdOn", []() {
    if (pin3Cmd == "off") {
      pin3Cmd = "on";
    }

    //carrega a pagina anterior
    handleRootInicial();
  });

  servidorWeb.on("/Pin3CmdOff", []() {
    if (pin3Cmd == "on") {
      pin3Cmd = "off";
    }

    //carrega a pagina anterior
    handleRootInicial();
  });
  //Pin4 Cmd
  servidorWeb.on("/Pin4CmdOn", []() {
    if (pin4Cmd == "off") {
      pin4Cmd = "on";
    }

    //carrega a pagina anterior
    handleRootInicial();
  });
  servidorWeb.on("/Pin4CmdOff", []() {
    if (pin4Cmd == "on") {
      pin4Cmd = "off";
    }

    //carrega a pagina anterior
    handleRootInicial();
  });

  //Vai para o controle
  servidorWeb.on("/htmlControl", []() {
    //carrega a pagina de controle
    handleControl();
  });

  servidorWeb.on("/htmlRootInicial", []() {
    //carrega a pagina de controle
    handleRootInicial();
  });
  
  //Ativa o webserver
  servidorWeb.begin();

  // Nome MDS
  InicializaMDNS();

  // OTA
  InicializaServicoAtualizacao();

  // Inicializa biblioteca de temperatura
  temp_sensors.begin();
  //seta a resolucao dos sensores de temperatura
  temp_sensors.setResolution(12);
  //contador de
  tempDeviceCount = temp_sensors.getDeviceCount();

  //MQTT configuracao
  int portaInt = atoi(servidor_mqtt_porta); //convert string to number
  client.setServer(servidor_mqtt, portaInt);
  client.setCallback(retornoMQTTSub);
  //Inicia os subscribes
  mqttSubscribe();

  //Watchdog
  delay(50);

}
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
/* Loop

*/
void loop() {

  //Try to connect in the Wifi server each 60 seconds
  if (WiFi.status() != WL_CONNECTED) {
    //if (millis() - wifireconnect_timeout > 500) {
    //wifireconnect_timeout = millis();
    //WiFi.persistent(false);
    //WiFi.disconnect();
    //delay(2000);
    //WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passwd);
    delay(500);
    Serial.print("Connecting again ");
    Serial.print(WiFi.status());
    Serial.print(".\n");
    //countWire++;

    // Nome MDS
    //InicializaMDNS();
    //}
  }  else {
    Serial.print("Connected ");
    Serial.print(WiFi.status());
    Serial.print(".\n");
  }

  //Try to connect in the MQTT server each 60 seconds if it has a wireless connection
  //if ((WiFi.status() == WL_CONNECTED) and (!client.connected())) {
  if (!client.connected() and (mqttButton == true)) {
    //if (millis() - mqttreconnect_timeout > 20000){
    //  mqttreconnect_timeout = millis();
    // reconnect the mqtt
    //troubleshooting the next line
    reconnectMQTT();
    Serial.println("reconectar mqtt");
    //countMQTT++;
    //}
  }

  //Calculo o tempo transcorrido
  updateTempo(mqtt_topico_subTempoCont);
  timemeasure = String("Tempo (D:H:M:mS): " + String(days) + ":" + String(hours) + ":" + String(minute) + ":" + String(second));



  //MQTT
  client.publish("cerveja/supervisorio", timemeasure.c_str());
  client.publish("cerveja/min", String(minute).c_str());
  client.publish("cerveja/seg", String(second).c_str());

  /*
     MQTT Sub - Test
  */
  /*
    if (strcmp(mqtt_topico.c_str(), mqtt_topico_sub1) == 0) {
    if (mqtt_mensagem == "off") {
      pin1Temp = String("off");

    } else {
      pin1Temp = String("on");
    }
    Serial.print("pin1Temp ");
    Serial.print(pin1Temp);
    Serial.print("\n");
    }

    if (strcmp(mqtt_topico.c_str(), mqtt_topico_sub2) == 0) {
    if (mqtt_mensagem == "off") {
      pin2Temp = String("off");
    } else {
      pin2Temp = String("on");
    }
    Serial.print("pin2Temp ");
    Serial.print(pin2Temp);
    Serial.print("\n");
    }
  */
  /*// Teste tempo de led
    if (second - time_timeout > time1sec) {
    time_timeout = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.print(timemeasure);
    Serial.print("\n");
    }
  */


  /*
     Comando de Atuador
  */
  atuaPino(PIN1, mqtt_topico_sub1, mqtt_topico_pub1, &pin1Estado, pin1Cmd, &pin1CmdEstado);
  atuaPino(PIN2, mqtt_topico_sub2, mqtt_topico_pub2, &pin2Estado, pin2Cmd, &pin2CmdEstado);
  atuaPino(PIN3, mqtt_topico_sub3, mqtt_topico_pub3, &pin3Estado, pin3Cmd, &pin3CmdEstado);
  atuaPino(PIN4, mqtt_topico_sub4, mqtt_topico_pub4, &pin4Estado, pin4Cmd, &pin4CmdEstado);

  /*
      Leitura de Temperatura
  */

  //Verifica se ha algum sensor de temperatura
  if (tempDeviceCount > 0) {
    temp_sensors.requestTemperatures();
    // faz a loop entre o sensores
    for (int i = 0; i < tempDeviceCount; i++) {

      //temp1
      if (i == 0) {
        temp1 = (round(temp_sensors.getTempCByIndex(i) * 100) / 100);
        client.publish("cerveja/temp1", String(temp1).c_str());
      }
      //temp2
      if (i == 1) {
        temp2 = (round(temp_sensors.getTempCByIndex(i) * 100) / 100);
        client.publish("cerveja/temp2", String(temp2).c_str());
      }
      //temp3
      if (i == 2) {
        temp3 = (round(temp_sensors.getTempCByIndex(i) * 100) / 100);
        client.publish("cerveja/temp3", String(temp3).c_str());
      }
      //temp4
      if (i == 3) {
        temp4 = (round(temp_sensors.getTempCByIndex(i) * 100) / 100);
        client.publish("cerveja/temp4", String(temp4).c_str());
      }
    }
  }

  //Atualiza a lista de sensores a cada 60 segundos
  if ((millis() - tempoTempSensor) > 60000) {
    // Inicializa biblioteca de temperatura
    temp_sensors.begin();
    //seta a resolucao dos sensores de temperatura
    temp_sensors.setResolution(12);
    //contador de
    tempDeviceCount = temp_sensors.getDeviceCount();
    tempoTempSensor = millis();
  }
  //Debug leitura de temperatura
  //client.publish("cerveja/temp3", String(tempoTempSensor).c_str());
  //client.publish("cerveja/temp4", String(tempDeviceCount).c_str());
  /*
     Atua o Led do nodemcu
  */
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));


  //Servidor Web
  servidorWeb.handleClient();
  //Wifi MQTT;
  client.loop();

  ESP.wdtFeed();

  //Watchdog behavior
  delay(250);
}
