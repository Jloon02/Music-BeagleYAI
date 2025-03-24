import requests
import os
from dotenv import load_dotenv

def get_api_token():
    """
    Get the API token from environment variables.
    """
    # Load the environment variables
    load_dotenv()

    api_token = os.getenv("AUDD_API_TOKEN")
    if not api_token:
        raise ValueError("API token not found in environment variables.")
    return api_token

def recognize_song_using_audd_api(audio_file_path, api_token):
    """
    Recognize a song using the AudD API.
    
    :param audio_file_path: Path to the .wav file
    :param api_token: Your AudD API token
    :return: Song metadata (title, artist, etc.)
    """
    # AudD API endpoint
    url = "https://api.audd.io/"
    
    # Open the audio file in binary mode
    with open(audio_file_path, "rb") as audio_file:
        # Prepare the payload for the API request
        files = {
            "file": audio_file
        }
        data = {
            "api_token": api_token,
            "return": "timecode,apple_music,spotify",  # Optional: Get additional metadata
        }
        
        # Send the request to the API
        response = requests.post(url, files=files, data=data)
        
        # Parse the JSON response
        result = response.json()
        
        # Check if the request was successful
        if result.get("status") == "success":
            return result.get("result")
        else:
            print("Error:", result.get("error", "Unknown error"))
            return None
