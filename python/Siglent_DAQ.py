#!/usr/bin/python3

"""
Date: June 2025

@author: Jorge Marqués García
@author: JuanAn García Pascual
@description:
    This script is designed to work with the Siglent SDS7000A series oscilloscopes.
    It connects to the oscilloscope, configures it for sequence mode, and captures
    multiple frames of data. The captured data is saved to a CSV file.
"""

import pyvisa
import struct
import os
import time
import math
import os
import pandas as pd
import datetime
import gc
import yaml
import signal
import threading
import re # Added for robust filename parsing
import argparse # Added for command-line argument parsing

## Global Variables, need to check if they are correct
running = True
CHANNEL = "C1"
TDIV_NUM = [100e-12, 200e-12, 500e-12, 1e-9, 2e-9, 5e-9, 10e-9, 20e-9, 50e-9, 100e-9, 200e-9, 500e-9, 
            1e-6, 2e-6, 5e-6, 10e-6, 20e-6, 50e-6, 100e-6, 200e-6, 500e-6,
            1e-3, 2e-3, 5e-3, 10e-3, 20e-3, 50e-3, 100e-3, 200e-3, 500e-3,
            1, 2, 5, 10, 20, 50, 100, 200, 500, 1000]


def signal_handler(sig, frame):
    """
    Este es el manejador de señales.
    Se ejecuta cuando se recibe una señal como SIGINT (Ctrl+C).
    """
    global running # Access global variable
    print(f"\n¡Signal {sig} (eg. Ctrl+C) detected! Stopping...")
    running = False 


def connect_to_scope(address="TCPIP0::192.168.0.102::INSTR"):
    rm = pyvisa.ResourceManager()
    sds = rm.open_resource(f"{address}")
    sds.timeout = 20000  # 20 seconds
    sds.chunk_size = 20 * 1024 * 1024  # 20 MB
    idn = sds.query('*IDN?')
    print(f"IDN: {idn.strip()}")
    sds.clear() # Clean buffers

    return sds

def setup_scope_for_sequence(sds, channel="C1", seq_count=5):
    print("⚙️ Configurando osciloscopio...")
    ch = channel[-1]
    sds.write(":STOP")  # Stop current acquisition
    sds.write("*CLS")  # Clean errors
    sds.write(f":CHAN{ch}:STAT ON")
    sds.write(":ACQ:MODE SEQ")
    sds.write(f":ACQ:SEQ:COUN {seq_count}")
    sds.write(":TRIG:MODE SING")
    #sds.write(":TRIG:RUN")

    print(f"✅ Osciloscope ready for sequence...")

def wait_for_trigger(sds):
    while True:
        status = sds.query(":TRIG:STAT?").strip()
        if status == "Stop":
            #print("🎯 Adquisition completed.")
            break
        time.sleep(0.05)

def get_preamble(sds, channel="C1"):
    sds.write(f":WAV:SOUR {channel}")
    sds.write(":WAV:PRE?")
    pre = sds.read_raw()
    preamble = pre[pre.find(b'#') + 11:]
    #print(f"Preamble: {pre}")

    # Extract preamble basic parameters
    interval = struct.unpack('f', preamble[0xB0:0xB4])[0]
    delay = struct.unpack('d', preamble[0xB4:0xBC])[0]
    tdiv_idx = struct.unpack('h', preamble[0x144:0x146])[0]
    tdiv = TDIV_NUM[tdiv_idx]
    vdiv = struct.unpack('f', preamble[0x9C:0xA0])[0]
    offset = struct.unpack('f', preamble[0xA0:0xA4])[0]
    code = struct.unpack('f', preamble[0xA4:0xA8])[0]
    adc_bit = struct.unpack('h', preamble[0xAC:0xAE])[0]
    one_frame_pts = struct.unpack('i', preamble[0x74:0x78])[0]
    sum_frame = struct.unpack('i', preamble[0x94:0x98])[0]
    read_frame = struct.unpack('i',preamble[0x90:0x93+1])[0]
    
    tmstp = preamble[346:]
    
    preamble_data = {
        "interval": interval, "delay": delay, "tdiv": tdiv, "vdiv": vdiv,
        "offset": offset, "code": code, "adc_bit": adc_bit,
        "one_frame_pts": one_frame_pts, "sum_frame": sum_frame,
        "read_frame": read_frame
    }
    return pd.Series(preamble_data), tmstp 



