/*
    ======================================================================= 
   |                                                                       |
   |                    STAZIONE METEO BY APEX (V1.02)                     |
   |                                                                       |
   |-----------------------------------------------------------------------|
   |                                                                       |
   |         Dati meteo prelevati dall'NTP  www.openweathermap.org         |
   |           Data di oggi prelevata via GET/HTTP da google.com           |
   |                                                                       |
   |     Per un corretto funzionamento bisogna prima creare un account     |
   |    su openweather e generare quindi una ID KEY che verrà inserita     |
   |   nel listato più avanti. Oltre alla ID KEY bisogna prelevare anche   |
   |     un numero (ID città) sulla pagina del meteo della Vs. città       |
   |              che andrà inserito nel listato più avanti.               |
   |     Esempio: (andare a questo indirizzo web della città di ROMA)      |
   |              https://openweathermap.org/city/3169070                  |
   |          il numero finale (3169070) è il vostro CITY ID.              |
   |                                                                       |
   |  Per la connessione con la vostra wifi, inserire il nome della SSID   |
   |        e la vostra password abituale nel listato più avanti.          |
   |                                                                       |
   |       PS: Questa è una modifica ad un sorgente già esistente.         |
   |        Sono ben accette implementazioni e suggerimenti vari.          |
   |                   - apex.elettronica@gmail.com -                      |
   |                                                                       |
   |                APEX is (Antonio Postiglione ElectroniX)               |
   |                                                                       |
    ======================================================================= 




  . =======================================================================
  . MATERIALI USATI E PIEDINI DI COLLEGAMENTO:
  . =======================================================================

  . 8 x Led Matrix (8x64) MAX7219 (2 da 4 in serie) - (china)
  . NodeMCU1.0(ESP-12E Module) ESP8266 V3 - Vers. LoLin (china)
  . DHT22 Shield - Sensore (già pronto con resistore e condensatore - china)
  . LDR Shield - Sensore luce (già pronto con resistore) - china)


  +----NodeMCU-----+--7219--+
  | D3 (GPIO0)     |  CS    |
  +----------------+--------+
  | D5 (GPIO14)    |  CLK   |
  +----------------+--------+
  | D7 (GPIO13)    |  DIN   |
  +----------------+--------+
  | 3.3 V          |  Vcc   |
  | Gnd            |  Gnd   |
  +----------------+--------+


  +----NodeMCU-----+-DHT22--+
  | D4 (GPIO2)     + Signal +
  +----------------+--------+
  | 3.3 V          |  Vcc   |
  | Gnd            |  Gnd   |
  +----------------+--------+


  +----NodeMCU-----+--LDR---+
  | A0 (ADC0)      + Signal +
  +----------------+--------+
  | 3.3 V          |  Vcc   |
  | Gnd            |  Gnd   |
  +----------------+--------+


  . =======================================================================
  . NOTE:
  . PER IL DEBUG, SETTARE IL SERIAL MONITOR A 115200 BAUD
  . IL SENSORE DHT22 HA UNA CERTA LATENZA DI AVVIO. CON MOLTA PROBABILITA'
  . DARA' "nan" COME VALORE RILEVATO: ATTENDERE ALMENO UN CICLO COMPLETO
  . DI LOOP OPPURE, IN ALTERNATIVA, PREMERE RESET SUL NODEMCU OPPURE
  . RICONTROLLARE I CABLAGGI AI VARI PIN.
  . -----------------------------------------------------------------------
  . QUESTO SKETCH USA IL 24% DELLO SPAZIO PROGRAMMI DISPONIBILE
  . LE VARIABILI GLOBALI USANO 33824 BYTE (41%) DI MEMORIA DINAMICA
  . =======================================================================

*/




