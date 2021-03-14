#include <WiFi.h>
#include <TextDisplay.h>
// Makes dealing with buttons a little bit more sane
#include <Button2.h>

#include "config.h"
#include <messages.h>

// The io port, the display, the graphic library, and our own text wrapper.
GxIO_Class io(SPI, /*CS=5*/ EINK_SS, /*DC=*/ EINK_DC, /*RST=*/ EINK_RESET);
GxEPD_Class e_paper(io, /*RST=*/ EINK_RESET, /*BUSY=*/ EINK_BUSY);
TextDisplay text_display(e_paper);
TextDisplay text_attribution(e_paper);

static IMAPConnectionData imap_connection_data = {
  host     : IMAP_HOST,
  port     : 993,
  email    : EMAIL,
  password : IMAP_PASSWORD,
  folder   : IMAP_FOLDER
};

IMAPFetcher fetcher(imap_connection_data);

// The button used.
Button2 button = Button2(USER_BUTTON);

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

std::string currentMessage;
std::string currentAuthor;

static void refreshScreen() {
  e_paper.fillScreen(GxEPD_WHITE);
  
  e_paper.setRotation(1);
  text_display.update(currentMessage);

  const Rect &position = text_display.position();
  e_paper.drawFastVLine(position.x - 3, position.y, position.height, GxEPD_BLACK);

  if (currentAuthor.length()) {
    e_paper.setRotation(0);
    text_attribution.update(currentAuthor);
  }

  e_paper.update();
}

void setup() {

	Serial.begin(115200);
  delay(200); // Give some time for the serial port to start.

  SPI.begin(EINK_SPI_CLK, EINK_SPI_MISO, EINK_SPI_MOSI, EINK_SS);
  e_paper.init();
    
  // No need to persist anything for WiFi, the network is hardcoded anyway.
  WiFi.persistent(false);
  WiFiEventId_t eventID = WiFi.onEvent(&eventHandler);  
  previousWiFiStatus = WiFi.status();
  
  // Turn on batery monitoring.
  // ttgo->power->adc1Enable(AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, true);
  
  static const int margin = 3;
  static const int attribution_height = 40;
  
  e_paper.setRotation(1);
  text_display.setDisplayPosition(
      Rect(attribution_height, margin,
           e_paper.width() - attribution_height - margin * 2, e_paper.height() -  margin * 2));
  
  e_paper.setRotation(0);
  text_attribution.setDisplayPosition(
      Rect(margin, margin, e_paper.width() - margin, attribution_height - margin * 2));
    
  currentMessage = "Loading";
  refreshScreen();
  //button.setPressedHandler(press);
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
          //WiFi.enableIpV6();
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

    // AXP20X_Class *power = ttgo->power;
    // if (power->isVBUSPlug()) {
    //   status = status + "\n   Power " + String(power->getVbusVoltage()) + " mV / " +
    //          String(power->getVbusCurrent()) + "mA";
    // }
    // if (power->isBatteryConnect()) {
    //   status = status + "\n   Battery " + String(power->getBattVoltage()) + " mV - ";
    //   if (power->isChargeing()) {
    //     status = status + "Charging " + String(power->getBattChargeCurrent()) + " mA (" +
    //       String(power->getBattPercentage()) + " %)";
    //   } else {
    //     status = status + "Discharge " + String(power->getBattDischargeCurrent()) + " mA (" +
    //       String(power->getBattPercentage()) + " %)";
    //   }
    // }
    //
    // status = status + "\n   Temp : " + String(power->getTemp()) + " C - " +
    //   String(ttgo->mpu->readTemperature()) + " C";
    Serial.println(status);
    previousStatus = currentStatus;
    previousWiFiStatus = currentWiFiStatus;
  }
  
  // If there are no messages, go fetch one.
  if (currentStatus == STATUS_NO_MESSAGE) {
    currentStatus = STATUS_NETWORK_STARTING;
    WiFi.mode(WIFI_STA);
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
    bool result = fetcher.getFirstUnreadMessage(
        [] (const std::string &message, const std::string &author, MessageError error) -> void {
          if (error == MESSAGES_OK) {
            currentMessage = message;
            currentAuthor = author;
            Serial.print("Message is: ");
            Serial.println(message.c_str());
            refreshScreen();
            currentStatus = STATUS_ACTION_DONE;
          } else {
            Serial.print(error);
            Serial.print(" ");
            Serial.println(message.c_str());
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

