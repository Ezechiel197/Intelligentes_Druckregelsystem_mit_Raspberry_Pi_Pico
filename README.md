## :sparkles:**Function**
This project focuses on reading pressure data using a dedicated sensor and displaying it graphically via a custom user interface developed with PySide6. 
It is primarily designed for various testing environments, including tests for cold gas thrusters as well as flow and pressure control units.

---

## **Key Features**
**Dual-Core Architecture:** 

* :white_check_mark:**Core 0:** Handles user interaction via a Serial Command interface, system safety limits, and the active pressure regulation logic.

* :white_check_mark:**Core 1:** Dedicated solely to continuous Modbus RTU (RS485) communication with the pressure sensor, ensuring zero latency.

* **Hardware FIFO Inter-Core Communication:** Sensor data is passed between Core 1 and Core 0 asynchronously using the RP2040's hardware FIFO, preventing loop blocking.

* **Smart Valve Control (Kick-and-Hold PWM):** The system opens the 24V valve with a full-power "kick" pulse (e.g., 20ms) and then drops to a low-power PWM duty cycle to hold it open.
This drastically reduces power consumption and prevents coil overheating.

* **Hysteresis Regulation:** Uses a configurable two-point controller with hysteresis to prevent rapid, damaging valve oscillation.

* **Failsafe & Keep-Alive:** Includes a configurable watchdog. If the host PC stops sending commands, the system automatically shuts off the valve to prevent overpressure.

---

## **Hardware Setup**
:white_check_mark:**Microcontroller:** Raspberry Pi Pico 

:white_check_mark:**Communication HAT:** Waveshare RS485 UART HAT (connected to UART0: TX Pin 0, RX Pin 1)

:white_check_mark:**Sensor:** IDCT531 Pressure Sensor (Modbus RTU)

--- 

## :man_technologist: **Autor**

**Ezechiel Tonkeme**


![WhatsApp Image 2026-03-28 at 00 19 24](https://github.com/user-attachments/assets/5cf37172-645e-4ec7-848c-8637e817d7f0)


![WhatsApp Image 2026-03-28 at 00 22 56](https://github.com/user-attachments/assets/f3640e3c-806f-4b5e-9650-6b8f9221b8e0)
