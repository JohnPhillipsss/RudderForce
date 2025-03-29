import serial
import serial.tools.list_ports
import csv
import os
from datetime import datetime
import re
import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from collections import deque
import time

# Configuration
LOG_DIR = os.path.join(os.path.expanduser('~'), 'Desktop')
BAUD_RATE = 115200
UPDATE_INTERVAL_MS = 100
MAX_DATA_POINTS = 200  # Number of points to show on graph

# Data storage
timestamps = deque(maxlen=MAX_DATA_POINTS)
force_values = deque(maxlen=MAX_DATA_POINTS)
start_time = None  # Track test start time

# Generate unique CSV filename
log_filename = f"rudder_force_log_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.csv"
LOG_FILE = os.path.join(LOG_DIR, log_filename)

# Find Serial Port
def find_serial_port():
    ports = serial.tools.list_ports.comports()
    for p in ports:
        if 'USB' in p.description or 'Serial' in p.description or 'COM' in p.device:
            return p.device
    return None

# Serial Connection Handler
def connect_serial():
    port = find_serial_port()
    if port:
        try:
            ser = serial.Serial(port, BAUD_RATE, timeout=1)
            print(f"Connected to {ser.port}")
            return ser
        except Exception as e:
            print(f"Failed to connect: {e}")
    return None

ser = connect_serial()

# CSV Setup
with open(LOG_FILE, 'w', newline='') as f:
    csv.writer(f).writerow(["Date", "Time", "Force (N)"])

# Create main window
root = tk.Tk()
root.title("Rudder Force Monitor")
root.geometry("1000x600")

# Create figure for plotting
fig = plt.Figure(figsize=(9, 5), dpi=100)
ax = fig.add_subplot(111)
ax.set_xlabel('Time (s)')
ax.set_ylabel('Force (N)')
ax.set_xlim(0, 150)
ax.set_ylim(0, 150)
ax.set_yticks(range(0, 151, 25))  # Markers every 25N
ax.grid(True)
line, = ax.plot([], [], 'b-')

# Create canvas and add to GUI
canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)

# Force display label
force_label = ttk.Label(root, text="Force: -- N", font=("Arial", 24))
force_label.pack(pady=10)

# Status bar
status_var = tk.StringVar()
status_var.set("Ready")
status_bar = ttk.Label(root, textvariable=status_var, relief=tk.SUNKEN)
status_bar.pack(side=tk.BOTTOM, fill=tk.X)

def update_display():
    global ser, start_time
    
    if ser is None or not ser.is_open:
        status_var.set("Reconnecting...")
        ser = connect_serial()
        root.after(2000, update_display)
        return
    
    try:
        if ser.in_waiting > 0:
            raw_data = ser.readline().decode().strip()
            status_var.set(f"Received: {raw_data}")
            
            # Extract numeric value
            match = re.search(r"[-+]?\d*\.\d+|\d+", raw_data)
            if match:
                force_g = float(match.group())
                force_n = force_g * 0.00980665  # Convert grams to Newtons
                now = datetime.now()
                
                # Track start time
                if start_time is None:
                    start_time = now
                elapsed_time = (now - start_time).total_seconds()
                
                # Update data storage
                timestamps.append(elapsed_time)
                force_values.append(force_n)
                
                # Update label
                force_label.config(text=f"Force: {force_n:.2f} N")
                
                # Update plot
                line.set_data(timestamps, force_values)
                ax.set_xlim(0, max(10, elapsed_time))
                ax.relim()
                ax.autoscale_view()
                canvas.draw()
                
                # Log to CSV
                with open(LOG_FILE, 'a', newline='') as f:
                    csv.writer(f).writerow([
                        now.strftime("%Y-%m-%d"),
                        now.strftime("%H:%M:%S"),
                        f"{force_n:.6f}"
                    ])
                
    except serial.SerialException:
        status_var.set("Serial Disconnected. Reconnecting...")
        ser = None  # Force reconnection
    except Exception as e:
        status_var.set(f"Error: {str(e)}")
        print(f"Error: {e}")
    
    root.after(UPDATE_INTERVAL_MS, update_display)

# Configure plot
ax.set_title('Real-time Force Measurement')
ax.grid(True)

# Start updates
update_display()
root.mainloop()

# Cleanup
if ser and ser.is_open:
    ser.close()
