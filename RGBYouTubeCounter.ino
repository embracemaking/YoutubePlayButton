// ----------------------------
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h> // Library used for parsing Json from the API responses
#include <MD_MAX72xx.h>  // Library used for interfacing with the 8x8x4 led matrix
#include <MD_Parola.h>   // Library used for displaying info on the matrix
#include <NTPClient.h>   // Library to get the current time
#include <WiFiUdp.h>     // Library to get the time from a udp server

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

//LED SHIELD DEFINITION
#define PIN   D4
#define LED_NUM 7

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_NUM, PIN, NEO_GRB + NEO_KHZ800);

// LED MATRIX DISPLAY DEFINITION
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES  4
#define CLK_PIN   D6  // or SCK
#define DATA_PIN  D8  // or MOSI
#define CS_PIN    D7  // or SS

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
//

const int timeDelay = 3000; // Delay between Time / Day / Subs / Views
const int fadeSpeed = 40;   // Speed of the fade
int i = 0; // looping through the display init at 0
int statusCode;

const int fadeDuration = 3000/2;

const float numIterations = 15.0;
const int clockedIterations = 50;
const float iterationOffset = (fadeDuration/clockedIterations) * 1.0;

String st;
String content;


enum class State : uint8_t {
  fetch,
  noop,
  wait,
  fadeIn,
  fadeOut
};

enum class Values : uint8_t {
  curTime = 0,
  weekDay = 1,
//  yt_title = 2,
  yt_channel = 2,
  yt_subscribers = 3,
  yt_views = 4,
//   tw_title = 6,
//   tw_account = 7,
//   tw_followers = 8,
//   tw_friends = 9,
//   tw_tweets = 10,
  count = 5 //11
};

State nextState;
State state;

uint32_t prevTime = millis();
uint32_t currentTime = millis();
uint32_t waitDelay;

uint32_t waitDelay_fetch = 5 * 60 * 1000;
//uint32_t waitDelay_fetch = 20 * 1000;

uint32_t prevTime_fetch = millis() + waitDelay_fetch * 2;

String feed[(uint8_t)Values::count] = {};

//YouTube
String youtube_views = "";
String youtube_channel = "";
String youtube_subscribers = "";
String youtube_videos = "";



//Time Variables
WiFiUDP ntpUDP; // Define NTP Client to get time
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; //Day Names
char *AMPM = "AM"; // AM / PM // Variable

//-----------------------------
// For HTTPS requests - keep this
WiFiClientSecure client;

// Just the base of the URL you want to connect to
#define SOCIAL_GENIUS_HOST "api.socialgenius.io" //leave this alone


/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "WiFi SSID";
char password[32] = "WiFi Password";
char project[32] = "Copy From SocialGenius.io";


// Web server
ESP8266WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);


/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
unsigned long lastConnectTry = 0;

/** Current WLAN status */
unsigned int status = WL_IDLE_STATUS;

/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

void setup() {
  delay(1000);
  Serial.begin(115200);

  leds.begin(); // This initializes the NeoPixel library

  //initialize the LED MAtrix 4, 8x8
  myDisplay.begin();
  myDisplay.setIntensity(10);
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.setPause(2000);
  myDisplay.setSpeed(40);
  myDisplay.displayClear();
  myDisplay.print("Loading...");

  //Get the local time
  timeClient.begin(); // Initialize NTPClient --> get time
  timeClient.setTimeOffset(-14400);  // Set offset time in seconds to adjust for your timezone: 3600 sec per Hour
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
}

void led_set(uint8 R, uint8 G, uint8 B) {
  for (int i = 0; i < LED_NUM; i++) {
    leds.setPixelColor(i, leds.Color(R, G, B));
    leds.show();
    delay(50);
  }
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int connRes = WiFi.waitForConnectResult();
  client.setInsecure();
  Serial.print("connRes: ");
  Serial.println(connRes);
}

