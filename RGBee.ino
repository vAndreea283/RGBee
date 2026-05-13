#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32_CAR";
const char* password = "12345678";

WebServer server(80);

// ======================================================
// MOTOR FATA (DIRECTIE)
// ======================================================

int IN1 = 27;
int IN2 = 26;
int ENA = 14;

// ======================================================
// MOTOR SPATE (TRACTIUNE)
// ======================================================

int IN3 = 25;
int IN4 = 33;
int ENB = 32;

// ======================================================
// ULTRASONIC
// ======================================================

#define TRIG 18
#define ECHO 19

// ======================================================
// BUZZER
// ======================================================

#define BUZZER 23
#define LEDC_CHANNEL 0
#define LEDC_FREQ    2000
#define LEDC_RES     8

float distance = 0;

unsigned long lastBeep      = 0;
unsigned long lastRadioNote = 0;

// ======================================================
// GAME OVER — non-blocking
// ======================================================

bool     crashPlaying    = false;
int      crashNoteIndex  = 0;
unsigned long crashNoteStart = 0;
#define  CRASH_NOTE_DUR  180   // ms per nota
#define  CRASH_PAUSE     600   // pauza dupa melodie

int gameOver[] = { 523, 494, 440, 392, 349, 330, 262 };
int gameOverSize = 7;

// ======================================================
// RADIO MELODY
// ======================================================

int melody[]   = { 262, 294, 330, 349, 392, 440, 494, 523 };
int melodySize = 8;
int currentNote = 0;

// ======================================================
// HELPER BUZZER — core 2.x API
// ======================================================

void buzzerTone(int freq) {
  if (freq <= 0) {
    ledcWrite(LEDC_CHANNEL, 0);
  } else {
    ledcWriteTone(BUZZER, freq);
    ledcWrite(BUZZER, 128);
  }
}

void buzzerOff() {
  ledcWrite(BUZZER, 0);
}

// ======================================================
// DIRECTIE
// ======================================================

void left() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void right() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
}

void stopSteering() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

// ======================================================
// TRACTIUNE
// ======================================================