// =======================================================================
// LIBRERIE USATE
// =======================================================================
#include <ESP8266WiFi.h> // Libreria per il NodeMCU ESP-12E WiFi
#include <SPI.h> // Libreria Serial Peripheral Interface
#include <Adafruit_GFX.h> // Libreria grafica
#include <Max72xxPanel.h> // Libreria per il MAX7219
#include <ArduinoJson.h> // Libreria JavaScriptObjectNotation
#include <DHT.h> // Libreria per il sensore DHT22
// =======================================================================




// =======================================================================
// DEFINIZIONE PIN CS DISPLAY LED MATRIX MAX7219
// =======================================================================
#define pinCS             D3         // Pin del NodeMCU
// =======================================================================




// =======================================================================
// DEFINIZIONE PIN SENSORE DHT22 E TIPO
// =======================================================================
#define DHTPIN            D4         // Pin del NodeMCU
#define DHTTYPE           DHT22      // Tipo di sensore, DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);            // Istanzia la classe dht
// =======================================================================




// =======================================================================
// DEFINIZIONE PIN SENSORE LDR
// =======================================================================
#define photoCell         A0         // Pin del NodeMCU
// =======================================================================









// =======================================================================
// DATI DI CONNESSIONE NodeMCU ESP-12E WIFI E IND. SERVER NTP METEO
// =======================================================================
const char* ssid        = "qui la SSID";   // SSID RETE WIFI
const char* password    = "qui la PASSWORD"; // PASSWORD RETE WIFI

const char *weatherHost = "api.openweathermap.org";   // IND. NTP METEO

// Key generata da http://openweathermap.org/api
String weatherKey = "qui la API KEY"; // API KEY

String weatherLang = "&lang=it"; // Lingua Italiana (it, en, es, ru ..)

// ID Location - presa dall'URL di openweathermap.org
String cityID = "qui la CITY ID"; // CITY ID
// =======================================================================



// =======================================================================
// TIME UTC OFFSET
// =======================================================================
float utcOffset = +1;
// =======================================================================


WiFiClient client; // ISTANZIA CLIENT


// =======================================================================
// VARIABILI GLOBALI
// =======================================================================

String weatherMain = "";
String weatherDescription = "";
String weatherLocation = "";
String country;
String date;
String currencyRates;
String weatherString;
String decodedMsg;
String mmStr;
String dataDiOggi;

// =================================================================
String myData[2]; // necessariamente prima di String deg
String deg = String(char(247)); // unico simbolo dei gradi (°) disp.
// =================================================================

int humidity, pressure, clouds, visibility;
int wait = 40; // Velocità di scorrimento ticker text pausa in millisec.
int offset = 1, refresh = 0;
int numberOfHorizontalDisplays = 12; // N° di MAX7219 in orizzontale (esteso a 12)
int numberOfVerticalDisplays = 1; // N° di MAX7219 in verticale
int spacer = 1; // Spaziatura caratteri display (1 = normale)
int width = 5 + spacer;
int photoCellValue;
int brLevel;
int updCnt = 0;
int dots = 0;
int dx = 0, dy = 0;
int gg = 1, mm = 1, aa = 2017; // pre-impostazione
int h, m, s;
int timeSinceLastRead = 0;

float tp, hm, hic;
float temp;
float tempMin, tempMax;
float windSpeed;

long sunrise, sunset, period;
long dotTime = 0;
long clkTime = 0;
long localEpoc = 0;
long localMillisAtUpdate = 0;

byte del = 0;

// FINE VARIABILI ========================================================









// =======================================================================
// ISTANZIA LED MATRIX - MAX7219
// =======================================================================
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
// =======================================================================







// =======================================================================
// ================================ SETUP ================================
// =======================================================================

