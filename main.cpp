#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

#define LED_PIN 15  // Change this to the pin YOU used for connecting Din on your LED strip
#define NUM_LEDS 75 // Number of LEDs in your strip
#define BRIGHTNESS 255 // Max brightness (0-255)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

const char* ssid = "XXXXXXXXXXXX";       // Your Wi-Fi network name
const char* password = "XXXXXXXXXXXXXX"; // Your Wi-Fi password

WebServer server(80);

bool randomPaletteOn = false;
#define UPDATES_PER_SECOND 100
CRGBPalette16 currentPalette = RainbowColors_p;
TBlendType currentBlending = LINEARBLEND;

// Variables to store RGB values from sliders
int redValue = 0;
int greenValue = 0;
int blueValue = 0;

// these are forward declarations of functions used later
// this is for platform IO compatibility
void handleRandomToggle();
void FillLEDsFromPaletteColors(uint8_t colorIndex);
void handleColor();
void handleRandomToggle();
void ChangePalettePeriodically();

// THE HTML page with RGB sliders / BUTTON to toggle random palette
// This is a simple HTML page that will be served by the ESP32 web server
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 RGB Control</title>
<style>
  body {
    font-family: Arial, sans-serif;
    background: #111;
    color: white;
    text-align: center;
    margin: 0;
    padding: 0;
  }
  h1 {
    font-size: 1.5em;
    margin-top: 20px;
  }
  label {
    display: block;
    font-size: 1.2em;
    margin-top: 20px;
  }
  input[type=range] {
    -webkit-appearance: none;
    width: 90%;
    height: 40px;
    margin: 10px 0;
    background: #333;
    border-radius: 10px;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 30px;
    height: 30px;
    background: #fff;
    border-radius: 50%;
    cursor: pointer;
  }
  /* New Button Styling */
  button {
    background-color: #ccc;       /* light grey */
    color: #000;                  /* black text for contrast */
    font-size: 1.1em;
    padding: 12px 24px;
    border: none;
    border-radius: 8px;
    margin-top: 30px;              /* pushes buttons lower down */
    margin-left: 10px;
    margin-right: 10px;
    cursor: pointer;
  }
  button:hover {
    background-color: #bbb;        /* slightly darker on hover */
  }
</style>
</head>
<body>
<h1>LED Color Control</h1>

<label for="red">Red</label>
<input type="range" id="red" min="0" max="255" value="0" oninput="sendValue()">

<label for="green">Green</label>
<input type="range" id="green" min="0" max="255" value="0" oninput="sendValue()">

<label for="blue">Blue</label>
<input type="range" id="blue" min="0" max="255" value="0" oninput="sendValue()">

<!-- Random Palette Buttons -->
<div>
  <button onclick="toggleRandom(true)">Random Palette ON</button>
  <button onclick="toggleRandom(false)">Random Palette OFF</button>
</div>

<script>
function toggleRandom(state) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/random?state=" + (state ? "on" : "off"), true);
  xhr.send();
}

function sendValue() {
  var r = document.getElementById('red').value;
  var g = document.getElementById('green').value;
  var b = document.getElementById('blue').value;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", `/color?red=${r}&green=${g}&blue=${b}`, true);
  xhr.send();
}
</script>

</body>
</html>
)rawliteral";

// Handle root webpage request
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
    uint8_t brightness = BRIGHTNESS;
    for (int i = 0; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

// Handle RGB values from sliders
void handleColor() {
  if (server.hasArg("red") && server.hasArg("green") && server.hasArg("blue")) {
    redValue = server.arg("red").toInt();
    greenValue = server.arg("green").toInt();
    blueValue = server.arg("blue").toInt();

    // Update all LEDs with new color
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(redValue, greenValue, blueValue);
    }
    FastLED.show();
    server.send(200, "text/plain", "Color updated");
  } else {
    server.send(400, "text/plain", "Missing color parameters");
  }
}

void handleRandomToggle() {
  if (server.hasArg("state")) {
    String state = server.arg("state"); // expected "on" or "off"
    randomPaletteOn = (state == "on");
    server.send(200, "text/plain", String("Random palette ") + (randomPaletteOn ? "enabled" : "disabled"));
  } else {
    server.send(400, "text/plain", "Missing state parameter");
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/color", handleColor);
  server.on("/random", handleRandomToggle);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  if (randomPaletteOn) {
    static uint8_t startIndex = 0;
    startIndex++;
    FillLEDsFromPaletteColors(startIndex);
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
}

