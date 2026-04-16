#include <WiFi.h>
#include <WebServer.h>

// --- T' HOTSPOT CREDENTIALS ---
const char* ssid = "ARCEE";
const char* password = "transform";

WebServer server(80);

// --- T' HARDWARE PINS ---
// Motor Pins
#define L_FWD 32
#define L_REV 33
#define R_FWD 27
#define R_REV 14

// Light Pins
#define HEADLIGHTS 18
#define TAILLIGHTS 19

// --- STATE VARIABLES ---
bool headState = false;
bool tailState = false;
bool sirenMode = false;

// Siren Timing Variables
unsigned long lastSirenTime = 0;
int sirenStep = 0;

// --- T' HTML & JS CONTROLLER INTERFACE ---
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Arcee Command Center</title>
  <style>
    /* ARMOR AGAINST TEXT SELECTION AND LONG PRESS MENUS */
    body {
      margin: 0; padding: 0; background-color: #111; color: white;
      font-family: monospace; overflow: hidden;
      -webkit-touch-callout: none;
      -webkit-user-select: none;
      -khtml-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
      user-select: none;
      touch-action: none; /* Stops pull-to-refresh and panning */
    }
    
    /* TOP BAR */
    .top-bar { position: absolute; top: 0; left: 0; width: 100%; height: 60px; background: #222; display: flex; justify-content: space-around; align-items: center; border-bottom: 2px solid #444; z-index: 10;}
    .toggle-btn { padding: 10px 20px; font-size: 16px; font-weight: bold; border-radius: 5px; border: 2px solid #555; background: #333; color: white; }
    .toggle-btn.active { background: #ff9800; border-color: #ffc107; color: black; }
    .siren-btn { border-color: #f44336; }
    .siren-btn.active { background: #f44336; color: white; animation: pulse 0.5s infinite; }
    @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }

    /* LAYOUT GRID */
    .container { position: absolute; top: 60px; bottom: 0; width: 100%; display: flex; }
    
    /* LEFT D-PAD AREA */
    .left-panel { width: 50%; position: relative; border-right: 2px solid #333; }
    .d-pad { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 220px; height: 220px; }
    .dir-btn { position: absolute; width: 60px; height: 60px; background: #444; border: 2px solid #666; border-radius: 10px; font-size: 24px; font-weight: bold; color: white; text-align: center; line-height: 60px; }
    .dir-btn:active { background: #4CAF50; }
    #btnF { top: 0; left: 80px; }
    #btnR { bottom: 0; left: 80px; }
    #btnL { top: 80px; left: 0; }
    #btnRt { top: 80px; right: 0; }
    
    /* SPEED CONTROLS (Center of D-Pad) */
    .speed-panel { position: absolute; top: 60px; left: 70px; width: 80px; height: 100px; text-align: center; }
    .spd-btn { width: 35px; height: 35px; background: #222; border: 1px solid #888; color: white; font-size: 20px; border-radius: 5px; margin-bottom: 5px; }
    .spd-btn:active { background: #888; }
    #spd-val { font-size: 18px; font-weight: bold; color: #03a9f4; }

    /* RIGHT JOYSTICK AREA */
    .right-panel { width: 50%; position: relative; background: radial-gradient(circle, #222 0%, #111 100%); }
    #joy-thumb { position: absolute; width: 50px; height: 50px; background: rgba(3, 169, 244, 0.7); border-radius: 50%; pointer-events: none; display: none; transform: translate(-50%, -50%); box-shadow: 0 0 15px #03a9f4; }
  </style>
</head>
<body oncontextmenu="return false;">

  <div class="top-bar">
    <button id="btnHead" class="toggle-btn" onclick="toggleLight('head')">HEADLIGHT</button>
    <button id="btnTail" class="toggle-btn" onclick="toggleLight('tail')">TAILLIGHT</button>
    <button id="btnSiren" class="toggle-btn siren-btn" onclick="toggleSiren()">SIREN MODE</button>
  </div>

  <div class="container">
    <div class="left-panel">
      <div class="d-pad">
        <div id="btnF" class="dir-btn">F</div>
        <div id="btnL" class="dir-btn">L</div>
        
        <div class="speed-panel">
          <button class="spd-btn" onclick="chgSpd(20)">+</button>
          <div id="spd-val">60%</div>
          <button class="spd-btn" onclick="chgSpd(-20)">-</button>
        </div>

        <div id="btnRt" class="dir-btn">R</div>
        <div id="btnR" class="dir-btn">B</div>
      </div>
    </div>

    <div class="right-panel" id="joyArea">
      <div id="joy-thumb"></div>
    </div>
  </div>

  <script>
    // --- THROTTLE & COMM LOGIC ---
    let lastSend = 0;
    function sendReq(url) {
      let now = Date.now();
      if (now - lastSend > 40) { // Limit to ~25 times a second so ESP doesn't crash
        fetch(url).catch(e => console.log(e));
        lastSend = now;
      }
    }
    // Force send for crucial stops
    function sendReqForce(url) { fetch(url).catch(e => console.log(e)); }

    // --- TOGGLES ---
    let hOn = false; let tOn = false; let sOn = false;
    function toggleLight(type) {
      if(type === 'head') { hOn = !hOn; document.getElementById('btnHead').classList.toggle('active', hOn); sendReqForce('/light?t=head&s=' + (hOn?1:0)); }
      if(type === 'tail') { tOn = !tOn; document.getElementById('btnTail').classList.toggle('active', tOn); sendReqForce('/light?t=tail&s=' + (tOn?1:0)); }
    }
    function toggleSiren() {
      sOn = !sOn;
      document.getElementById('btnSiren').classList.toggle('active', sOn);
      sendReqForce('/siren?s=' + (sOn?1:0));
    }

    // --- SPEED LOGIC (20% to 100%) ---
    let speedPcnt = 60;
    function chgSpd(val) {
      speedPcnt += val;
      if (speedPcnt > 100) speedPcnt = 100;
      if (speedPcnt < 20) speedPcnt = 20;
      document.getElementById('spd-val').innerText = speedPcnt + '%';
    }

    // --- D-PAD LOGIC (MOMENTARY) ---
    const dirs = ['btnF', 'btnR', 'btnL', 'btnRt'];
    const cmds = ['fwd', 'rev', 'left', 'right'];
    for(let i=0; i<4; i++) {
      let btn = document.getElementById(dirs[i]);
      btn.addEventListener('touchstart', (e) => { e.preventDefault(); let pwm = Math.floor((speedPcnt/100)*255); sendReqForce('/dpad?cmd=' + cmds[i] + '&spd=' + pwm); });
      btn.addEventListener('touchend', (e) => { e.preventDefault(); sendReqForce('/stop'); });
      btn.addEventListener('touchcancel', (e) => { e.preventDefault(); sendReqForce('/stop'); });
    }

    // --- JOYSTICK LOGIC (PROPORTIONAL) ---
    const joyArea = document.getElementById('joyArea');
    const thumb = document.getElementById('joy-thumb');
    let jActive = false; let sX = 0; let sY = 0;
    const maxR = 100; // Max drag distance

    joyArea.addEventListener('touchstart', (e) => {
      e.preventDefault(); jActive = true;
      sX = e.touches[0].clientX; sY = e.touches[0].clientY;
      thumb.style.left = (sX - joyArea.getBoundingClientRect().left) + 'px';
      thumb.style.top = sY + 'px'; // Top bar is absolute, so clientY works roughly
      thumb.style.display = 'block';
    });

    joyArea.addEventListener('touchmove', (e) => {
      if(!jActive) return;
      e.preventDefault();
      let cX = e.touches[0].clientX; let cY = e.touches[0].clientY;
      
      let dx = cX - sX; let dy = cY - sY;
      let dist = Math.sqrt(dx*dx + dy*dy);
      
      // Limit visual thumb
      if(dist > maxR) { dx = (dx/dist)*maxR; dy = (dy/dist)*maxR; }
      thumb.style.left = (sX + dx - joyArea.getBoundingClientRect().left) + 'px';
      thumb.style.top = (sY + dy - 60) + 'px'; // Offset top bar

      // Math for differential drive
      // Invert Y so up is positive
      let joyX = dx / maxR; // -1 to 1
      let joyY = -dy / maxR; // -1 to 1
      
      // Mix them
      let left = (joyY + joyX) * 255;
      let right = (joyY - joyX) * 255;
      
      // Constrain
      left = Math.max(-255, Math.min(255, left));
      right = Math.max(-255, Math.min(255, right));

      sendReq(`/joy?l=${Math.round(left)}&r=${Math.round(right)}`);
    });

    function joyStop(e) {
      if(!jActive) return;
      e.preventDefault(); jActive = false;
      thumb.style.display = 'none';
      sendReqForce('/stop');
    }
    joyArea.addEventListener('touchend', joyStop);
    joyArea.addEventListener('touchcancel', joyStop);

  </script>
</body>
</html>
)rawliteral";

// --- MOTOR CONTROL HELPER (V3.0.0 API) ---
void setMotors(int leftPWM, int rightPWM) {
  // Left Track
  if (leftPWM > 0) {
    ledcWrite(L_FWD, leftPWM);
    ledcWrite(L_REV, 0);
  } else {
    ledcWrite(L_FWD, 0);
    ledcWrite(L_REV, -leftPWM);
  }

  // Right Track
  if (rightPWM > 0) {
    ledcWrite(R_FWD, rightPWM);
    ledcWrite(R_REV, 0);
  } else {
    ledcWrite(R_FWD, 0);
    ledcWrite(R_REV, -rightPWM);
  }
}

// --- WEB SERVER ROUTES ---
void handleRoot() { server.send(200, "text/html", htmlPage); }

void handleStop() {
  setMotors(0, 0);
  server.send(200, "text/plain", "STOP");
}

void handleDPad() {
  String cmd = server.arg("cmd");
  int spd = server.arg("spd").toInt();
  
  if (cmd == "fwd") setMotors(spd, spd);
  else if (cmd == "rev") setMotors(-spd, -spd);
  else if (cmd == "left") setMotors(-spd, spd);
  else if (cmd == "right") setMotors(spd, -spd);
  
  server.send(200, "text/plain", "OK");
}

void handleJoy() {
  int l = server.arg("l").toInt();
  int r = server.arg("r").toInt();
  setMotors(l, r);
  server.send(200, "text/plain", "OK");
}

void handleLight() {
  String t = server.arg("t");
  int s = server.arg("s").toInt();
  if (t == "head") headState = (s == 1);
  if (t == "tail") tailState = (s == 1);
  
  if(!sirenMode) {
    digitalWrite(HEADLIGHTS, headState);
    digitalWrite(TAILLIGHTS, tailState);
  }
  server.send(200, "text/plain", "OK");
}

void handleSiren() {
  sirenMode = (server.arg("s") == "1");
  sirenStep = 0; // Reset sequence
  if(!sirenMode) {
    // Restore normal lights immediately when turned off
    digitalWrite(HEADLIGHTS, headState);
    digitalWrite(TAILLIGHTS, tailState);
  }
  server.send(200, "text/plain", "OK");
}

// --- MAIN SETUP ---
void setup() {
  Serial.begin(115200);

  // Setup Lights
  pinMode(HEADLIGHTS, OUTPUT);
  pinMode(TAILLIGHTS, OUTPUT);
  digitalWrite(HEADLIGHTS, LOW);
  digitalWrite(TAILLIGHTS, LOW);

  // Setup PWM Channels using new Core v3.0 API
  ledcAttach(L_FWD, 2000, 8);
  ledcAttach(L_REV, 2000, 8);
  ledcAttach(R_FWD, 2000, 8);
  ledcAttach(R_REV, 2000, 8);
  setMotors(0, 0);

  // Create t' Arcee Hotspot
  Serial.println("Forging Hotspot...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Arcee Command Center Online at: ");
  Serial.println(IP);

  // Bind server routes
  server.on("/", handleRoot);
  server.on("/dpad", handleDPad);
  server.on("/joy", handleJoy);
  server.on("/stop", handleStop);
  server.on("/light", handleLight);
  server.on("/siren", handleSiren);

  server.begin();
}

// --- MAIN LOOP ---
void loop() {
  server.handleClient(); // Listen for t' web commands

  // --- NON-BLOCKING SIREN LOGIC ---
  if (sirenMode) {
    unsigned long currentMillis = millis();
    // Flash every 120 milliseconds
    if (currentMillis - lastSirenTime >= 120) {
      lastSirenTime = currentMillis;
      
      // Sequence: 0-5 Head flashes, 6-11 Tail flashes
      if (sirenStep < 6) {
        // Headlights flash on evens (0,2,4), off on odds (1,3,5)
        digitalWrite(HEADLIGHTS, (sirenStep % 2 == 0) ? HIGH : LOW);
        digitalWrite(TAILLIGHTS, LOW);
      } else {
        // Taillights flash on evens (6,8,10), off on odds (7,9,11)
        digitalWrite(HEADLIGHTS, LOW);
        digitalWrite(TAILLIGHTS, (sirenStep % 2 == 0) ? HIGH : LOW);
      }

      sirenStep++;
      if (sirenStep >= 12) {
        sirenStep = 0; // Loop t' sequence back
      }
    }
  }
}
