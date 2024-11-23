#!/usr/bin/env python3

import subprocess
import matplotlib.pyplot as plt

def run_simulation(N, S, trace_file):
    """Run the simulator with given parameters and extract IPC"""
    try:
        # Run the simulator command
        cmd = f"./ooosim {N} {S} {trace_file}"
        print(f"Running: {cmd}")
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        
        # Parse the output to find IPC
        for line in result.stdout.split('\n'):
            if "IPC:" in line:
                ipc = float(line.split(":")[-1].strip())
                print(f"N={N}, S={S}: IPC = {ipc}")
                return ipc
    except Exception as e:
        print(f"Error running simulation with N={N}, S={S}: {e}")
        return None
    
    print(f"Warning: No IPC found in output for N={N}, S={S}")
    return None

def generate_plots(trace_files):
    """Generate plots for each trace file"""
    # Parameters to test
    N_values = [2, 4, 8]
    S_values = [8, 16, 32, 64, 128, 256]
    
    for trace_file in trace_files:
        print(f"\nProcessing trace file: {trace_file}")
        
        # Dictionary to store results for each N value
        results = {N: {'S': [], 'IPC': []} for N in N_values}
        
        # Run simulations for all combinations
        for N in N_values:
            for S in S_values:
                ipc = run_simulation(N, S, trace_file)
                if ipc is not None:
                    results[N]['S'].append(S)
                    results[N]['IPC'].append(ipc)
        
        # Create the plot
        plt.figure(figsize=(12, 8))
        
        # Plot lines for each N value
        for N in N_values:
            if results[N]['IPC']:  # Only plot if we have data
                plt.plot(results[N]['S'], results[N]['IPC'], 
                        marker='o', label=f'N={N}', linewidth=2)
        
        # Customize the plot
        plt.xscale('log', base=2)
        plt.xlabel('Scheduling Queue Size (S)', fontsize=12)
        plt.ylabel('Instructions Per Cycle (IPC)', fontsize=12)
        plt.title(f'IPC vs Scheduling Queue Size\nTrace: {trace_file}', fontsize=14)
        plt.legend(fontsize=10)
        plt.grid(True)
        plt.grid(True, which='minor', linestyle=':', alpha=0.4)
        
        # Ensure all S values are shown on x-axis
        plt.xticks(S_values, S_values)
        
        # Save the plot
        output_file = f"ipc_plot_{trace_file.replace('.txt', '')}.png"
        plt.savefig(output_file, bbox_inches='tight')
        plt.close()
        
        print(f"\nGenerated plot: {output_file}")
        
        # Print results in table format
        print("\nNumerical Results:")
        print("S value |", end=" ")
        for N in N_values:
            print(f"N={N:<6}", end=" ")
        print("\n" + "-"*40)
        
        for i, S in enumerate(S_values):
            print(f"{S:<7} |", end=" ")
            for N in N_values:
                if i < len(results[N]['IPC']):
                    print(f"{results[N]['IPC'][i]:<7.3f}", end=" ")
                else:
                    print(f"{'N/A':<7}", end=" ")
            print()

if __name__ == "__main__":
    # List of trace files to process
    trace_files = ["gcc_trace.txt"]  # Add other trace files as needed
    generate_plots(trace_files)