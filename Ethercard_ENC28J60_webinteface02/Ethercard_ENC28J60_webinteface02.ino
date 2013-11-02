// This demo started off from:
// Present a "Will be back soon web page", as stand-in webserver.
// 2011-01-30 <jc@wippler.nl> 
// connection ENC28J60 Ethernet Module: http://www.geeetech.com/wiki/index.php/Arduino_ENC28J60_Ethernet_Module    
// switching LEDs on and off (post 88): http://forum.arduino.cc/index.php/topic,113496.0.html  does work, but there is a problem with: if (ether.dhcpExpired()), this part was deleted
// sketch for using the ethercard to send/receive info over the internet, switching on/off LED and reading from input pin A0:  http://homepage.ntlworld.com/alan.blackham/arduino/ethernet1.txt   does not work out of the box
// I used some of this: http://winkleink.blogspot.nl/2012/08/arduino-ethernet-ethercard-xamp-web.html 
//
// DS1820 stuff:  http://tushev.org/articles/arduino/item/52-how-it-works-ds18b20-and-arduino    
//
// This code works, 
// For a float to string conversion the function dtostrf() was used initially, but this appeared to work only for l variable and when employed in addition for another, the sketch ran improperly.
// therefore function floatToString() was used, see: http://forum.arduino.cc/index.php/topic,37391.0.html 
//
// Next steps: 
// for sending data from the Arduino to a web-based MySQL database, see: http://forum.jeelabs.net/node/677.html  
// http://stackoverflow.com/questions/17791876/sending-post-data-with-arduino-and-enc28j60-ethernet-lan-network-module   
// 
 
#include <EtherCard.h>

#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire DS1820 is plugged into port 6 on the Arduino
#define ONE_WIRE_BUS 3
#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)


// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

#if STATIC
// ethernet interface ip address
static byte myip[] = { 
  192,168,178,200 };
// gateway ip address
static byte gwip[] = { 
  192,168,178,1 };
#endif

// ethernet mac address - must be unique on your network
static byte mymac[] = { 
  0x74,0x69,0x69,0x2D,0x30,0x31 };
#define LED 6  // define LED pin
#define Heater 4 // Resistor 150 ohm, fixed on DS1820, connected between D4 and GND
#define Relay1 6 // relay for CV temporary ON
#define Relay2 7 // relay for reset CV

int sensorPin = A0;           // select the input pin for the potentiometer to display as test data
bool ledStatus = false;
bool ledStatus2= false;
bool statuschange = false;
char Tsensor_data[2][10];  // set up an array of char[] strings
char Lsensor_data[10];     // set up char array for LDR data
// char *Tsensor[2];   // set up an array of pointers to the Tsensor char[] strings  
//float LsensorValue;
//float TMP1;

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer
BufferFiller bfill;

void setup(){
  Serial.begin(57600);
  Serial.println("\nTesting ethernet connection ......");
  
  // Start up the DS1820 library
  sensors.begin();

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode (Relay1, OUTPUT);
  digitalWrite(Relay1, LOW);  
  pinMode (Relay2, OUTPUT);
  digitalWrite(Relay2, LOW);
  pinMode(Heater, OUTPUT);
  digitalWrite(Heater, LOW);

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");
#endif
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  
}

