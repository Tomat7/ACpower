// Внимание! Проверь пароль WIFI_PASS WIFI_NAME

#ifndef USE_DHCP  // Настроить под свою сеть если не используется DHCP!
#define IP_NETWORK 192, 168, 1, 0
#define IP_ADDR_BASE IP_NETWORK + 30
#define ETHERNET_IP IP_ADDR_BASE + PLC_ID  // задаём IP адрес Ethernet модуля
#endif

#define PRINTF(a, ...) Serial.print(a); Serial.println(__VA_ARGS__)
#define PRINTLN(...) (Serial.println(__VA_ARGS__))
#define PRINT(...) (Serial.print(__VA_ARGS__))

void setup_Network()  
{
  init_WiFi();
  check_WiFi();
  if (!wifiErrors) { PRINTLN("+ WiFi connected!"); }
  else { PRINTLN("-- WiFi NOT FOUND! Will try to connect later."); }
  serialIP();
}

void init_WiFi()
{
#ifdef USE_DHCP
  PRINTF("+ WiFi will use DCHP");
#else
  if (WiFi.config(local_IP, gateway, subnet)) {
    PRINTF("+ WiFi set static ip-address OK");
  }
  else {
    PRINTF("-- WiFi set static IP-address ERROR");
  }
#endif

  PRINTF(".  WiFi connecting to ", WIFI_NAME);
  WiFi.mode(WIFI_STA);
  //WiFi.setSleep(false);
  WiFi.begin(WIFI_NAME, WIFI_PASS);
  PRINTF(".  MAC: ", WiFi.macAddress());

  static char macbuff[17] = { "                " };
  String macStr = WiFi.macAddress();
  macStr.toCharArray(macbuff, 18);

  uint16_t wifiAttempts = WIFI_TIMEOUT;

  while ((WiFi.status() != WL_CONNECTED) && (wifiAttempts > 0));
  {
    //check_WiFi();
    delay(100);
    wifiAttempts--;
    PRINT(".");
  }

  PRINTLN("");
}

void check_WiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiErrors = 0;
    PRINTLN("+ WiFi ok");
  }
  else
  {
    wifiErrors++;
    PRINTF("-- WiFi ERRORs: ", wifiErrors);
  }

  if (wifiErrors > WIFI_TIMEOUT)
  {
    PRINTLN("-- CAN'T CONNECT WiFi");
    PRINTLN("-- RESTART NOW.");
    delay(5000);
    ESP.restart();
  }
}

void serialIP()
{
  PRINTF(".  IP: ", WiFi.localIP());
  PRINTF(".  MASK: ", WiFi.subnetMask());
  PRINTF(".  GATEWAY: ", WiFi.gatewayIP());
}
