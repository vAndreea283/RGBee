#include <WiFi.h>
#include <WebServer.h>

const char* ssid     = "ESP32_CAR";
const char* password = "12345678";

WebServer server(80);

// ======================================================
// MOTOR FATA (DIRECTIE)
// ======================================================

int IN3 = 25;
int IN4 = 33;
int ENB = 32;

// ======================================================
// MOTOR SPATE (TRACTIUNE)
// ======================================================

int IN1 = 27;
int IN2 = 26;
int ENA = 14;

// ======================================================
// ULTRASONIC
// ======================================================

#define TRIG 18
#define ECHO 19

// ======================================================
// BUZZER
// ======================================================

#define BUZZER    23
#define LEDC_FREQ 2000
#define LEDC_RES  8

float distance = 0;

// ======================================================
// NOTE DEFINITIONS
// ======================================================

#define REST      0
#define NOTE_C4   262
#define NOTE_CS4  277
#define NOTE_D4   294
#define NOTE_DS4  311
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_FS4  370
#define NOTE_G4   392
#define NOTE_GS4  415
#define NOTE_A4   440
#define NOTE_AS4  466
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_CS5  554
#define NOTE_D5   587
#define NOTE_DS5  622
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_FS5  740
#define NOTE_G5   784
#define NOTE_GS5  831
#define NOTE_A5   880
#define NOTE_AS5  932
#define NOTE_B5   988

// ======================================================
// SUPER MARIO THEME — Robson Couto (robsoncouto/arduino-songs)
// Format: nota, durata  (4=patrime, 8=optime, -4=cu punct, etc.)
// ======================================================

#define TEMPO 200

int marioMelody[] = {
  NOTE_E5,8,  NOTE_E5,8,  REST,8,     NOTE_E5,8,
  REST,8,     NOTE_C5,8,  NOTE_E5,8,  REST,8,
  NOTE_G5,4,  REST,4,     NOTE_G4,8,  REST,4,

  NOTE_C5,-4, NOTE_G4,8,  REST,4,     NOTE_E4,-4,
  NOTE_A4,4,  NOTE_B4,4,  NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4,  NOTE_F5,8,  NOTE_G5,8,
  REST,8,     NOTE_E5,4,  NOTE_C5,8,  NOTE_D5,8,  NOTE_B4,-4,

  NOTE_C5,-4, NOTE_G4,8,  REST,4,     NOTE_E4,-4,
  NOTE_A4,4,  NOTE_B4,4,  NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4,  NOTE_F5,8,  NOTE_G5,8,
  REST,8,     NOTE_E5,4,  NOTE_C5,8,  NOTE_D5,8,  NOTE_B4,-4,

  REST,4,     NOTE_G5,8,  NOTE_FS5,8, NOTE_F5,8,  NOTE_DS5,4, NOTE_E5,8,
  REST,8,     NOTE_GS4,8, NOTE_A4,8,  NOTE_C5,8,  REST,8,     NOTE_A4,8,  NOTE_C5,8, NOTE_D5,8,
  REST,4,     NOTE_DS5,4, REST,8,     NOTE_D5,-4,
  NOTE_C5,2,  REST,2,

  REST,4,     NOTE_G5,8,  NOTE_FS5,8, NOTE_F5,8,  NOTE_DS5,4, NOTE_E5,8,
  REST,8,     NOTE_GS4,8, NOTE_A4,8,  NOTE_C5,8,  REST,8,     NOTE_A4,8,  NOTE_C5,8, NOTE_D5,8,
  REST,4,     NOTE_DS5,4, REST,8,     NOTE_D5,-4,
  NOTE_C5,2,  REST,2,

  NOTE_C5,8,  NOTE_C5,4,  NOTE_C5,8,  REST,8,     NOTE_C5,8,  NOTE_D5,4,
  NOTE_E5,8,  NOTE_C5,4,  NOTE_A4,8,  NOTE_G4,2,

  NOTE_C5,8,  NOTE_C5,4,  NOTE_C5,8,  REST,8,     NOTE_C5,8,  NOTE_D5,8,  NOTE_E5,8,
  REST,1,

  NOTE_C5,8,  NOTE_C5,4,  NOTE_C5,8,  REST,8,     NOTE_C5,8,  NOTE_D5,4,
  NOTE_E5,8,  NOTE_C5,4,  NOTE_A4,8,  NOTE_G4,2,

  NOTE_E5,8,  NOTE_E5,8,  REST,8,     NOTE_E5,8,  REST,8,     NOTE_C5,8,  NOTE_E5,4,
  NOTE_G5,4,  REST,4,     NOTE_G4,4,  REST,4,

  NOTE_C5,-4, NOTE_G4,8,  REST,4,     NOTE_E4,-4,
  NOTE_A4,4,  NOTE_B4,4,  NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4,  NOTE_F5,8,  NOTE_G5,8,
  REST,8,     NOTE_E5,4,  NOTE_C5,8,  NOTE_D5,8,  NOTE_B4,-4,

  NOTE_C5,-4, NOTE_G4,8,  REST,4,     NOTE_E4,-4,
  NOTE_A4,4,  NOTE_B4,4,  NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4,  NOTE_F5,8,  NOTE_G5,8,
  REST,8,     NOTE_E5,4,  NOTE_C5,8,  NOTE_D5,8,  NOTE_B4,-4,
};

