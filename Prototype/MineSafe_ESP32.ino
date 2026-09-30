#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// WIFI
// =====================================================

const char* ssid = "Sashank";
const char* password = "************";

WebServer server(80);

// =====================================================
// L298N MOTOR DRIVER
// =====================================================

#define IN1 25
#define IN2 26
#define IN3 27
#define IN4 14

#define ENA 32
#define ENB 33

// =====================================================
// ULTRASONIC SENSORS
// =====================================================

// CENTER
#define FRONT_TRIG 5
#define FRONT_ECHO 18

// LEFT
#define LEFT_TRIG 2
#define LEFT_ECHO 35

// RIGHT
#define RIGHT_TRIG 15
#define RIGHT_ECHO 36

// =====================================================
// FOG / SMOKE SENSOR
// =====================================================

#define SMOKE_PIN 34

// =====================================================
// LEDS
// =====================================================

#define GREEN_LED 19
#define YELLOW_LED 23
#define RED_LED 4

// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN 13

// =====================================================
// LCD
// =====================================================

#define SDA_PIN 21
#define SCL_PIN 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// MOTOR SPEED
// =====================================================

#define SAFE_SPEED 110
#define CAUTION_SPEED 80
#define WARNING_SPEED 50

// =====================================================
// DISTANCE
// =====================================================

#define STOP_DISTANCE 20
#define SIDE_WARNING_DISTANCE 20
#define CAUTION_DISTANCE 30

// =====================================================
// FOG SENSOR SETTINGS
// =====================================================
//
// IMPORTANT:
// This is a RELATIVE fog/smoke index,
// NOT a real atmospheric fog percentage.
//
// 4095 is the ESP32 ADC maximum.
//
// We use 1200 ADC counts as the change
// corresponding to 100% demonstration level.
//
// If your sensor reaches 100% too quickly,
// increase this value to 1500 or 2000.
//
// =====================================================

#define FOG_FULL_SCALE 1200

// Ignore tiny ADC fluctuations
#define FOG_DEADBAND 20

// Number of readings used for averaging
#define SMOKE_SAMPLES 20

// =====================================================
// VARIABLES
// =====================================================

int smokeValue = 0;
int smokeBaseline = 0;

int fogPercent = 0;

// Smoothed fog value
float fogFiltered = 0;

// Ultrasonic distances
long frontDistance = 999;
long leftDistance = 999;
long rightDistance = 999;

String statusText = "SAFE";
String actionText = "MOVE";

// =====================================================
// BUZZER TIMER
// =====================================================

unsigned long buzzerTimer = 0;

bool buzzerState = false;

// =====================================================
// READ SMOKE SENSOR
// =====================================================

int readSmoke()
{
  long total = 0;

  for (int i = 0; i < SMOKE_SAMPLES; i++)
  {
    total += analogRead(SMOKE_PIN);
    delay(2);
  }

  return total / SMOKE_SAMPLES;
}

// =====================================================
// CALIBRATE CLEAN AIR
// =====================================================

void calibrateSmoke()
{
  Serial.println();
  Serial.println("======================================");
  Serial.println("       FOG SENSOR CALIBRATION");
  Serial.println("======================================");
  Serial.println("Keep sensor in CLEAN AIR.");
  Serial.println("Do NOT use agarbathi now.");
  Serial.println("Calibration starts in 5 seconds...");

  delay(5000);

  long total = 0;

  // Take 100 clean-air readings
  for (int i = 0; i < 100; i++)
  {
    total += analogRead(SMOKE_PIN);

    delay(20);
  }

  smokeBaseline = total / 100;

  Serial.println();
  Serial.print("Clean Air Baseline = ");
  Serial.println(smokeBaseline);

  Serial.println("Calibration completed.");
  Serial.println("Now you can introduce smoke.");
  Serial.println("======================================");
  Serial.println();
}

// =====================================================
// CALCULATE FOG PERCENTAGE
// =====================================================

void calculateFog()
{
  smokeValue = readSmoke();

  // Difference from clean-air baseline
  int change = abs(smokeValue - smokeBaseline);

  // Ignore small sensor noise
  if (change <= FOG_DEADBAND)
  {
    change = 0;
  }

  // Convert ADC change to relative percentage
  float percentage =
    ((float)change / FOG_FULL_SCALE) * 100.0;

  percentage = constrain(
    percentage,
    0,
    100
  );

  // Smooth the reading
  fogFiltered =
    (fogFiltered * 0.75) +
    (percentage * 0.25);

  fogPercent =
    (int)(fogFiltered + 0.5);

  fogPercent =
    constrain(
      fogPercent,
      0,
      100
    );
}

// =====================================================
// READ ULTRASONIC
// =====================================================

