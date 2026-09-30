# HAUL SENTINEL Dashboard

## Structure

Dashboard/
├── index.html
├── style.css
├── script.js
└── assets/

## Important

Your original ESP32 C++ code currently creates the HTML page directly inside
`handleRoot()` using `page += ...` statements.

This folder separates the frontend into HTML, CSS and JavaScript.

For the ESP32 to serve these files, the C++ program must be changed to use
LittleFS or SPIFFS and serve the files from flash.

The `script.js` expects a JSON endpoint:

GET /api

Example response:
{
  "status": "SAFE",
  "action": "MOVE",
  "front": 100,
  "left": 100,
  "right": 100,
  "fog": 0,
  "raw": 500,
  "baseline": 500,
  "ip": "192.168.1.10"
}

Your existing sensor and motor logic remains in the ESP32 C++ program.
