@echo off
setlocal enabledelayedexpansion

:: Check if any files are passed as arguments (for drag-and-drop)
if "%~1"=="" (
    :: No files are dragged and dropped
    echo Welcome to the ESP32 Audio Converter!
    echo Drop audio files onto this script to convert them to ESP32 compatible format.
    echo Converting to 48kHz, unsigned 8-bit WAV...
    echo No files provided for conversion.
    echo Drag and drop audio files onto this script to convert them.
    pause
    exit
)

:: Files are dragged and dropped
echo Welcome to the ESP32 Audio Converter!
echo Converting to 48kHz, unsigned 8-bit WAV...
echo.

:: Define the destination folder for converted files
set "destination_folder=Converted"

:: Create the destination folder if it doesn't exist
if not exist "%destination_folder%" mkdir "%destination_folder%"

:: Set the path to the FFmpeg executable
set "ffmpeg_executable=C:\ffmpeg\bin\ffmpeg.exe"

:: Check if FFmpeg is installed
if not exist "%ffmpeg_executable%" (
    echo FFmpeg is not installed or not found at "%ffmpeg_executable%"
    echo Please install FFmpeg to use this script.
    echo Download FFmpeg from https://ffmpeg.org/download.html
    echo Guide for installation: https://ffmpeg.org/documentation.html
    pause
    exit
)

:: Function to convert files
:ConvertFile
set "input_file=%~1"
set "output_file=%destination_folder%\%~n1_converted.wav"

:: Use ffmpeg to perform the conversion to 48 kHz unsigned 8-bit WAV
"%ffmpeg_executable%" -i "!input_file!" -acodec pcm_u8 -ar 48000 "!output_file!" -y
echo Converted "!input_file!" to "!output_file!"
goto :eof

:: Convert drag-and-drop files
for %%A in (%*) do call :ConvertFile "%%A"
echo.
echo Conversion complete.
:: Open the 'Converted' folder in File Explorer
start "" "%destination_folder%"
pause
