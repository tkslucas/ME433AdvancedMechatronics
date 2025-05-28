import csv
import matplotlib.pyplot as plt
import numpy as np
import os
from coefficients import h_1, h_2, h_3, h_4

def load_signal_data(address):
    time = []
    signal = []
    with open(address, "r") as f:
        reader = csv.reader(f)
        for row in reader:
            time.append(float(row[0]))
            signal.append(float(row[1]))
    return time, signal

def calculate_fft(time, signal):
    dt = (time[-1] - time[0]) / len(time)
    Fs = 1 / dt
    n = len(signal)
    Y = np.fft.fft(signal) / n
    frq = np.arange(n) / (n / Fs)
    return frq[:n//2], abs(Y[:n//2])

def plot_results(raw_time, raw_signal, filtered_time, filtered_signal, filter_name, params, signal_name):
    # Calculate FFTs
    raw_frq, raw_Y = calculate_fft(raw_time, raw_signal)
    filt_frq, filt_Y = calculate_fft(filtered_time, filtered_signal)
    
    # Create plots
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
    
    ax1.set_xlabel('Time')
    ax1.set_ylabel('Amplitude')
    ax2.set_xlabel('Freq (Hz)')
    ax2.set_ylabel('|Y(freq)|')

    ax1.plot(raw_time, raw_signal, 'k', label='Raw')
    ax1.plot(filtered_time, filtered_signal, 'r', label=filter_name)
    ax2.loglog(raw_frq, raw_Y, 'k', label='Raw')
    ax2.loglog(filt_frq, filt_Y, 'r', label=f"{filter_name} {params}")

    ax1.legend()
    ax2.legend()

    # Sanitize filename and save
    os.makedirs("output_plots", exist_ok=True)
    safe_params = str(params).replace(" ", "_").replace(",", "_").replace("=", "-").replace("(", "").replace(")", "")
    filename = f"output_plots/{signal_name}_{filter_name}_{safe_params}.png"
    plt.savefig(filename)
    plt.close(fig)

def MAF(address, window_size):
    time, signal = load_signal_data(address)
    
    filtered_signal = []
    for i in range(len(signal) - window_size):
        window = signal[i:i+window_size]
        filtered_signal.append(sum(window) / window_size)
    
    filtered_time = np.linspace(time[0], time[-1], len(filtered_signal))
    signal_name = os.path.splitext(os.path.basename(address))[0]
    plot_results(time, signal, filtered_time, filtered_signal, "MAF", f"window={window_size}", signal_name)

def IIR(address, weight_A, weight_B):
    if not np.isclose(weight_A + weight_B, 1.0):
        print("Warning: Weights should sum to 1")

    time, signal = load_signal_data(address)
    
    filtered_signal = [0] * len(signal)
    filtered_signal[0] = signal[0]
    
    for i in range(1, len(signal)):
        filtered_signal[i] = weight_A * filtered_signal[i-1] + weight_B * signal[i]
    
    signal_name = os.path.splitext(os.path.basename(address))[0]
    plot_results(time, signal, time, filtered_signal, "IIR", f"A={weight_A}, B={weight_B}", signal_name)

def FIR(address, coefficients, method):
    time, signal = load_signal_data(address)
    
    filtered_signal = []
    for i in range(len(signal) - len(coefficients)):
        window = signal[i:i+len(coefficients)]
        filtered_signal.append(np.dot(coefficients, window))
    
    filtered_time = np.linspace(time[0], time[-1], len(filtered_signal))
    signal_name = os.path.splitext(os.path.basename(address))[0]
    plot_results(time, signal, filtered_time, filtered_signal, "FIR", f"{method} ({len(coefficients)} taps)", signal_name)

# Signal processing paths and parameters
SIGNAL_PATHS = [
    "sigA.csv",
    "sigB.csv", 
    "sigC.csv",
    "sigD.csv"
]

# Process signals with MAF
for path in SIGNAL_PATHS:
    window = 60 if path != "sigC.csv" else 1
    MAF(path, window)

# Process signals with IIR
IIR_PARAMS = [
    (0.985, 0.015),
    (0.985, 0.015),
    (0, 1),
    (0.95, 0.05)
]
for path, (a, b) in zip(SIGNAL_PATHS, IIR_PARAMS):
    IIR(path, a, b)

# Process signals with FIR
FIR_METHODS = [
    ("Low-Pass Window", h_1),
    ("Low-Pass Window", h_2),
    ("Moving Average", h_3),
    ("Low-Pass Window", h_4)
]
for path, (method, coeffs) in zip(SIGNAL_PATHS, FIR_METHODS):
    FIR(path, coeffs, method)