long readUltrasonic(
  int trigPin,
  int echoPin
)
{
  digitalWrite(
    trigPin,
    LOW
  );

  delayMicroseconds(5);

  digitalWrite(
    trigPin,
    HIGH
  );

  delayMicroseconds(10);

  digitalWrite(
    trigPin,
    LOW
  );

  long duration =
    pulseIn(
      echoPin,
      HIGH,
      30000
    );

  if (duration == 0)
  {
    return 999;
  }

  long distance =
    duration * 0.0343 / 2;

  if (
    distance <= 0 ||
    distance > 400
  )
  {
    return 999;
  }

  return distance;
}

// =====================================================
// READ ALL ULTRASONIC SENSORS
// =====================================================

void readAllDistances()
{
  // CENTER
  frontDistance =
    readUltrasonic(
      FRONT_TRIG,
      FRONT_ECHO
    );

  delay(80);

  // LEFT
  leftDistance =
    readUltrasonic(
      LEFT_TRIG,
      LEFT_ECHO
    );

  delay(80);

  // RIGHT
  rightDistance =
    readUltrasonic(
      RIGHT_TRIG,
      RIGHT_ECHO
    );

  delay(80);
}

// =====================================================
// MOTOR SPEED
// =====================================================

void setMotorSpeed(
  int speedValue
)
{
  speedValue =
    constrain(
      speedValue,
      0,
      255
    );

  ledcWrite(
    ENA,
    speedValue
  );

  ledcWrite(
    ENB,
    speedValue
  );
}

// =====================================================
// MOVE FORWARD
// =====================================================

void moveForward(
  int speedValue
)
{
  digitalWrite(
    IN1,
    HIGH
  );

  digitalWrite(
    IN2,
    LOW
  );

  digitalWrite(
    IN3,
    HIGH
  );

  digitalWrite(
    IN4,
    LOW
  );

  setMotorSpeed(
    speedValue
  );
}

// =====================================================
// STOP TRUCK
// =====================================================

void stopTruck()
{
  digitalWrite(
    IN1,
    LOW
  );

  digitalWrite(
    IN2,
    LOW
  );

  digitalWrite(
    IN3,
    LOW
  );

  digitalWrite(
    IN4,
    LOW
  );

  setMotorSpeed(0);
}

// =====================================================
// GREEN LED
// =====================================================

void greenLED()
{
  digitalWrite(
    GREEN_LED,
    HIGH
  );

  digitalWrite(
    YELLOW_LED,
    LOW
  );

  digitalWrite(
    RED_LED,
    LOW
  );
}

// =====================================================
// YELLOW LED
// =====================================================

void yellowLED()
{
  digitalWrite(
    GREEN_LED,
    LOW
  );

  digitalWrite(
    YELLOW_LED,
    HIGH
  );

  digitalWrite(
    RED_LED,
    LOW
  );
}

// =====================================================
// RED LED
// =====================================================

void redLED()
{
  digitalWrite(
    GREEN_LED,
    LOW
  );

  digitalWrite(
    YELLOW_LED,
    LOW
  );

  digitalWrite(
    RED_LED,
    HIGH
  );
}

// =====================================================
// BUZZER OFF
// =====================================================

void buzzerOff()
{
  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  buzzerState = false;
}

// =====================================================
// DANGER BUZZER
// CONTINUOUS
// =====================================================

void dangerBuzzer()
{
  digitalWrite(
    BUZZER_PIN,
    HIGH
  );

  buzzerState = true;
}

// =====================================================
// WARNING BUZZER
// BEEP ... BEEP ... BEEP
// =====================================================

void warningBuzzer()
{
  if (
    millis() - buzzerTimer >= 500
  )
  {
    buzzerTimer = millis();

    buzzerState =
      !buzzerState;

    digitalWrite(
      BUZZER_PIN,
      buzzerState
    );
  }
}

// =====================================================
// LCD LINE
// =====================================================

void printLCDLine(
  int row,
  String text
)
{
  if (text.length() > 16)
  {
    text =
      text.substring(
        0,
        16
      );
  }

  while (text.length() < 16)
  {
    text += " ";
  }

  lcd.setCursor(
    0,
    row
  );

  lcd.print(text);
}

// =====================================================
// LCD UPDATE
// =====================================================

