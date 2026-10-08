#include <Arduino.h>
// #include <M5Stack.h>
#include <WiFi.h>
#include "time.h"
#include <WiFiUdp.h>
#include <HTTPClient.h>

// Debug flag - set to false for production
#define DEBUG_ENABLED false

// put function declarations here:
void printLocalTime(bool send);
void setWifi();
String formatGPRMC(struct tm *timeinfo);
bool testNetworkConnectivity();
bool testDNSResolution(const char *hostname);
bool testHTTPConnection(const char *url);
bool testNTPConnection(const char *ntpServer);
void displayNetworkInfo();
const int ledPin = 32; // LED pin number
#ifndef WIFI_SSID
#error "WIFI_SSID must be defined. Set environment variable WIFI_SSID before building."
#endif

#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD must be defined. Set environment variable WIFI_PASSWORD before building."
#endif

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// const char *ntpServer = "time.google.com";
const char *ntpServer = "192.168.2.50";
const char *ntpServer2 = "pool.ntp.org";
const char *ntpServer3 = "time.nist.gov";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
static unsigned long lastWifiCall = 0;
static unsigned long wifiCount = 0;
static bool timeInitialized = false;

void setup()
{
  Serial2.begin(9600, SERIAL_8N1, 0, 26);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);

  setWifi();
  digitalWrite(ledPin, HIGH);
}

void displayNetworkInfo()
{
  if (!DEBUG_ENABLED)
    return;

  Serial.println("=== Network Information ===");
  Serial.print("WiFi Network: ");
  Serial.println(ssid);
  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("DNS 1: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("DNS 2: ");
    Serial.println(WiFi.dnsIP(1));
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Channel: ");
    Serial.println(WiFi.channel());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
  }
  Serial.println("===========================");
}

