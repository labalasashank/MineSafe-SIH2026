async function updateDashboard() {
  try {
    /*
      The current ESP32 program exposes the values at "/".
      To use this separate dashboard directly on the ESP32,
      the C++ code should provide a JSON endpoint such as /api.

      Expected JSON:
      {
        "status": "SAFE",
        "action": "MOVE",
        "front": 100,
        "left": 100,
        "right": 100,
        "fog": 0,
        "raw": 0,
        "baseline": 0,
        "ip": "192.168.x.x"
      }
    */

    const response = await fetch("/api");
    if (!response.ok) throw new Error("API unavailable");

    const data = await response.json();

    document.getElementById("status").textContent = data.status ?? "---";
    document.getElementById("action").textContent = data.action ?? "---";
    document.getElementById("front").textContent = formatDistance(data.front);
    document.getElementById("left").textContent = formatDistance(data.left);
    document.getElementById("right").textContent = formatDistance(data.right);

    const fog = Number(data.fog ?? 0);
    document.getElementById("fog").textContent = fog;
    document.getElementById("fogBar").style.width = `${Math.max(0, Math.min(100, fog))}%`;

    document.getElementById("raw").textContent = data.raw ?? "---";
    document.getElementById("baseline").textContent = data.baseline ?? "---";
    document.getElementById("ip").textContent = data.ip ?? "---";

    setStatusStyle(data.status);
    document.getElementById("mode").textContent =
      data.status === "DANGER" ? "EMERGENCY" :
      data.status === "WARNING" || data.status === "CAUTION" ? "ACTIVE SAFETY" :
      "MONITORING";

  } catch (error) {
    console.log("Dashboard waiting for ESP32 API:", error.message);
  }
}

function formatDistance(value) {
  if (value === undefined || value === null || Number(value) >= 999) return "---";
  return value;
}

function setStatusStyle(status) {
  const element = document.getElementById("status");
  element.classList.remove("status-warning", "status-danger");

  if (status === "DANGER") {
    element.classList.add("status-danger");
  } else if (status === "WARNING" || status === "CAUTION") {
    element.classList.add("status-warning");
  }
}

updateDashboard();
setInterval(updateDashboard, 1000);
