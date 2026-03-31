# Nuvoton MA35D1 SPI Slave with PDMA (Linux 5.10.y)

This project provides a kernel patch and a user-space test application for high-speed, stable SPI communication using **SPI Slave mode** and **PDMA** (Peripheral Direct Memory Access) on the **Nuvoton MA35D1 MPU** (Linux Kernel 5.10.y).

The default SPI kernel driver in the Nuvoton BSP often forces hardware registers into Master mode even when Slave mode is declared in the Device Tree. This leads to bus contention and voltage drops on the Chip Select (CS) pin. This repository provides a kernel driver patch to resolve these issues, along with application code to verify proper operation.

## IMPORTANT: Hardware Modification Required
On the NUMAKER-MA35D1 evaluation board, pins PL.0 to PL.3 are hard-wired to the **UART11** interface by default via series resistors. To use these pins for SPI2 communication without signal interference, you must remove the series resistors connected to **UART11** on the board.

---

## 🛠️ Kernel Domain Changes

The main SPI driver has been patched to ensure the MA35D1 hardware operates as a full Slave.

* **Target Kernel Version**: Linux 5.10.y
* **Modified File**: `drivers/spi/spi-ma35d1-spi.c`

### Key Patch Details
1.  **Slave Register Control**:
    * Defined the `SLAVE_MODE` macro (`0x01 << 18`) for hardware control.
    * Added conditional logic to the `nuvoton_spi_hw_init()` function to enable bit 18 of the `REG_CTL` register when in Slave mode, preventing the hardware from generating its own clock.
2.  **Device Tree (DTS) Attribute Parsing**:
    * Updated `nuvoton_spi_probe()` to read the `spi-slave` attribute from the DTS and correctly set the `master->slave = true;` flag.
3.  **Blocking CS (Chip Select) Output**:
    * In Slave mode, the external Master must drive the CS line. Logic was added to the beginning of `nuvoton_spi_set_cs()` (`if (spi->master->slave) return;`) to prevent the MA35D1 from driving the CS pin, thereby avoiding bus contention.
4.  **Extended Timeout**:
    * To provide sufficient time for manual testing and Master device operation, the hardcoded `SPI_GENERAL_TIMEOUT_MS` value was increased from 1 second (1000) to **60 seconds (60000)**.

### Mandatory Device Tree (DTS) Configuration
To use the patched driver, configure the SPI node in your target DTS file as follows:

```dts
&spi2 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_spi2>;
    
    spimode = <1>;     /* Nuvoton-specific: Force Slave mode (Mandatory) */
    spi-slave;         /* Standard Linux SPI Slave framework declaration */
    use_pdma = <1>;    /* Enable PDMA usage */

    /* The child node name must start with 'slave@0' per framework rules */
    spidev2: slave@0 {
        compatible = "rohm,dh2228fv";
        reg = <0>;
        spi-max-frequency = <10000000>;
    };
};
```

---

## 💻 User-space Domain Files

Application source and build scripts to test SPI communication via the patched kernel.

### 1. `main.c`
A C program that handles SPI Slave reception using the standard `spidev` interface.
* **Device Path**: Opens `/dev/spidev1.0` as the target node.
* **Buffer Size**: Set to `100` bytes to match the data sent by the external Master.
* **Operation**: Since it meets the DMA threshold of the kernel driver, calling `ioctl(fd, SPI_IOC_MESSAGE(1), &tr)` triggers a PDMA burst transfer. The app remains in a blocking state until a clock signal is received from the Master.

### 2. `makefile`
A build script for cross-compiling the application.
* The default compiler (`CC`) is set to `aarch64-nuvoton-linux-gcc`. You can generate the `spi_slave_test` binary by simply running `make` within the Nuvoton Buildroot toolchain environment.

---

## 🚀 Build & Test Guide

### Hardware Modification (Mandatory for NUMAKER-MA35D1)
Before testing, ensure the following hardware adjustment is made to avoid signal contention:

* **Remove UART11 Series Resistors**: On the NUMAKER-MA35D1 board, pins **PL.0, PL.1, PL.2, and PL.3** are shared with **UART11**. You must physically remove the series resistors connecting these pins to the UART11 interface to ensure clean SPI2 signals.

### Build Instructions
1.  **Kernel Build**: Overwrite the existing kernel source with the provided `spi-ma35d1-spi.c` and update your DTS file. Rebuild the kernel image and flash it to the board.
2.  **App Build**: Run `make` in the App directory and copy the resulting executable to the target board.

### Hardware Wiring (MA35D1 <-> External Master MCU)
| MA35D1 (Slave) | External MCU (Master) | Notes |
| :--- | :--- | :--- |
| **GND** | **GND** | **Mandatory (Common Reference)** |
| PL.3 (SPI2_CLK) | SPI_CLK | |
| PL.0 (SPI2_MOSI) | SPI_MOSI | |
| PL.1 (SPI2_MISO) | SPI_MISO | |
| PL.2 (SPI2_SS0) | SPI_CS | *330Ω series resistor recommended for safety* |

### Execution Steps (Critical)
1.  **[MA35D1]** Run `./spi_slave_test` on the target board first. It will display `SPI2 (PL.0~PL.3 pins) Slave waiting...` and enter a blocking wait state.
2.  **[External MCU]** Within 60 seconds, start the communication from the Master device to send 100 bytes of data.
3.  **[MA35D1]** Once the transfer is complete, the program will immediately print `Reception successful!`, display the received data, and exit.