void setup(void) {

  // =====================================================================
  // SETTA IL LED MATRIX
  // =====================================================================
  matrix.setIntensity(10); // BRIGHTNESS 0-15 ..iniziale 10
  matrix.fillScreen(LOW); // CLS
  matrix.setPosition(0, 9, 0); // Il primo display a <0, 0>
  matrix.setPosition(1, 8, 0);
  matrix.setPosition(2, 7, 0);
  matrix.setPosition(3, 6, 0);
  matrix.setPosition(4, 5, 0);
  matrix.setPosition(5, 4, 0);
  matrix.setPosition(6, 3, 0);
  matrix.setPosition(7, 2, 0);
  matrix.setPosition(8, 1, 0);
  matrix.setPosition(9, 0, 0);

  // ================ SETTA LA GIUSTA ROTAZIONE DEL MATRIX ===============
  // ================= Valore 3 = ruota display di 90° ===================

  matrix.setRotation(0, 3); // Rotazione dei display di 90°
  matrix.setRotation(1, 3); // ""
  matrix.setRotation(2, 3); // ""
  matrix.setRotation(3, 3); // ""
  matrix.setRotation(4, 3); // ""
  matrix.setRotation(5, 3); // ""
  matrix.setRotation(6, 3); // ""
  matrix.setRotation(7, 3); // ""
  matrix.setRotation(8, 3); // ""
  matrix.setRotation(9, 3); // ""
  // =======================================================================

  // Rileva dati dal sensore interno DHT22 (avvio)
  tp =  dht.readTemperature(); // temperatura
  hm =  dht.readHumidity(); // umidità
  hic = dht.computeHeatIndex(tp, hm); // temperatura percepita

  Serial.begin(115200); // AVVIA LA SERIALE A 115200 BAUD
  WiFi.mode(WIFI_STA); // MANTIENE SEMPRE ATTIVA LA WIFI (NO STBY)
  WiFi.begin(ssid, password); // AVVIA LA WIFI

  while (WiFi.status() != WL_CONNECTED) { // fino a che wifi non connessa
    delay(500);
    Serial.println("Wifi in connessione ...");

    // ======================= MESSAGGIO DI BENVENUTO ======================
    // ================== IN ATTESA DELLA CONNESSIONE WIFI =================

    String text1 = "Meteo WiFi";
    DisplayText(text1);
    delay(2000);
    String text2 = " Tenerife";
    DisplayText(text2);
    delay(2000);

    String text3 = "APEX V1.02";
    DisplayText(text3);
    delay(4000);

    matrix.fillScreen(LOW); // CLS

  } // Fine While
} // FINE DEL SETUP







// =======================================================================
// /\/\/\/\/\/\/\/\/\/\/\/\/\ INIZIO DEL LOOP /\/\/\/\/\/\/\/\/\/\/\/\/\/\
// =======================================================================
void loop(void) {
  // aggiorna l'ora e il meteo una volta all'inizio ..poi ogni 10 min.
  if (updCnt <= 0) {
    updCnt = 10;
    Serial.println("Wifi OK");
    Serial.println("Richiesta dati dal server NTP ...");

    getTime();
    getWeatherData();

    Serial.println("Connessione stabilita.");
    Serial.println("Dati importati !");
    clkTime = millis();
  } // fine if
  // ogni minuto lancia il ticker meteo
  if (millis() - clkTime > 60000 && !del && dots) {
    ScrollText(weatherString);
    updCnt--;
    clkTime = millis();
  } // fine if
  controlBR(); // aggiorna la luminosità del display
  DisplayTime(); // visualizza il time
  // avvia il timer del minuto
  if (millis() - dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  } // fine if
} // FINE DEL LOOP









// =======================================================================
// || ***************************************************************** ||
// || ******************** INIZIO DELLE FUNZIONI ********************** ||
// || ***************************************************************** ||
// =======================================================================

/*
  01 - void DisplayTime()               VISUALIZZA TIME
  02 - void DisplayText(String text)    STAMPA TEXT SUL DISPLAY
  03 - void ScrollText (String text)    SCROLL TICKER TEXT
  04 - void getWeatherData()            LETTURA/CALCOLO DATI METEO DAL WEB
  05 - void getTime()                   RILEVA LA DATA DA GOOGLE
  06 - void updateTime()                AGGIORNAMENTO DELL'OROLOGIO
  07 - void controlBR()                 BRIGHTNESS DEL LED MATRIX
*/





