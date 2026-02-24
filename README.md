# ESP8266 Humanoid Robot Controller

A real-time WiFi humanoid robot controller built with ESP8266, servo motors, and Node-RED.

This project allows wireless control of a 16-servo humanoid robot, motion recording, pose storage, and real-time position adjustment over TCP.

---

## Features

- Real-time servo control over WiFi
- TCP communication with Node-RED
- 16 servo motor control
- Motion recording and playback
- Adjustable speed and acceleration
- PID configuration support
- Stand position initialization
- Expandable robot control architecture

---

## Hardware Requirements

- ESP8266 board
- 16 servo humanoid robot platform
- Servo controller compatible with RoboBuilder protocol
- Power supply for servos

---

## Software Requirements

- Arduino IDE
- ESP8266 board package
- Node-RED (optional for UI control)

---

## Architecture


Node-RED → TCP → ESP8266 → Servo Controller → Robot


ESP8266 handles:

- real-time motor control
- motion execution
- communication

Node-RED handles:

- UI dashboard
- command generation
- pose storage
- automation logic

---

## Commands

### Show help

help


### Stand position

stand


### Adjust servo position

[ID] +[value]
[ID] -[value]


Example:


5 +20
10 -30


Servo IDs range from 0 to 15.

---

## Project Structure


/src → ESP8266 firmware
/docs → Documentation
/node-red → Node-RED flows
/examples → Example motions


---

## Future Improvements

- Smooth motion interpolation
- Servo calibration system
- Motion recording system
- MQTT support
- Web dashboard
- Sensor feedback
- Motion sequences

---

## License

MIT License