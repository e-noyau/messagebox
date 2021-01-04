#include <WiFi.h>

#include "messages.h"
#include "config.h"

#define ONBOARD_LED  2

typedef enum {
  STATUS_NO_MESSAGE,
	STATUS_IDLE,
  STATUS_NETWORK_STARTING,
  STATUS_NETWORK_CONNECTING,
  STATUS_NETWORK_STARTED,
	STATUS_ACTION_IN_PROGRESS,
  STATUS_ACTION_DONE,
  STATUS_ACTION_FAILED,
  STATUS_NETWORK_STOPPING,
} Status;

Status currentStatus = STATUS_NO_MESSAGE;
Status previousStatus = STATUS_NO_MESSAGE;
int previousWiFiStatus;
int ledState;

String currentMessage;

unsigned long previousMillis = 0; // last time LED was updated

void setup() {
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);
  ledState = LOW;
  
	Serial.begin(115200);
  WiFi.persistent(false);
  
  currentMessage = String("");
  setupMessages();
  
  WiFiEventId_t eventID = WiFi.onEvent(&eventHandler);
  
  previousWiFiStatus = WiFi.status();
}


void eventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch(event) {
      case WiFiEvent_t::SYSTEM_EVENT_STA_START:
          Serial.println("STA Started");
          WiFi.setHostname("MessageBox");
          currentStatus = STATUS_NETWORK_CONNECTING;
          break;

      case WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED:
          Serial.println("STA Connected");
          WiFi.enableIpV6();
          break;

      case WiFiEvent_t::SYSTEM_EVENT_AP_STA_GOT_IP6:
      case WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP:
          if ((currentStatus == STATUS_NETWORK_STARTING) || (currentStatus == STATUS_NETWORK_CONNECTING)) {
            currentStatus = STATUS_NETWORK_STARTED;
          }
          break;

      case WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED:
          Serial.println("STA Disconnected");
          break;

      case WiFiEvent_t::SYSTEM_EVENT_STA_STOP:
          Serial.println("STA Stopped");
          currentStatus = STATUS_IDLE;
          break;
      
      case WiFiEvent_t::SYSTEM_EVENT_WIFI_READY:
          break;

      default:
        Serial.println("UNEXPECTED SYSTEM_EVENT " + String(event));
        
        /*
            0  SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
            1  SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
            2  SYSTEM_EVENT_STA_START                < ESP32 station start
            3  SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
            4  SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
            5  SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from AP
            6  SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
            7  SYSTEM_EVENT_STA_GOT_IP               < ESP32 station got IP from connected AP
            8  SYSTEM_EVENT_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
            9  SYSTEM_EVENT_STA_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
            10 SYSTEM_EVENT_STA_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
            11 SYSTEM_EVENT_STA_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
            12 SYSTEM_EVENT_STA_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
            13 SYSTEM_EVENT_AP_START                 < ESP32 soft-AP start
            14 SYSTEM_EVENT_AP_STOP                  < ESP32 soft-AP stop
            15 SYSTEM_EVENT_AP_STACONNECTED          < a station connected to ESP32 soft-AP
            16 SYSTEM_EVENT_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
            17 SYSTEM_EVENT_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
            18 SYSTEM_EVENT_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
            19 SYSTEM_EVENT_GOT_IP6                  < ESP32 station or ap or ethernet interface v6IP addr is preferred
            20 SYSTEM_EVENT_ETH_START                < ESP32 ethernet start
            21 SYSTEM_EVENT_ETH_STOP                 < ESP32 ethernet stop
            22 SYSTEM_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
            23 SYSTEM_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
            24 SYSTEM_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
            25 SYSTEM_EVENT_MAX
        */
  }
  
}


void loop() {
  // Debug log.
  int currentWiFiStatus = WiFi.status();
  if ((previousStatus != currentStatus) || (currentWiFiStatus != previousWiFiStatus)) {
    String status = "Status changed from: " + String(previousStatus) + 
                    " to: " + String(currentStatus) + " WiFi: " + String(currentWiFiStatus);
    Serial.println(status);
    previousStatus = currentStatus;
    previousWiFiStatus = currentWiFiStatus;
  }
  
  
  // LED status
  // Led is off if radio is turned off, blinking while connecting/disconnecting, solid while doing a network action
  {
    int previousLEDState = ledState;
    switch(currentStatus) {
      case STATUS_NO_MESSAGE:
    	case STATUS_IDLE:
        ledState = LOW;
        break;
      case STATUS_NETWORK_STARTING:
      case STATUS_NETWORK_CONNECTING:
      case STATUS_NETWORK_STOPPING:
      case STATUS_ACTION_DONE:
      case STATUS_ACTION_FAILED:
        ledState = HIGH; 
        break;
      case STATUS_NETWORK_STARTED:
    	case STATUS_ACTION_IN_PROGRESS:
        {
          // blink
          unsigned long currentMillis = millis();

          if (currentMillis - previousMillis >= 100) {
            previousMillis = currentMillis;
            // if the LED is off turn it on and vice-versa:
            if (ledState == LOW) {
              ledState = HIGH;
            } else {
              ledState = LOW;
            }
          }
        }
        break;
    }
    // set the LED with the ledState of the variable:
    if (previousLEDState != ledState) {
      digitalWrite(ONBOARD_LED, ledState);
    }
  }
  
  // If there are no messages, go fetch one.
  if (currentStatus == STATUS_NO_MESSAGE) {
    currentStatus = STATUS_NETWORK_STARTING;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
  
  if (currentStatus == STATUS_IDLE) {
    // [TODO] If a button is pressed, go mark the current message read, and move on to the next one.
    delay(200);
    return;
  }

  // Once the network is started, do the necessary action.
  if (currentStatus == STATUS_NETWORK_STARTED) {
    currentStatus = STATUS_ACTION_IN_PROGRESS;
    // Despite taking a function as a parameter, this is mostly synchronous.
    bool result = getFirstUnreadMessage([] (String message, MessageError error) -> void {
      if (error == MESSAGES_OK) {
        currentMessage = message;
        Serial.print("Message is: ");
        Serial.println(message);
        currentStatus = STATUS_ACTION_DONE;
      } else {
        Serial.print(error);
        Serial.print(" ");
        Serial.println(message);
        currentStatus = STATUS_ACTION_FAILED;
      }
    });
    return;
  }

  // [TODO] Add a timeout for the action ?
  if ((currentStatus == STATUS_ACTION_DONE) || (currentStatus == STATUS_ACTION_FAILED)) {
    currentStatus = STATUS_NETWORK_STOPPING;
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    return;
  }

  delay(10);
}

