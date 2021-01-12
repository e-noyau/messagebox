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

// The io port, the display, the graphic library, and our own text wrapper.
GxIO_Class io(SPI, /*CS=5*/ EINK_SS, /*DC=*/ EINK_DC, /*RST=*/ EINK_RESET);
GxEPD_Class e_paper(io, /*RST=*/ EINK_RESET, /*BUSY=*/ EINK_BUSY);
TextDisplay text_display(e_paper);
TextDisplay text_attribution(e_paper);

#define MARGIN 3

// Main Display
static void setupScreen() {
  bool is_large = max(e_paper.height(), e_paper.width()) > 200;
  if (!is_large) {
    text_display.setDisplayPosition(
        Rect(MARGIN, MARGIN,
             e_paper.width() - MARGIN * 2, e_paper.height() -  MARGIN * 2));
  } else {
    int attribution_height = 40;
    
    e_paper.setRotation(1);
    text_display.setDisplayPosition(
        Rect(attribution_height, MARGIN,
             e_paper.width() - attribution_height - MARGIN * 2, e_paper.height() -  MARGIN * 2));
             
    e_paper.setRotation(0);
    text_attribution.setDisplayPosition(
       Rect(MARGIN, MARGIN, e_paper.width() - MARGIN, attribution_height - MARGIN * 2));
  }  
}

static void updateScreen(std::string currentMessage, std::string currentAuthor) {  
  bool is_large = max(e_paper.height(), e_paper.width()) > 200;
  e_paper.fillScreen(GxEPD_WHITE);
  
  if (!is_large) {
    text_display.update(currentMessage + " --" + currentAuthor);
  } else {    
    const Rect &position = text_display.position();
    e_paper.setRotation(1);
    text_display.update(currentMessage);

    e_paper.drawFastVLine(position.x - 3, position.y, position.height, GxEPD_BLACK);

    if (currentAuthor.length()) {
      e_paper.setRotation(0);
      text_attribution.update(currentAuthor);
    }
  }
  e_paper.update();
}

//***************************
// The IoT subsystem
//***************************

// The network block of the IoT.
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(HOSTNAME, &dnsServer, &server, INITIAL_PASSWORD, "1.0");

// Handle web requests to "/" path
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>";
  s += HOSTNAME;
  s += "</title></head><body>Hello world!";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

// *************************
// The IMAP fetcher
// *************************

// All the data to retrieve the emails.
static IMAPConnectionData imap_connection_data = {
  host     : IMAP_HOST,
  port     : 993,
  email    : EMAIL,
  password : IMAP_PASSWORD,
  folder   : IMAP_FOLDER
};

IMAPFetcher fetcher(imap_connection_data);

// *************************
// Setup and all
// *************************
void setup() 
{
  Serial.begin(115200);

  SPI.begin(EINK_SPI_CLK, EINK_SPI_MISO, EINK_SPI_MOSI, EINK_SS);
  e_paper.init();

  setupScreen();

  // -- Initializing the configuration.
  iotWebConf.setConfigPin(USER_BUTTON);
#if defined(USER_LED)
  iotWebConf.setStatusPin(USER_LED);
#endif  // defined(USER_LED)
  
  iotWebConf.skipApStartup();
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
}


std::string currentState = "";
std::string previousState = "";
bool online = false;

void loop() {
  bool currentlyOnline = false;
  
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
  currentState = content.str();
  if (currentState != previousState) {
    updateScreen(currentState, title);
    
    Serial.println(currentState.c_str());
    previousState = currentState;
  }
  
  if (currentlyOnline != online) {
    online = currentlyOnline;
    
    if (online) {  
      bool result = fetcher.getFirstUnreadMessage(
          [] (const std::string &message, const std::string &author, MessageError error) -> void {
            if (error == MESSAGES_OK) {
              Serial.print("Message is: ");
              Serial.println(message.c_str());
              updateScreen(message, author);
            } else {
              Serial.print(error);
              Serial.print(" ");
              updateScreen("Reboot, something failed :" + error, "OOOOPS!");
            }         
          });
    }
  }
}