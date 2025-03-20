import socket
import os
from scipy.io import wavfile

# Server configuration
HOST = '0.0.0.0'  # Listen on all available interfaces
PORT = 12345      # Port to listen on
BUFFER_SIZE = 4096  # Buffer size for receiving data
SAVE_DIR = "audio_files"  # Directory to save received .wav files

# Ensure the save directory exists
if not os.path.exists(SAVE_DIR):
    os.makedirs(SAVE_DIR)

def process_wav_file(file_path):
    """
    Process the received .wav file.
    For example, read its metadata or perform analysis.
    """
    try:
        sample_rate, data = wavfile.read(file_path)
        print(f"Processing .wav file: {file_path}")
        print(f"Sample rate: {sample_rate} Hz")
        print(f"Data shape: {data.shape}")
        print(f"Data type: {data.dtype}")
        # Add more processing logic here if needed
    except Exception as e:
        print(f"Error processing .wav file: {e}")

def save_file(file_path, data):
    """
    Save the received data to a file.
    """
    with open(file_path, 'wb') as f:
        f.write(data)
    print(f"File saved: {file_path}")

def start_server():
    """
    Start the TCP server to receive .wav files.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((HOST, PORT))
        server_socket.listen(1)
        print(f"Server listening on {HOST}:{PORT}")

        while True:
            print("Waiting for a client to connect...")
            client_socket, client_address = server_socket.accept()
            print(f"Client connected: {client_address}")

            try:
                # Receive the file data
                file_data = b""
                while True:
                    chunk = client_socket.recv(BUFFER_SIZE)
                    if not chunk:
                        break
                    file_data += chunk

                # Save the received file
                file_name = "received_audio.wav"
                file_path = os.path.join(SAVE_DIR, file_name)
                save_file(file_path, file_data)

                # Process the .wav file
                process_wav_file(file_path)

                # Send a response to the client
                client_socket.sendall(b"File received and processed successfully.")
            except Exception as e:
                print(f"Error handling client: {e}")
                client_socket.sendall(b"Error processing file.")
            finally:
                client_socket.close()
                print(f"Connection with {client_address} closed.")

if __name__ == "__main__":
    start_server()