// =======================================================================
// 01 - FUNZIONE VISUALIZZA TIME
// =======================================================================

void DisplayTime() {
  updateTime();
  matrix.fillScreen(LOW);
  int y = (matrix.height() - 8) / 2;

  // Commentare le 2 linee, se si vuole intermittenza dei punti orari separatori
  matrix.drawChar(37, y, (String(":"))[0], HIGH, LOW, 1); // dist marg SX pallini H:M
  matrix.drawChar(53, y, (String(":"))[0], HIGH, LOW, 1); // dist marg SX pallini M:S

  // Togliere il commento, se si vuole intermittenza dei punti orari separatori
  /*
    if (s & 1) { // INTERMITTENZA ON PUNTINI SEPARATORI DELL'ORARIO
      matrix.drawChar(37, y, (String(":"))[0], HIGH, LOW, 1); // dist marg SX pallini H:M
      matrix.drawChar(53, y, (String(":"))[0], HIGH, LOW, 1); // dist marg SX pallini M:S
    }
    else {
      matrix.drawChar(37, y, (String(" "))[0], HIGH, LOW, 1); // dist marg SX pallini H:M
      matrix.drawChar(53, y, (String(" "))[0], HIGH, LOW, 1);
    }
  */

  int xh = 26; // distanza ore dal margine sinistro del display
  int xm = 42; // distanza minuti dal margine sinistro del display
  int xs = 58; // distanza secondi dal margine sinistro del display
  String hour1 = String (h / 10);
  String hour2 = String (h % 10);
  String min1 = String (m / 10);
  String min2 = String (m % 10);
  String sec1 = String (s / 10);
  String sec2 = String (s % 10);
  matrix.drawChar(xh, y, hour1[0], HIGH, LOW, 1); // prima cifra ORE
  matrix.drawChar(xh + 6, y, hour2[0], HIGH, LOW, 1); // seconda cifra ORE
  matrix.drawChar(xm, y, min1[0], HIGH, LOW, 1); // prima cifra MINUTI
  matrix.drawChar(xm + 6, y, min2[0], HIGH, LOW, 1); // seconda cifra MINUTI
  matrix.drawChar(xs, y, sec1[0], HIGH, LOW, 1); // prima cifra SECONDI
  matrix.drawChar(xs + 6, y, sec2[0], HIGH, LOW, 1); // seconda cifra SECONDI
  matrix.write(); // VISUALIZZA L'ORARIO SUL MATRIX
} // FINE FUNZIONE








// =======================================================================
// 02 - FUNZIONE STAMPA TEXT SU DISPLAY
// =======================================================================

void DisplayText(String text) {
  matrix.fillScreen(LOW); // CLS
  for (int i = 0; i < text.length(); i++) {
    int letter = (matrix.width()) - i * width; ///// width = 6
    int x = (matrix.width() + 19) - letter;
    int y = (matrix.height() - 8) / 2;
    matrix.drawChar(x, y, text[i], HIGH, LOW, 1);
    matrix.write(); // STAMPA
  } // fine for
} // FINE FUNZIONE









// =======================================================================
// 03 - FUNZIONE DISPLAY TICKER TEXT
// =======================================================================

void ScrollText (String text) {
  // =================== Visualizza i dati meteo ticker ==================
  for ( int i = 0 ; i < width * text.length() + (matrix.width() - 1) - spacer; i++ ) {
    if (refresh == 1) i = 0;
    refresh = 0;
    matrix.fillScreen(LOW); // CLS
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2;
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < text.length() ) {
        matrix.drawChar(x, y, text[letter], HIGH, LOW, 1);
      } // fine if
      letter--;
      x -= width;
    } // fine while
    matrix.write(); // STAMPA
    delay(wait); // velocita' di scorrimento
  } // fine for
} // FINE FUNZIONE









