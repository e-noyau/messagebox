#include <string>
#include <sstream>

#include <IotWebConf.h>
#include <TextDisplay.h>
#include <messages.h>

// Where the various password and configurations are hidden.
#include "config.h"

//***************************
// The display subsystem
//***************************

#define MARGIN 3

class Screen {
public:
  Screen(): io_(SPI, /*CS=5*/ EINK_SS, /*DC=*/ EINK_DC, /*RST=*/ EINK_RESET),
            e_paper_(io_, /*RST=*/ EINK_RESET, /*BUSY=*/ EINK_BUSY),
            text_display_(e_paper_), text_attribution_(e_paper_) {};
  void init();
  void updateScreen(std::string currentMessage, std::string currentAuthor);
private:
  // The io port, the display, the graphic library, and our own text wrapper.
  GxIO_Class io_;
  GxEPD_Class e_paper_;
  TextDisplay text_display_;
  TextDisplay text_attribution_;
};

void Screen::init() {
  SPI.begin(EINK_SPI_CLK, EINK_SPI_MISO, EINK_SPI_MOSI, EINK_SS);
  e_paper_.init();

  bool is_large = max(e_paper_.height(), e_paper_.width()) > 200;
  if (!is_large) {
    text_display_.setDisplayPosition(
        Rect(MARGIN, MARGIN,
             e_paper_.width() - MARGIN * 2, e_paper_.height() -  MARGIN * 2));
  } else {
    int attribution_height = 40;

    e_paper_.setRotation(1);
    text_display_.setDisplayPosition(
        Rect(attribution_height,
             MARGIN,
             e_paper_.width() - attribution_height - MARGIN * 2,
             e_paper_.height() -  MARGIN * 2));
 
    e_paper_.setRotation(0);
    text_attribution_.setDisplayPosition(
       Rect(MARGIN, MARGIN,
            e_paper_.width() - MARGIN, attribution_height - MARGIN * 2));
  }  
};

void Screen::updateScreen(std::string currentMessage,
                          std::string currentAuthor) {  
  bool is_large = max(e_paper_.height(), e_paper_.width()) > 200;
  e_paper_.fillScreen(GxEPD_WHITE);
  
  if (!is_large) {
    text_display_.update(currentMessage + " --" + currentAuthor);
  } else {    
    const Rect &position = text_display_.position();
    e_paper_.setRotation(1);
    text_display_.update(currentMessage);

    e_paper_.drawFastVLine(position.x - 3, position.y, position.height,
                           GxEPD_BLACK);

    if (currentAuthor.length()) {
      e_paper_.setRotation(0);
      text_attribution_.update(currentAuthor);
    }
  }
  e_paper_.update();
}

//***************************
// The IoT subsystem. The device will try to connect to a known WiFi. If there
// is no know WiFi credentials, or if this particular Wifi is not found, the
// device will create its own AP, with a config page to set it up.
//***************************

class IoT {
public:
  IoT(): server_(80), iotWebConf_(HOSTNAME, &dnsServer_, &server_, INITIAL_PASSWORD, "1.0") {};
  void init();
  void handleRoot();
private:
  // The network block of the IoT.
  DNSServer dnsServer_;
  WebServer server_;
public:
  IotWebConf iotWebConf_;
};

void IoT::init() {
  // -- Initializing the configuration.
  iotWebConf_.setConfigPin(USER_BUTTON);
#if defined(USER_LED)
  iotWebConf_.setStatusPin(USER_LED);
#endif  // defined(USER_LED)

  iotWebConf_.skipApStartup();
  iotWebConf_.init();

  // -- Set up required URL handlers on the web server.
  server_.on("/",       [this]{ handleRoot(); });
  server_.on("/config", [this]{ iotWebConf_.handleConfig(); });
  server_.onNotFound(   [this]{ iotWebConf_.handleNotFound(); });
};

