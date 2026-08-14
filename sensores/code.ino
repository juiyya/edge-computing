const int PINO_TMP = A0;       // TMP36 (temperatura) 
const int PINO_LDR = A1;       // LDR  (luminosidade) 
const int LED_VERDE    = 11; 
const int LED_AMARELO  = 12; 
const int LED_VERMELHO = 13; 

const float TEMP_ALTA  = 30.0;  // °C  -> vermelho 
const float TEMP_MEDIA = 25.0;  // °C  -> atencao 
const float LUZ_MIN    = 40.0;  // %   -> abaixo disso, pouca luz 

void setup() { 

  pinMode(LED_VERDE, OUTPUT); 
  pinMode(LED_AMARELO, OUTPUT); 
  pinMode(LED_VERMELHO, OUTPUT); 
  Serial.begin(9600);           // TinkerCad usa 9600 bps 
} 

// desliga os tres LEDs (garante 1 aceso por vez) 

void apagaTodos() { 

  digitalWrite(LED_VERDE, LOW); 
  digitalWrite(LED_AMARELO, LOW); 
  digitalWrite(LED_VERMELHO, LOW); 
} 

void loop() { 

  // --- Temperatura (TMP36) --- 

  int rawTemp = analogRead(PINO_TMP);            // 0..1023 
  float vTemp = rawTemp * (5.0 / 1023.0);        // volts 
  float tempC = (vTemp * 1000.0 - 500.0) / 10.0; // 10mV/C, offset 500mV 

  // --- Luminosidade (LDR) --- 

  int rawLuz = analogRead(PINO_LDR);        // 0..1023 
  float luzPct = rawLuz * (100.0 / 974);    // 0% escuro .. 100% claro 

  // --- Decisao local: tres condicoes (if / else-if / else) --- 

  if (tempC > TEMP_ALTA) { 
    digitalWrite(LED_VERMELHO, HIGH);          // alerta: muito quente 
  } 
  else if (tempC > TEMP_MEDIA || luzPct < LUZ_MIN) { 

    digitalWrite(LED_AMARELO, HIGH);           // atencao: calor ou pouca luz 

  } else { 

    digitalWrite(LED_VERDE, HIGH);             // ok: confortavel e iluminado 

  } 

  Serial.print("Temp: ");   

  Serial.print(tempC); 

  Serial.print(" C | Luz: ");  

  Serial.print(luzPct); 

  Serial.println(" %"); 

  delay(500); 

} 