// =======================================================================
// 04 - FUNZIONE JSON OPENWEATHERMAP.ORG E CALCOLO DATI METEO
// =======================================================================

void getWeatherData()
{
  Serial.print("Connessione ad "); Serial.println(weatherHost);
  if (client.connect(weatherHost, 80)) {
    client.println(String("GET /data/2.5/weather?id=") + cityID + "&units=metric&appid=" +
                   weatherKey + weatherLang + "\r\n" +
                   "Host: " + weatherHost + "\r\nUser-Agent: ArduinoWiFiTest/1.1\r\n" +
                   "Connection: close\r\n\r\n");
  } // fine if
  else {
    Serial.println("Connessione fallita !"); // DEBUG
    return;
  } // fine else
  String line;

  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    Serial.println("Attesa..."); // DEBUG
    repeatCounter++;
  } // fine while
  while (client.connected() && client.available()) {
    char c = client.read();
    if (c == '[' || c == ']') c = ' ';
    line += c; // elimina le parentesi quadre dalla stringa ricevuta
  } // fine while

  client.stop(); // chiudi la connessione

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(line); // elabora la stringa con json

  if (!root.success()) // DEBUG
  {
    Serial.println("Elaborazione <jsonBuf.parseObject(line)> fallita !"); // DEBUG
    return;
  } // fine if

  // =========================   LETTURA DATI DA OPENWEATHERMAP.ORG =========================
  weatherDescription = root["weather"]["description"].as<String>();
  weatherDescription.toLowerCase();
  weatherLocation = root["name"].as<String>(); // Luogo
  country = root["sys"]["country"].as<String>(); // Nazione
  double sunrise = root["sys"]["sunrise"]; // Alba - Unix, UTC
  double sunset = root["sys"]["sunset"]; // Tramonto - Unix, UTC
  temp = root["main"]["temp"]; // temperatura
  humidity = root["main"]["humidity"]; // umidità
  pressure = root["main"]["pressure"]; // pressione
  tempMin = root["main"]["temp_min"]; // temperatura minima
  tempMax = root["main"]["temp_max"]; // temperatura massima
  visibility = root["visibility"]; // visibilità in Km
  windSpeed = root["wind"]["speed"]; // velocità vento in m/s
  clouds = root["clouds"]["all"]; // descrizione del tempo
  //  int long dateTime = root["dt"]; // data UNIX da openweather (implem. futura)

  // Rileva dati dal sensore interno DHT22
  tp =  dht.readTemperature(); // temperatura
  hm =  dht.readHumidity(); // umidità
  hic = dht.computeHeatIndex(tp, hm); // temperatura percepita


  // ============= IMPOSTAZIONE SEQUENZA DELLA STRINGA TICKER DA VISUALIZZARE ==============

  //  Gli array provengono dalla funzione successiva a questa (05 - Google)
  weatherString = "Meteo del " + myData[0] + "/" + myData[1] + "/20" + myData[2] + "     >>     ";

  // non commentare, se si vuole anche la scritta del luogo del meteo
  // weatherString += String(weatherLocation) + " (" + country + ")     ";

  weatherString += "T.Est:" + String(temp, 2) + deg + "C     ";

  // sostituire con String(tempMin).substring(0, 1) (se si vuole una sola cifra decimale)
  weatherString += "Minima:" + String(tempMin) + deg + "C     ";
  // sostituire con String(tempMax).substring(0, 1) (se si vuole una sola cifra decimale)
  weatherString += "Massima:" + String(tempMax) + deg + "C     ";

  weatherString += "Interna:" + String(tp) + deg + "C     "; // dati dal DHT22
  weatherString += "Percepita:" + String(hic) + deg + "C     "; // dati dal DHT22
  weatherString += "Umid.Est:" + String(humidity) + "%     ";
  weatherString += "Umid.Int:" + String(hm) + "%     "; // dati dal DHT22
  weatherString += "Pressione:" + String(pressure) + " hPa     ";
  weatherString += "Pioggia:" + String(clouds) + "% ";
  weatherString += "(" + weatherDescription + ")     ";
  weatherString += "Vento:" + String(windSpeed, 1) + " m/s     ";
  weatherString += "Visib.:" + String(int(visibility) / 1000) + "Km     ";

  // ========================= CALCOLO ALBA E TRAMONTO DEL SOLE =========================
  char buff1[5]; // buffer
  char buff2[5];
  char buff3[5];
  char buff4[5];

  // ----------------------------------------- ALBA -------------------------------------

  double gg_SoleSorge = sunrise / 86400L; // calcolo UNIX dal 1/1/1970
  int Giorni_1 = gg_SoleSorge; // numero intero dei giorni senza la parte decimale
  // ore sorgere del sole
  double oo_SoleSorge = (((gg_SoleSorge - Giorni_1) * 86400) / 3600) + utcOffset;
  int OreSS = oo_SoleSorge; // numero intero delle ore senza la parte decimale
  String albaOra = itoa(OreSS, buff1, 10); // converte numero int in string (ORE ALBA)
  if (OreSS < 9) { // se ore è ad una sola cifra, converte in due cifre
    weatherString += "Alba: 0" + albaOra + ":"; // ticker text (ORE ALBA)
  } // fine if
  else {
    weatherString += "Alba: " + albaOra + ":"; // ticker text (ORE ALBA)
  } // fine else
  double mm_SoleSorge = ((oo_SoleSorge - OreSS) * 3600) / 60; // minuti sorgere del sole
  int MinSS = mm_SoleSorge; // numero intero dei minuti senza la parte decimale
  String albaMin = itoa(MinSS, buff2, 10); // converte numero int in string (MINUTI ALBA)
  if (MinSS < 9) { // se minuti è ad una sola cifra, converte in due cifre
    weatherString += "0" + albaMin + "     "; // ticker text (MINUTI ALBA)
  } // fine if
  else {
    weatherString += albaMin + "     "; // ticker text (MINUTI ALBA)
  } // fine else

  // --------------------------------------- TRAMONTO ------------------------------------

  double gg_SoleTramonta = sunset / 86400L; // calcolo UNIX dal 1/1/1970
  int Giorni_2 = gg_SoleTramonta; // numero intero dei giorni senza la parte decimale
  // ore tramonto del sole
  double oo_SoleTramonta = (((gg_SoleTramonta - Giorni_2) * 86400) / 3600) + utcOffset;
  int OreST = oo_SoleTramonta; // numero intero delle ore senza la parte decimale
  String tramOra = itoa(OreST , buff3, 10); // converte numero int in string (ORE TRAMONTO)
  if (OreST < 9) { // se ore è ad una sola cifra, converte in due cifre
    weatherString += "Tramonto: 0" + tramOra + ":"; // ticker text (ORE TRAMONTO)
  } // fine if
  else {
    weatherString += "Tramonto: " + tramOra + ":"; // ticker text (ORE TRAMONTO)
  } // fine else
  double mm_SoleTramonta = ((oo_SoleTramonta - OreST) * 3600) / 60; // minuti sorgere del sole
  int MinST = mm_SoleTramonta; // numero intero dei minuti senza la parte decimale
  String tramMin = itoa(MinST, buff4, 10); // converte numero int in string (MINUTI TRAMONTO)
  if (MinST < 9) { // se minuti è ad una sola cifra, converte in due cifre
    weatherString += "0" + tramMin + "     "; // ticker text (MINUTI TRAMONTO)
  } // fine if
  else {
    weatherString += tramMin + "     "; // ticker text (MINUTI TRAMONTO)
  } // fine else
} // FINE FUNZIONE