def main_time_stamp_deal(time):
    seconds = time[0x00:0x08]   ## type:long double
    minutes = time[0x08:0x09] ## type:char
    hours = time[0x09:0x0a] ## type:char
    days = time[0x0a:0x0b] ## type:char
    months = time[0x0b:0x0c] ## type:char
    year = time[0x0c:0x0e] ## type:short
    seconds = struct.unpack('d',seconds)[0]
    minutes = struct.unpack('c', minutes)[0]
    hours = struct.unpack('c', hours)[0]
    days = struct.unpack('c', days)[0]
    months = struct.unpack('c', months)[0]
    year = struct.unpack('h', year)[0]
    months = int.from_bytes(months, byteorder='big', signed=False)
    days = int.from_bytes(days, byteorder='big', signed=False)
    hours = int.from_bytes(hours, byteorder='big', signed=False)
    minutes = int.from_bytes(minutes, byteorder='big', signed=False)
    try:
        base_time = datetime.datetime(year, months, days, hours, minutes)
        full_time = base_time + datetime.timedelta(seconds=seconds)
        return full_time.timestamp()
    except Exception as e:
         print(f"❌ Error parsing timestamp: {e}")
         return 0

def read_sequence_raw_frames(sds, channel="C1"):
    ##Setup sequence
    sds.write(f":WAV:SOUR {channel}")
    sds.write(":WAV:STAR 0")
    sds.write(":WAV:INT 1")
    sds.write(":WAV:POIN 0")
    sds.write(":WAV:WIDT BYTE")  # 8-bit
    sds.write(":WAVeform:SEQUence 0,0")
    
    
    df_preamble, tmstp = get_preamble(sds, channel="C1")
    one_frame_pts = df_preamble["one_frame_pts"]
    total_frames = df_preamble["sum_frame"]
    read_frame= df_preamble["read_frame"] # Max frames oscilloscope can send in one WAV:DATA? response
    adc_bit = df_preamble["adc_bit"]
    #print(f"TMstp: {tmstp}")
    # This list will store dictionaries, each representing a frame with its ADC data and its timestamp.
    all_frames_data = []

    read_times = math.ceil(total_frames/read_frame)

    for i in range(0,read_times):
        sds.write(":WAVeform:SEQUence {},{}".format(0,read_frame*i+1)) #First sequence acquisition
        if i+1 == read_times:     #frame num of last read time
            read_frame = total_frames -(read_times-1)*read_frame
        
        if adc_bit > 8:
            sds.write(":WAVeform:WIDTh WORD")
        sds.write(":WAVeform:DATA?")        #get data for each sequence acquisition
        raw = sds.read_raw().rstrip()
        #print(f"{raw}")
        block_start = raw.find(b'#')
        data_digit = int(raw[block_start + 1:block_start+2])
        data_start = block_start + 2 + data_digit
        data = raw[data_start:]
        
        for j in range(0,int(read_frame)):
            time = tmstp[16*j:16*(j+1)]
            frame_timestamp = main_time_stamp_deal(time)
            if adc_bit > 8:
                start = int(j * one_frame_pts*2)
                end = int((j + 1) * one_frame_pts*2)
                adc_data = struct.unpack("%dh" % one_frame_pts, data[start:end])
            else:
                start = int(j*one_frame_pts)
                end = int((j+1)*one_frame_pts)
                adc_data = struct.unpack("%db" % one_frame_pts, data[start:end])
            
            frame_info = {
                "timestamp": frame_timestamp,
                "adc_data": adc_data
            }
            all_frames_data.append(frame_info)

    del data
    gc.collect()
                
    df_all_frames = pd.DataFrame(all_frames_data)

    return df_all_frames, df_preamble


def save_acquisition_data_to_csv(df_preamble, df_frames, deadtime_s: float = 0.0, filename: str = "acquisition_data.csv"):
    """
    Store DAQ data in a CSV file.
    First preamble and then the datafranes
    """
    if df_preamble.empty and df_frames.empty:
        print(f"Dataframe and preamble empty, not saved on {filename}")
        return

    df_preamble['deadtime_us'] = deadtime_s * 1_000_000 # Store in us for convenience

    with open(filename, 'a', newline='') as f:
        if not df_frames.empty:
            df_frames.to_csv(f, index=False, header=True, mode="a")

def find_next_available_filename(base_filename: str) -> int:
    """
    Finds the next available filename based on a base name, an incrementing
    file number, and a fixed 'n_files' suffix.
    """
    file_number = 0  # Will be incremented before first use
    n_files_suffix = 0

    while True:
        # Format the filename using f-strings for clear and concise formatting
        # %04d for file_number (e.g., 0000, 0001, ...)
        # %02d for n_files_suffix (e.g., 01, 02, ...)
        output_filename = f"{base_filename}{file_number:04d}.csv.{n_files_suffix:02d}"
        file_number += 1  # Increment file number

        # Check if the file exists. os.path.exists() is the Python equivalent of stat() == 0
        if not os.path.exists(output_filename):
            break  # File does not exist, so this is our target filename

    return file_number

