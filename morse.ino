#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define SDA_PIN 5
#define SCL_PIN 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int buttonPin = 10;

unsigned long pressStart = 0;
unsigned long pressDuration = 0;
unsigned long lastSignalTime = 0;

const unsigned long MODE_HOLD_TIME = 10000;

enum DecodeMode {
  TEXT_MODE,
  LETTER_MODE
};

DecodeMode mode = TEXT_MODE;

String morseSymbol = "";
String decodedText = "";

struct MorseMap {
  const char* code;
  char letter;
};

MorseMap morseTable[] = {
  {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'},
  {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
  {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
  {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
  {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'},
  {"--..", 'Z'},
  {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
  {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'},
  {"---..", '8'}, {"----.", '9'}
};

const int morseTableSize = sizeof(morseTable) / sizeof(MorseMap);

const char* ssid1 = "YOUR_FIRST_WIFI";
const char* password1 = "YOUR_FIRST_WIFI_PASSWORD";

const char* ssid2 = "YOUR_SECOND_WIFI";
const char* password2 = "YOUR_SECOND_WIFI_PASSWORD";

WiFiServer server(80);

int currentWiFi = 1;
unsigned long lastWiFiAttempt = 0;
const unsigned long WIFI_RETRY_INTERVAL = 15000;

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found!");

    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("MORSE");

  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("Starting...");
  display.display();

  delay(1000);

  WiFi.mode(WIFI_STA);
  startWiFi();

  server.begin();

  displayReady();
}

void loop() {
  unsigned long currentTime = millis();

  handleWiFi();
  handleWiFiClients();

  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW && pressStart == 0) {
    pressStart = currentTime;
    Serial.println("Button pressed");
  }

  if (
    buttonState == LOW &&
    pressStart != 0 &&
    (currentTime - pressStart >= MODE_HOLD_TIME)
  ) {
    if (mode == TEXT_MODE) {
      mode = LETTER_MODE;
      Serial.println("MODE: LETTER");
    } else {
      mode = TEXT_MODE;
      Serial.println("MODE: TEXT");
    }

    pressStart = 0;
    morseSymbol = "";

    displayModeChanged();

    while (digitalRead(buttonPin) == LOW) {
      delay(10);
    }

    lastSignalTime = millis();

    return;
  }

  if (buttonState == HIGH && pressStart != 0) {
    pressDuration = currentTime - pressStart;
    pressStart = 0;

    if (pressDuration < 300) {
      morseSymbol += ".";
      Serial.println("DOT");
    } else {
      morseSymbol += "-";
      Serial.println("DASH");
    }

    lastSignalTime = currentTime;

    displayMorse();
  }

  if (
    morseSymbol.length() > 0 &&
    pressStart == 0 &&
    (currentTime - lastSignalTime) > 1000
  ) {
    char decodedChar = decodeMorse(morseSymbol);

    if (mode == LETTER_MODE) {
      decodedText += decodedChar;

      Serial.print("Letter: ");
      Serial.println(decodedChar);

      morseSymbol = "";

      displayLetter(decodedChar);
    } else {
      decodedText += decodedChar;

      Serial.print("Decoded: ");
      Serial.println(decodedText);

      morseSymbol = "";

      displayText();
    }
  }
}

void startWiFi() {
  Serial.println();
  Serial.println("Starting Wi-Fi...");

  currentWiFi = 1;

  WiFi.begin(ssid1, password1);

  Serial.print("Trying: ");
  Serial.println(ssid1);

  lastWiFiAttempt = millis();
}

void handleWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long currentTime = millis();

  if (currentTime - lastWiFiAttempt < WIFI_RETRY_INTERVAL) {
    return;
  }

  lastWiFiAttempt = currentTime;

  if (currentWiFi == 1) {
    Serial.println("First Wi-Fi failed. Trying second Wi-Fi...");

    WiFi.disconnect();
    delay(100);

    currentWiFi = 2;

    WiFi.begin(ssid2, password2);

    Serial.print("Trying: ");
    Serial.println(ssid2);
  } else {
    Serial.println("Second Wi-Fi failed. Trying first Wi-Fi...");

    WiFi.disconnect();
    delay(100);

    currentWiFi = 1;

    WiFi.begin(ssid1, password1);

    Serial.print("Trying: ");
    Serial.println(ssid1);
  }
}

char decodeMorse(String symbol) {
  for (int i = 0; i < morseTableSize; i++) {
    if (symbol == morseTable[i].code) {
      return morseTable[i].letter;
    }
  }

  return '?';
}

void displayReady() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("READY");

  display.setTextSize(1);
  display.setCursor(0, 25);

  if (mode == TEXT_MODE) {
    display.println("MODE: TEXT");
  } else {
    display.println("MODE: LETTER");
  }

  display.setCursor(0, 40);
  display.println("Short = DOT");

  display.setCursor(0, 52);
  display.println("Long = DASH");

  display.display();
}

void displayMorse() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);

  if (mode == TEXT_MODE) {
    display.println("MODE: TEXT");
  } else {
    display.println("MODE: LETTER");
  }

  display.setCursor(0, 12);
  display.println("Morse:");

  display.setTextSize(3);
  display.setCursor(0, 25);
  display.println(morseSymbol);

  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("Text: ");

  int len = decodedText.length();

  if (len > 14) {
    display.println(decodedText.substring(len - 14));
  } else {
    display.println(decodedText);
  }

  display.display();
}

void displayLetter(char letter) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MODE: LETTER");

  display.setCursor(0, 12);
  display.println("Decoded:");

  display.setTextSize(4);
  display.setCursor(50, 22);
  display.println(letter);

  display.display();
}

void displayText() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);

  if (mode == TEXT_MODE) {
    display.println("MODE: TEXT");
  } else {
    display.println("MODE: LETTER");
  }

  display.setCursor(0, 12);
  display.println("Decoded:");

  display.setTextSize(2);
  display.setCursor(0, 27);

  int len = decodedText.length();

  if (len > 10) {
    display.println(decodedText.substring(len - 10));
  } else {
    display.println(decodedText);
  }

  display.display();
}

void displayModeChanged() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MODE CHANGED");

  display.setTextSize(2);
  display.setCursor(0, 22);

  if (mode == LETTER_MODE) {
    display.println("LETTER");
  } else {
    display.println("TEXT");
  }

  display.display();

  delay(1200);

  displayReady();
}

void handleWiFiClients() {
  WiFiClient client = server.available();

  if (!client) {
    return;
  }

  Serial.println("Client connected");

  unsigned long timeout = millis();

  while (
    !client.available() &&
    millis() - timeout < 1000
  ) {
    delay(1);
  }

  while (client.available()) {
    client.read();
  }

  String response =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Connection: close\r\n"
    "\r\n";

  response += decodedText;
  response += "\r\n";

  client.print(response);

  delay(1);

  client.stop();

  Serial.println("Client disconnected");
}