void makeHTTPRequest() {
  Serial.println(project);
  Serial.println(SOCIAL_GENIUS_HOST);
  // Opening connection to server (Use 80 as port if HTTP)
  if (!client.connect(SOCIAL_GENIUS_HOST, 443))
  {
    Serial.println(F("Connection failed"));
    return;
  }

  // give the esp a breather
  yield();

  // Send HTTP request
  client.print(F("GET "));
  // This is the second half of a request (everything that comes after the base URL)
  client.print("/"); // %2C == , mFljAbJMtaaDxSxDN48DuK0hpDc2
  client.print(String(project));
  client.println(F(" HTTP/1.1"));
  Serial.println(client);
  //Headers
  client.print(F("Host: "));
  client.println(SOCIAL_GENIUS_HOST);

  client.println(F("Cache-Control: max-age=0"));


  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
    //  {
    //    Serial.print(F("Unexpected response: "));
    //    Serial.println(status);
    //    return;
    //  }

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
  //  if (!client.find(endOfHeaders))
  //  {
  //    Serial.println(F("Invalid response"));
  //    return;
  //  }


  // peek() will look at the character, but not take it off the queue
  while (client.available() && client.peek() != '{')
  {
    char c = 0;
    client.readBytes(&c, 1);
    Serial.print(c);
    Serial.println("BAD");
  }

  //Use the ArduinoJson Assistant to calculate this:
  DynamicJsonDocument doc(1536); //For ESP32/ESP8266 you'll mainly use dynamic.

  DeserializationError error = deserializeJson(doc, client);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  JsonObject youtube = doc["youtube"];
  youtube_views = youtube["views"].as<String>(); 
  youtube_channel = youtube["channel"].as<String>(); 
  youtube_subscribers = youtube["subscribers"].as<String>(); 
  youtube_videos = youtube["videos"].as<String>(); 


  //YouTube
//  feed[(uint8_t)Values::yt_title] = "Youtube";
  feed[(uint8_t)Values::yt_channel] = youtube_channel;


}

char *displaySocialString(const char* string, const char* desc) {
  static char displayString[200] = {""};
  sprintf(displayString, "%s %s", string, desc);
  return displayString;
}



void nextStateAfter(State ss, uint32_t d) {
  waitDelay = d;
  nextState = ss;
  state = State::wait;
  prevTime = currentTime;
}

void statemachine() {
  static float i = 0.0;
  static int textValue = 0;

  switch (state) {
    case State::noop:
      //maybe wait for input? (for now just send anything to serial);
      if (Serial.available() > 0) {
        Serial.readString();
        state = State::fadeIn;
      }
      break;
    case State::fetch:
      if (currentTime - prevTime_fetch > waitDelay_fetch) {
        prevTime_fetch = currentTime;
        makeHTTPRequest();
        Serial.println("Getting Data after 5 Minutes");
      }
      nextStateAfter(State::fadeIn, 50);
      break;
    case State::wait:
      if (currentTime - prevTime > waitDelay) {
        state = nextState;
        prevTime = currentTime;
      }
      break;
    case State::fadeIn:
      //clear the display
      Serial.println(feed[textValue]);
      Serial.println(i);
      Serial.println(numIterations / iterationOffset);

      myDisplay.print(feed[textValue]);
      //set the display intensity

      i += numIterations / iterationOffset;
     

      if (i > numIterations) {
        nextStateAfter(State::fadeOut, clockedIterations);
      } else if (i == 0) {
        myDisplay.displayClear();
      } else {

        nextStateAfter(State::fadeIn, clockedIterations);
        myDisplay.setIntensity(i);
      }
      break;

    case State::fadeOut:

      Serial.println(feed[textValue]);
      //Serial.println(i);
      myDisplay.print(feed[textValue]);

      i -= numIterations / iterationOffset;

      if (i == 0) {
        textValue++;
        if (textValue == (uint8_t)Values::count) {

          textValue = 0;
          myDisplay.displayClear();
          nextStateAfter(State::fetch, 0);
        } else {
          nextStateAfter(State::fadeIn, clockedIterations);
        }
      } else {
        nextStateAfter(State::fadeOut, clockedIterations);
        myDisplay.setIntensity(i);
      }
      break;

  }
}

