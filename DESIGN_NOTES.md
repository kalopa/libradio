# DESIGN.md: TDMA Radio Swarm MAC Protocol & Implementation

## 1. System Overview
This document outlines the software architecture and protocol design for a swarm of master/client devices communicating over a shared 434MHz radio channel. The system is designed for an ATmega328p microcontroller.

**Core Constraints & Requirements:**
*   **Radio Window:** 20ms transmission window.
*   **Packet Size:** 16 bytes maximum.
*   **Half-Duplex:** The radio module cannot receive data while transmitting.
*   **Traffic Profile:** Mixed frequencies (some devices send 10 packets/sec, some 5/sec, others 1/sec).
*   **Hardware Interface:** The radio is interfaced via the SPI bus.
*   **Topology:** A single "Team Leader" (Master) coordinates multiple "Followers" (Clients) to eliminate collisions. 

## 2. MAC Protocol Selection: Time Division Multiple Access (TDMA)
Initially, a CSMA/CA (listen-before-talk) approach was considered. However, to guarantee Quality of Service (QoS), support high-frequency traffic without collisions, and allow client devices to sleep extensively, a **Master-coordinated TDMA protocol** was chosen over CSMA/CA and Slotted ALOHA.

### 2.1 The 1-Second Superframe
Time is divided into a repeating 1-second "superframe," which is further subdivided into **50 equal slots of 20ms each**.

*   **Slot 0:** Reserved for the Master's **BEACON** packet. This synchronizes the network and assigns slots.
*   **Slots 12, 24, 36, 48:** Reserved for **Master LEADER_CMD TX**. The master uses these slots to send commands or relay RS-232 data from an upstream Unix host to the clients.
*   **Slot 49:** Reserved for **Master SYSTEM_SYNC TX**. A periodic time/date update broadcast.
*   **Remaining 44 Slots:** Assigned to Client devices based on the Beacon schedule.

### 2.2 Beacon Payload & Slot Scheduling
The 16-byte packet allows for a 2-byte timestamp, 1-byte sender ID, 1-byte command (4-byte header), leaving **14 bytes for payload**. 

To efficiently compress the schedule for 44 slots into 14 bytes, the Master assigns 14 **"Roles"** to specific Client IDs in the Beacon payload.
*   `Payload Bytes [0-1]`: 10 packets/sec (10 slots each) - 20 slots total.
*   `Payload Bytes [2-4]`: 5 packets/sec (5 slots each) - 15 slots total.
*   `Payload Bytes [5-13]`: 1 packet/sec (1 slot each) - 9 slots total.

*Client Logic:* A client parses the 14-byte payload. If its ID is in Byte 0 (Role 1), it knows its assigned slots are [1, 6, 11, 15, 19, 23, 27, 31, 35, 39]. It calculates the exact timer ticks for these slots and sleeps otherwise.

## 3. Microcontroller Architecture (ATmega328p)
The system is built as a reusable C library (`libradio.a`) with specialized main loops for the Master and Client.

### 3.1 Hardware Resource Allocation
*   **SPI Bus:** Used for configuring the radio and transferring FIFO data.
*   **Timer 0 (8-bit):** Real-Time Clock (RTC) / `millis()`.
*   **Timer 1 (16-bit):** TDMA Slot Synchronizer (Targeting ~10kHz tick rate for $100\mu s$ resolution).
*   **Timer 2 (8-bit):** RS-232 Data Timeout / Flow Control (Master only).
*   **USART:** RS-232 Communication to Upstream Unix Host (Master only).

### 3.2 Time-Critical SPI Execution
**Design Decision:** While SPI is relatively slow (400kHz), the actual command to switch the radio into TX mode is only 2-3 bytes ($\sim 60\mu s$). 
*   **Why?** Relying on a main loop flag introduces unacceptable timing jitter.
*   **Implementation:** The data payload is pre-loaded into the radio's FIFO during non-critical time. The precise **Timer 1 Compare Match ISR** executes the $60\mu s$ SPI command to initiate transmission exactly at the slot boundary. 

## 4. Master Node Architecture
The Master's software is entirely **interrupt-driven**. It does not use a polling loop for radio tasks.

### 4.1 Master Timer 1 ISR
The `TIMER1_COMPA` ISR fires every 20ms. It uses a switch/case logic based on the current slot number:
*   **Slot 0:** Transmits the BEACON, schedules next interrupt for Slot 1.
*   **Slots 12, 24, 36, 48:** Checks a `tx_pending` flag. If RS-232 data from Unix is waiting, it transmits. Otherwise, sets radio to RX.
*   **Slot 49:** Transmits the Time/Date sync packet.
*   **All other slots:** Ensures the radio is in RX mode to receive Client transmissions.

### 4.2 RS-232 Flow Control
Because the upstream Unix host can send data asynchronously, but the Master only has 4 command slots per second, software flow control is required.
*   The Master maintains an RS-232 RX ring buffer and a TX ring buffer.
*   The Master periodically sends a `SLOT_UPDATE <slots_available>` message over RS-232 to the Unix host.
*   The Unix host must only send commands when the Master indicates available capacity, preventing buffer overruns.

## 5. Client Node Architecture
The Client is optimized for power efficiency and a clean separation of MAC logic from application logic.

### 5.1 The `libradio_poll_loop()`
The client's `main()` function contains an asynchronous polling loop. 
*   **Interrupts handle time-critical tasks:** The Radio RX ISR receives packets and sets flags. The Timer 1 Compare ISR handles TX initiation.
*   **Polling loop handles data:** `libradio_poll_loop()` checks RX flags, parses the Beacon schedule, and passes application data to user functions (e.g., `operation_command()`).
*   **Responses:** The user application calls `status_request()`, which calls `libradio_enqueue_response()`. This library function pre-loads the TX buffer and sets a flag for the Timer 1 ISR to transmit at the next assigned slot.

### 5.2 Latency Compensation & Synchronization
When the Radio RX ISR receives a valid BEACON:
1.  Timer 1 is reset.
2.  A hard-coded latency offset (e.g., `160` ticks, representing 16ms of receive/processing latency) is loaded into `TCNT1` (or the equivalent tracking variable) to ensure the client's clock is perfectly aligned with the Master's slot boundaries.
3.  The client calculates the `Compare Match` value for its next assigned TX slot.

## 6. Power Management & "Lost Beacon" Watchdog
To handle scenarios where the Master fails or goes out of range, clients must avoid staying active indefinitely.

### 6.1 Timeout Detection
*   A software counter tracks missed BEACONS. If no BEACON is received for approximately **6.5 seconds**, the system transitions from the `ONLINE` state to a `LOST_BEACON` state.

### 6.2 `LOST_BEACON` Recovery State
*   **Power Down:** The precise 10kHz Timer 1 is stopped to save battery. The main loop disables non-essential peripherals.
*   **Periodic Check:** The system enters a deep sleep state (e.g., `SLEEP_MODE_PWR_DOWN`). It wakes up briefly on a slow interval (e.g., 1Hz using the Watchdog Timer or Timer 2) to check the radio RX buffer for a BEACON.
*   **Recovery:** Once a valid BEACON is detected, Timer 1 is restarted, latency compensation is applied, and the system state returns to `ONLINE`.