const char http_OK[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n\r\n"
"<meta http-equiv='refresh' content='5'/>"
"<title>Ethercard interface</title>" 
"<H1><center>Prive pagina</center></H1><UL>";

const char http_Found[] PROGMEM =
"HTTP/1.0 302 Found\r\n"
"Location: /\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
"HTTP/1.0 401 Unauthorized\r\n"
"Content-Type: text/html\r\n\r\n"
"<h1>401 Unauthorized</h1><UL>";

void homePage() { 
//  ds1820(0);
//  ds1820(1);
  bfill.emit_p(PSTR(
    "$F"                                //$F represents a string, $D represents word or byte, $S represents char[].
    "<LI>Temperature sensor-0 is $S degrees Celcius"   
    "<LI>TricolorLED is <a href=\"?led=$F\">$F</a> -> "
   "varying LDR voltage is $S <br><p>"
    "<LI>Heating resistor is <a href=\"?led2=$F\">$F</a> -> "
    "Temperature sensor-1 is $S degrees Celcius<br>"),
  http_OK,
  ds1820(1),
  ledStatus?PSTR("off"):PSTR("on"),  // deze strings worden gebruikt op de 4 plaatsen van de $F hierboven, de small-caps in de URL string, de CAPS in de link op de pagina.     
  ledStatus?PSTR("ON"):PSTR("OFF"),   // maar hoe deze (:) notatie precies werkt ??.... : use ledStatus true or false to choose between the two strings
//  Lsensor_data,
  ldr(),
  ledStatus2?PSTR("off"):PSTR("on"),   // dit zijn de variabelen voor de URL string
  ledStatus2?PSTR("ON"):PSTR("OFF"),   // en deze voor overeenkomstige link op webpagina
  ds1820(0));
}

void loop(){
  // DHCP expiration is a bit brutal, because all other ethernet activity and
  // incoming packets will be ignored until a new lease has been acquired
//  if (!STATIC && ether.dhcpExpired() ) {                                          //  dhcpExpired() no more in library
//    Serial.println("Acquiring DHCP lease again");
//   ether.dhcpSetup();
//  }

// do a DS1820 reading
//  Serial.print ds1820();
  
  if (statuschange) {
     digitalWrite(LED, ledStatus);
     digitalWrite(Heater, ledStatus2); 
     Serial.print("Led status: ");
     Serial.println(ledStatus);
     Serial.print("Heater status: ");
     Serial.print(ledStatus2);
     Serial.println("");
     statuschange = false;
  }
  // wait for an incoming TCP packet, but ignore its contents
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len); 
  if (pos) {
    // write to LED digital output
    
    delay(1);   // necessary for my system
    bfill = ether.tcpOffset();
    char *data = (char *) Ethernet::buffer + pos;
    if (strncmp("GET /", data, 5) != 0) {
      // Unsupported HTTP request
      // 304 or 501 response would be more appropriate
      bfill.emit_p(http_Unauthorized);
    }
    else {
      
      data += 5;
      if (data[0] == ' ') {
        // Return home page
        homePage();
      }                                                                   // hier kunnen we via extra info na de URL zaken regelen, bv CV aan/uit
        else if (strncmp("?relay01", data, 8) == 0) {
        relay(1);  
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?led=off", data, 8) == 0) {
        if (ledStatus) {ledStatus = false; statuschange = true;}  
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?led=on", data, 7) == 0) {
        if (!ledStatus) {ledStatus=true; statuschange = true;}
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?led2=off", data, 9) == 0) {
        if (ledStatus2) { ledStatus2=false;  statuschange = true;}
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?led2=on", data, 8) == 0) {
        if (!ledStatus2) { ledStatus2=true;  statuschange = true;}
        bfill.emit_p(http_Found);
      }
      else {
        // Page not found
        bfill.emit_p(http_Unauthorized);
      }
    }
    ether.httpServerReply(bfill.position());    // send http response
  }
}

void relay(int number) {
  if (number == 1) {
    digitalWrite(Relay1, HIGH);
    delay(300);
    digitalWrite(Relay1, LOW);
  }
    else {
      if (number == 2) {
      digitalWrite(Relay2, HIGH);
      delay(300);
      digitalWrite(Relay2, LOW);
    }
  }
}

char * ds1820(int Tsensor_number) {
    sensors.requestTemperatures(); // Send the command to get temperatures, request to all devices on the bus
//    TMP1 = sensors.getTempCByIndex(Tsensor_number);
//    Serial.println((float)(TMP1),2);
    floatToString(Tsensor_data[Tsensor_number], sensors.getTempCByIndex(Tsensor_number), 2, 6);
//    Serial.println(Tsensor_data[Tsensor_number]);
    return Tsensor_data[Tsensor_number];
}

char * ldr() {
// do a LDR voltage measurement  
//  LsensorValue = float(analogRead(sensorPin))/205;
//  LsensorValue = LsensorValue/205;
//  Serial.println((float)(LsensorValue),7);
//  dtostrf(LsensorValue, 4, 1, Lsensor_data);  // convert float to char-string
  floatToString(Lsensor_data, float(analogRead(sensorPin))/205, 2, 6);
//  Serial.println(Lsensor_data);
  return Lsensor_data;
}


char * floatToString(char * outstr, double val, byte precision, byte widthp){
  char temp[16];
  byte i;
  // compute the rounding factor and fractional multiplier
  double roundingFactor = 0.5;
  unsigned long mult = 1;
  for (i = 0; i < precision; i++)
  {
    roundingFactor /= 10.0;
    mult *= 10;
  }
  temp[0]='\0';
  outstr[0]='\0';
  if(val < 0.0){
    strcpy(outstr,"-\0");
    val = -val;
  }
  val += roundingFactor;
  strcat(outstr, itoa(int(val),temp,10));  //prints the int part
  if( precision > 0) {
    strcat(outstr, ".\0"); // print the decimal point
    unsigned long frac;
    unsigned long mult = 1;
    byte padding = precision -1;
    while(precision--)
      mult *=10;
    if(val >= 0)
      frac = (val - int(val)) * mult;
    else
      frac = (int(val)- val ) * mult;
    unsigned long frac1 = frac;
    while(frac1 /= 10)
      padding--;
    while(padding--)
      strcat(outstr,"0\0");
    strcat(outstr,itoa(frac,temp,10));
  }
  // generate space padding 
  if ((widthp != 0)&&(widthp >= strlen(outstr))){
    byte J=0;
    J = widthp - strlen(outstr);
    for (i=0; i< J; i++) {
      temp[i] = ' ';
    }
    temp[i++] = '\0';
    strcat(temp,outstr);
    strcpy(outstr,temp);
  }
  return outstr;
}


