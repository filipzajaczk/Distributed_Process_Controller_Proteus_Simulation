# Distributed Process Controller (Proteus Simulation)

## Project Overview
This project features a distributed embedded system designed and simulated in **Proteus**. It utilizes four **AT89C52** microcontrollers communicating over an **RS-485 bus** (using MAX487 transceivers) to control an industrial process. 

The system demonstrates advanced Master-Slave architecture, hardware interrupt handling, and precise peripheral control.

## System Architecture & Node Roles
The system is divided into four specialized nodes, each driven by its own AT89C52 microcontroller. Communication is handled via a custom addressed protocol over the UART interface.

### U1. Main Controller
* **Function:** Acts as the brain of the system, scanning the matrix keypad and orchestrating the other nodes.
* **Key Features:**
  * Translates user input into actionable commands (e.g., Set Turns, Set Power, Change Direction).
  * Sends structured data packets `[Address | Command] + [Data]` to the respective slave nodes.

### U3. 7-Segment Display Node
* **Function:** Provides real-time visual feedback of the remaining motor turns.
* **Key Features:**
  * Multiplexes two 7-segment displays using Timer 0 (`interrupt 1`).
  * Uses external memory mapping (`xdata`) to drive the display segments.
 
### U5. Motor & Lamps
* **Function:** Executes physical processes. It manages the DC motor and simulates heating elements (lamps).
* **Key Features:**
  * **Software PWM:** Uses Timer 0 interrupt (`interrupt 1`) to generate PWM signals for the lamps (0-100% heat) and motor speed control.
  * **Hardware Counter:** Utilizes External Interrupt 0 (`interrupt 0`) to accurately count physical motor rotations.
  * Autonomously reports rotation updates back to the 7-segment display node.

### U8. LCD Interface Node
* **Function:** Drives a 16x4 character LCD.
* **Key Features:** * Receives display state commands from the Master to render dynamic menus (e.g., "Set Turn Count", "Motor running...").
  * Operates on a 4-bit data bus configuration.

<img width="1252" height="879" alt="image" src="https://github.com/user-attachments/assets/99c71643-5655-4208-bf4e-e8a2d5f36d72" />

---

## Communication Protocol & Logic

The nodes communicate using standard 8051 UART (Mode 1), configured for a multi-drop half-duplex topology via MAX487 transceivers. 

* Each slave node is assigned a unique hex address (e.g., `Keyboard = 0xF0`, `Motor = 0xB0`, `LCD = 0xE0`, `7SEG = 0xD0`).
* Data is transmitted in 2-byte packets: `[Address (4 bits) + Command (4 bits)]` followed by `[Data (8 bits)]`.

### 1. Sending Data
To transmit data over the RS-485 bus, the transmitting microcontroller must control the Driver Enable (DE) pin on the MAX487. The Master activates transmission mode, sends the address/command byte, waits for the UART buffer to clear, and then sends the payload.

```c
void Send(unsigned char Value){
    P3_4 = 1;           //Enable DE (Driver Enable) on MAX487 for transmission
    TI = 0;             //Clear Transmit Interrupt flag
    SBUF = Value;       //Load data into UART buffer
    while(TI==0){;}     //Wait for transmission to complete
    TI = 0;
    P3_4 = 0;           //Disable DE to release the bus for other nodes
}

void wyslij_pakiet(unsigned char adresat, unsigned char komenda, unsigned int dana) {
    unsigned char pakiet1 = adresat | komenda; //Merge address and command into one byte
    Send(pakiet1);
    Send(dana);
}
```

### 2. Receiving Data
Instead of polling the UART and blocking the main loop, all slave nodes use the UART Receive Interrupt (`interrupt 4`). The interrupt routine acts as a lightweight state machine. It first checks if the incoming byte's address matches the node's defined address. If so, it waits for the second byte (payload) and sets a flag for the main loop to process.

```c
void odebranie() interrupt 4 {
    static unsigned char czekam = 0;
    static unsigned char temp = 0;
    unsigned char bajt;
    
    if(RI) {
        bajt = SBUF; //Read incoming byte
        RI = 0;      //Clear Receive Interrupt flag
        
        if (czekam == 0) {
            //Check if the top 4 bits match this specific Node's Address (e.g., SILNIK = 0xB0)
            if ((bajt & 0xF0) == SILNIK) {
                temp = bajt & 0x0F; //Extract the 4-bit command
                czekam = 1;         //Advance state machine to wait for the data byte
            }
        } else {
            //Second byte received - store data and notify main loop
            odebrana_komenda = temp;
            odebrana_dana = bajt;
            odebrano_nowy_pakiet = 1; //Flag to trigger action in main()
            czekam = 0;               //Reset state machine
        }
    }
}
```

