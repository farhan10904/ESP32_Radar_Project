#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Pin definitions
#define TRIG_PIN 5
#define ECHO_PIN 18
#define SERVO_PIN 13

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Servo
Servo radarServo;

// Web server
WebServer server(80);

// Shared variables
int currentAngle = 0;
float currentDistance = 0;
int sweepDirection = 1;

void setup() {
  Serial.begin(115200);

  // Servo
  radarServo.attach(SERVO_PIN);
  radarServo.write(0);

  // HC-SR04
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  // WiFi
  WiFi.begin(ssid, password, 6);
  display.println("Connecting WiFi...");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Show IP on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connected!");
  display.println(WiFi.localIP());
  display.display();

  // Web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/favicon.ico", []() {
  server.send(204);
});
  server.begin();
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  if (duration == 0) return -1;
  
  float distance = (duration * 0.0343) / 2;
  return distance;
}

void sweepRadar() {
  unsigned long currentMillis = millis();
  static unsigned long lastMoveTime = 0;
  static int angle = 0;
  static int direction = 1;

  if (currentMillis - lastMoveTime >= 20) {
    lastMoveTime = currentMillis;

    angle += direction;

    if (angle >= 180) direction = -1;
    if (angle <= 0) direction = 1;

    radarServo.write(angle);
    currentDistance = getDistance();
    currentAngle = angle;

    // Update OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Angle: ");
    display.println(angle);
    display.print("Dist: ");
    if (currentDistance == -1) {
      display.println("No object");
    } else {
      display.print(currentDistance);
      display.println(" cm");
    }
    display.display();
  }
}

void handleData() {
  String json = "{\"angle\":" + String(currentAngle) + 
                ",\"distance\":" + String(currentDistance) + "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Radar</title>
  <style>
    body { background: black; display: flex; 
           justify-content: center; align-items: center; 
           height: 100vh; margin: 0; }
    canvas { border: 2px solid #00ff00; }
  </style>
</head>
<body>
<canvas id="radar" width="500" height="500"></canvas>
<script>
  const canvas = document.getElementById('radar');
  const ctx = canvas.getContext('2d');
  const cx = 250, cy = 250, radius = 230;

  function drawBackground() {
    ctx.fillStyle = 'black';
    ctx.fillRect(0, 0, 500, 500);
    ctx.strokeStyle = '#003300';
    ctx.lineWidth = 1;
    [0.25, 0.5, 0.75, 1].forEach(r => {
      ctx.beginPath();
      ctx.arc(cx, cy, radius * r, Math.PI, 2 * Math.PI);
      ctx.stroke();
    });
    for (let a = 0; a <= 180; a += 30) {
      let rad = (a * Math.PI) / 180;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(cx + radius * Math.cos(rad), cy - radius * Math.sin(rad));
      ctx.stroke();
    }
  }

  function drawSweep(angle, distance) {
    drawBackground();
    let rad = (angle * Math.PI) / 180;

    // Sweep line
    ctx.strokeStyle = '#00ff00';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + radius * Math.cos(rad), cy - radius * Math.sin(rad));
    ctx.stroke();

    // Object dot
    if (distance > 0 && distance < 400) {
      let scale = (distance / 400) * radius;
      let dx = cx + scale * Math.cos(rad);
      let dy = cy - scale * Math.sin(rad);
      ctx.fillStyle = '#ff0000';
      ctx.beginPath();
      ctx.arc(dx, dy, 6, 0, 2 * Math.PI);
      ctx.fill();
    }
  }

  setInterval(() => {
    fetch('/data')
      .then(r => r.json())
      .then(d => drawSweep(d.angle, d.distance));
  }, 50);
</script>
</body>
</html>
  )";
  server.send(200, "text/html", html);
}

void loop() {
  server.handleClient();
  sweepRadar();
}
