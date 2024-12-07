# **************************************************************************************************
# @file Decoder.py
# @brief Listens to the serial port, decodes the input and stores it on .csv files.
#
# @project   MIDDS
# @version   1.0
# @date      2024-12-07
# @author    @dabecart
#
# @license   This project is licensed under the MIT License - see the LICENSE file for details.
# **************************************************************************************************

import serial
import signal
from io import TextIOWrapper
from time import perf_counter, perf_counter_ns

numberOfChannels: int   = 4
decodeInASCII: bool     = False

if not decodeInASCII:
    import struct

class LoopKiller:
    exitNow = False
    def __init__(self):
        signal.signal(signal.SIGINT, self.exitLoop)
        signal.signal(signal.SIGTERM, self.exitLoop)

    def exitLoop(self, signum, frame):
        self.exitNow = True

class TimerChannel:
    def __init__(self, name: str):
        self.name: str = name
        self.file: TextIOWrapper = open(f'Input{name}.csv', 'w')
        self.initTx: int = perf_counter_ns() 
        self.lastTx: int = perf_counter_ns()
        self.totalCounts: int = 0
    
    def getFrequency(self) -> float:
        return self.totalCounts * 1e9 / (self.lastTx - self.initTx)

if __name__ == '__main__':
    killer = LoopKiller()
    serial = serial.Serial("COM5", baudrate=921600)
    inputBuffer: bytearray = bytearray()

    channels: dict[str, TimerChannel] = {}
    lastPrint: int = 0

    # Clear waiting bytes before entering the loop.
    serial.read(serial.in_waiting)
    
    while not killer.exitNow:
        if (perf_counter() - lastPrint) > 5.0:
            lastPrint = perf_counter()
            for key, ch in channels.items():
                if ch.totalCounts < 2: continue
                print(f'{key}: {ch.getFrequency():.4f} ')
                ch.initTx = perf_counter_ns()
                ch.totalCounts = 0

        if serial.in_waiting > 0:
            inputBuffer += serial.read(serial.in_waiting)
            
            if b'\n' not in inputBuffer:
                continue

            parsedData: list[int]|None = None
            if decodeInASCII:
                lowIndex: int = inputBuffer.find(b'\n')
                message: bytearray = inputBuffer[:lowIndex+1]
                inputBuffer = inputBuffer[lowIndex+1:]

                if len(message) < 8: 
                    continue

                if not message.startswith(b'T'):
                    print("Missing T at the start")
                    continue

                # Message format: T{counterID}.{channelID}.{countNumber:02}:{data}
                #                  \              HEADER                  / \DATA/
                bulk = message[1:].split(b':')
                header = bulk[0].split(b'.')
                data = bulk[1]

                if len(header) != 3:
                    print("Wrong header length")
                    continue

                try:
                    counterID   = int(header[0])
                    channelID   = int(header[1])
                    countNumber = int(header[2])
                except:
                    print(f"Cannot parse header")
                    continue
                
                if 0 > channelID >= numberOfChannels:
                    print("Wrong channel ID")
                    continue

                # Start from 1 forward, as the {data} format is x{hex number}x{hex number}
                parsedData = [int(hexNum, 16) for hexNum in data[1:].split(b'x')]
                if len(parsedData) != countNumber:
                    print(message)
                    print("Data count wrong")
                    continue

            else:
                lowIndex: int = inputBuffer.find(b'T')
                while(lowIndex != -1):
                    message: bytearray = inputBuffer[lowIndex:]

                    if len(message) < 8 \
                       or message[2] != ord(b'.') or message[4] != ord(b'.') \
                       or message[7] != ord(b':'):
                        lowIndex = inputBuffer.find(b'T', lowIndex+1)
                        continue

                    try:
                        counterID   = int(chr(message[1]))
                        channelID   = int(chr(message[3]))
                        countNumber = int(message[5:7])
                    except:
                        lowIndex = inputBuffer.find(b'T', lowIndex+1)
                        continue
                    
                    if 0 > channelID >= numberOfChannels:
                        print("Wrong channel ID")
                        lowIndex = inputBuffer.find(b'T', lowIndex+1)
                        continue

                    data = message[8:(8 + 8*countNumber)]

                    if len(data) != 8*countNumber:
                        lowIndex = inputBuffer.find(b'T', lowIndex+1)
                        continue
                    
                    # A single data sample is represented by 8 bytes.
                    parsedData = [struct.unpack('<Q', data[i*8:(i + 1)*8])[0] for i in range(countNumber)]

                    # If this point was reached, the input buffer can be cleansed from this previous 
                    # message.
                    inputBuffer = inputBuffer[lowIndex + (8 + 8*countNumber):]
                    break

            if parsedData is None:
                continue

            timChKey: str = f'T{counterID}.{channelID}'
            timCh: TimerChannel|None = channels.get(timChKey, None)
            if timCh is None:
                # If the timer save file didn't exist, create it.
                channels[timChKey] = TimerChannel(timChKey)
                timCh = channels[timChKey]

            # The captured values also have the current level of the signal on its LSB.
            values = [(x >> 1) for x in parsedData]

            timCh.totalCounts += len(values)
            timCh.lastTx = perf_counter_ns()
            for hexNum in values:
                timCh.file.write(f'{hexNum}\n')    


    for ch in channels.values():
        ch.file.close()

    print("Exit")