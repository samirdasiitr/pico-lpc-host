# LPC Host Controller for Raspberry Pi Pico

This project implements an LPC (Low Pin Count) bus host controller using the Raspberry Pi Pico's PIO (Programmable I/O) hardware. It supports memory and I/O operations with LPC slave devices.

I started this project to avoid buying expensive flash programmers (Svod, RT809F/H) for programming EC/SIO chips (kb9012, it8586).
They support LPC HOST to read/write flash memory. Thus by bitbanging LPC HOST, I am planning to avoid buying.

## Features

- Full LPC protocol implementation
- Support for Memory Read/Write operations
- Support for I/O Read/Write operations
- 33MHz LPC bus timing
- Binary file transfer capability via UART
- Dual state machine design for optimized performance

## Hardware Requirements

- Raspberry Pi Pico
- LPC target device (slave)
- Logic level converters (if target uses 3.3V/5V different from Pico)

### Pin Connections

| Signal   | Pico Pin | Description            |
|----------|----------|------------------------|
| LFRAME#  | GPIO2    | LPC Frame signal       |
| LAD[0]   | GPIO3    | LPC Address/Data bit 0 |
| LAD[1]   | GPIO4    | LPC Address/Data bit 1 |
| LAD[2]   | GPIO5    | LPC Address/Data bit 2 |
| LAD[3]   | GPIO6    | LPC Address/Data bit 3 |
| LCLK     | GPIO7    | LPC Clock (33MHz)      |
| SYNC     | GPIO8    | SYNC ready signal      |

Note: Pin assignments can be modified in the code.

## Software Setup

### Prerequisites

1. Raspberry Pi Pico SDK
2. CMake (3.13 or later)
3. Build tools (make, gcc)

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/pico-lpc-host.git
cd pico-lpc-host

# Create build directory
mkdir build
cd build

# Generate build files
cmake ..

# Build the project
make
```

### Project Structure

```
pico-lpc-host/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.c
│   ├── lpc_init.c
│   ├── lpc_host.pio
│   └── include/
│       └── lpc_host.h
└── tools/
    └── file_transfer.py
```

## Usage

### Basic Operations

```c
// Initialize LPC host
lpc_host_init(pio, sm_write, sm_read, offset_write, offset_read,
              PIN_LFRAME, PIN_LAD0, PIN_LCLK, PIN_SYNC, clk_sys);

// Memory write example
lpc_write(pio, sm_write, LPC_CYCLE_MEM, 0, address, data);

// Memory read example
uint32_t data = lpc_read(pio, sm_read, LPC_CYCLE_MEM, 0, address);

// I/O write example
lpc_io_write(pio, sm_write, port, LPC_SIZE_BYTE, data);

// I/O read example
uint8_t data = lpc_io_read(pio, sm_read, port);
```

### File Transfer

The project includes a Python script for transferring binary files to the LPC target through the Pico:

```python
# On your development machine
python tools/file_transfer.py --port /dev/ttyACM0 --file firmware.bin
```

## Protocol Details

The LPC bus operates at 33MHz and uses the following signals:
- LFRAME# - Indicates start of transaction
- LAD[3:0] - Multiplexed address/data bus
- LCLK - Clock signal
- SYNC - Ready/wait state indication

### Transaction Types

1. Memory Read/Write
   - 32-bit address space
   - Byte/Word/DWord access

2. I/O Read/Write
   - 16-bit port address
   - Byte/Word access

## Troubleshooting

Common issues and solutions:

1. Timing Issues
   - Verify clock divider settings
   - Check logic analyzer timing
   - Ensure proper pull-up resistors

2. Communication Failures
   - Check pin connections
   - Verify voltage levels
   - Monitor SYNC signal

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Raspberry Pi Pico SDK team
- LPC specification by Intel