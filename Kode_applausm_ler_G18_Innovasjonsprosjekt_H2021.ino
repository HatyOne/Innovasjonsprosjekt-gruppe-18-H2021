#include <Servo.h> //Bibliotek for kontroll av servo
#include <WiFi.h>
#include <WiFiClientSecure.h>
//--------------------------------Applausmeter
static const int servoPin = 14;
Servo servo1;
#define LedP1 18
#define LedP2 19
#define LedP3 21
#define LedP4 22

//-------------------------------------------
//--------------------------------Lydmåler
int LowerDbLimit = 65; //Nedre grense for måleområdet servoen reagerer
int UpperDbLimit = 105; //Øvre grense for måleområdet servoen reagerer

#define SoundSensorPin 34  //Pin for lesing av lydsensoren.
#define VREF  5.0  //Spenning lydmåleren får. 

//--------------------------------Database

int counter = 0;


#define ON_Board_LED 2  //--> Definerer innebygd led på esp32, brukes for indikasjon under tilkobling til internett.

//----------------------------------------SSID og passord til WiFi router.
const char* ssid = "Trygve"; //--> wifi navn eller SSID.
const char* password = "godSusan"; //--> WiFi passord.
//----------------------------------------
//----------------------------------------Host & httpsPort
const char* host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------
WiFiClientSecure client; //--> Lager ett WiFiClientSecure objekt.
String GAS_ID = "AKfycbwQR5atZS06xtimUTtCK3xTOCeE4cKj01jHaMbc12ujloMgnGRGk0qyrXtXSzVn48GU8w"; //--> spreadsheet script ID

void setup() {

  Serial.begin(115200); 
  servo1.attach(servoPin);
  pinMode(LedP1,OUTPUT);
  pinMode(LedP2,OUTPUT);
  pinMode(LedP3,OUTPUT);
  pinMode(LedP4,OUTPUT); //Definerer modusen til pinnene. 
  digitalWrite(LedP1, LOW); 
  digitalWrite(LedP2, LOW);
  digitalWrite(LedP3, LOW);
  digitalWrite(LedP4, LOW); //Skrur pinnen av
  
  WiFi.begin(ssid, password); //--> Kobler til wifirouter
  Serial.println("");
    
  pinMode(ON_Board_LED,OUTPUT); //--> On Board LED port Direction output
  digitalWrite(ON_Board_LED, HIGH); //--> Turn off Led On Board

  //----------------------------------------Venter til tilkobling etablert
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { 
    Serial.print(".");
    //---------------------------------------- Blinker den innebygde LED-en i prosessen den tilkobles wifi ruteren. 
    digitalWrite(ON_Board_LED, LOW);
    delay(250);
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
    //----------------------------------------
  }
  //----------------------------------------
  digitalWrite(ON_Board_LED, HIGH); //--> Skrur av den innebygde LED-en når tilkoblet wifi ruteren.
  //---------------------------------------- Ved vellykket tilkobling til wifi ruteren, vil den besøkte IP adressen fremvises på skjermen i seriell monitor.
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //----------------------------------------

  client.setInsecure();
}

void loop() {
  float voltageValue,dbValue;
  voltageValue = analogRead(SoundSensorPin) / 6206.0 * VREF; //Tallet 6206.0 kommer fra (4096/3.3)*5.0, Dette nye tallet er brukt da lydsensoren er kalibrert for en Arduino Uno som måler fra 0V til 5V og gir målingene en digital verdi fra 0 til 1023 mens ESP32 måler fra 0V til 3.3V og tildeler verdier fra 0 til 4095
  dbValue = voltageValue * 50.0;  //convert voltage to decibel value
  Serial.print(dbValue,1);
  Serial.println(" dBA");
  //-----------------------------------------------------------------------------------------Applausmeter
  float dBToRadians,ServoChange,ServoPos; 
  dBToRadians = (dbValue-LowerDbLimit)*180/(UpperDbLimit - LowerDbLimit); //formel for dB til radianer
  servo1.write((180.0-dBToRadians));
  float Quadrant;
  Quadrant = (UpperDbLimit-LowerDbLimit)/4; //Tall som tilsvarer den kvantifiserte endringen i desibelmåling som kreves for at posisjonen på servomotoren skal endre sin posisjon med 45 grader. Dvs. endringen for å endre kvradrant i halvsirkelen.
  
  if (Quadrant*3+UpperDbLimit <= dbValue){//skrur på alle led om målt desibel er høyt nok
     digitalWrite(LedP4, HIGH); 
     digitalWrite(LedP3, HIGH);
     digitalWrite(LedP2, HIGH);
     digitalWrite(LedP1, HIGH);
  }
  else if (Quadrant*2+UpperDbLimit <= dbValue){//skrur på 3/4 av alle led.
     digitalWrite(LedP4, LOW); 
     digitalWrite(LedP3, HIGH);
     digitalWrite(LedP2, HIGH);
     digitalWrite(LedP1, HIGH);
  }
  else if (Quadrant+UpperDbLimit <= dbValue){//Skrur på 2/4 av alle led.
     digitalWrite(LedP4, LOW); 
     digitalWrite(LedP3, LOW);
     digitalWrite(LedP2, HIGH);
     digitalWrite(LedP1, HIGH);
  }
  else if (UpperDbLimit <= dbValue){//Skrur på 1/4 av alle led.
     digitalWrite(LedP4, LOW); 
     digitalWrite(LedP3, LOW);
     digitalWrite(LedP2, LOW);
     digitalWrite(LedP1, HIGH);
  }
  else{                             //skrur av alle led
     digitalWrite(LedP4, LOW); 
     digitalWrite(LedP3, LOW);
     digitalWrite(LedP2, LOW);
     digitalWrite(LedP1, LOW);
  }
 
 
 
  //-----------------------------------------------------------------------------------------Database
  String Deci = "Decibel : " + String(dbValue) + " dB";
  Serial.println(Deci);
  
  counter = counter + 1;
  
  if (counter = 3){
    sendData(dbValue); //--> Kaller på funksjonen "sendata" og sender verdien dBValue. 
    counter = 0;
  }
  
  delay(1000);  
  

  
}


void sendData(float dec) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  //----------------------------------------Kobler til Google host
  if (!client.connect(host, httpsPort)) {//skriver feilmelding om tap av tilkobling. 
    Serial.println("connection failed");
    return;
  }
  //----------------------------------------

  //----------------------------------------Prosesserer data og sending av data
  String string_decibel =  String(dec);
  String url = "/macros/s/" + GAS_ID + "/exec?decibel=" + string_decibel;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  //----------------------------------------Sjekker om sending av data var vellykket
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n'); //Div feilmeldinger/kotrollmeldinger følger under.
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp32/Arduino CI successfull!");
  } else {
    Serial.println("esp32/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  //----------------------------------------
} 