void updateLCD()
{
  String line1;

  if (statusText == "DANGER")
  {
    line1 = "DANGER STOP";
  }
  else if (statusText == "WARNING")
  {
    line1 = "WARNING SLOW";
  }
  else if (statusText == "CAUTION")
  {
    line1 = "CAUTION SLOW";
  }
  else
  {
    line1 = "SAFE MOVE";
  }

  printLCDLine(
    0,
    line1
  );

  // Find nearest object
  long displayDistance =
    frontDistance;

  if (
    leftDistance <
    displayDistance
  )
  {
    displayDistance =
      leftDistance;
  }

  if (
    rightDistance <
    displayDistance
  )
  {
    displayDistance =
      rightDistance;
  }

  String line2;

  if (displayDistance == 999)
  {
    line2 = "D:---cm F:";
  }
  else
  {
    line2 =
      "D:" +
      String(displayDistance) +
      "cm F:";
  }

  line2 +=
    String(fogPercent);

  line2 += "%";

  printLCDLine(
    1,
    line2
  );
}

// =====================================================
// WEB PAGE
// =====================================================

void handleRoot()
{
  String page = "";

  page += "<!DOCTYPE html>";
  page += "<html>";

  page += "<head>";

  page +=
    "<meta name='viewport' "
    "content='width=device-width,initial-scale=1'>";

  page +=
    "<meta http-equiv='refresh' "
    "content='2'>";

  page +=
    "<title>HAUL SENTINEL</title>";

  page += "</head>";

  page += "<body>";

  page +=
    "<h1>HAUL SENTINEL</h1>";

  page +=
    "<h2>Sentinel Coal Mine</h2>";

  page += "<hr>";

  page += "<h2>Status: ";
  page += statusText;
  page += "</h2>";

  page += "<h2>Action: ";
  page += actionText;
  page += "</h2>";

  page += "<h3>Center: ";
  page += String(frontDistance);
  page += " cm</h3>";

  page += "<h3>Left: ";
  page += String(leftDistance);
  page += " cm</h3>";

  page += "<h3>Right: ";
  page += String(rightDistance);
  page += " cm</h3>";

  page += "<h3>Fog Index: ";
  page += String(fogPercent);
  page += "%</h3>";

  page += "<h3>Raw Sensor: ";
  page += String(smokeValue);
  page += "</h3>";

  page += "<h3>Baseline: ";
  page += String(smokeBaseline);
  page += "</h3>";

  page += "<hr>";

  page +=
    "<p>Center &lt; 20 cm = "
    "STOP</p>";

  page +=
    "<p>Left/Right &lt; 20 cm = "
    "WARNING + SLOW</p>";

  page +=
    "<p>20-30 cm = "
    "CAUTION + SLOW</p>";

  page +=
    "<p>Fog = DISPLAY ONLY</p>";

  page += "<p>ESP32 IP: ";

  page +=
    WiFi.localIP().toString();

  page += "</p>";

  page += "</body>";

  page += "</html>";

  server.send(
    200,
    "text/html",
    page
  );
}

// =====================================================
// WIFI
// =====================================================