void loop() {
  currentTime = millis();

  if (connect) {
    Serial.println("Connect requested");
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    unsigned int s = WiFi.status();
    if (s == 0 && millis() > (lastConnectTry + 60000)) {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connect = true;
    }
    if (status != s) { // WLAN status change
      Serial.print("Status: ");
      Serial.println(s);
      status = s;
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
      }
    }
    if (s == WL_CONNECTED) {
      timeClient.update();

    }
  }
  // Do work:
  configureData();
  //STATE
  statemachine();

  //HTTP
  server.handleClient();

  //RGB LEDs
  
  led_set(10, 0, 0);//red
  led_set(0, 0, 0);

  led_set(0, 10, 0);//green
  led_set(0, 0, 0);

  led_set(0, 0, 10);//blue
  led_set(0, 0, 0);
}


void configureData() {
  timeClient.update();

  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  //AM or PM adjustment
  if (currentHour >= 12)
  {
    AMPM = "PM";
  }
  else(AMPM = "AM");
  if (currentHour >= 13) currentHour = currentHour - 12;
  if (currentHour == 0) currentHour = 12;
  
  String weekDay = weekDays[timeClient.getDay()];
  //  Serial.println(weekDay);

  char time[40];
  sprintf(time, "%d:%i %s", currentHour, currentMinute, AMPM);

  if (currentMinute < 10) //Adds a leading 0 before a single digit minute
  {
    sprintf(time, "%d:0%i %s", currentHour, currentMinute, AMPM);
  }
  else sprintf(time, "%d:%i %s", currentHour, currentMinute, AMPM);
  //  Serial.println(time);

  float subs = (youtube_subscribers.toFloat());
  float subDec;
  char subTrunc[30];
  String subEnd = "Subs";

  if (subs >= 1000000.0) {
    subDec = (subs) / 1000000.0;
    subEnd = "M Subs";
    sprintf(subTrunc, "%4.2f%s", subDec, subEnd);
  }
  /*else if (subs >= 10000.0 && subs <= 1000000.0) {
    subDec = (subs) / 1000.0;
    subEnd = "K Subs";
    sprintf(subTrunc, "%4.0f%s", subDec, subEnd);
  }*/
  else if (subs >= 1.0) {
    subDec = (subs);
    subEnd = " Subs";
    sprintf(subTrunc, "%4.0f%s", subDec, subEnd);
  }
//  Serial.println("Subs: ");
//  Serial.println(subTrunc);

  float views = (youtube_views.toFloat());
  float viewDec;
  char viewTrunc[30];
  String viewEnd = "Views";

  if (views >= 10000000.0) {
    viewDec = (views) / 1000000.0;
    viewEnd = "M Views";
    sprintf(viewTrunc, "%4.0f%s", viewDec, viewEnd);
  }
  else if (views >= 1000000.0) {
    viewDec = (views) / 1000000.0;
    viewEnd = "M Views";
    sprintf(viewTrunc, "%4.2f%s", viewDec, viewEnd);
  }
  else if (views >= 10000.0 && views <= 1000000.0) {
    viewDec = (views) / 1000.0;
    viewEnd = "K Views";
    sprintf(viewTrunc, "%4.2f%s", viewDec, viewEnd);
  }
  else if (views >= 1000.0) {
    viewDec = (views);
    viewEnd = " Views";
    sprintf(viewTrunc, "%4.0f%s", viewDec, viewEnd);
  }
  //  Serial.println(viewTrunc);



// 
  myDisplay.setTextAlignment(PA_CENTER);

  feed[(uint8_t)Values::curTime] = time;
  feed[(uint8_t)Values::weekDay] = weekDay;
  //YouTube
  feed[(uint8_t)Values::yt_views] = viewTrunc;
  feed[(uint8_t)Values::yt_subscribers] = subTrunc;

}