int marioNotes = sizeof(marioMelody) / sizeof(marioMelody[0]) / 2;

// ======================================================
// GAME OVER SOUND
// ======================================================

int gameOver[]   = { 300, 600, 900, 1200, 800, 400 };
int gameOverSize = 6;

// ======================================================
// STATE
// ======================================================

// Mario state machine
int           marioNote     = 0;
unsigned long marioNextTime = 0;
bool          marioSilent   = false; // true in timpul beep-ului + pauza

// Beep / parking
unsigned long lastBeep      = 0;
unsigned long lastBeepTime  = 0;
bool          beepActive    = false;
#define       BEEP_PAUSE_MS 2000

// Game over state machine
bool          crashPlaying   = false;
int           crashNoteIndex = 0;
unsigned long crashNoteStart = 0;
#define       CRASH_NOTE_DUR 180
#define       CRASH_PAUSE    600

// ======================================================
// BUZZER HELPERS
// ======================================================

void buzzerTone(int freq) {
  if (freq <= 0) {
    ledcWrite(BUZZER, 0);
  } else {
    ledcWriteTone(BUZZER, freq);
    ledcWrite(BUZZER, 128);
  }
}

void buzzerOff() {
  ledcWrite(BUZZER, 0);
}

// ======================================================
// DIRECTIE — IN3/IN4 (OUT3/OUT4 pe driver)
// ======================================================

