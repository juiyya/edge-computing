#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h" // NOVO: biblioteca do sensor DHT

// ---- Wi-Fi (rede simulada do Wokwi) ----
const char *SSID = "Wokwi-GUEST";
const char *SENHA = "";

// ---- MQTT ----
const char *BROKER = "broker.hivemq.com";
const int PORTA = 1883;
const char *TOPICO_CMD = "fiap/edge/yamazaki/led/cmd";
const char *TOPICO_STATUS = "fiap/edge/yamazaki/led/status";
const char *TOPICO_TEMP = "fiap/edge/yamazaki/temp"; // NOVO: telemetria

// ---- Hardware ----
#define LED 21
#define BOTAO 4
#define DHTPIN 15     // pino de dados do DHT22
#define DHTTYPE DHT22 // modelo do sensor (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// ---- Envio periodico da temperatura (nao bloqueante) ----
const unsigned long INTERVALO_TEMP = 2000; // NOVO: a cada 2 s
unsigned long ultimoEnvioTemp = 0;         // NOVO

// ---- Estado compartilhado (ISR <-> loop) ----
volatile bool estadoLed = false;        // estado logico do LED
volatile bool botaoPressionado = false; // flag levantado pela ISR
volatile unsigned long ultimoISR = 0;   // marca de tempo p/ debounce
bool ultimoAplicado = false;            // ultimo estado escrito no LED

WiFiClient rede;
PubSubClient mqtt(rede);

// ISR do botao: curta, so faz debounce e levanta o flag
void IRAM_ATTR isrBotao()
{
    unsigned long agora = millis();
    if (agora - ultimoISR > 200)
    { // ignora repiques < 200 ms
        botaoPressionado = true;
        ultimoISR = agora;
    }
}

// Comando remoto: o callback so ajusta o estado
void callback(char *topico, byte *payload, unsigned int tamanho)
{
    String msg;
    for (unsigned int i = 0; i < tamanho; i++)
        msg += (char)payload[i];

    Serial.print("MQTT [");
    Serial.print(topico);
    Serial.print("]: ");
    Serial.println(msg);

    if (msg == "1" || msg == "on")
        estadoLed = true;
    else if (msg == "0" || msg == "off")
        estadoLed = false;
}

void conectarWiFi()
{
    Serial.print("Conectando ao Wi-Fi");
    WiFi.begin(SSID, SENHA, 6);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi conectado! IP: " + WiFi.localIP().toString());
}

void conectarMQTT()
{
    mqtt.setServer(BROKER, PORTA);
    mqtt.setCallback(callback);
    while (!mqtt.connected())
    {
        Serial.print("Conectando ao broker MQTT...");
        String id = "esp32-fiap-" + String(random(0xffff), HEX);
        if (mqtt.connect(id.c_str()))
        {
            Serial.println(" conectado!");
            mqtt.subscribe(TOPICO_CMD);
            Serial.println("Inscrito em: " + String(TOPICO_CMD));
        }
        else
        {
            Serial.print(" falhou, estado=");
            Serial.println(mqtt.state());
            delay(2000);
        }
    }
}

// NOVO: le e publica a temperatura, sem bloquear o loop
void enviarTemperatura()
{
    unsigned long agora = millis();
    if (agora - ultimoEnvioTemp < INTERVALO_TEMP)
        return;
    ultimoEnvioTemp = agora;

    float t = dht.readTemperature(); // graus Celsius
    if (isnan(t))
    {
        Serial.println("Falha ao ler o DHT22");
        return;
    }

    char buf[8];
    dtostrf(t, 4, 1, buf);          // float -> texto com 1 casa
    mqtt.publish(TOPICO_TEMP, buf); // QoS 0
    Serial.print("Temp -> ");
    Serial.print(buf);
    Serial.println(" C");
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    pinMode(BOTAO, INPUT_PULLUP);                 // repouso em ALTO
    attachInterrupt(digitalPinToInterrupt(BOTAO), // dispara ao apertar
                    isrBotao, FALLING);

    dht.begin(); // NOVO: inicia o sensor

    conectarWiFi();
    conectarMQTT();
}

void loop()
{
    if (!mqtt.connected())
        conectarMQTT();
    mqtt.loop();

    // Entrada local: o botao faz toggle do estado
    if (botaoPressionado)
    {
        botaoPressionado = false;
        estadoLed = !estadoLed;
    }

    // Saida: um unico ponto aplica o estado (venha do botao ou do MQTT)
    if (estadoLed != ultimoAplicado)
    {
        digitalWrite(LED, estadoLed);
        mqtt.publish(TOPICO_STATUS, estadoLed ? "1" : "0", true); // retained
        Serial.println(estadoLed ? "LED -> ON" : "LED -> OFF");
        ultimoAplicado = estadoLed;
    }

    // Telemetria: envia a temperatura periodicamente (nao bloqueante)
    enviarTemperatura();
}