bool testHTTPConnection(const char *url)
{
  if (!DEBUG_ENABLED)
    return true; // Skip test in production

  Serial.print("Testing HTTP connection to: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(5000);

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
    http.end();
    return httpResponseCode == 200;
  }
  else
  {
    Serial.print("HTTP Error: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

bool testDNSResolution(const char *hostname)
{
  if (!DEBUG_ENABLED)
    return true; // Skip test in production

  Serial.print("Testing DNS resolution for: ");
  Serial.println(hostname);

  IPAddress resolvedIP;
  if (WiFi.hostByName(hostname, resolvedIP))
  {
    Serial.print("Resolved to IP: ");
    Serial.println(resolvedIP);
    return true;
  }
  else
  {
    Serial.println("DNS resolution FAILED!");
    return false;
  }
}

bool testNTPConnection(const char *ntpServer)
{
  if (!DEBUG_ENABLED)
    return true; // Skip test in production

  Serial.print("Testing NTP connection to: ");
  Serial.println(ntpServer);

  // First test DNS resolution
  if (!testDNSResolution(ntpServer))
  {
    Serial.println("NTP server DNS resolution failed");
    return false;
  }

  WiFiUDP udp;
  udp.begin(8888);

  IPAddress serverIP;
  if (!WiFi.hostByName(ntpServer, serverIP))
  {
    Serial.println("Failed to resolve NTP server IP");
    udp.stop();
    return false;
  }

  // Send NTP request packet
  byte packetBuffer[48];
  memset(packetBuffer, 0, 48);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision

  Serial.print("Sending NTP packet to: ");
  Serial.print(serverIP);
  Serial.println(":123");

  udp.beginPacket(serverIP, 123);
  udp.write(packetBuffer, 48);
  udp.endPacket();

  // Wait for response
  unsigned long startTime = millis();
  while (millis() - startTime < 5000)
  {
    if (udp.parsePacket())
    {
      Serial.println("Received NTP response!");
      udp.stop();
      return true;
    }
    delay(10);
  }

  Serial.println("No NTP response received (timeout)");
  udp.stop();
  return false;
}

bool testNetworkConnectivity()
{
  if (!DEBUG_ENABLED)
    return true; // Skip comprehensive tests in production

  Serial.println("\n=== COMPREHENSIVE NETWORK TEST ===");

  displayNetworkInfo();

  bool allTestsPassed = true;

  // Test 1: Gateway connectivity
  Serial.println("\n--- Test 1: Gateway Connectivity ---");
  IPAddress gateway = WiFi.gatewayIP();
  Serial.print("Pinging gateway: ");
  Serial.println(gateway);

  // Simple connectivity test using HTTP to gateway (if it responds)
  bool gatewayReachable = true; // Assume reachable for now, as ping isn't available
  Serial.println("Gateway assumed reachable (ping not available on ESP32)");

  // Test 2: DNS Resolution
  Serial.println("\n--- Test 2: DNS Resolution ---");
  bool dnsGoogle = testDNSResolution("google.com");
  bool dnsNTP1 = testDNSResolution(ntpServer);
  bool dnsNTP2 = testDNSResolution(ntpServer2);
  bool dnsNTP3 = testDNSResolution(ntpServer3);

  if (!dnsGoogle || !dnsNTP1 || !dnsNTP2 || !dnsNTP3)
  {
    allTestsPassed = false;
  }

  // Test 3: HTTP Connectivity
  Serial.println("\n--- Test 3: HTTP Connectivity ---");
  bool httpGoogle = testHTTPConnection("http://google.com");
  bool httpCloudflare = testHTTPConnection("http://1.1.1.1");

  if (!httpGoogle || !httpCloudflare)
  {
    allTestsPassed = false;
  }

  // Test 4: NTP Server Connectivity
  Serial.println("\n--- Test 4: NTP Server Connectivity ---");
  bool ntpTest1 = testNTPConnection(ntpServer);
  bool ntpTest2 = testNTPConnection(ntpServer2);
  bool ntpTest3 = testNTPConnection(ntpServer3);

  if (!ntpTest1 && !ntpTest2 && !ntpTest3)
  {
    allTestsPassed = false;
  }

  Serial.println("\n=== NETWORK TEST SUMMARY ===");
  Serial.print("Gateway Reachable: ");
  Serial.println(gatewayReachable ? "PASS" : "FAIL");
  Serial.print("DNS google.com: ");
  Serial.println(dnsGoogle ? "PASS" : "FAIL");
  Serial.print("DNS NTP servers: ");
  Serial.println((dnsNTP1 && dnsNTP2 && dnsNTP3) ? "PASS" : "PARTIAL/FAIL");
  Serial.print("HTTP Connectivity: ");
  Serial.println((httpGoogle && httpCloudflare) ? "PASS" : "FAIL");
  Serial.print("NTP Connectivity: ");
  Serial.println((ntpTest1 || ntpTest2 || ntpTest3) ? "PASS" : "FAIL");
  Serial.print("Overall Result: ");
  Serial.println(allTestsPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
  Serial.println("==============================\n");

  return allTestsPassed;
}

bool syncTimeWithRetry(const char *server, int maxRetries = 3)
{
  if (DEBUG_ENABLED)
  {
    Serial.print("Attempting NTP sync with server: ");
    Serial.println(server);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, server);

  for (int i = 0; i < maxRetries; i++)
  {
    if (DEBUG_ENABLED)
    {
      Serial.print("NTP sync attempt ");
      Serial.print(i + 1);
      Serial.print(" of ");
      Serial.println(maxRetries);
    }

    delay(2000); // Wait 2 seconds for NTP sync

    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      Serial.println("NTP sync successful!");
      if (DEBUG_ENABLED)
      {
        Serial.print("Current time: ");
        Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      }
      return true;
    }

    if (DEBUG_ENABLED)
    {
      Serial.println("NTP sync failed, retrying...");
    }
    delay(1000);
  }

  Serial.print("Failed to sync with server: ");
  Serial.println(server);
  return false;
}

void setWifi()
{
  wifiCount++;
  if (DEBUG_ENABLED)
  {
    Serial.println("=== Starting WiFi Connection ===");
  }

  WiFi.begin(ssid, password);
  if (DEBUG_ENABLED)
  {
    Serial.print("Connecting to WiFi");
  }

  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20)
  {
    delay(500);
    if (DEBUG_ENABLED)
    {
      Serial.print(".");
    }
    wifiTimeout++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nWiFi connection failed!");
    Serial.print("WiFi Network: ");
    Serial.println(ssid);
    return;
  }

  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());

  if (DEBUG_ENABLED)
  {
    Serial.print("WiFi signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    Serial.println("=== Running Network Diagnostics ===");
    bool networkOK = testNetworkConnectivity();

    if (!networkOK)
    {
      Serial.println("WARNING: Network connectivity issues detected!");
      Serial.println("Proceeding with NTP sync anyway...");
    }

    Serial.println("=== Starting NTP Time Sync ===");
  }

  // Try multiple NTP servers with retry logic
  bool timeSync = false;

  timeSync = syncTimeWithRetry(ntpServer);
  if (!timeSync)
  {
    if (DEBUG_ENABLED)
      Serial.println("Trying backup NTP server 1...");
    timeSync = syncTimeWithRetry(ntpServer2);
  }
  if (!timeSync)
  {
    if (DEBUG_ENABLED)
      Serial.println("Trying backup NTP server 2...");
    timeSync = syncTimeWithRetry(ntpServer3);
  }

  if (timeSync)
  {
    timeInitialized = true;
    if (DEBUG_ENABLED)
    {
      Serial.println("=== Time Sync Complete ===");
      printLocalTime(false);
    }
  }
  else
  {
    Serial.println("All NTP servers failed");
    timeInitialized = false;
  }

  if (DEBUG_ENABLED)
  {
    Serial.println("Keeping WiFi connected for 3 more seconds to ensure sync...");
  }
  delay(3000);

  if (DEBUG_ENABLED)
  {
    Serial.println("Disconnecting WiFi...");
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop()
{
  struct timeval tv;
  struct tm *tm_info;

  if (DEBUG_ENABLED)
  {
    Serial.println("=== Loop Start ===");
  }

  if (gettimeofday(&tv, NULL) == 0)
  {
    if (DEBUG_ENABLED)
    {
      Serial.print("gettimeofday() successful - Unix timestamp: ");
      Serial.println(tv.tv_sec);
    }

    time_t t = tv.tv_sec;
    tm_info = localtime(&t);

    if (tm_info != NULL)
    {
      if (DEBUG_ENABLED)
      {
        Serial.printf("System time: %04d-%02d-%02d %02d:%02d:%02d\n",
                      tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                      tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
      }

      // Check if it's time to resync (8 AM and at least 1 hour has passed)
      if ((tm_info->tm_hour == 8) && tm_info->tm_min == 0 && (tv.tv_sec - lastWifiCall) >= 3600)
      {
        if (DEBUG_ENABLED)
        {
          Serial.println("Time to resync! Calling setWifi()...");
        }
        setWifi();
        lastWifiCall = tv.tv_sec;
      }

      if (DEBUG_ENABLED)
      {
        Serial.print("WiFi Count: ");
        Serial.print(wifiCount);
        Serial.print(" - Hour: ");
        Serial.print(tm_info->tm_hour);
        Serial.print(" - Minute: ");
        Serial.print(tm_info->tm_min);
        Serial.print(" - Unix Time: ");
        Serial.print(tv.tv_sec);
        Serial.print(" - Last WiFi Call: ");
        Serial.print(lastWifiCall);
        Serial.print(" - Time Since Last Call: ");
        Serial.print((tv.tv_sec - lastWifiCall));
        Serial.println(" seconds");
      }

      delay(((1000000.0 - tv.tv_usec) / 1000000.0) * 1000);
    }
    else
    {
      if (DEBUG_ENABLED)
      {
        Serial.println("ERROR: localtime() returned NULL!");
      }
      delay(900);
    }
  }
  else
  {
    if (DEBUG_ENABLED)
    {
      Serial.println("ERROR: gettimeofday() failed!");
    }
    delay(900);
  }

  if (DEBUG_ENABLED)
  {
    Serial.print("Time initialized status: ");
    Serial.println(timeInitialized ? "YES" : "NO");
  }

  printLocalTime(true);

  digitalWrite(ledPin, LOW);
  delay(100);
  digitalWrite(ledPin, HIGH);

  if (DEBUG_ENABLED)
  {
    Serial.println("=== Loop End ===\n");
  }
}

void printLocalTime(bool send)
{
  struct tm timeinfo;

  if (DEBUG_ENABLED)
  {
    Serial.print("Attempting to get local time... ");
  }

  if (!getLocalTime(&timeinfo))
  {
    if (DEBUG_ENABLED)
    {
      Serial.println("FAILED!");
      Serial.print("Time initialized status: ");
      Serial.println(timeInitialized ? "YES" : "NO");
    }

    // Try to get system time as fallback
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0 && tv.tv_sec > 1000000000) // Basic sanity check
    {
      if (DEBUG_ENABLED)
      {
        Serial.println("Using system time as fallback...");
      }
      time_t t = tv.tv_sec;
      struct tm *tm_ptr = localtime(&t);
      if (tm_ptr != NULL)
      {
        timeinfo = *tm_ptr;
        if (DEBUG_ENABLED)
        {
          Serial.println("Fallback time obtained successfully!");
        }
      }
      else
      {
        if (DEBUG_ENABLED)
        {
          Serial.println("Failed to get fallback time - using default time");
        }
        // Set a default time (e.g., Jan 1, 2024, 00:00:00)
        timeinfo.tm_year = 2024 - 1900;
        timeinfo.tm_mon = 0;
        timeinfo.tm_mday = 1;
        timeinfo.tm_hour = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
      }
    }
    else
    {
      if (DEBUG_ENABLED)
      {
        Serial.println("No valid time source available - using default time");
      }
      // Set a default time
      timeinfo.tm_year = 2024 - 1900;
      timeinfo.tm_mon = 0;
      timeinfo.tm_mday = 1;
      timeinfo.tm_hour = 0;
      timeinfo.tm_min = 0;
      timeinfo.tm_sec = 0;
    }
  }
  else
  {
    if (DEBUG_ENABLED)
    {
      Serial.println("SUCCESS!");
      Serial.printf("Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
  }

  char buffer[100];
  strftime(buffer, sizeof(buffer), "GPRMC,%H%M%S.000,A,5321.6802,N,00630.3372,W,0.02,31.66,%d%m%y,,A", &timeinfo);

  String gprmc = String(buffer);
  unsigned int checksum = 0;
  for (unsigned int i = 0; i < gprmc.length(); i++)
  {
    checksum ^= gprmc[i];
  }

  char finalStr[120];
  snprintf(finalStr, sizeof(finalStr), "$%s*%02X\n", gprmc.c_str(), checksum);

  Serial.print(finalStr);
  if (send)
    Serial2.print(finalStr);
}
