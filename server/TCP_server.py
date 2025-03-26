import socket
import os
import uuid
import json
from song_recognition import recognize_and_download_song

# Server configuration
HOST = '192.168.6.1'  # Listen on all available interfaces
PORT = 12345      # Port to listen on
BUFFER_SIZE = 4096  # Buffer size for receiving data
SAVE_DIR = "received_audio_files"  # Directory to save received .wav files

# Ensure the save directory exists
if not os.path.exists(SAVE_DIR):
    os.makedirs(SAVE_DIR)


def save_file(file_path, data):
    """
    Save the received data to a file.
    """
    with open(file_path, 'wb') as f:
        f.write(data)
    print(f"File saved: {file_path}")


def generate_unique_filename():
    """
    Generate a unique filename for the received .wav file.
    """
    return f"received_audio_{uuid.uuid4().hex}.wav"


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

                    # Check if the end of the file was reached
                    if file_data.endswith(b"<END>"):
                        file_data = file_data[:-5]
                        break
                    
                    print(f"Received {len(file_data)} bytes.")

                # Save the received file
                file_name = generate_unique_filename()
                file_path = os.path.join(SAVE_DIR, file_name)
                save_file(file_path, file_data)

                # Process the .wav file
                song_metadata, song_file_path = recognize_and_download_song(file_path)
                print("Formatted metadata:", song_metadata)
                print("File path:", song_file_path)

                # Send the metadata to the client
                metadata_json = json.dumps(song_metadata)
                metadata_bytes = metadata_json.encode()

                client_socket.sendall(metadata_bytes)

                # Send a delimiter to indicate end of metadata
                client_socket.sendall(b"<END_OF_METADATA>")

                # Send the processed .wav file to the client
                with open(song_file_path, 'rb') as f:
                    while True:
                        file_chunk = f.read(BUFFER_SIZE)
                        if not file_chunk:
                            break
                        client_socket.sendall(file_chunk)
                        
                print("Metadata and file sent to the client.")
            except Exception as e:
                print(f"Error handling client: {e}")
                client_socket.sendall(b"Error processing file.")
            finally:
                client_socket.close()
                print(f"Connection with {client_address} closed.")

if __name__ == "__main__":
    start_server()