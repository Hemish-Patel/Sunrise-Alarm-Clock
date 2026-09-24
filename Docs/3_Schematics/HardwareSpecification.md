# Hardware Specification & Master Pinout Map
**Project:** ESP32 Dual-Core Sunrise Alarm Clock  
**Date:** September 2026  
**Document Status:** Final / Approved PCB Revision (Hand-Routed)  

---

## 1. PCB Specifications & Physical Metrics
* **Board Dimensions:** 114.30 mm × 73.66 mm (4.500" × 2.900")
* **Layer Stack:** 2-Layer FR-4 (1.6 mm thickness, 1 oz copper)
* **Routing Architecture:** Point-to-point manually hand-routed dual-layer board across Top Layer (`F.Cu`) and Bottom Layer (`B.Cu`).
* **Grounding Topology:** Discrete point-to-point manual ground return traces connecting each ground pad. **Zero filled copper zones / no solid copper pour**.
* **Vias:** **Exactly 8 vias** (0.60 mm pad diameter, 0.30 mm drill hole) bridging signals and power between `F.Cu` and `B.Cu` layers:
  1. `+5V` Power Via at (113.67 mm, 87.63 mm)
  2. `GND` Return Via at (58.42 mm, 58.42 mm)
  3. `GPIO_21` (I2C SDA) Via at (96.52 mm, 50.80 mm)
  4. `GPIO_16` (DFPlayer TX) Via at (60.57 mm, 35.17 mm)
  5. `GPIO_17` (DFPlayer RX) Via at (63.50 mm, 63.50 mm)
  6. `GPIO_32` (Encoder 1 DT) Via at (106.68 mm, 83.82 mm)
  7. `GPIO_27` (MOSFET Gate) Via at (86.78 mm, 58.84 mm)
  8. `GPIO_27` (MOSFET Gate) Via at (114.30 mm, 48.26 mm)
* **Track Widths:** Multi-tiered trace routing:
  * **1.20 mm Power & Ground Traces:** High-current paths for Twin-Bell Motor (`Net-(D1-K)`, `Net-(D1-A)`, `Q1` Source GND), Qi Wireless Charger (`Net-(J13-Pin_2)`, `J13` GND), and the main `+5V` / `GND` distribution trunks.
  * **0.60 mm Decoupling Return:** Audio decoupling capacitor `C1` ground return link into the main ground trunk.
  * **0.20 mm Signal & Logic Traces:** All standard 3.3V/5V logic, I2C, UART, switch sensing, and spare GPIO lines.
* **Mounting:** 4× M3 (3.2 mm diameter) mechanical mounting holes located at the corners.
* **Power Supply:** Mean Well IRM-30-5ST (30W, 5V @ 6.0A continuous).

---

## 2. Master Pinout Table

| ESP32 GPIO | Direction | Function / Connected Subsystem | Power Rail | Electrical Protection & Circuit Details |
| :--- | :--- | :--- | :--- | :--- |
| **GPIO 21** | In/Out | I2C SDA (DS3231 RTC + HT16K33 Display) | 3.3V Logic | Shared hardware I2C bus (`0x68` RTC, `0x70` Display) |
| **GPIO 22** | Output | I2C SCL (DS3231 RTC + HT16K33 Display) | 3.3V Logic | Shared hardware I2C bus (`0x68` RTC, `0x70` Display) |
| **GPIO 36 (VP)**| Input (ISR)| RobotDyn AC Dimmer Zero-Cross (Z-C) | 3.3V Logic | Input-only pin. Pull-up tied to 3.3V rail |
| **GPIO 26** | Output | RobotDyn AC Dimmer Gate Pulse | 3.3V Logic | Fires 20–50 µs pulse to MOC3021 optocoupler |
| **GPIO 34** | Input | Toggle Switch 1 (Light Sequence Arm/Disarm)| 3.3V Logic | Input-only pin. 10kΩ external pull-down to GND (`R4`) |
| **GPIO 35** | Input | Toggle Switch 2 (Audio Chimes Arm/Disarm) | 3.3V Logic | Input-only pin. 10kΩ external pull-down to GND (`R5`) |
| **GPIO 39 (VN)**| Input | Toggle Switch 3 (Bell Arm/Disarm) | 3.3V Logic | Input-only pin. 10kΩ external pull-down to GND (`R6`) |
| **GPIO 25** | Input (ISR)| Rotary Encoder 1 CLK (Time Adjust) | 3.3V Logic | KY-040 onboard 10kΩ pull-ups to 3.3V |
| **GPIO 32** | Input (ISR)| Rotary Encoder 1 DT (Time Adjust) | 3.3V Logic | KY-040 onboard 10kΩ pull-ups to 3.3V |
| **GPIO 33** | Input | Rotary Encoder 1 SW (Pushbutton) | 3.3V Logic | Internal `INPUT_PULLUP` enabled in firmware |
| **GPIO 4** | Input (ISR)| Rotary Encoder 2 CLK (Menu Navigate) | 3.3V Logic | KY-040 onboard 10kΩ pull-ups to 3.3V |
| **GPIO 13** | Input (ISR)| Rotary Encoder 2 DT (Menu Navigate) | 3.3V Logic | KY-040 onboard 10kΩ pull-ups to 3.3V |
| **GPIO 23** | Input | Rotary Encoder 2 SW (Pushbutton) | 3.3V Logic | Internal `INPUT_PULLUP` enabled in firmware |
| **GPIO 17** | Output | DFPlayer Mini RX (ESP32 TX2) | 5V Module | **1kΩ series resistor (`R1`)** for level matching |
| **GPIO 16** | Input | DFPlayer Mini TX (ESP32 RX2) | 5V Module | Direct 3.3V logic return |
| **GPIO 27** | Output(PWM)| Twin-Bell Motor Driver (MOSFET Gate) | 5V Motor | IRLZ44N Gate: 330Ω series (`R3`) + 10kΩ pull-down (`R2`) |
| **GPIO 18** | Spare | Reserved for Future Expansion | N/A | Previously TM1637 CLK (Now Free!) |
| **GPIO 19** | Spare | Reserved for Future Expansion | N/A | Previously TM1637 DIO (Now Free!) |

