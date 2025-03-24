import socket
import json
import hashlib
import os

# Client configuration
HOST = '127.0.0.1'  # Server IP address
PORT = 12345        # Server port

def print_song_metadata(song_data):
    """
    Print the metadata of a song.
    """
    print("Matched Song:")
    print(f"Title: {song_data.get('title')}")
    print(f"Artist: {song_data.get('artist')}")
    print(f"Album: {song_data.get('album')}")
    print(f"Release Date: {song_data.get('release_date')}")
    print(f"Spotify URL: {song_data.get('spotify_url')}")
    print(f"Apple Music URL: {song_data.get('apple_music_url')}")


def calculate_md5(file_path):
    """
    Calculate the MD5 checksum of a file.
    """
    hash_md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def send_file(file_path):
    """
    Send a .wav file to the server and receive metadata + processed file.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((HOST, PORT))

        # Send the .wav file
        with open(file_path, 'rb') as f:
            while True:
                chunk = f.read(4096)
                if not chunk:
                    break
                client_socket.sendall(chunk)
                print(f"Sent {len(chunk)} bytes.")
        client_socket.sendall(b"<END>")
        print("File sent to the server.")

         # Receive metadata
        metadata_bytes = b""
        while True:
            chunk = client_socket.recv(4096)
            metadata_bytes += chunk
            if b"<END_OF_METADATA>" in metadata_bytes:
                break

        # Parse metadata
        metadata_json = metadata_bytes.split(b"<END_OF_METADATA>")[0].decode()
        song_metadata = json.loads(metadata_json)
        print("Received metadata:")
        print_song_metadata(song_metadata)

        # Receive the processed .wav file
        received_file_path = "processed_audio.wav"
        with open(received_file_path, 'wb') as f:
            while True:
                chunk = client_socket.recv(4096)
                if not chunk:
                    break
                f.write(chunk)
                
        # Verify the received file
        received_file_md5 = calculate_md5(received_file_path)
        print(f"MD5 checksum of received file: {received_file_md5}")
        print(f"Processed file received and saved as '{received_file_path}'.")


if __name__ == "__main__":
    # Example usage
    file_path = "Never_gonna_give_you_up_cut.wav"  # Path to the .wav file to send
    send_file(file_path)