def read_config_from_yaml(file_path: str) -> dict:
    
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"The configuration file was not found at: {file_path}")

    try:
        with open(file_path, 'r') as file:
            config = yaml.safe_load(file)
            
            # Ensure all required fields are present
            required_fields = ['address', 'fileName', 'nFrames', 'eventsPerFile']
            for field in required_fields:
                if field not in config:
                    raise ValueError(f"Missing required field in YAML: '{field}'")
            
            # Optional: Add type checking for the loaded values
            if not isinstance(config['address'], str):
                raise ValueError("address must be a string.")
            if not isinstance(config['fileName'], str):
                raise ValueError("fileName must be a string.")
            if not isinstance(config['nFrames'], int):
                raise ValueError("nFrames must be an integer.")
            if not isinstance(config['eventsPerFile'], int):
                raise ValueError("eventsPerFile must be an integer.")
            
            return config
    except yaml.YAMLError as exc:
        # Catch specific YAML parsing errors
        raise yaml.YAMLError(f"Error parsing YAML file '{file_path}': {exc}")
    except Exception as e:
        # Catch any other unexpected errors during file processing
        raise Exception(f"An unexpected error occurred while reading '{file_path}': {e}")


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    
    # 1. Setup argparse for command-line arguments
    parser = argparse.ArgumentParser(description="Script to adquire Siglent SDS7000A pulses in sequence mode.")
    parser.add_argument(
        "-c","--config", 
        type=str, 
        default="siglent.yaml", 
        help="Config file in YAML (default: siglent.yaml)"
    )
    parser.add_argument(
        "-e","--events", 
        type=int, 
        default=None, 
        help="Maximum number of events to acquire (optional). Otherwise press CTR-C to stop the acquisition."
    )
    
    pargs = parser.parse_args()

    sds = None
    save_thread = None # Initialize save_thread to None
    
    try:
        nEvents_in_current_file = 0
        total_events = 0
        config = read_config_from_yaml(pargs.config)

        print("\n--- YAML Configuration ---")
        for key, value in config.items():
            print(f"{key}: {value}")
        print("--------------------------\n")

        base_name = config['fileName']
        # Get initial file number and suffix components
        current_file_number = find_next_available_filename(base_name)
        file_number = find_next_available_filename(config['fileName'])
        n_files_suffix = 0
        output_filename = f"{base_name}{file_number:04d}.csv.{n_files_suffix:02d}"
        sds = connect_to_scope(ip=config['address'])
        # Configura para capturar nFrames
        setup_scope_for_sequence(sds, channel="C1", seq_count=config['nFrames'])

        while running:
            sds.write(":TRIG:RUN") # Re-arm trigger for continuous acquisition
            # Wait for acquisition to be completed
            wait_for_trigger(sds)
            # --- Time counter for read_sequence_raw_frames (Deadtime check) ---
            start_time_read_frames = time.perf_counter()
            
            df_captured_frames, df_global_preamble = read_sequence_raw_frames(sds, channel="C1")

            end_time_read_frames = time.perf_counter()
            deadtime_s = end_time_read_frames - start_time_read_frames
            
            # Start saving data in a separate thread
            # Use .copy() to ensure dataframes are not modified by the main thread after being passed
            save_thread = threading.Thread(
                target=save_acquisition_data_to_csv,
                args=(df_global_preamble.copy(), df_captured_frames.copy(), deadtime_s, output_filename)
            )
            save_thread.start() # Start the thread
            
            nEvents_in_current_file += config['nFrames']
            total_events += config['nFrames']
            print(f"Acquired {base_name} frames. Total events for '{output_filename}': {nEvents_in_current_file} / {total_events}")
            
            # Check if the events per file limit is reached
            if nEvents_in_current_file >= config['eventsPerFile'] == 0:
                #print(f"Events per file limit ({config['eventsPerFile']}) reached for '{output_filename}'.")
                current_n_files_suffix += 1
                nEvents_in_current_file = 0
                output_filename = f"{base_name}{current_file_number:04d}.csv.{current_n_files_suffix:02d}"
                #print(f"Next output file will be: {output_filename}")

            # Check if max_events limit has been reached
            if pargs.events is not None and total_events >= pargs.events:
                print(f"Reached maximum total events ({pargs.events}). Stopping acquisition.")
                running = False
                break # Exit the while loop

        print("🎯 Adquisition completed.")

    except pyvisa.errors.VisaIOError as e:
        print(f"❌ VISA ERROR: {e}")
        print("Make sure that the oscilloscope address is correct, scope is connected and SCPI is enabled.")
    except FileNotFoundError as e:
        print(f"❌ Config YAML error: {e}. Make sure that {pargs.events} exist.")
    except ValueError as e:
        print(f"❌ Config YAML error: {e}. Check fileds in {pargs.events}.")
    except yaml.YAMLError as e:
        print(f"❌ Error parsing YAML: {e}. Check sintax in {pargs.events}.")
    except Exception as e:
        print(f"❌ Unkown error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        if sds:
            sds.write(":STOP")
            sds.close()
            print("🔌 Conection to scope closed.")
        # Ensure the saving thread completes before the program truly exits
        if save_thread and save_thread.is_alive():
            print("Waiting for final data saving to complete...")
            save_thread.join()
            print("Final data saving thread finished.")



