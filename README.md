# Dynamic Scheduling Simulator

A superscalar out-of-order processor simulator based on Tomasulo's algorithm, developed for CS6600 (July-Nov '24) at IIT Madras. The simulator models an N-way superscalar processor that can fetch, dispatch, and issue N instructions per cycle.

## Project Overview

This simulator implements an out-of-order superscalar processor with:
- N-way superscalar design
- Dynamic scheduling using Tomasulo's algorithm
- Perfect caches and branch prediction
- Configurable scheduling queue size
- Universal function units

### Key Features

- Configurable superscalar width (N)
- Adjustable scheduling queue size (S)
- 2N-sized dispatch queue
- N universal function units
- Three operation types with different latencies:
  - Type 0: 1 cycle
  - Type 1: 2 cycles
  - Type 2: 10 cycles
- Full pipeline simulation with 5 stages: IF, ID, IS, EX, WB
- Instruction-level timing information
- Performance metrics (IPC calculation)

## Project Structure

```
dynamic-scheduling-simulator/
├── proj_tomasulo.cpp      # Main simulator implementation
├── proj_tomasulo.o        # Header file
├── oosim                  # Simulator executable
├── gcc_trace.txt          # GCC benchmark trace
├── perl_trace.txt         # PERL benchmark trace
├── output.txt            # Simulation output file
├── val_outputs/          # Validation outputs directory
├── plot.py              # Performance analysis plotting script
├── Dockerfile           # Docker configuration
└── Makefile            # Build configuration
```

## Building and Running

### Using Docker (Recommended)

1. Build the Docker image:
   ```bash
   docker build -t assign3 .
   ```

2. Create and run container:
   ```bash
   docker run -it assign3 /bin/bash
   ```

3. Inside container:
   ```bash
   cd Assignment_files
   make
   ./oosim 8 64 gcc_trace.txt
   ```

### Local Build

1. Compile the simulator:
   ```bash
   make
   ```

2. Run the simulator:
   ```bash
   ./oosim <N> <S> <trace_file>
   ```

Parameters:
- `N`: Superscalar width (number of instructions that can be fetched/dispatched/issued per cycle)
- `S`: Scheduling queue size
- `trace_file`: Path to trace file

Example:
```bash
# Run with different configurations
./oosim 2 16 gcc_trace.txt > output.txt   # 2-way superscalar, 16-entry queue
./oosim 4 32 gcc_trace.txt > output.txt   # 4-way superscalar, 32-entry queue
./oosim 8 64 gcc_trace.txt > output.txt   # 8-way superscalar, 64-entry queue
```

## Trace File Format

Input traces follow this format:
```
<PC> <operation_type> <dest_reg#> <src1_reg#> <src2_reg#>
```

Example:
```
bc020064 0 1 2 3    # Type 0 instruction: R1 ← R2, R3
bc020068 1 4 1 3    # Type 1 instruction: R4 ← R1, R3
bc02006c 2 -1 4 7   # Type 2 instruction: - ← R4, R7
```

## Performance Analysis

The project includes a plotting script (`plot.py`) to analyze processor performance:

```bash
python3 plot.py
```

This script generates IPC (Instructions Per Cycle) plots comparing different configurations:
- Y-axis: IPC (Instructions Per Cycle)
- X-axis: Scheduling Queue size (S = 8, 16, 32, 64, 128, 256)
- Three curves for different superscalar widths (N = 2, 4, 8)

Key analysis points:
1. Impact of scheduling queue size on IPC
2. Effect of superscalar width on performance
3. Interaction between queue size and width
4. Performance scaling analysis

## Simulator Implementation

The simulator implements a 5-stage pipeline:
1. IF (Fetch): Fetches N instructions per cycle into dispatch queue
2. ID (Dispatch): Renames and dispatches N instructions to scheduling queue
3. IS (Issue): Issues up to N ready instructions to function units
4. EX (Execute): Executes instructions with type-specific latencies
5. WB (Writeback): Updates register file and wakes up dependent instructions

Key components:
- Circular ROB (Reorder Buffer) for instruction tracking
- Dispatch queue (size = 2N)
- Scheduling queue (size = S)
- N universal function units
- Register file with renaming support

## Validation

To validate simulator output:
```bash
# Run simulation
./oosim <params> > your_output.txt

# Compare with reference
diff -iw your_output.txt validation/reference_output.txt
```

The simulator outputs:
1. Total instruction count
2. Total cycle count
3. IPC (Instructions Per Cycle)
4. Per-instruction timing information

## Common Issues and Solutions

1. If Docker build fails:
   ```bash
   # Clean and rebuild
   docker system prune -a
   docker build -t assign3 .
   ```

2. If permission denied:
   ```bash
   sudo chmod +x oosim
   ```

3. If make fails:
   ```bash
   # Clean build files
   make clean
   make
   ```

## License

This project is part of the CS6600 course at IIT Madras. All rights reserved.
