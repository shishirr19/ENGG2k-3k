const char MAIN_page[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>
<head>
  <title>Bridge Application Control Interface</title>
  <link rel="stylesheet" href="styles.css">
</head>
<body>

<h1>Bridge Application Control Interface</h1>

<div class="section">
  <h2>Bridge Status</h2>
  <p><span id="closed" class="status-light green active"></span> Bridge Closed</p> 
  <p><span id="open" class="status-light red"></span> Bridge Open</p>
  <p><span id="moving" class="status-light yellow"></span> Bridge Moving</p>
</div>

<!--BRIDGE CONTROL STATES-->
<div class="section">
  <h2>Bridge Control States</h2>
  <button onclick="OpenBridge()" style="background-color: hsl(125, 83%, 47%);">Open Bridge</button>
  <button onclick="CloseBridge()" style="background-color: hsl(125, 83%, 47%);">Close Bridge</button>
  <button onclick="EmergencyStop()" style="background-color: hsl(0, 100%, 62%);">Emergency Stop</button>
  <button onclick="ResetState()" style="background-color: hsl(45, 17%, 48%);">Reset State</button>
</div>

<!-- MOTOR SPEED SLIDER -->
<div class="section">
  <h2>Motor Speed Control</h2>
  <label for="speedSlider">Motor Speed (0-255):</label>
  <input type="range" id="speedSlider" min="0" max="255" value="100" step="1">
  <span id="speedValue">100</span>
</div>

<!-- ESP32 TESTING STATES-->
<div class="section">
  <h2>Bridge Testing States</h2>
  <button onclick="startMotor()" style="background-color: rgb(189, 222, 169);">Motor Start (Up)</button>
  <button onclick="motorDown()" style="background-color: rgb(189, 222, 169);">Motor Down</button>
  <button onclick="stopMotor()" style="background-color: rgb(189, 222, 169);">Motor Stop</button>
  <button onclick="ledOn()" style="background-color: rgb(189, 222, 169);">LED On (Red)</button>
  <button onclick="ledOff()" style="background-color: rgb(189, 222, 169);">LED Off (Green)</button>
  //<button onclick="checkLimits()" style="background-color: rgb(189, 222, 169);">Check Limit Switches</button>
  <button onclick="getUltrasonic1()" style="background-color: rgb(189, 222, 169);">Ultrasonic 1</button>
  <button onclick="getUltrasonic2()" style="background-color: rgb(189, 222, 169);">Ultrasonic 2</button>
  <button onclick="getUltrasonic3()" style="background-color: rgb(189, 222, 169);">Ultrasonic 3</button>
</div>

<div class="section"> <!--Should eventually invoke a response from the ESP, todo-->
  <h2>Sensors</h2>
  <p>Boat Detected: <span id="boatDetected" class="status-light green"></span></p>
  <p>Car Detected: <span id="carDetected" class="status-light red"></span></p>
  <p>Pedestrian Detected: <span id="pedDetected" class="status-light yellow"></span></p>
</div>

<div class="section">
  <h2>Bridge Animation Status</h2>
  <div class="bridge-area">
    <div class="bridge-road" id="bridge"></div>
    <div class="boat"></div>
    <div class="river"></div>
  </div>
</div>

<script>
  const ESP32_IP = "http://192.168.4.1";

  // ---- Bridge Control Functions ----
  function OpenBridge() {
    fetch(`${ESP32_IP}/motor/up`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));

    // UI updates
    document.getElementById("bridge").classList.add("open");
    document.getElementById("open").classList.add("active");
    document.getElementById("closed").classList.remove("active");
    let moving = document.getElementById("moving");
    moving.classList.add("flashing");
    setTimeout(() => {
      moving.classList.remove("flashing");
    }, 3000); // Simulate movement time
  }

  function CloseBridge() {
    fetch(`${ESP32_IP}/motor/down`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));

    
    document.getElementById("bridge").classList.remove("open");
    document.getElementById("closed").classList.add("active");
    document.getElementById("open").classList.remove("active");
    let moving = document.getElementById("moving");
    moving.classList.add("flashing");
    setTimeout(() => {
      moving.classList.remove("flashing");
    }, 3000); // Simulate movement time
  }

  function EmergencyStop() {
    fetch(`${ESP32_IP}/motor/stop`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));

    // UI updates (assume stops in current position, but reset to closed for simplicity)
    document.getElementById("moving").classList.remove("flashing");
  }

  function ResetState() {
    fetch(`${ESP32_IP}/motor/stop`)
      .then(r => r.text())
      .then(() => {
        // Reset UI
        document.getElementById("bridge").classList.remove("open");
        document.getElementById("closed").classList.add("active");
        document.getElementById("open").classList.remove("active");
        document.getElementById("moving").classList.remove("flashing");
      })
      .catch(err => console.error("Error:", err));
  }

  // ---- Motor Speed Slider ----
  const speedSlider = document.getElementById("speedSlider");
  const speedValue = document.getElementById("speedValue");
  speedSlider.addEventListener("input", () => {
    speedValue.textContent = speedSlider.value;
    fetch(`${ESP32_IP}/motor/speed?value=${speedSlider.value}`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));
  });


  function ledOn() {
    fetch(`${ESP32_IP}/led/on`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));
  }

  function ledOff() {
    fetch(`${ESP32_IP}/led/off`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));
  }

  function startMotor() {
    fetch(`${ESP32_IP}/motor/up`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));
  }

  function motorDown() {
    fetch(`${ESP32_IP}/motor/down`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));
  }

  function stopMotor() {
    fetch(`${ESP32_IP}/motor/stop`)
      .then(r => r.text())
      .then(console.log)
      .catch(err => console.error("Error:", err));

    // Reset bridge status
    document.getElementById("moving").classList.remove("flashing");
  }

  function getUltrasonic1() { fetch(`${ESP32_IP}/ultrasonic/1`).then(r => r.json()).then(data => alert(`Distance: ${data.distance_cm} cm`)).catch(err => console.error("Error:", err)); }
  function getUltrasonic2() { fetch(`${ESP32_IP}/ultrasonic/2`).then(r => r.json()).then(data => alert(`Distance: ${data.distance_cm} cm`)).catch(err => console.error("Error:", err)); }
  function getUltrasonic3() { fetch(`${ESP32_IP}/ultrasonic/3`).then(r => r.json()).then(data => alert(`Distance: ${data.distance_cm} cm`)).catch(err => console.error("Error:", err)); }
</script>

</body> 
</html>
)rawliteral"; 
