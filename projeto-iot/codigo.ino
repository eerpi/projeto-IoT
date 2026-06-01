#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ===== WIFI =====
const char* ssid = "";
const char* password = "";

// ===== MQTT =====
const char* mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;

const char* topico_contador = "fila/pessoas";
const char* topico_status = "fila/status";

// ===== MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);

// ===== LCD =====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== SENSORES =====
int trigEntrada = 5;
int echoEntrada = 18;

int trigSaida = 19;
int echoSaida = 25;

// ===== BUZZER =====
int buzzer = 13;

// ===== VARIÁVEIS =====
int pessoas = 0;

bool detectouEntrada = false;
bool detectouSaida = false;

long distanciaEntrada;
long distanciaSaida;

// ===== CONECTAR WIFI =====
void conectarWiFi() {

  Serial.println("Conectando WiFi...");

  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Conectando");
  lcd.setCursor(0,1);
  lcd.print("WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi OK");

  delay(1000);
}

// ===== CONECTAR MQTT =====
void conectarMQTT() {

  while (!client.connected()) {

    Serial.println("Conectando MQTT...");

    lcd.setCursor(0,1);
    lcd.print("MQTT...");

    if (client.connect("ESP32_FILA")) {

      Serial.println("MQTT conectado!");

      lcd.setCursor(0,1);
      lcd.print("MQTT OK      ");

    } else {

      Serial.print("Erro MQTT: ");
      Serial.println(client.state());

      delay(2000);
    }
  }
}

// ===== MEDIR DISTÂNCIA =====
long medirDistancia(int trig, int echo) {

  digitalWrite(trig, LOW); //começa desligado
  delayMicroseconds(2); //estabilizar

  digitalWrite(trig, HIGH); //gera o pulso ultrassonico
  delayMicroseconds(10); //tempo p sensor funcionar

  digitalWrite(trig, LOW); //finaliza o pulso ultrassonico

  long duracao = pulseIn(echo, HIGH); //cronometro de qnt tempo levou p som ir e voltar

  long distancia = duracao * 0.034 / 2; //conversão tempo em distancia, dividimos pela metade pra ter apenas a distancia de ida

  return distancia;
}

// ===== ENVIAR MQTT =====
void enviarMQTT() {

  char textoPessoas[10];

  itoa(pessoas, textoPessoas, 10);

  client.publish(topico_contador, textoPessoas);

  String status;

  if (pessoas <= 4) {

    status = "Baixa";

  } else if (pessoas <= 8) {

    status = "Media";

  } else {

    status = "Lotada";
  }

  client.publish(topico_status, status.c_str());

  Serial.println("Dados enviados MQTT");
}

// ===== SETUP =====
void setup() {

  Serial.begin(115200);

  // ===== LCD =====
  Wire.begin(21,22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Contador Fila");

  lcd.setCursor(0,1);
  lcd.print("Iniciando");

  delay(1500);

  // ===== WIFI =====
  conectarWiFi();

  // ===== MQTT =====
  client.setServer(mqtt_broker, mqtt_port);

  // ===== PINOS =====
  pinMode(trigEntrada, OUTPUT);
  pinMode(echoEntrada, INPUT);

  pinMode(trigSaida, OUTPUT);
  pinMode(echoSaida, INPUT);

  pinMode(buzzer, OUTPUT);

  lcd.clear();
}

// ===== LOOP =====
void loop() {

  // ===== MQTT =====
  if (!client.connected()) {

    conectarMQTT();
  }

  client.loop();

  // ===== LEITURA =====
  distanciaEntrada =
  medirDistancia(trigEntrada, echoEntrada);

  distanciaSaida =
  medirDistancia(trigSaida, echoSaida);

  // ===== ENTRADA =====
  if (distanciaEntrada < 30 && !detectouEntrada) {

    pessoas++;

    detectouEntrada = true;

    Serial.println("Pessoa entrou");

    enviarMQTT();

    delay(300);
  }

  if (distanciaEntrada > 35) {

    detectouEntrada = false;
  }

  // ===== SAÍDA =====
  if (distanciaSaida < 30 && !detectouSaida) {

    if (pessoas > 0) {

      pessoas--;

      Serial.println("Pessoa saiu");

      enviarMQTT();

      delay(300);
    }

    detectouSaida = true;
  }

  if (distanciaSaida > 35) {

    detectouSaida = false;
  }

  // ===== LCD =====
  lcd.setCursor(0,0);
  lcd.print("Fila: ");
  lcd.print(pessoas);
  lcd.print("   ");

  lcd.setCursor(0,1);

  if (pessoas <= 4) {

    lcd.print("Fila Baixa   ");

    noTone(buzzer);

  }

  else if (pessoas <= 8) {

    lcd.print("Fila Media   ");

    noTone(buzzer);

  }

  else {

    lcd.print("Fila Lotada  ");

    tone(buzzer, 2000);
  }

  delay(200);
}