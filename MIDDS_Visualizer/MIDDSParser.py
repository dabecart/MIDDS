import struct
from datetime import datetime

class MIDDSParser:
    COMMS_MSG_SYNC                  = '$'

    COMMS_MSG_INPUT_LEN             = 13
    COMMS_MSG_OUTPUT_LEN            = 13
    COMMS_MSG_MONITOR_HEADER_LEN    = 8
    COMMS_MSG_FREQ_LEN              = 28
    COMMS_MSG_CHANNEL_SETT_LEN      = 8
    COMMS_MSG_SYNC_SETT_LEN         = 29

    # TODO: Substitute them also for bytes constants down bellow (too tired to do it now).
    COMMS_MSG_INPUT_HEAD            = "I"
    COMMS_MSG_OUTPUT_HEAD           = "O"
    COMMS_MSG_FREQ_HEAD             = "F"
    COMMS_MSG_MONITOR_HEAD          = "M"
    COMMS_MSG_CHANNEL_SETT_HEAD     = "SC"
    COMMS_MSG_SYNC_SETT_HEAD        = "SY"
    COMMS_MSG_ERROR_HEAD            = "E"

    COMMS_ERROR_MAX_LEN             = 64

    FILENAME_HEAD: str              = "MIDDS_REC_"

    def __init__(self):
        self.inputMsg: bytes                 = b''
        self.recordingFileName: str          = ""

    def popNFromInputMsg(self, n: int) -> bytes:
        toRet = self.inputMsg[:n]
        self.inputMsg = self.inputMsg[n:]
        return toRet 

    def discardMessage(self, readMsg: bytes):
        # Discard the first byte of the read message, append it to the deque again and 
        # recalculate.
        self.inputMsg = readMsg[1:] + self.inputMsg
    
    # Decodes a stream of data. Each serialData get appended and get used to form the message which
    # may have been splitted apart.
    def decodeMessage(self, serialData: bytes, record: bool = False) -> dict[str,any] | None:
        # Record data if currently recording.
        if record:
            if self.recordingFileName == "":
                # The recording has just started. Generate a file name. It should not enter this 
                # line! Call createNewRecordingFile beforehand.
                self.createNewRecordingFile()

            with open(self.recordingFileName, 'ab') as f:
                f.write(serialData)
        elif self.recordingFileName != "":
            # End recording.
            self.recordingFileName = ""

        # Add the new data to the previous stored data.
        self.inputMsg += serialData
        return self.decodeLoop()

    def decodeLoop(self)  -> dict[str,any] | None:
        readMsg: bytes = b''
        while len(self.inputMsg) > 0:
            if self.inputMsg[0:1] != b'$':
                # Ignore invalid start character.
                self.inputMsg = self.inputMsg[1:]
                continue  

            if len(self.inputMsg) < 2:
                break

            readMsg = self.inputMsg[0:2]
            if readMsg == b'$I':
                if len(self.inputMsg) < MIDDSParser.COMMS_MSG_INPUT_LEN:
                    # Not enough data. Wait for the next iteration.
                    continue
                
                readMsg = self.popNFromInputMsg(MIDDSParser.COMMS_MSG_INPUT_LEN)
                try:
                    return MIDDSParser.decodeInput(readMsg) 
                except:
                    # Error while decoding. 
                    self.discardMessage(readMsg)

            elif readMsg == b'$O':
                if len(self.inputMsg) < MIDDSParser.COMMS_MSG_OUTPUT_LEN:
                    # Not enough data. Wait for the next iteration.
                    continue

                readMsg = self.popNFromInputMsg(MIDDSParser.COMMS_MSG_OUTPUT_LEN)
                try:
                    return MIDDSParser.decodeOutput(readMsg) 
                except:
                    # Error while decoding. Discard the first byte of the read message, append it
                    # to the deque again and recalculate.
                    self.discardMessage(readMsg)
                
            elif readMsg == b'$M':
                if len(self.inputMsg) < MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN:
                    # Not enough data. Wait for the next iteration.
                    break

                try:
                    sampleCount = int(self.inputMsg[4:8].decode())
                except:
                    # If the sample count is not a number, the message is to be discarded.
                    # As nothing has been popped, remove the first item of the input message.
                    self.inputMsg = self.inputMsg[1:]
                    break

                if len(self.inputMsg) < (MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN + sampleCount*8):
                    # Not enough bytes.
                    break

                readMsg = self.popNFromInputMsg(MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN + sampleCount * 8)
                try:
                    return MIDDSParser.decodeMonitor(readMsg) 
                except:
                    # Error while decoding. Discard the first byte of the read message, append it
                    # to the deque again and recalculate.
                    self.discardMessage(readMsg)
                
            elif readMsg == b'$F':
                if len(self.inputMsg) < MIDDSParser.COMMS_MSG_FREQ_LEN:
                    # Not enough data. Wait for the next iteration.
                    break

                readMsg = self.popNFromInputMsg(MIDDSParser.COMMS_MSG_FREQ_LEN)
                
                try:
                    return MIDDSParser.decodeFrequency(readMsg) 
                except:
                    # Error while decoding. Discard the first byte of the read message, append it
                    # to the deque again and recalculate.
                    self.discardMessage(readMsg)

            elif readMsg == b'$E':
                while (len(self.inputMsg) > 0) and \
                      (len(readMsg) < MIDDSParser.COMMS_ERROR_MAX_LEN) and \
                      ((readChar := self.popNFromInputMsg(1)) != b'\n'):
                    readMsg += readChar

                if readChar != b'\n':
                    # There are still some characters missing from the error message.
                    self.reconstructMessage(readMsg)
                    break

                if len(readMsg) >= MIDDSParser.COMMS_ERROR_MAX_LEN:
                    # Too many characters for an error message. Discard message.
                    self.discardMessage(readMsg)
                    continue

                try:
                    return MIDDSParser.decodeError(readMsg)
                except:
                    # Error while decoding. Discard the first byte of the read message, append it
                    # to the deque again and recalculate.
                    self.discardMessage(readMsg)
                
        return None

    def createNewRecordingFile(self):
        # The recording has just started. Generate a file name.
        self.recordingFileName = MIDDSParser.FILENAME_HEAD + datetime.now().strftime('%Y-%m-%d_%H-%M-%S')

    @staticmethod
    def encodeMessage(msgDict: dict[str,any] | None) -> bytes | None:
        if msgDict is None or "command" not in msgDict:
            return

        command = msgDict["command"]
        if command == MIDDSParser.COMMS_MSG_INPUT_HEAD:
            return MIDDSParser.encodeInput(msgDict.get("channel"), 
                                           msgDict.get("readValue"), 
                                           msgDict.get("time"))
        elif command == MIDDSParser.COMMS_MSG_OUTPUT_HEAD:
            return MIDDSParser.encodeOutput(msgDict.get("channel"), 
                                            msgDict.get("writeValue"), 
                                            msgDict.get("time"))
        elif command == MIDDSParser.COMMS_MSG_FREQ_HEAD:
            return MIDDSParser.encodeFrequency(msgDict.get("channel"),
                                               msgDict.get("frequency"))
        elif command == MIDDSParser.COMMS_MSG_CHANNEL_SETT_HEAD:
            return MIDDSParser.encodeSettingsChannel(msgDict.get("channel"),
                                                    msgDict.get("mode"),
                                                    msgDict.get("signal"))
        elif command == MIDDSParser.COMMS_MSG_SYNC_SETT_HEAD:
            return MIDDSParser.encodeSettingsSYNC(msgDict.get("channel"),
                                                  msgDict.get("mode"),
                                                  msgDict.get("signal"))
        return None
    
    @staticmethod
    def encodeInput(channel: int|None, readValue: int|None, time: int|datetime|None) -> bytes:
        if channel is None or readValue is None or time is None:
            raise ValueError("Invalid encode input values")
        if type(time) is datetime:
            time = MIDDSParser.fromDatetimeToUNIX_(time)
        return struct.pack('<cc2ssQ', b'$', b'I', f'{channel:02}'.encode(), f'{readValue:01}'.encode(), time)
    
    @staticmethod
    def decodeInput(data: bytes):
        _, _, channel, readValue, time = struct.unpack('<cc2s1sQ', data)
        if readValue == b'0' or readValue == b'1':
            return {
                "command":      MIDDSParser.COMMS_MSG_INPUT_HEAD,
                "channel":      int(channel.decode()),
                "readValue":    int(readValue.decode()),
                "time":         MIDDSParser.fromUNIXToTimestamp_(time)
            }
        else:
            return {
                "command":      MIDDSParser.COMMS_MSG_INPUT_HEAD,
                "channel":      int(channel.decode()),
                "time":         MIDDSParser.fromUNIXToTimestamp_(time)
            }
    
    @staticmethod
    def encodeOutput(channel: int|None, writeValue: int|None, time: datetime|int|None) -> bytes:
        if channel is None or writeValue is None or time is None:
            raise ValueError("Invalid encode write values")
        if type(time) is datetime:
            time = MIDDSParser.fromDatetimeToUNIX_(time)
        return struct.pack('<cc2ssQ', b'$', b'O', f'{channel:02}'.encode(), f'{writeValue:01}'.encode(), time)
    
    @staticmethod
    def decodeOutput(data: bytes):
        _, _, channel, writeValue, time = struct.unpack('<cc2s1sQ', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_OUTPUT_HEAD,
            "channel":      int(channel.decode()),
            "writeValue":   int(writeValue.decode()),
            "time":         MIDDSParser.fromUNIXToTimestamp_(time)
        }
    
    @staticmethod
    def encodeMonitor(channel: int|None, samples: list|None) -> bytes:
        if channel is None or samples is None:
            raise ValueError("Invalid encode Monitor values")

        sampleCount = f'{len(samples):04}'.encode()
        sampleData = b''.join(struct.pack('<Q', s) for s in samples)
        return b'$M' + f'{channel:02}'.encode() + sampleCount + sampleData
    
    @staticmethod
    def decodeMonitor(data: bytes):
        _, _, channel, sampleCount = struct.unpack('<cc2s4s', data[:MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN])
        samples = [struct.unpack('<Q', data[8+i*8:16+i*8])[0] for i in range(int(sampleCount))]
        return {
            "command":      MIDDSParser.COMMS_MSG_MONITOR_HEAD,
            "channel":      int(channel.decode()),
            "sampleCount":  int(sampleCount.decode()),
            "samples":      samples
        }

    @staticmethod
    def encodeFrequency(channel: int|None, time: int|datetime|None) -> bytes:
        if channel is None or time is None:
            raise ValueError("Invalid encode frequency values")
        if type(time) is datetime:
            time = MIDDSParser.fromDatetimeToUNIX_(time)
        return struct.pack('<cc2sddQ', b'$', b'F', f'{channel:02}'.encode(), 0.0, 0.0, time)
    
    @staticmethod
    def decodeFrequency(data: bytes):
        _, _, channel, frequency, dutyCycle, time = struct.unpack('<cc2sddQ', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_FREQ_HEAD,
            "channel":      int(channel.decode()),
            "frequency":    frequency,
            "dutyCycle":    dutyCycle,
            "time":         MIDDSParser.fromUNIXToTimestamp_(time)
        }

    @staticmethod
    def encodeSettingsChannel(channel: int|None, mode: str|None, signal: str|None) -> bytes:
        if channel is None or mode is None or signal is None:
            raise ValueError("Invalid encode Settings Channel values")
        return struct.pack('<ccc2s2s1s', b'$', b'S', b'C', f'{channel:02}'.encode(), mode.encode(), signal.encode())
    
    @staticmethod
    def decodeSettingsChannel(data: bytes):
        _, _, _, channel, mode, signal, sync = struct.unpack('<ccc2s2s1s1s', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_CHANNEL_SETT_HEAD,
            "channel":      int(channel.decode()),
            "mode":         mode.decode(),
            "signal":       signal.decode()
        }
    
    @staticmethod
    def encodeSettingsSYNC(channel: int|None, frequency: float|None, dutyCycle: float|None, time: int|datetime|None) -> bytes:
        if channel is None or frequency is None or dutyCycle is None or time is None:
            raise ValueError("Invalid encode Settings SYNC values")
        if type(time) is datetime:
            time: int = MIDDSParser.fromDatetimeToUNIX_(time)
        return struct.pack('<ccc2sddQ', b'$', b'S', b'Y', f'{channel:02}'.encode(), frequency, dutyCycle, time)
    
    @staticmethod
    def decodeSettingsSYNC(data: bytes):
        _, _, _, channel, frequency, dutyCycle, time = struct.unpack('<ccc2sddQ', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_SYNC_SETT_HEAD,
            "channel":      int(channel.decode()),
            "frequency":    frequency,
            "dutyCycle":    dutyCycle,
            "time":         MIDDSParser.fromUNIXToTimestamp_(time)
        }
    
    @staticmethod
    def encodeError(message: str|None) -> bytes:
        if message is None:
            raise ValueError("Invalid encode Error values")
        if len(message) > 63:
            raise ValueError("Invalid error message's length in encode Error values")
        return b'$E' + message.encode() + b'\n'
    
    @staticmethod
    def decodeError(data: bytes):
        return {
            "command": MIDDSParser.COMMS_MSG_ERROR_HEAD,
            "message": data[2:-1].decode(errors="backslashreplace")
        }
    
    @staticmethod
    def fromDatetimeToUNIX_(d: datetime) -> int:
        # MIDDS accepts UNIX timestamps in nanoseconds.
        return int(d.timestamp() * 1e9)
    
    @staticmethod
    def fromUNIXToTimestamp_(t: int) -> datetime:
        # MIDDS returns UNIX time in nanoseconds, convert to datetime.
        return datetime.fromtimestamp(t / 1e9)