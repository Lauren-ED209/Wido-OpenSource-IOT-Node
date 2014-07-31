/*************************************************** 
  This is an example for the DFRobot WiDo - Wifi Integrated IoT lite sensor and control node

  Designed specifically to work with the DFRobot WiDo products:
  
  
  The main library is forked from Adafruit

  Written by Lauren
  BSD license, all text above must be included in any redistribution
  
 ****************************************************/
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

#define WiDo_IRQ   7
#define WiDo_VBAT  5
#define WiDo_CS    10

Adafruit_CC3000 WiDo = Adafruit_CC3000(WiDo_CS, WiDo_IRQ, WiDo_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "DFRobot WIFI"           // cannot be longer than 32 characters!
#define WLAN_PASS       "DFRobot2014"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

// To get the full feature of CC3000 library from Adafruit, please comment the define command below
#define CC3000_TINY_DRIVER    // saving the flash memory space for leonardo

void setup(){
  
  Serial.begin(115200);
  Serial.println(F("Hello, WiDo!\n")); 

  displayDriverMode();

  /* Initialise the module and test the hardware connection */
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!WiDo.begin())
  {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    while(1);
  }
  
  /* Check WG1300 firmware version
  uint16_t firmware = checkFirmwareVersion();
  if (firmware < 0x113) {
    Serial.println(F("Wrong firmware version!"));
    for(;;);
  } 
   */
  displayMACAddress();

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); 
  Serial.println(ssid);

  /* NOTE: Secure connections are not available in 'Tiny' mode!
   By default connectToAP will retry indefinitely, however you can pass an
   optional maximum number of retries (greater than zero) as the fourth parameter.
   */
  if (!WiDo.connectToAP(WLAN_SSID,WLAN_PASS,WLAN_SECURITY)) {
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


void loop(){
  
  static Adafruit_CC3000_Client IoTclient;
  
//    Update the time stamp
//    Serial.println("Unable to connect the Local Server");

    //Get DFRobot IoT Server IP    
    uint32_t ip = WiDo.IP2U32(182,254,130,180);
    IoTclient = WiDo.connectTCP(ip,8124);
    if(IoTclient.connected()){
      Serial.println("Connected");
      
      int sensorData = 100;
      
      String httpString = "";
      
      //Get TOKEN from the IoT server auto generated - My token: f42efb351afcdbbf4a8b9bc9172a108b
      httpString = httpString + "Get /data?token=" + "f42efb351afcdbbf4a8b9bc9172a108b" 
                   + "&param=" + String(sensorData) 
                     + " HTTP/1.1";
                     
      // attach the token to the IOT Server and Upload the sensor dataIoTclient
      IoTclient.println(httpString);
      // Send the Host IP and DFRobot community IoT service port
      // Data Upload service PORT:  8124
      // Real time controling service PORT: 9120
      IoTclient.fastrprintln(F("Host: 182.254.130.180:8124"));
      IoTclient.fastrprintln(F("Connection: close"));
      IoTclient.println();
      
      Serial.println("Upload data to the IoT Server");
      
      
      /* Read data until either the connection is closed, or the idle timeout is reached. */ 
      unsigned long lastRead = millis();
      while (IoTclient.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
        while (IoTclient.available()) {
          char c = IoTclient.read();
          Serial.print(c);
          lastRead = millis();
        }
      }
      
      IoTclient.close();

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
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
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


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!WiDo.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); WiDo.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); WiDo.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); WiDo.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); WiDo.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); WiDo.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint8_t valid, rssi, sec, index;
  char ssidname[33]; 

  index = WiDo.startSSIDscan();

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = WiDo.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  WiDo.stopSSIDscan();
}
