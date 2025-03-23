import struct
from serial import Serial

class MIDDSParser:
    COMMS_MSG_INPUT_LEN             = 13
    COMMS_MSG_OUTPUT_LEN            = 13
    COMMS_MSG_MONITOR_HEADER_LEN    = 8
    COMMS_MSG_FREQ_LEN              = 20
    COMMS_MSG_CHANNEL_SETT_LEN      = 8
    COMMS_MSG_SYNC_SETT_LEN         = 23

    # TODO: Substitute them also for bytes constants down bellow (too tired to do it now).
    COMMS_MSG_INPUT_HEAD            = "I"
    COMMS_MSG_OUTPUT_HEAD           = "O"
    COMMS_MSG_FREQ_HEAD             = "F"
    COMMS_MSG_MONITOR_HEAD          = "M"
    COMMS_MSG_CHANNEL_SETT_HEAD     = "SC"
    COMMS_MSG_SYNC_SETT_HEAD        = "SY"
    COMMS_MSG_ERROR_HEAD            = "E"

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
    def decodeMessage(serial: Serial) -> dict[str,any] | None:
        while True:
            startByte: bytes = serial.read(1)
            if not startByte:
                # No more messages.
                break
            
            if startByte != b'$':
                # Ignore invalid start character.
                continue  
            
            command: bytes = serial.read(1)
            if not command:
                break
            
            if command == b'I':
                data = serial.read(MIDDSParser.COMMS_MSG_INPUT_LEN - 2)
                if len(data) == (MIDDSParser.COMMS_MSG_INPUT_LEN - 2):
                    return MIDDSParser.decodeInput(b'$I' + data) 
            elif command == b'O':
                data = serial.read(MIDDSParser.COMMS_MSG_OUTPUT_LEN - 2)
                if len(data) == (MIDDSParser.COMMS_MSG_OUTPUT_LEN - 2):
                    return MIDDSParser.decodeOutput(b'$O' + data)
            elif command == b'M':
                header = serial.read(MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN - 2)
                if len(header) == (MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN - 2):
                    sampleCount = int(header[4:].decode())
                    sampleData = serial.read(sampleCount * 8)
                    if len(sampleData) == sampleCount * 8:
                        return MIDDSParser.decodeMonitor(b'$M' + header + sampleData)
            elif command == b'F':
                data = serial.read(MIDDSParser.COMMS_MSG_FREQ_LEN - 2)
                if len(data) == (MIDDSParser.COMMS_MSG_FREQ_LEN - 2):
                    return MIDDSParser.decodeFrequency(b'$F' + data) 
            elif command == b'E':
                errorMsg = serial.readline()
                return MIDDSParser.decodeError(b'$E' + errorMsg)
                
        return None

    @staticmethod
    def encodeInput(channel: int|None, readValue: int|None, time: float|None) -> bytes:
        if channel is None or readValue is None or time is None:
            raise ValueError("Invalid encode input values")
        return struct.pack('<cc2ssQ', b'$', b'I', f'{channel:02}'.encode(), f'{readValue:01}'.encode(), time)
    
    @staticmethod
    def decodeInput(data: bytes):
        if len(data) != MIDDSParser.COMMS_MSG_INPUT_LEN:
            raise ValueError("Invalid input message length")
        _, _, channel, readValue, time = struct.unpack('<cc2s1sQ', data)
        if readValue == b'0' or readValue == b'1':
            return {
                "command":      MIDDSParser.COMMS_MSG_INPUT_HEAD,
                "channel":      int(channel.decode()),
                "readValue":    int(readValue.decode()),
                "time":         time
            }
        else:
            return {
                "command":      MIDDSParser.COMMS_MSG_INPUT_HEAD,
                "channel":      int(channel.decode()),
                "time":         time
            }
    
    @staticmethod
    def encodeOutput(channel: int|None, writeValue: int|None, time: float|None) -> bytes:
        if channel is None or writeValue is None or time is None:
            raise ValueError("Invalid encode write values")
        return struct.pack('<cc2ssQ', b'$', b'O', f'{channel:02}'.encode(), f'{writeValue:01}'.encode(), time)
    
    @staticmethod
    def decodeOutput(data: bytes):
        if len(data) != MIDDSParser.COMMS_MSG_OUTPUT_LEN:
            raise ValueError("Invalid output message length")
        _, _, channel, writeValue, time = struct.unpack('<cc2s1sQ', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_OUTPUT_HEAD,
            "channel":      int(channel.decode()),
            "writeValue":   int(writeValue.decode()),
            "time":         time
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
        if len(data) < MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN:
            raise ValueError("Invalid monitor message length")
        _, _, channel, sampleCount = struct.unpack('<cc2s4s', data[:MIDDSParser.COMMS_MSG_MONITOR_HEADER_LEN])
        samples = [struct.unpack('<Q', data[8+i*8:16+i*8])[0] for i in range(int(sampleCount))]
        return {
            "command":      MIDDSParser.COMMS_MSG_MONITOR_HEAD,
            "channel":      int(channel.decode()),
            "sampleCount":  int(sampleCount.decode()),
            "samples":      samples
        }

    @staticmethod
    def encodeFrequency(channel: int|None, time: float|None) -> bytes:
        if channel is None or time is None:
            raise ValueError("Invalid encode frequency values")
        return struct.pack('<cc2sQQ', b'$', b'F', f'{channel:02}'.encode(), 0.0, time)
    
    @staticmethod
    def decodeFrequency(data: bytes):
        if len(data) != MIDDSParser.COMMS_MSG_FREQ_LEN:
            raise ValueError("Invalid frequency message length")
        _, _, channel, frequency, time = struct.unpack('<cc2sQQ', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_OUTPUT_HEAD,
            "channel":      int(channel.decode()),
            "frequency":    frequency,
            "time":         time
        }

    @staticmethod
    def encodeSettingsChannel(channel: int|None, mode: str|None, signal: str|None) -> bytes:
        if channel is None or mode is None or signal is None:
            raise ValueError("Invalid encode Settings Channel values")
        return struct.pack('<ccc2s2s1s', b'$', b'S', b'C', f'{channel:02}'.encode(), mode.encode(), signal.encode())
    
    @staticmethod
    def decodeSettingsChannel(data: bytes):
        if len(data) != 8:
            raise ValueError("Invalid settings channel message length")
        _, _, _, channel, mode, signal, sync = struct.unpack('<ccc2s2s1s1s', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_CHANNEL_SETT_HEAD,
            "channel":      int(channel.decode()),
            "mode":         mode.decode(),
            "signal":       signal.decode()
        }
    
    @staticmethod
    def encodeSettingsSYNC(frequency: str|None, dutyCycle: str|None, time: float|None) -> bytes:
        if frequency is None or dutyCycle is None or time is None:
            raise ValueError("Invalid encode Settings SYNC values")
        return struct.pack('<ccc5s5sQ', b'$', b'S', b'Y', frequency.encode(), dutyCycle.encode(), time)
    
    @staticmethod
    def decodeSettingsSYNC(data: bytes):
        if len(data) != 23:
            raise ValueError("Invalid settings SY message length")
        _, _, _, channel, frequency, dutyCycle, time = struct.unpack('<ccc2s5s5sQ', data)
        return {
            "command":      MIDDSParser.COMMS_MSG_SYNC_SETT_HEAD,
            "channel":      int(channel.decode()),
            "frequency":    frequency.decode(),
            "dutyCycle":    dutyCycle.decode(),
            "time":         time
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
        if not data.startswith(b'$E') or not data.endswith(b'\n'):
            raise ValueError("Invalid error message format")
        return {
            "command": MIDDSParser.COMMS_MSG_ERROR_HEAD,
            "message": data[2:-1].decode()
        }