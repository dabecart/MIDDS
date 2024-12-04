import serial
import signal
from io import TextIOWrapper
from time import perf_counter, perf_counter_ns

numberOfChannels: int = 4

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
                print(f'{key}: {ch.getFrequency():.4f} ')
                ch.initTx = perf_counter_ns()
                ch.totalCounts = 0

        if serial.in_waiting > 0:
            inputBuffer += serial.read(serial.in_waiting)
            
            if b'\n' not in inputBuffer:
                continue

            lowIndex: int = inputBuffer.find(b'\n')
            message: bytearray = inputBuffer[:lowIndex+1]
            inputBuffer = inputBuffer[lowIndex+1:]

            if len(message) == 0: 
                print("Null length")
                continue

            if message[0] != ord(b'C'):
                print("Missing C")
                continue

            # Message format: C{counterID}.{channelID}:{data}
            #                  \        HEADER       / \DATA/
            bulk = message[1:].split(b':')
            header = bulk[0].split(b'.')
            data = bulk[1]

            if len(header) != 2:
                print("Wrong header length")
                continue

            try:
                counterID = int(header[0])
                channelID = int(header[1])
            except:
                print("Cannot parse header")
                continue
            
            if 0 > channelID >= numberOfChannels:
                print("Wrong channel ID")
                continue

            timChKey: str = f'T{counterID}.{channelID}'
            timCh: TimerChannel|None = channels.get(timChKey, None)
            if timCh is None:
                channels[timChKey] = TimerChannel(timChKey)
                timCh = channels[timChKey]
            
            # Start from 1 forward, as the {data} format is x{hex number}x{hex number}
            numberChunks = data[1:].split(b'x')

            timCh.totalCounts += len(numberChunks)
            timCh.lastTx = perf_counter_ns()
            for hexNum in numberChunks:
                timCh.file.write(f'{int(hexNum, 16)}\n')

    for ch in channels.values():
        ch.file.close()

    print("Exit")