---

## Hardware Interrupts Management

The system heavily relies on hardware interrupts to ensure real-time responsiveness without blocking the main execution loops. Three main types of interrupts are utilized across the nodes:

### 1. Serial Communication Interrupt (`interrupt 4`)
Used by all slave nodes to capture incoming UART bytes asynchronously. As detailed in the communication section, it operates a state machine that filters packets by address and extracts the payload without halting the processor.

### 2. Timer 0 Interrupt (`interrupt 1`)
This interrupt handles time-critical multiplexing and signal generation tasks:
* **In the Motor Node (Slave 0xB0):** It generates a custom Software PWM (100-step resolution) to control the motor speed and the heat intensity of the simulated lamps.
* **In the 7-Segment Node (Slave 0xD0):** It handles the fast multiplexing of the dual 7-segment displays to prevent flickering, updating the visible digits based on the latest received data.

```c
//Example: Timer 0 Interrupt for Software PWM (Motor Node)
void zarowa() interrupt 1 {
    TH0 = 0xFF; 
    TL0 = 0x00;
    
    licznik++;
    if(licznik >= 100) licznik = 0; //0-99% duty cycle resolution
    
    //PWM output for lamps and motor
    if(licznik < lampa1) P2_2 = 1; else P2_2 = 0;
    if(prawo && (licznik < moc_silnika)) P2_5 = 1; else P2_5 = 0;
}
```

### 3. External Interrupt 0 (`interrupt 0`)
Used exclusively by the Motor Node to track physical motor rotations. Every full rotation triggers a hardware pulse on the `INT0` pin. The interrupt routine decrements the target turn counter and stops the motor instantly when the target is reached, ensuring zero physical overshoot.

```c
//Example: External Interrupt 0 for Rotation Counting (Motor Node)
void licz_obroty(void) interrupt 0 {
    obroty_licznik--; 
    
    if (obroty_licznik > 0) {
        aktualizuj_bcd = 1; //Flag to send update to 7SEG display
    } else {
        P2_3 = 0; //Target reached - cut power to motor
        P2_4 = 0;
    }
}
```

---

## Configurable Parameters

The Master node provides a menu-driven interface via the 16x4 LCD and a matrix keypad. The user can configure the following process parameters:

1. **Turns:** Sets the exact number of cycles the motor must complete before stopping.
2. **Power:** Configures the motor speed (PWM duty cycle from 0% to 100%).
3. **Direction:** Sets the motor rotation logic to either Left (1) or Right (2).
4. **Lamp 1 / Lamp 2:** Independently sets the heating power (PWM duty cycle) for two separate simulated process lamps.

*Navigation Keys:*
* **Number keys (0-9):** Input parameter values.
* **`*` :** Save the current parameter and return to the main menu.
* **`#` :** Quick-set the current parameter to maximum (100%).

---

## Process Execution (Start Sequence)

Once the parameters are configured and the user selects **`6. Start`** from the main menu, the following sequence occurs:

1. The Master node transmits the `START` command (`0x06`) across the RS-485 bus.
2. **LCD Node:** Receives the command, clears the menu, and displays the status: *"Motor running... Please wait..."*.
3. **Motor Node:** Receives the command and activates the motor driver pins (`P2_3 = 1`, `P2_4 = 1`). The motor begins rotating in the configured direction and at the specified PWM power.
4. **Monitoring:** The External Interrupt (`INT0`) counts the rotations. With each turn, the Motor node sends an update to the **7-Segment Node**, which actively counts down the remaining turns on the display.
5. **Completion:** Upon reaching 0 turns, the hardware interrupt immediately cuts motor power.

---

## How to Run the Simulation

To test the system locally without compiling the C code:

1. Download the repository and open the Proteus project file (`PROJ2.pdsprj`) in **Proteus Design Suite**.
2. Double-click on each of the four AT89C52 microcontrollers in the schematic.
3. In the "Program File" property, browse and attach the corresponding compiled `.hex` file from the repository folders (e.g., attach `klawiatura.hex` to the Master node, `silnik.hex` to the Motor node).
4. Click the **Play** button at the bottom left of the Proteus interface to start the simulation.
5. Use the interactive keypad component on the screen to navigate the menu and start the process.

---

## Project Demonstration

Below is a short capture of the Proteus simulation in action. It demonstrates the Master node accepting user input via the matrix keypad, transmitting the parameters over the RS-485 bus, and the Motor node executing the physical rotation while updating the 7-segment display.

![Nagrywanie 2026-02-24 171805](https://github.com/user-attachments/assets/1f46990c-1b71-4ef2-a198-503b89abdf1b)