// Handle web requests to "/" path
void IoT::handleRoot() {
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf_.handleCaptivePortal()) {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<html><head><title>";
  s += HOSTNAME;
  s += "</title></head><body>Go to <a href='config'>configure page</a> to change settings.</body></html>";
  server_.send(200, "text/html", s);
}


IoT iot;
Screen screen;

// *************************
// The IMAP fetcher
// *************************

IMAPFetcher *fetcher;

// *************************
// Setup and all
// *************************
void setup() 
{
  Serial.begin(115200);
  delay(200); // Give some time for the serial port to start.

  Serial.println("BOOT");
  screen.init();
  iot.init();
}


static std::string previousState = "";
static bool online = false;

void loop() {
  bool currentlyOnline = false;
  IotWebConf &iotWebConf = iot.iotWebConf_;
  
  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();
  
  std::string title;
  std::ostringstream content;
  auto state = iotWebConf.getState();
  switch (state) {
    case IOTWEBCONF_STATE_BOOT:
      title = "BOOT";
      break;
      
    case IOTWEBCONF_STATE_NOT_CONFIGURED:
      title = "NOT CONFIGURED";
      content << "Connect to WiFi network " << iotWebConf.getThingName() 
              << " and open http://" << WiFi.softAPIP().toString().c_str()
              << ". Username is 'admin'. password is: '" << INITIAL_PASSWORD << "'.";
      break;
      
    case IOTWEBCONF_STATE_AP_MODE:
      title = "AP MODE";
      content << "Connect to WiFi network " << iotWebConf.getThingName()
              << " with your configured password and open http://" 
              << WiFi.softAPIP().toString().c_str()
              << ". If password is unknown, restart the device while pressing the button.";
      break;
    case IOTWEBCONF_STATE_CONNECTING:
      title = "CONNECTING";
    
      content << "Attempting connection to the network named '" 
              << iotWebConf.getWifiAuthInfo().ssid << "'.";
      break;
    case IOTWEBCONF_STATE_ONLINE:
      currentlyOnline = true;
      title = "ONLINE";
      content << "Connected to network '" << iotWebConf.getWifiAuthInfo().ssid << "' with IP"
              << WiFi.localIP().toString().c_str() << ". "
              << "Connect to the device via http://" << iotWebConf.getThingName() << ".local  "
              << "Fetching data…";
              // Despite taking a function as a parameter, this is mostly synchronous.
      break;
    default:
      content << "UNEXPECTED STATE.";
      break;
  }
  std::string currentState = content.str();
  if (currentState != previousState) {
    screen.updateScreen(currentState, title);
    previousState = currentState;
  }
  
  if (currentlyOnline != online) {
    online = currentlyOnline;
    
    if (online) {
      IMAPConnectionData imap_connection_data = {
        host     : IMAP_HOST,
        port     : 993,
        email    : EMAIL,
        password : IMAP_PASSWORD,
        folder   : IMAP_FOLDER
      };
      fetcher = new IMAPFetcher(imap_connection_data);
      
      bool result = fetcher->getFirstUnreadMessage(
          [] (const std::string &message, const std::string &author, MessageError error) -> void {
            switch(error) {
              case MESSAGES_OK:
                screen.updateScreen(message, author);
                break;
              case MESSAGES_NO_MESSAGE_FOUND:
                screen.updateScreen("No more messages! Ask your loved ones for more.", "Add Love");
                break;
            	case MESSAGES_NO_NETWORK:
                screen.updateScreen(message, "No Network");
                break;
            	case MESSAGES_IMAP_CONNECTION_FAILED:
                screen.updateScreen(message, "IMAP Connection");
                break;
            	case MESSAGES_FOLDER_NOT_FOUND:
                screen.updateScreen(message, "Invalid Folder");
                break;
            	case MESSAGES_FLAG_SET_FAIL:
                screen.updateScreen(message, "Flag");
                break;
              case MESSAGES_CONNECTION_IN_PROGRESS:
                screen.updateScreen(message, "Reentry");
                break;
              default:
                screen.updateScreen("Reboot, something failed", "OOOOPS!");
                break;
            }
          });
    }
  }
}