import serial 
import json
import time
import datetime
import sys
from Adafruit_IO import *

import Adafruit_GPIO.SPI as SPI
import Adafruit_SSD1306

from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont

# Create the Adafruit IO client
aio = Client('YOUR_TOKEN_HERE')

serialPort = serial.Serial(port="/dev/ttyS0", baudrate=9600)
disp = Adafruit_SSD1306.SSD1306_128_32(rst=None)

def initDisplay():
    disp.begin()
    disp.clear()
    disp.display()

def updateScreen(title, info):
    image = Image.new('1', (disp.width, disp.height))

    draw = ImageDraw.Draw(image)

    # Draw a black filled box to clear the image.
    draw.rectangle((0, 0, disp.width, disp.height), outline=0, fill=0)
    padding = -2

    font = ImageFont.truetype("Nunito-Regular.ttf", 24)
    titleFont = ImageFont.truetype("Nunito-Regular.ttf", 10)

    draw.rectangle((0, 0, disp.width, disp.height), outline=0, fill=0)
    draw.text((0, padding), title, font=titleFont, fill=255)
    draw.text((0, padding + 7), info, font=font, fill=255)

    # Show the generated image on the screen
    disp.image(image)
    disp.display()

def readSerialLine(ser):
    global last_received

    buffer_string = ''
    while True:
        buffer_string = buffer_string + ser.read(ser.inWaiting())
        if '\n' in buffer_string:
            lines = buffer_string.split('\n') # Guaranteed to have at least 2 entries
            last_received = lines[-2]
            buffer_string = lines[-1]

            return last_received

# Open an error log 
errorLogFile = open("error.log","a+",0)
initDisplay()

while True:
    try:
        serialPort.flushOutput()
        serialPort.flushInput()
        serialPort.write("\n\r")
        serialPort.write("\n\r")
        
        serialPort.flushOutput()
        line = readSerialLine(serialPort)
        
        line = line.strip()
        
        try:
            try:
                jsonObject = json.loads(line)
                
                if jsonObject["iaQStatus"] == 0:
                    
                    updateScreen("Temp", str(jsonObject["temp"]) + "c")
                    
                    # Send the data to Adafruit
                    aio.send('co2', jsonObject["co2"])
                    aio.send('tvoc', jsonObject["tvoc"])
                    aio.send('temp', jsonObject["temp"])
                    aio.send('humidity', jsonObject["humidity"])
                    aio.send('pressure', jsonObject["pressure"])
                else:
                    # The IAQ-Core is not ready yet, any data it sends is invalid
                    aio.send('errors', "Waiting for ready. Status: " + str(jsonObject["iaQStatus"]))
                    
                    if jsonObject["iaQStatus"] == 160:
                        # The IAQ-Core needs to warm up when it first starts. This is normal
                        updateScreen("Please wait", "Warmup")
                    else:
                        # Some other error happened. 
                        updateScreen("Please wait", "Err" + str(jsonObject["iaQStatus"]))

            except ValueError:
                # The JSON object from the sensor modual was not valid, log the error and continue
                aio.send('errors', "Failed to parse JSON object: " + line)
                updateScreen("Error", "JsonParse")

            except RequestError:
                # Failed to connect to Adafruit IO, drop the reading and back off for two mins before retring.
                errorLogFile.write("RequestError: " + str(e) + "\n")
                updateScreen("Error", "HTTP")

                time.sleep(2 * 60)

        except:
            # Some other exception. Log the error continue.
            e = sys.exc_info()
            errorLogFile.write("Unhandled exception: " + str(e) + "\n")
            updateScreen("Error", "Exception")

    except:
        # Failed to communicate with the sensor modual. Log the error and continue.
        e = sys.exc_info()
        errorLogFile.write("Unhandled exception: " + str(e) + "\n")
        updateScreen("Error", "SerialFail")

    time.sleep(30)
