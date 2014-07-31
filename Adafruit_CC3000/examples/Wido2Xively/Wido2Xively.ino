#include <Metro.h>
Metro TempMetro = Metro(100,true);
Metro uploadMetro = Metro(2000,true);

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <avr/wdt.h>
const int chipSelect = 4;

#define WiDo_IRQ   7
#define WiDo_VBAT  5
#define WiDo_CS    10
Adafruit_CC3000 WiDo = Adafruit_CC3000(WiDo_CS, WiDo_IRQ, WiDo_VBAT,
SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "DFRobot WIFI"           // cannot be longer than 32 characters!
#define WLAN_PASS       "DFRobot2014"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  2000
#define TCP_TIMEOUT      3000

#define CC3000_TINY_DRIVER
//
#define WEBSITE  "api.xively.com"
#define API_key  "Nm8vxZaYtkCreW9oBL74VIxY93ONHsvNlpizj6QkIM8hCXwT"
#define feedID  "1802204668"

void setup(){

  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 

  displayDriverMode();

  /* Initialise the module */
  Serial.println(F("\nInitialising the CC3000 ..."));
//  if (!WiDo.begin())
//  {
//    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
//    while(1);
//  }

  uint16_t firmware = checkFirmwareVersion();
  if (firmware < 0x113) {
    Serial.println(F("Wrong firmware version!"));
    for(;;);
  } 
  displayMACAddress();

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); 
  Serial.println(ssid);

  /* NOTE: Secure connections are not available in 'Tiny' mode!
   By default connectToAP will retry indefinitely, however you can pass an
   optional maximum number of retries (greater than zero) as the fourth parameter.
   */
  if (!WiDo.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }

  Serial.println(F("Connected!"));

  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!WiDo.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  
}

uint32_t ip = 0;
float temp = 0;

void loop(){

  static Adafruit_CC3000_Client WiDoClient;
  static unsigned long RetryMillis = millis();

  if(!WiDoClient.connected() && millis() - RetryMillis > TCP_TIMEOUT){
    // Update the time stamp
    RetryMillis = millis();


    Serial.println(F("Unable to connect the Local Server"));
    WiDoClient.close();

    //Get DFRobot IOT Server IP    
    ip = WiDo.IP2U32(216,52,233,120);  //uint32_t ip = cc3000.IP2U32(216,52,233,120);
    WiDoClient = WiDo.connectTCP(ip, 80);
  }
  else if(WiDoClient.connected() && uploadMetro.check()){

    wdt_enable(WDTO_8S);  
    // Prepare JSON for Xively & get length
    int length = 0;
    // JSON beginning
    String data_start = "";
    data_start = data_start + "\n" 
      + "{\"version\":\"1.0.0\",\"datastreams\" : [ ";
    // JSON for temperature & humidity
    String data_temperature = "{\"id\" : \"Temperature\",\"current_value\" : \"" 
      + String(int(temp)) + "\"}]}";
    // Get length
    length = data_start.length() + data_temperature.length();
    wdt_reset();
    
    Serial.println(F("Connected to Xively server."));
    
    // Send headers
    Serial.print(F("Sending headers"));
    WiDoClient.fastrprint(F("PUT /v2/feeds/"));
    WiDoClient.fastrprint(feedID);
    WiDoClient.fastrprintln(F(".json HTTP/1.0"));
    Serial.print(F("."));
    WiDoClient.fastrprintln(F("Host: api.xively.com"));
    Serial.print(F("."));
    WiDoClient.fastrprint(F("X-ApiKey: "));
    WiDoClient.fastrprintln(API_key);
    Serial.print(F("."));
    WiDoClient.fastrprint(F("Content-Length: "));
    WiDoClient.println(length);
    Serial.print(F("."));
    WiDoClient.fastrprint(F("Connection: close"));
    Serial.println(F(" done."));
    
    // Reset watchdog
    wdt_reset();
    
    // Send data
    Serial.print(F("Sending data"));
    WiDoClient.fastrprintln(F(""));    
    WiDoClient.print(data_start);
    Serial.print(F("."));
    wdt_reset();
    WiDoClient.print(data_temperature);
    Serial.print(F("."));
    WiDoClient.fastrprintln(F(""));
    Serial.println(F(" done."));
    
    // Reset watchdog
    wdt_reset();
    
    /* Get the http page info
    Serial.println(F("Reading answer..."));
    while (WiDoClient.connected()) {
      while (WiDoClient.available()) {
        char c = WiDoClient.read();
        Serial.print(c);
      }
    }
    */
  }

  if(TempMetro.check()){
    int reading = analogRead(0);
    temp = reading *0.0048828125*100;
    Serial.print(F("Real Time Temp: ")); 
    Serial.println(temp); 
  }
}


/**************************************************************************/
/*!
 @brief  Displays the driver mode (tiny of normal), and the buffer
 size if tiny mode is not being used
 
 @note   The buffer size and driver mode are defined in cc3000_common.h
 */
/**************************************************************************/
void displayDriverMode(void)
{
#ifdef CC3000_TINY_DRIVER
  Serial.println(F("CC3000 is configure in 'Tiny' mode"));
#else
  Serial.print(F("RX Buffer : "));
  Serial.print(CC3000_RX_BUFFER_SIZE);
  Serial.println(F(" bytes"));
  Serial.print(F("TX Buffer : "));
  Serial.print(CC3000_TX_BUFFER_SIZE);
  Serial.println(F(" bytes"));
#endif
}

/**************************************************************************/
/*!
 @brief  Tries to read the CC3000's internal firmware patch ID
 */
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;

#ifndef CC3000_TINY_DRIVER  
  if(!WiDo.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); 
    Serial.print(F(".")); 
    Serial.println(minor);
    version = major; 
    version <<= 8; 
    version |= minor;
  }
#endif
  return version;
}


/**************************************************************************/
/*!
 @brief  Tries to read the 6-byte MAC address of the CC3000 module
 */
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];

  if(!WiDo.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    WiDo.printHex((byte*)&macAddress, 6);
  }
}



