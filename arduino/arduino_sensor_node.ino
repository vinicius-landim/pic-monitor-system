#include <Ultrasonic.h>
#include "Wire.h"

#define DS1307_ADDRESS 0x68
byte zero = 0x00;

HC_SR04 sensor1(12, 13); //(trigger, echo)
const int lm35Pin = A0;
const int ldrPin = A1;
const int ledLdrPin = 7;
const int ledLm35Pin = 6;
const int ledHCPin = 5;
const int buzzerHCPin = 4;
const int buzzerLdrPin = 3;
const int buzzerLm35Pin = 2;

//HC-SR04
int distMeas;
int distThr = -9999;

//LM35
int lm35Read;
float temp;
float tempThr = 9999.0;

//LDR
int ldrRead;
int ldrThr = -9999;

unsigned long previousMillis = 0;
const int buzzerInterval = 500;
int buzzerHCState = 0;
int buzzerLm35State = 0;
int buzzerLdrState = 0;

int flagHCAlert = 0;
int flagLm35Alert = 0;
int flagLdrAlert = 0;
int systemActive = 0;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  Wire.begin();
  setDateTime();

  pinMode(lm35Pin, INPUT);
  pinMode(ldrPin, INPUT);

  pinMode(buzzerLdrPin, OUTPUT);
  pinMode(buzzerLm35Pin, OUTPUT);
  pinMode(buzzerHCPin, OUTPUT);
  pinMode(ledLdrPin, OUTPUT);
  pinMode(ledLm35Pin, OUTPUT);
  pinMode(ledHCPin, OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();

  distMeas = sensor1.distance();

  lm35Read = analogRead(lm35Pin);
  temp = lm35Read * 0.4887; //ADC * 500°C/1023

  ldrRead = analogRead(ldrPin);
  
  if(Serial2.available()){
    char cmd = Serial2.read();
    if(cmd == 'T'){
      Serial2.print(temp);
      Serial2.print('\n');

    } 
    else if(cmd == 'L'){
      Serial2.print(ldrRead);
      Serial2.print('\n');
    }
    else if(cmd == 'N'){
      Serial2.print(distMeas);
      Serial2.print('\n');
    } 
    else if(cmd == 'A'){
      tempThr = Serial2.parseFloat();
      printDate();
      Serial.print(" - LIMITE DE TEMPERATURA DEFINIDO: T = ");
      Serial.print(tempThr);
      Serial.println("*C");
    }
    else if(cmd == 'B'){
      ldrThr = Serial2.parseInt();
      printDate();
      Serial.print(" - LIMITE DE LUMINOSIDADE DEFINIDO: L = ");
      Serial.println(ldrThr);
    }
    else if(cmd == 'C'){
      distThr = Serial2.parseInt();
      printDate();
      Serial.print(" - LIMITE DE DISTÂNCIA DEFINIDO: D = ");
      Serial.print(distThr);
      Serial.println("cm");
    }
    else if(cmd == 'X'){
      systemActive = 1;
      printDate();
      Serial.println(" - SISTEMA LIGADO");
    }
    else if(cmd == 'x'){
      systemActive = 0;
      printDate();
      Serial.println(" - SISTEMA DESLIGADO");
    }
  }

  if(systemActive && distMeas < distThr){
    if(!flagHCAlert){
      printDate();
      Serial.println(" - ALERTA: NÍVEL ULTRAPASSADO");
      flagHCAlert = 1;
    }
    digitalWrite(ledHCPin, HIGH);

    if(currentMillis - previousMillis >= buzzerInterval){
      previousMillis = currentMillis;

      buzzerHCState = !buzzerHCState;
      digitalWrite(buzzerHCPin, buzzerHCState);
    }
  } else {
    if(flagHCAlert){
      printDate();
      Serial.println(" - SOLUCIONADO: ALERTA DE NÍVEL ");
      flagHCAlert = 0;
    }
    digitalWrite(ledHCPin, LOW);
    digitalWrite(buzzerHCPin, LOW);
    buzzerHCState = 0; 
  }

  if(systemActive && ldrRead < ldrThr){
    if(!flagLdrAlert){
      printDate();
      Serial.println(" - ALERTA: LUMINOSIDADE ULTRAPASSADA");
      flagLdrAlert = 1;
    }
    digitalWrite(ledLdrPin, HIGH);

    if(currentMillis - previousMillis >= buzzerInterval){
      previousMillis = currentMillis;

      buzzerLdrState = !buzzerLdrState;
      digitalWrite(buzzerLdrPin, buzzerLdrState);
    }
  } else {
    if(flagLdrAlert){
      printDate();
      Serial.println(" - SOLUCIONADO: ALERTA DE LUMINOSIDADE");
      flagLdrAlert = 0;
    }
    digitalWrite(ledLdrPin, LOW);
    digitalWrite(buzzerLdrPin, LOW);
    buzzerLdrState = 0; 
  }

  if(systemActive && temp > tempThr){
    if(!flagLm35Alert){
      printDate();
      Serial.println(" - ALERTA: TEMPERATURA ULTRAPASSADA");
      flagLm35Alert = 1;
    }
    digitalWrite(ledLm35Pin, HIGH);
    if(currentMillis - previousMillis >= buzzerInterval){
      previousMillis = currentMillis;

      buzzerLm35State = !buzzerLm35State;
      digitalWrite(buzzerLm35Pin, buzzerLm35State);
    }
  } else {
    if(flagLm35Alert){
      printDate();
      Serial.println(" - SOLUCIONADO: ALERTA DE TEMPERATURA");
      flagLm35Alert = 0;
    }
    digitalWrite(ledLm35Pin, LOW);
    digitalWrite(buzzerLm35Pin, LOW);
    buzzerLm35State = 0; 
  }

  delay(200);
}

void printZeros(int valor){
  if(valor < 10){
    Serial.print(0);
  }
  Serial.print(valor);
}

void setDateTime(){

  // byte segundo =      00;  //0-59
  byte minuto =        28;  //0-59
  byte hora =           14;  //0-23
  byte diasemana =    10;  //1-7
  byte dia =               10;  //1-31
  byte mes =            07; //1-12
  byte ano  =            26; //0-99

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(1); 

  // Wire.write(decToBcd(segundo));
  Wire.write(decToBcd(minuto));
  Wire.write(decToBcd(hora));
  Wire.write(decToBcd(diasemana));
  Wire.write(decToBcd(dia));
  Wire.write(decToBcd(mes));
  Wire.write(decToBcd(ano));

  Wire.write(zero); 

  Wire.endTransmission();

}

byte decToBcd(byte val){
// Conversão de decimal para binário
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)  {
// Conversão de binário para decimal
  return ( (val/16*10) + (val%16) );
}

void printDate(){

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(1); //apontar para o registrador 1 (minutos)
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 6);

  // int segundo = bcdToDec(Wire.read());
  int minuto = bcdToDec(Wire.read());
  int hora = bcdToDec(Wire.read() & 0b111111);    //Formato 24 horas
  int diasemana = bcdToDec(Wire.read());             //0-6 -> Domingo - Sábado
  int dia = bcdToDec(Wire.read());
  int mes = bcdToDec(Wire.read());
  int ano = bcdToDec(Wire.read());

//Exibe a data e hora. Ex.:   3/12/13 19:00:00
  
  printZeros(dia);
  Serial.print("/");
  printZeros(mes);
  Serial.print("/");
  printZeros(ano);
  Serial.print(" ");
  printZeros(hora);
  Serial.print(":");
  printZeros(minuto);
  // Serial.print(":");
  // Serial.print(segundo);
}
