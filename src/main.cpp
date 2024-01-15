#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <EEPROM.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 2. Define the API Key */
#define API_KEY "AIzaSyDU3ZkIMAfI8dJjA6nNyjsYvSJzR-nJpN8"

/* 3. Define the RTDB URL */
#define DATABASE_URL "the-greenhouse-6adef-default-rtdb.firebaseio.com" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "admin@gmail.com"
#define USER_PASSWORD "admin123"

#define sensorDHT 27
#define sensorLDR 35

#define ventilador 32
#define bomba 33
#define luces 25

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson myJson;
FirebaseJsonData myJsonData;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

String mediciones;
int temp, hum, lu, tempSup, tempInf, humInf, humSup, luInf, luSup, lu2;

DHT dht(sensorDHT,DHT11);

String ssid1 = "Microcontroladores";
String pass1 = "raspy123";
String ssid2 = "NETLIFE-ROSA";
String pass2 = "Hunter*2023+";

void escribirStringEnEEPROM(String cadena, int direccion) {
  int longitudCadena = cadena.length();
  for (int i = 0; i < longitudCadena; i++) {
    EEPROM.write(direccion + i, cadena[i]);
  }
  EEPROM.write(direccion + longitudCadena, '\0'); // Null-terminated string
  EEPROM.commit(); // Guardamos los cambios en la memoria EEPROM
}

String leerStringDeEEPROM(int direccion) {
  String cadena = "";
  char caracter = EEPROM.read(direccion);
  int i = 0;
  while (caracter != '\0' && i < 100) {
    cadena += caracter;
    i++;
    caracter = EEPROM.read(direccion + i);
  }
  return cadena;
}

bool esDireccionOcupada(int direccion) {
  char primerCaracter = EEPROM.read(direccion);
  return primerCaracter != '\0';
}

void borrarEEPROM(){
  for(int i= 0;i<700;i++){
    EEPROM.write(i,0);
  }
  EEPROM.commit();
}

void setup(){
  Serial.begin(115200);
  EEPROM.begin(700); //inicializar el tamano de memoria eeprom a usar

  pinMode(sensorLDR, INPUT);
  pinMode(ventilador, OUTPUT);
  pinMode(bomba, OUTPUT);
  pinMode(luces, OUTPUT);

  dht.begin();

  if (!esDireccionOcupada(50)){
  escribirStringEnEEPROM(ssid1,50);
  }

  if (!esDireccionOcupada(200)){
  escribirStringEnEEPROM(pass1,200);
  }

  if (!esDireccionOcupada(350)){
  escribirStringEnEEPROM(ssid2,350);
  }

  if (!esDireccionOcupada(500)){
  escribirStringEnEEPROM(pass2,500);
  }

  // Arreglo para almacenar nombres y contraseñas de las redes WiFi
  String redes[2][2];
  redes[0][0] = leerStringDeEEPROM(50);
  redes[0][1] = leerStringDeEEPROM(200);
  redes[1][0] = leerStringDeEEPROM(350);
  redes[1][1] = leerStringDeEEPROM(500);

  // Intentar conectarse a cualquiera de las redes WiFi disponibles
  for (int i = 0; i < sizeof(redes) / sizeof(redes[0]); i++) {
    Serial.print("Intentando conectar a ");
    Serial.println(redes[i][0]);

    WiFi.begin(redes[i][0].c_str(), redes[i][1].c_str()); // Intentar conectar con el nombre y contraseña actuales

    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 10) {
      delay(1000);
      Serial.print(".");
      intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Conectado a la red WiFi");
      Serial.print("Dirección IP: ");
      Serial.println(WiFi.localIP());
      break; // Si se conecta a alguna red, salir del bucle
    } else {
      Serial.println("");
      Serial.println("No se pudo conectar a esta red");
    }
  }
  borrarEEPROM();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  fbdo.setBSSLBufferSize(4096 , 1024 );

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);

}

void loop(){
  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  hum = dht.readHumidity();
  temp = dht.readTemperature();
  lu = analogRead(sensorLDR);
  lu2 = map(lu,0,4095,100,0);

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    Firebase.set(fbdo, "/Usuario1/temp", temp);
    Firebase.set(fbdo, "/Usuario1/hum", hum);
    Firebase.set(fbdo, "/Usuario1/ilum", lu2);

    Firebase.get(fbdo,"/Usuario1");
    mediciones = fbdo.jsonString();
    myJson.setJsonData(mediciones);
    myJson.get(myJsonData,"/tempInf");
    tempInf = myJsonData.intValue;
    myJson.get(myJsonData,"/tempSup");
    tempSup = myJsonData.intValue;
    myJson.get(myJsonData,"/humInf");
    humInf = myJsonData.intValue;
    myJson.get(myJsonData,"/humSup");
    humSup = myJsonData.intValue;
    myJson.get(myJsonData, "/ilumInf");
    luInf = myJsonData.intValue;
    myJson.get(myJsonData, "/ilumSup");
    luSup = myJsonData.intValue;
    delay(500);

    count++;
  }

  if(temp>tempInf || temp>tempSup){
    digitalWrite(ventilador,HIGH);
    Serial.println("Se enciende el ventilador");
  }else{
    Serial.println("Se apaga el ventilador");
    digitalWrite(ventilador,LOW);
  }

  if(hum<humInf || hum<humSup){
    digitalWrite(bomba,HIGH);
    Serial.println("Se enciende la bomba");
  }else{
    Serial.println("Se apaga la bomba");
    digitalWrite(bomba,LOW);
  }

  if(lu<luInf || lu<luSup){
    digitalWrite(luces,HIGH);
    Serial.println("Se enciende los led");
  }else{
    digitalWrite(luces,LOW);
    Serial.println("Se apaga los led");
  }



  Serial.print("Humedad: ");
  Serial.println(hum);
  Serial.print("Temperatura: ");
  Serial.println(temp);
  Serial.print("Luz: ");
  Serial.println(lu2);
  Serial.println("--------------------------------------");
  delay(2000);
}