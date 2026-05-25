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
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Radar Scanner</title>
  <style>
    * {
      box-sizing: border-box;
      font-family: Arial, Helvetica, sans-serif;
    }

    body {
      margin: 0;
      min-height: 100vh;
      background: radial-gradient(circle at top, #102010, #000000 65%);
      color: #00ff66;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .dashboard {
      width: 960px;
      padding: 24px;
      border: 1px solid #00ff66;
      box-shadow: 0 0 25px rgba(0, 255, 100, 0.25);
      background: rgba(0, 0, 0, 0.85);
    }

    .header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 18px;
      border-bottom: 1px solid rgba(0, 255, 100, 0.35);
      padding-bottom: 12px;
    }

    h1 {
      margin: 0;
      font-size: 28px;
      letter-spacing: 1px;
    }

    .subtitle {
      color: #77ff99;
      font-size: 14px;
      margin-top: 4px;
    }

    .status-pill {
      padding: 8px 14px;
      border: 1px solid #00ff66;
      border-radius: 999px;
      font-weight: bold;
      font-size: 14px;
      box-shadow: 0 0 12px rgba(0, 255, 100, 0.35);
    }

    .content {
      display: grid;
      grid-template-columns: 700px 1fr;
      gap: 20px;
      align-items: stretch;
    }

    canvas {
      width: 700px;
      height: 430px;
      border: 1px solid #00ff66;
      background: black;
      box-shadow: inset 0 0 25px rgba(0, 255, 100, 0.15);
    }

    .panel {
      border: 1px solid rgba(0, 255, 100, 0.45);
      padding: 18px;
      background: rgba(0, 30, 10, 0.35);
    }

    .reading {
      margin-bottom: 20px;
    }

    .label {
      font-size: 13px;
      color: #77ff99;
      text-transform: uppercase;
      letter-spacing: 1px;
    }

    .value {
      font-size: 30px;
      font-weight: bold;
      margin-top: 4px;
      color: #ffffff;
    }

    .small {
      font-size: 13px;
      color: #77ff99;
      line-height: 1.5;
      margin-top: 20px;
    }

    .detected {
      color: #ff3333;
    }

    .clear {
      color: #00ff66;
    }
  </style>
</head>

<body>
  <div class="dashboard">
    <div class="header">
      <div>
        <h1>ESP32 Radar Scanner</h1>
        <div class="subtitle">HC-SR04 ultrasonic sensor + SG90 servo + live WiFi dashboard</div>
      </div>
      <div id="statusPill" class="status-pill">SCANNING</div>
    </div>

    <div class="content">
      <canvas id="radar" width="700" height="430"></canvas>

      <div class="panel">
        <div class="reading">
          <div class="label">Current Angle</div>
          <div id="angleValue" class="value">0°</div>
        </div>

        <div class="reading">
          <div class="label">Measured Distance</div>
          <div id="distanceValue" class="value">-- cm</div>
        </div>

        <div class="reading">
          <div class="label">Object Status</div>
          <div id="objectValue" class="value clear">Clear</div>
        </div>

        <div class="small">
          Range scale: 0-400 cm<br>
          Update rate: 20 Hz target<br>
          Data source: ESP32 /data endpoint
        </div>
      </div>
    </div>
  </div>

<script>
  const canvas = document.getElementById('radar');
  const ctx = canvas.getContext('2d');

  const cx = 350;
  const cy = 360;
  const radius = 300;
  const maxDistance = 400;

  let points = [];

  function drawBackground() {
    ctx.fillStyle = 'black';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.strokeStyle = 'rgba(0, 255, 80, 0.25)';
    ctx.lineWidth = 1;

    // Distance rings
    [0.25, 0.5, 0.75, 1].forEach((r, i) => {
      ctx.beginPath();
      ctx.arc(cx, cy, radius * r, Math.PI, 2 * Math.PI);
      ctx.stroke();

      ctx.fillStyle = 'rgba(0, 255, 80, 0.65)';
      ctx.font = '12px Arial';
      ctx.fillText((i + 1) * 100 + ' cm', cx + radius * r - 38, cy - 8);
    });

    // Angle lines and labels
    for (let a = 0; a <= 180; a += 30) {
      let rad = (a * Math.PI) / 180;
      let x = cx + radius * Math.cos(rad);
      let y = cy - radius * Math.sin(rad);

      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(x, y);
      ctx.stroke();

      ctx.fillStyle = 'rgba(0, 255, 80, 0.75)';
      ctx.font = '12px Arial';

      let labelX = cx + (radius + 18) * Math.cos(rad);
      let labelY = cy - (radius + 18) * Math.sin(rad);

      ctx.fillText(a + '\u00B0', labelX - 10, labelY + 5);
    }

    // Base line
    ctx.strokeStyle = '#00ff66';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(cx - radius, cy);
    ctx.lineTo(cx + radius, cy);
    ctx.stroke();
  }

  function drawSweep(angle) {
    let rad = (angle * Math.PI) / 180;

    ctx.strokeStyle = '#00ff66';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + radius * Math.cos(rad), cy - radius * Math.sin(rad));
    ctx.stroke();

    // Glow effect
    ctx.strokeStyle = 'rgba(0, 255, 100, 0.25)';
    ctx.lineWidth = 10;
    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(cx + radius * Math.cos(rad), cy - radius * Math.sin(rad));
    ctx.stroke();
  }

  function drawPoints() {
    points.forEach((p, index) => {
      let fade = (index + 1) / points.length;
      let rad = (p.angle * Math.PI) / 180;
      let scale = (p.distance / maxDistance) * radius;

      let x = cx + scale * Math.cos(rad);
      let y = cy - scale * Math.sin(rad);

      ctx.fillStyle = `rgba(255, 40, 40, ${fade})`;
      ctx.beginPath();
      ctx.arc(x, y, 6, 0, 2 * Math.PI);
      ctx.fill();
    });
  }

  function updateText(angle, distance) {
    document.getElementById('angleValue').textContent = angle + '\u00B0';

    const statusPill = document.getElementById('statusPill');
    const objectValue = document.getElementById('objectValue');

    if (distance > 0 && distance <= maxDistance) {
      document.getElementById('distanceValue').textContent = distance.toFixed(1) + ' cm';
      objectValue.textContent = 'Detected';
      objectValue.className = 'value detected';
      statusPill.textContent = 'OBJECT DETECTED';
      statusPill.style.color = '#ff3333';
      statusPill.style.borderColor = '#ff3333';
    } else {
      document.getElementById('distanceValue').textContent = '-- cm';
      objectValue.textContent = 'Clear';
      objectValue.className = 'value clear';
      statusPill.textContent = 'SCANNING';
      statusPill.style.color = '#00ff66';
      statusPill.style.borderColor = '#00ff66';
    }
  }

  function drawRadar(angle, distance) {
    drawBackground();

    if (distance > 0 && distance <= maxDistance) {
      points.push({ angle, distance });

      if (points.length > 40) {
        points.shift();
      }
    }

    drawPoints();
    drawSweep(angle);
    updateText(angle, distance);
  }

  setInterval(() => {
    fetch('/data')
      .then(response => response.json())
      .then(data => {
        drawRadar(data.angle, data.distance);
      })
      .catch(error => {
        console.log('Data fetch error:', error);
      });
  }, 50);
</script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void loop() {
  server.handleClient();
  sweepRadar();
}