void forward() {
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void backward() {
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stopDrive() {
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// ======================================================
// DISTANCE
// ======================================================

void readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  distance = duration * 0.034 / 2.0;
}

// ======================================================
// GAME OVER — non-blocking state machine
// ======================================================

void updateGameOver() {
  if (!crashPlaying) return;

  unsigned long now = millis();

  // Toate notele terminate — pauza finala
  if (crashNoteIndex >= gameOverSize) {
    if (now - crashNoteStart >= CRASH_PAUSE) {
      crashPlaying   = false;
      crashNoteIndex = 0;
      buzzerOff();
    }
    return;
  }

  // Treci la nota urmatoare dupa CRASH_NOTE_DUR ms
  if (now - crashNoteStart >= CRASH_NOTE_DUR) {
    crashNoteStart = now;
    buzzerTone(gameOver[crashNoteIndex]);
    crashNoteIndex++;

    // Dupa ultima nota, opreste sunetul si incepe pauza
    if (crashNoteIndex >= gameOverSize) {
      buzzerOff();
    }
  }
}

void startGameOver() {
  if (crashPlaying) return;
  crashPlaying    = true;
  crashNoteIndex  = 0;
  crashNoteStart  = millis();
  stopDrive();
  buzzerTone(gameOver[0]);
  crashNoteIndex  = 1;
}

// ======================================================
// RADIO
// ======================================================

void playRadio() {
  if (crashPlaying) return;
  if (distance > 0 && distance < 70) {
    buzzerOff();
    return;
  }

  unsigned long now = millis();
  if (now - lastRadioNote > 250) {
    lastRadioNote = now;
    buzzerTone(melody[currentNote]);
    currentNote++;
    if (currentNote >= melodySize) currentNote = 0;
  }
}

// ======================================================
// PARKING BEEP
// ======================================================

void parkingBeep() {
  if (crashPlaying) return;
  if (distance <= 0 || distance >= 70) return;

  if (distance < 5) {
    startGameOver();
    return;
  }

  unsigned long now = millis();
  int interval;

  if      (distance < 15) interval = 120;
  else if (distance < 30) interval = 250;
  else                    interval = 600;

  if (now - lastBeep >= (unsigned long)interval) {
    lastBeep = now;
    buzzerTone(1200);
    delay(50);   // scurt — nu blocheaza semnificativ
    buzzerOff();
  }
}

// ======================================================
// WEB PAGE
// ======================================================

void handleRoot() {

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body{
  margin:0;
  touch-action:none;
  user-select:none;
  -webkit-user-select:none;
  -webkit-touch-callout:none;
  overflow:hidden;
  background:#0f172a;
  color:white;
  font-family:Arial;
  text-align:center;
}
h1{ margin-top:20px; font-size:32px; }
h2{ color:#38bdf8; }
.container{
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
  margin-top:20px;
}
.row{
  display:flex;
  justify-content:center;
  align-items:center;
}
.btn{
  width:110px;
  height:110px;
  margin:12px;
  border:none;
  border-radius:25px;
  font-size:22px;
  font-weight:bold;
  background:#2563eb;
  color:white;
  box-shadow:0 6px 20px rgba(0,0,0,0.35);
}
.btn:active{ transform:scale(0.95); background:#1d4ed8; }
</style>
</head>
<body>
<h1>ESP32 LEGO CAR</h1>
<h2 id="distance">Distanta: -- cm</h2>
<div class="container">
  <div class="row">
    <button class="btn"
      onmousedown="startForward()" onmouseup="stopDrive()" onmouseleave="stopDrive()"
      ontouchstart="event.preventDefault();startForward()"
      ontouchend="event.preventDefault();stopDrive()"
      ontouchcancel="stopDrive()">suuus</button>
  </div>
  <div class="row">
    <button class="btn"
      onmousedown="startLeft()" onmouseup="stopSteering()" onmouseleave="stopSteering()"
      ontouchstart="event.preventDefault();startLeft()"
      ontouchend="event.preventDefault();stopSteering()"
      ontouchcancel="stopSteering()">hais</button>
    <button class="btn" onclick="allStop()">gata ba</button>
    <button class="btn"
      onmousedown="startRight()" onmouseup="stopSteering()" onmouseleave="stopSteering()"
      ontouchstart="event.preventDefault();startRight()"
      ontouchend="event.preventDefault();stopSteering()"
      ontouchcancel="stopSteering()">cea</button>
  </div>
  <div class="row">
    <button class="btn"
      onmousedown="startBackward()" onmouseup="stopDrive()" onmouseleave="stopDrive()"
      ontouchstart="event.preventDefault();startBackward()"
      ontouchend="event.preventDefault();stopDrive()"
      ontouchcancel="stopDrive()">jooos</button>
  </div>
</div>
<script>
function startForward()  { fetch('/forward'); }
function startBackward() { fetch('/backward'); }
function startLeft()     { fetch('/left'); }
function startRight()    { fetch('/right'); }
function stopDrive()     { fetch('/stopDrive'); }
function stopSteering()  { fetch('/stopSteering'); }
function allStop()       { fetch('/stopDrive'); fetch('/stopSteering'); }

setInterval(async () => {
  try {
    const r = await fetch('/distance');
    const t = await r.text();
    document.getElementById('distance').innerHTML = 'Distanta: ' + t + ' cm';
  } catch(e) {}
}, 250);
</script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ======================================================
// SETUP
// ======================================================

void setup() {
  Serial.begin(115200);

  // motoare
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);

  // ultrasonic
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // buzzer — API core 2.x
  ledcAttach(BUZZER, LEDC_FREQ, LEDC_RES);

  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);

  stopDrive();
  stopSteering();

  // wifi
  WiFi.softAP(ssid, password);
  Serial.println("WiFi pornit!");
  Serial.println(WiFi.softAPIP());

  // rute
  server.on("/", handleRoot);

  server.on("/forward",  []() { forward();       server.send(200); });
  server.on("/backward", []() { backward();      server.send(200); });
  server.on("/stopDrive",[]() { stopDrive();     server.send(200); });

  server.on("/left",         []() { left();         server.send(200); });
  server.on("/right",        []() { right();        server.send(200); });
  server.on("/stopSteering", []() { stopSteering(); server.send(200); });

  server.on("/distance", []() {
    server.send(200, "text/plain", String(distance, 1));
  });

  server.begin();
  Serial.println("Server pornit!");
}

// ======================================================
// LOOP
// ======================================================

void loop() {
  server.handleClient();
  readDistance();
  updateGameOver();  // non-blocking game over
  playRadio();
  parkingBeep();
}