void left()         { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
void right()        { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
void stopSteering() { digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);  }

// ======================================================
// TRACTIUNE — IN1/IN2 (OUT1/OUT2 pe driver)
// ======================================================

void forward()   { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
void backward()  { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
void stopDrive() { digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);  }

// ======================================================
// DISTANCE
// ======================================================

void readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long dur = pulseIn(ECHO, HIGH, 30000);
  distance = dur * 0.034 / 2.0;
}

// ======================================================
// GAME OVER — non-blocking state machine
// ======================================================

void startGameOver() {
  if (crashPlaying) return;
  crashPlaying   = true;
  crashNoteIndex = 0;
  crashNoteStart = millis();
  stopDrive();
  buzzerTone(gameOver[0]);
  crashNoteIndex = 1;
}

void updateGameOver() {
  if (!crashPlaying) return;
  unsigned long now = millis();

  if (crashNoteIndex >= gameOverSize) {
    if (now - crashNoteStart >= CRASH_PAUSE) {
      crashPlaying   = false;
      crashNoteIndex = 0;
      buzzerOff();
      marioNote     = 0;
      marioNextTime = now;
      marioSilent   = false;
    }
    return;
  }

  if (now - crashNoteStart >= CRASH_NOTE_DUR) {
    crashNoteStart = now;
    buzzerTone(gameOver[crashNoteIndex]);
    crashNoteIndex++;
    if (crashNoteIndex >= gameOverSize) buzzerOff();
  }
}

// ======================================================
// PARKING BEEP — non-blocking
// ======================================================

void parkingBeep() {
  if (crashPlaying) return;

  // Nu mai e obstacol
  if (distance <= 0 || distance >= 70) {
    if (beepActive) {
      beepActive   = false;
      lastBeepTime = millis(); // start cronometru pauza muzica
      buzzerOff();
    }
    return;
  }

  // Crash!
  if (distance < 5) {
    startGameOver();
    return;
  }

  // Beep activ
  beepActive = true;

  unsigned long now = millis();
  int interval;
  if      (distance < 15) interval = 120;
  else if (distance < 30) interval = 250;
  else                    interval = 600;

  if (now - lastBeep >= (unsigned long)interval) {
    lastBeep     = now;
    lastBeepTime = now;
    buzzerTone(1200);
    delay(50);
    buzzerOff();
  }
}

// ======================================================
// SUPER MARIO — non-blocking state machine
// Porneste doar daca nu e beep activ si au trecut 2s de la ultimul beep
// ======================================================

void updateMario() {
  if (crashPlaying) return;
  if (beepActive)   return;

  unsigned long now = millis();

  // Pauza de 2 secunde dupa ultimul beep
  if (now - lastBeepTime < BEEP_PAUSE_MS) return;
  if (now < marioNextTime) return;

  int wholenote = (60000 * 4) / TEMPO;
  int note    = marioMelody[marioNote * 2];
  int divider = marioMelody[marioNote * 2 + 1];

  int noteDur;
  if (divider > 0) {
    noteDur = wholenote / divider;
  } else {
    noteDur = (int)((wholenote / abs(divider)) * 1.5);
  }

  if (note == REST) {
    buzzerOff();
  } else {
    buzzerTone(note);
  }

  // Urmatoarea nota dupa 90% din durata (restul de 10% = pauza naturala intre note)
  marioNextTime = now + (unsigned long)(noteDur * 0.9);
  marioNote++;
  if (marioNote >= marioNotes) marioNote = 0;
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
      ontouchcancel="stopDrive()">inainte</button>
  </div>
  <div class="row">
    <button class="btn"
      onmousedown="startLeft()" onmouseup="stopSteering()" onmouseleave="stopSteering()"
      ontouchstart="event.preventDefault();startLeft()"
      ontouchend="event.preventDefault();stopSteering()"
      ontouchcancel="stopSteering()">stanga</button>
    <button class="btn"
      onmousedown="startRight()" onmouseup="stopSteering()" onmouseleave="stopSteering()"
      ontouchstart="event.preventDefault();startRight()"
      ontouchend="event.preventDefault();stopSteering()"
      ontouchcancel="stopSteering()">dreapta</button>
  </div>
  <div class="row">
    <button class="btn"
      onmousedown="startBackward()" onmouseup="stopDrive()" onmouseleave="stopDrive()"
      ontouchstart="event.preventDefault();startBackward()"
      ontouchend="event.preventDefault();stopDrive()"
      ontouchcancel="stopDrive()">inapoi</button>
  </div>
</div>
<script>
function startForward()  { fetch('/forward'); }
function startBackward() { fetch('/backward'); }
function startLeft()     { fetch('/left'); }
function startRight()    { fetch('/right'); }
function stopDrive()     { fetch('/stopDrive'); }
function stopSteering()  { fetch('/stopSteering'); }

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

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // buzzer — core 3.x API
  ledcAttach(BUZZER, LEDC_FREQ, LEDC_RES);

  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);

  stopDrive();
  stopSteering();

  WiFi.softAP(ssid, password);
  Serial.println("WiFi pornit!");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/forward",      []() { forward();      server.send(200); });
  server.on("/backward",     []() { backward();     server.send(200); });
  server.on("/stopDrive",    []() { stopDrive();    server.send(200); });
  server.on("/left",         []() { left();         server.send(200); });
  server.on("/right",        []() { right();        server.send(200); });
  server.on("/stopSteering", []() { stopSteering(); server.send(200); });
  server.on("/distance",     []() {
    server.send(200, "text/plain", String(distance, 1));
  });

  server.begin();
  Serial.println("Server pornit!");

  lastBeepTime  = 0;
  marioNextTime = millis();
}

// ======================================================
// LOOP
// ======================================================

void loop() {
  server.handleClient();
  readDistance();
  updateGameOver();
  parkingBeep();
  updateMario();
}