// =======================================================================
// 05 - FUNZIONE RILEVA DATA DA GOOGLE (VIA GET/HTTP)
// =======================================================================

void getTime()
{
  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("Connessione a google fallita !"); // DEBUG
    return;
  } // fine if
  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    Serial.println("."); // DEBUG
    repeatCounter++;
  } // fine while
  String line;
  client.setNoDelay(false);
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      date = "     " + line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);

      // solo per DEBUG ------------------------------------------------------
      //Serial.println(localEpoc);
      //Serial.println(line);                             // include data e ora
      //Serial.println(date.substring(10, 21));           // include solo la data
      //Serial.println(date.substring(10, 12).toInt());   // solo giorno
      //Serial.println(date.substring(13, 16));           // solo mese (inglese)
      //Serial.println(date.substring(19, 21).toInt());   // solo anno ultime 2 cifre
      // ---------------------------------------------------------------------

      gg = date.substring(10, 12).toInt();  // giorno

      // converte da inglese in numero
      if (date.substring(13, 16) == "JAN") {
        mm = 1; // mesi
      } // fine if
      else if (date.substring(13, 16) == "FEB") {
        mm = 2;
      } // fine if
      else if (date.substring(13, 16) == "MAR") {
        mm = 3;
      } // fine if
      else if (date.substring(13, 16) == "APR") {
        mm = 4;
      } // fine if
      else if (date.substring(13, 16) == "MAY") {
        mm = 5;
      } // fine if
      else if (date.substring(13, 16) == "JUN") {
        mm = 6;
      } // fine if
      else if (date.substring(13, 16) == "JUL") {
        mm = 7;
      } // fine if
      else if (date.substring(13, 16) == "AUG") {
        mm = 8;
      } // fine if
      else if (date.substring(13, 16) == "SEP") {
        mm = 9;
      } // fine if
      else if (date.substring(13, 16) == "OCT") {
        mm = 10;
      } // fine if
      else if (date.substring(13, 16) == "NOV") {
        mm = 11;
      } // fine if
      else if (date.substring(13, 16) == "DEC") {
        mm = 12;
      } // fine if

      aa = date.substring(19, 21).toInt();  // anno ultime 2 cifre

      if (mm <= 9) {
        mmStr = "0" + String(mm); // aggiunge uno zero dove serve
      } // fine if

      Serial.println("DATA: " + String(gg) + "/" + mmStr + "/" + String(aa)); // DEBUG

      dataDiOggi = " " + String(gg) + "/" + mmStr + "/" + String(aa); // Data di oggi

      myData[0] = String(gg);
      myData[1] = mmStr;
      myData[2] = String(aa);

      // SOLO PER DEBUG
      //Serial.println ("______ DATA IN ARRAY:");
      //Serial.println(myData[0]);
      //Serial.println(myData[1]);
      //Serial.println(myData[2]);
      //Serial.println ("_____________________");

    } // fine if
  } // fine while
  client.stop(); // chiude la connessione
} // FINE FUNZIONE









// =======================================================================
// 06 - FUNZIONE DI AGGIORNAMENTO DEL TIME RICHIAMATA OGNI SECONDO
// =======================================================================

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L) % 86400L;
  h = ((epoch % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;

  //Serial.println(String(h)+":"+String(m)+":"+String(s)); // DEBUG

} // FINE FUNZIONE











// =======================================================================
// 07 - FUNZIONE LUMINOSITA' LED MATRIX / LDR
// =======================================================================

void controlBR() {
  photoCellValue  = analogRead(photoCell); // legge valore analogico da ldr
  //Serial.println(photoCellValue); // DEBUG
  photoCellValue  = map(photoCellValue, 970, 90, 0, 10); // offset ldr cinese
  matrix.setIntensity(photoCellValue); // intensità matrix

} // FINE FUNZIONE














// END CODE
