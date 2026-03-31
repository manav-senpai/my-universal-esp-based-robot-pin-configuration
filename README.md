<div align="center">
  <h1>🤖 PROJECT ARCEE: The Universal ESP32 Core</h1>
  <p><strong>A battle-ready, modular robotics platform built for speed and expansion.</strong></p>
</div>

---

## ⚔️ The Vision
Arcee ain't just a single robot; she's a foundation. This repository documents the **Master Standard ESP32 Pinout** for a high-performance tracked chassis. By establishing a universal configuration, any custom remote or future attachment (robotic arms, sensor arrays, or audio modules) can be swapped seamlessly without rewriting the core architecture.

**Author:** Manav Ahire  
**Date:** March 2026  

> ⚠️ **CRITICAL WARNING:** Always test screw terminals with a multimeter before plugging in data lines. Mixing 12V battery power with 3.3V ESP32 logic will instantly destroy the microcontroller. **Common Ground (GND) must be shared across all components.**

---

## ⚡ Power Block (Heavy Duty)
Using a 4-pin screw terminal block for rugged, slip-free power distribution.

| Terminal | Voltage | Source / Purpose |
| :--- | :--- | :--- |
| **1** | `12V` | Direct from BMS / 18650 Pack (High current for motors) |
| **2** | `5V` | From Buck Converter (For logic and standard sensors) |
| **3** | `3.3V` | From ESP32 / Step-down (For I2C and delicate modules) |
| **4** | `GND` | Common Earth |

---

## ⚙️ The Base Build (Hardwired Core)
The permanent nervous system of the chassis. These pins drive the BTS7960 motor controllers, state LEDs, and lighting triggers.

### Drivetrain (Tank Controls)
| Component | Function | ESP32 GPIO |
| :--- | :--- | :--- |
| **Left Motor (BTS7960)** | RPWM (Forward) | `GPIO 32` |
| | LPWM (Reverse) | `GPIO 33` |
| **Right Motor (BTS7960)** | RPWM (Forward) | `GPIO 27` |
| | LPWM (Reverse) | `GPIO 14` |

### Core Controls & Lighting
| Component | Function | ESP32 GPIO |
| :--- | :--- | :--- |
| **System OK** | Status LED 1 | `GPIO 16` |
| **ESP-NOW Link** | Status LED 2 | `GPIO 17` |
| **Headlights** | NPN Transistor Trigger | `GPIO 18` |
| **Taillights** | NPN Transistor Trigger | `GPIO 19` |
| **Mode Switch** | Input w/ Pullup | `GPIO 13` |

---

## 🔊 Front Expansion Bay (Audio)
Exposed female headers dedicated to the internal DACs for true stereo sound. Designed for a modular PAM8403 amplifier and 2W speaker attachment.

| Channel | Function | ESP32 GPIO |
| :--- | :--- | :--- |
| **Left Audio** | DAC 1 | `GPIO 25` |
| **Right Audio** | DAC 2 | `GPIO 26` |

---

## 📡 Rear Expansion Bay (Data & Sensors)
A 10-pin female header block at the rear of the chassis for hot-swapping attachments like servo-driven storage boxes, robotic arms, or IR/Ultrasonic arrays.

### I2C Communication (OLEDs, Gyros)
* **SDA:** `GPIO 21`
* **SCL:** `GPIO 22`

### General Output / PWM (Servos, Arms, Triggers)
* **IO A:** `GPIO 4`
* **IO B:** `GPIO 5`
* **IO C:** `GPIO 23`
* **IO D:** `GPIO 15` *(Boot strapping pin: Must remain floating at startup)*

### Input Only (Analog Sensors, IR)
* **IN W:** `GPIO 34`
* **IN X:** `GPIO 35`
* **IN Y:** `GPIO 36 (VP)`
* **IN Z:** `GPIO 39 (VN)`

---
<div align="center">
  <p><em>"Winter is coming, but Arcee is ready."</em></p>
</div>