---

## 3. Complete 28-Component Bill of Materials (BOM)

| Ref | Value | Function | Exact Footprint | Connector / Package Type |
| :--- | :--- | :--- | :--- | :--- |
| **J1** | `5V Power In` | Main DC Input from PSU | `TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2_1x02_P5.00mm_Horizontal` | 2-pin 5.00mm Screw Terminal |
| **J2** | `J_ESP_Left` | ESP32 DevKit V1 Pins 1–19 | `Connector_PinSocket_2.54mm:PinSocket_1x19_P2.54mm_Vertical` | 19-pin 2.54mm Female Socket |
| **J3** | `J_ESP_Right` | ESP32 DevKit V1 Pins 20–38 | `Connector_PinSocket_2.54mm:PinSocket_1x19_P2.54mm_Vertical` | 19-pin 2.54mm Female Socket (25.40mm row spacing) |
| **J4** | `DS3231_Mini` | RTC Module Interface | `Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical` | 5-pin 2.54mm Male Pin Header |
| **J5** | `J_DF_Left` | DFPlayer Mini Pins 1–8 | `Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical` | 8-pin 2.54mm Female Socket |
| **J6** | `J_DF_Right` | DFPlayer Mini Pins 9–16 | `Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical` | 8-pin 2.54mm Female Socket (15.24mm row spacing) |
| **J7** | `J_SPEAKER` | 3W Speaker Audio Output | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | 2-pin 2.54mm Male Pin Header |
| **J8** | `J_OUT_TO_TRIAC_DIMMER`| Dimmer Cable Interface | `Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical` | 4-pin 2.54mm Male Pin Header |
| **J9** | `J_Display` | 7-Segment Cable Interface | `Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical` | 5-pin 2.54mm Male Pin Header |
| **J10** | `J_OutToMotor`| Motor Output Terminal | `TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2_1x02_P5.00mm_Horizontal` | 2-pin 5.00mm Screw Terminal |
| **J11** | `J_ToRotaryEncoder1` | Rotary Enc 1 (Time) Cable | `Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical` | 5-pin 2.54mm Male Pin Header |
| **J12** | `J_ToRotaryEncoder2` | Rotary Enc 2 (Menu) Cable | `Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical` | 5-pin 2.54mm Male Pin Header |
| **J13** | `J_ToWirelessCharger` | Qi Charger Dock Output | `TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2_1x02_P5.00mm_Horizontal` | 2-pin 5.00mm Screw Terminal |
| **J14** | `J_ToSwitch1`| Switch 1 (Light Arm) Cable| `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | 2-pin 2.54mm Male Pin Header |
| **J15** | `J_ToSwitch2`| Switch 2 (Audio Arm) Cable| `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | 2-pin 2.54mm Male Pin Header |
| **J16** | `J_ToSwitch3`| Switch 3 (Bell Arm) Cable | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` | 2-pin 2.54mm Male Pin Header |
| **Q1** | `IRLZ44N` | Bell Motor Logic-Level MOSFET | `Package_TO_SOT_THT:TO-220-3_Vertical` | TO-220 N-Channel Power MOSFET |
| **D1** | `1N5819` | Motor Schottky Flyback Diode| `Diode_THT:D_DO-41_SOD81_P7.62mm_Horizontal` | DO-41 Axial Schottky Diode |
| **F1** | `500mA PTC` | Motor Short/Runaway Polyfuse| `Fuse:Fuse_Bourns_MF-RG500` | 500mA Radial Resettable Polyfuse |
| **F2** | `1.5A/2A PTC`| Qi Charger Overcurrent Polyfuse| `Fuse:Fuse_Bourns_MF-RG500` | 1.85A–2.0A Radial Resettable Polyfuse |
| **C1** | `470 uF` | Audio 5V Bulk Decoupling Cap| `Capacitor_THT:CP_Radial_D8.0mm_P3.80mm` | 470µF 16V Radial Electrolytic Can |
| **C2** | `100nF` | Qi Charger Switching Filter | `Capacitor_THT:C_Disc_D4.7mm_W2.5mm_P5.00mm` | 100nF Ceramic Disc Capacitor |
| **R1** | `1Kohm` | DFPlayer RX Series Resistor | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal` | 1/4W Axial Resistor (7.62mm pitch) |
| **R2** | `10Kohm` | MOSFET Gate Pull-Down Resistor| `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal` | 1/4W Axial Resistor (7.62mm pitch) |
| **R3** | `330ohm` | MOSFET Gate Series Resistor | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal` | 1/4W Axial Resistor (7.62mm pitch) |
| **R4** | `10Kohm` | Switch 1 Pull-Down Resistor | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal` | 1/4W Axial Resistor (7.62mm pitch) |
| **R5** | `10Kohm` | Switch 2 Pull-Down Resistor | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal` | 1/4W Axial Resistor (7.62mm pitch) |
| **R6** | `10Kohm` | Switch 3 Pull-Down Resistor | `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal` | 1/4W Axial Resistor (7.62mm pitch) |
| **M3** | `MountingHoles`| 4 Corner Mechanical Standoffs| `MountingHole:MountingHole_3.2mm_M3` | 4× M3 Mechanical Mounting Holes |