void connectWiFi()
{
  Serial.println();
  Serial.println(
    "Connecting to WiFi..."
  );

  WiFi.mode(WIFI_STA);

  WiFi.setSleep(false);

  WiFi.begin(
    ssid,
    password
  );

  int attempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 30
  )
  {
    delay(500);

    Serial.print(".");

    attempts++;
  }

  Serial.println();

  if (
    WiFi.status() ==
    WL_CONNECTED
  )
  {
    Serial.println(
      "WiFi Connected!"
    );

    Serial.print(
      "ESP32 IP: "
    );

    Serial.println(
      WiFi.localIP()
    );
  }
  else
  {
    Serial.println(
      "WiFi Connection Failed"
    );
  }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);

  // ---------------------------------------------------
  // MOTOR PINS
  // ---------------------------------------------------

  pinMode(
    IN1,
    OUTPUT
  );

  pinMode(
    IN2,
    OUTPUT
  );

  pinMode(
    IN3,
    OUTPUT
  );

  pinMode(
    IN4,
    OUTPUT
  );

  // ---------------------------------------------------
  // ULTRASONIC
  // ---------------------------------------------------

  pinMode(
    FRONT_TRIG,
    OUTPUT
  );

  pinMode(
    FRONT_ECHO,
    INPUT
  );

  pinMode(
    LEFT_TRIG,
    OUTPUT
  );

  pinMode(
    LEFT_ECHO,
    INPUT
  );

  pinMode(
    RIGHT_TRIG,
    OUTPUT
  );

  pinMode(
    RIGHT_ECHO,
    INPUT
  );

  digitalWrite(
    FRONT_TRIG,
    LOW
  );

  digitalWrite(
    LEFT_TRIG,
    LOW
  );

  digitalWrite(
    RIGHT_TRIG,
    LOW
  );

  // ---------------------------------------------------
  // LEDS
  // ---------------------------------------------------

  pinMode(
    GREEN_LED,
    OUTPUT
  );

  pinMode(
    YELLOW_LED,
    OUTPUT
  );

  pinMode(
    RED_LED,
    OUTPUT
  );

  // ---------------------------------------------------
  // BUZZER
  // ---------------------------------------------------

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  // ---------------------------------------------------
  // SMOKE SENSOR
  // ---------------------------------------------------

  pinMode(
    SMOKE_PIN,
    INPUT
  );

  // ---------------------------------------------------
  // MOTOR PWM
  // ---------------------------------------------------

  ledcAttach(
    ENA,
    1000,
    8
  );

  ledcAttach(
    ENB,
    1000,
    8
  );

  // ---------------------------------------------------
  // INITIAL MOTOR STOP
  // ---------------------------------------------------

  stopTruck();

  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  lcd.init();

  lcd.backlight();

  printLCDLine(
    0,
    "HAUL SENTINEL"
  );

  printLCDLine(
    1,
    "SYSTEM STARTING"
  );

  delay(2000);

  // ---------------------------------------------------
  // WIFI
  // ---------------------------------------------------

  connectWiFi();

  server.on(
    "/",
    handleRoot
  );

  server.begin();

  Serial.println(
    "Web Server Started"
  );

  // ---------------------------------------------------
  // FOG CALIBRATION
  // ---------------------------------------------------

  calibrateSmoke();

  // ---------------------------------------------------
  // READY
  // ---------------------------------------------------

  fogFiltered = 0;

  fogPercent = 0;

  greenLED();

  buzzerOff();

  printLCDLine(
    0,
    "SAFE MOVE"
  );

  printLCDLine(
    1,
    "SYSTEM READY"
  );

  delay(1000);

  Serial.println();
  Serial.println(
    "======================================"
  );

  Serial.println(
    "       HAUL SENTINEL READY"
  );

  Serial.println(
    "======================================"
  );
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  // Web server
  server.handleClient();

  // ---------------------------------------------------
  // FOG SENSOR
  // DISPLAY ONLY
  // ---------------------------------------------------

  calculateFog();

  // ---------------------------------------------------
  // ULTRASONIC
  // ---------------------------------------------------

  readAllDistances();

  // ===================================================
  // 1. CENTER < 20 cm
  // DANGER + STOP
  // ===================================================

  if (
    frontDistance < STOP_DISTANCE
  )
  {
    statusText = "DANGER";
    actionText = "STOP";

    stopTruck();

    redLED();

    dangerBuzzer();
  }

  // ===================================================
  // 2. LEFT OR RIGHT < 20 cm
  // WARNING + SLOW
  // ===================================================

  else if (
    leftDistance < SIDE_WARNING_DISTANCE ||
    rightDistance < SIDE_WARNING_DISTANCE
  )
  {
    statusText = "WARNING";
    actionText = "SLOW";

    moveForward(
      WARNING_SPEED
    );

    yellowLED();

    warningBuzzer();
  }

  // ===================================================
  // 3. ANY SENSOR 20-30 cm
  // CAUTION + SLOW
  // ===================================================

  else if (
    frontDistance <= CAUTION_DISTANCE ||
    leftDistance <= CAUTION_DISTANCE ||
    rightDistance <= CAUTION_DISTANCE
  )
  {
    statusText = "CAUTION";
    actionText = "SLOW";

    moveForward(
      CAUTION_SPEED
    );

    yellowLED();

    buzzerOff();
  }

  // ===================================================
  // 4. ALL CLEAR
  // SAFE + NORMAL SPEED
  // ===================================================

  else
  {
    statusText = "SAFE";
    actionText = "MOVE";

    moveForward(
      SAFE_SPEED
    );

    greenLED();

    buzzerOff();
  }

  // ---------------------------------------------------
  // LCD
  // ---------------------------------------------------

  updateLCD();

  // ---------------------------------------------------
  // SERIAL MONITOR
  // ---------------------------------------------------

  Serial.println(
    "--------------------------------------"
  );

  Serial.print(
    "CENTER : "
  );

  Serial.print(
    frontDistance
  );

  Serial.println(
    " cm"
  );

  Serial.print(
    "LEFT   : "
  );

  Serial.print(
    leftDistance
  );

  Serial.println(
    " cm"
  );

  Serial.print(
    "RIGHT  : "
  );

  Serial.print(
    rightDistance
  );

  Serial.println(
    " cm"
  );

  Serial.print(
    "RAW    : "
  );

  Serial.println(
    smokeValue
  );

  Serial.print(
    "BASE   : "
  );

  Serial.println(
    smokeBaseline
  );

  Serial.print(
    "FOG    : "
  );

  Serial.print(
    fogPercent
  );

  Serial.println(
    "%"
  );

  Serial.print(
    "STATUS : "
  );

  Serial.println(
    statusText
  );

  Serial.print(
    "ACTION : "
  );

  Serial.println(
    actionText
  );

  Serial.println(
    "--------------------------------------"
  );

  delay(300);
}