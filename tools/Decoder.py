# **************************************************************************************************
# @file Decoder.py
# @brief Listens to the serial port, decodes the input and stores the incoming timestamps on 
# different .csv files, one file per channel.
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
    serial = serial.Serial("COM6", baudrate=10000000)
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
            
            while True:
                lowIndex: int = inputBuffer.find(b'$')
                if lowIndex < 0: break

                inputBuffer = inputBuffer[lowIndex:]
                if len(inputBuffer) < 2: break

                parsedData: list[int]|None = None
                try:
                    if inputBuffer[1] == ord(b'M'):
                        if len(inputBuffer) < 8: break

                        # Monitoring message.
                        channelNumber   = int(inputBuffer[2:4])
                        sampleCount     = int(inputBuffer[4:8])
                        # print(inputBuffer[:8+sampleCount*8])

                        if decodeInASCII:
                            if len(inputBuffer) < (8+16*sampleCount): break
                            # Timestamps are coming in bulks of ASCII hexadecimal numbers.
                            data        = inputBuffer[8 : 8+sampleCount*16]
                            parsedData  = [int(data[i*16:(i + 1)*16], 16) for i in range(sampleCount)]
                            # If everything went OK, remove the message from the input buffer.
                            inputBuffer = inputBuffer[8+sampleCount*16:]
                        else:
                            if len(inputBuffer) < (8+8*sampleCount): break
                            # Timestamps are in binary form, as uint64_t numbers.
                            data        = inputBuffer[8 : 8+sampleCount*8]
                            parsedData  = [struct.unpack('<Q', data[i*8:(i + 1)*8])[0] for i in range(sampleCount)]
                            # If everything went OK, remove the message from the input buffer.
                            inputBuffer = inputBuffer[8+sampleCount*8:]
                    else:
                        raise Exception()
                except:
                    # On exception, discard the '$' at the start. 
                    inputBuffer = inputBuffer[1:]

                if parsedData is None:
                    continue

                timChKey: str = f'Ch{channelNumber}'
                timCh: TimerChannel|None = channels.get(timChKey, None)
                if timCh is None:
                    # If the timer save file didn't exist, create it.
                    channels[timChKey] = TimerChannel(timChKey)
                    timCh = channels[timChKey]
                    print(f'Added new timer {timChKey}')

                # The captured values also have the current level of the signal on its LSB.
                # parsedData = [(x >> 1) for x in parsedData]

                timCh.totalCounts += len(parsedData)
                timCh.lastTx = perf_counter_ns()
                for hexNum in parsedData:
                    timCh.file.write(f'{hexNum}\n')    


    for ch in channels.values():
        ch.file.close()

    print("Exit")