import os
from audd_api import recognize_song_using_audd_api
from audd_api import get_api_token
from yt_dlp import YoutubeDL        

def download_song_from_youtube(song_name, artist_name, output_format="wav"):
    """
    Download a song from YouTube as a .wav file using yt-dlp.
    
    :param song_name: Name of the song
    :param artist_name: Name of the artist
    :param output_format: Desired output format (e.g., wav, mp3)
    :return: Path to the downloaded song file
    """
    # Create folder for downloaded songs
    output_folder = "predicted_audio_files"
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    query = f"{song_name} {artist_name} official audio"
    
    ydl_opts = {
        "format": "bestaudio/best",  # Download the best quality audio
        "postprocessors": [{
            "key": "FFmpegExtractAudio",  # Extract audio
            "preferredcodec": output_format,  # Output format
            "preferredquality": "192",  # Audio quality
        }],
        "outtmpl": os.path.join(output_folder, f"{song_name} - {artist_name}.%(ext)s"),  # Output file name
    }
    
    with YoutubeDL(ydl_opts) as ydl:
        ydl.download([f"ytsearch1:{query}"])  # Search YouTube and download the first result

    return os.path.join(output_folder, f"{song_name} - {artist_name}.{output_format}")


def print_song_metadata(song_data):
    """
    Print the metadata of a song.
    """
    print("Matched Song:")
    print(f"Title: {song_data.get('title')}")
    print(f"Artist: {song_data.get('artist')}")
    print(f"Album: {song_data.get('album')}")
    print(f"Release Date: {song_data.get('release_date')}")
    print(f"Spotify URL: {song_data.get('spotify', {}).get('external_urls', {}).get('spotify')}")
    print(f"Apple Music URL: {song_data.get('apple_music', {}).get('url')}")


def format_song_metadata(song_data):
    """
    Format the song metadata into a dictionary matching the print_song_metadata format.
    """
    return {
        "title": song_data.get("title"),
        "artist": song_data.get("artist"),
        "album": song_data.get("album"),
        "release_date": song_data.get("release_date"),
        "spotify_url": song_data.get("spotify", {}).get("external_urls", {}).get("spotify"),
        "apple_music_url": song_data.get("apple_music", {}).get("url"),
    }


def recognize_and_download_song(audio_file_path):
    """
    Recognize a song from a .wav file and download it from YouTube.
    
    :param audio_file_path: Path to the .wav file
    :param api_token: Your AudD API token
    :return: Song metadata and path to the downloaded song file
    """
    api_token = get_api_token()
    
    # Recognize the song
    song_data = recognize_song_using_audd_api(audio_file_path, api_token)
    
    if song_data:
        # Display the song metadata
        print_song_metadata(song_data)
        song_metadata = format_song_metadata(song_data)
        
        # Download the song from YouTube
        song_name = song_data.get("title")
        artist_name = song_data.get("artist")
        print(f"\nDownloading '{song_name}' by '{artist_name}' from YouTube...")
        predicted_song_path = download_song_from_youtube(song_name, artist_name, output_format="wav")
        print("Download complete!")
    else:
        print("No match found.")

    return song_metadata, predicted_song_path