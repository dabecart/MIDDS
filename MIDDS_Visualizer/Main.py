import threading
from MIDDSParser import MIDDSParser
from ProgramConfiguration import ProgramConfiguration
from GUI2BackendEvents import GUI2BackendEvents
from GUI import GUI
import serial
import time

CONFIG_ROUTE = "config.ini"

def backendProcess(events: GUI2BackendEvents, lock, config: ProgramConfiguration):
    def applyChannelsConfiguration(ser) -> bool:
        if ser is None:
            return False
        
        # Set channel configuration.
        for ch in config.channels:
            ser.write(
                MIDDSParser.encodeSettingsChannel(
                    channel = ch.number,
                    mode = ch.mode,
                    signal = ch.signal
                )
            )
            
            time.sleep(5e-3) #Await a response from MIDDS (if any). 
            
            ch.wellConfigured = True
            while True:
                newMsg = MIDDSParser.decodeMessage(ser)
                if newMsg is None:
                    # No messages, continue with the next channel's settings.
                    break

                if newMsg.get("command") == MIDDSParser.COMMS_MSG_ERROR_HEAD:
                    ch.wellConfigured = False
                    events.setError("MIDDS Error", newMsg.get("message", "(undefined)"))
                    break

        return True

    try:
        ser = None 
        while not events.stopEvent.is_set():
            if events.closeSerialPort.is_set():
                if ser is not None:
                    ser.write(b"$DISC")

                    ser.close()
                    ser = None
                events.deviceConnected = False
                events.closeSerialPort.clear()

            if events.openSerialPort.is_set():
                try:
                    ser = serial.Serial(port = config['ProgramConfig']['SERIAL_PORT'], 
                                        baudrate = 1000000, 
                                        timeout = 0.001)
                except Exception as e:
                    ser = None
                    events.deviceConnected = False
                    events.setError("Serial port error", str(e))
                    events.openSerialPort.clear()
                    continue

                # Stablish connection.
                ser.write(b"$CONN")

                events.deviceConnected = True

                applyChannelsConfiguration(ser)
                events.openSerialPort.clear()

            if events.applyConfiguration.is_set():
                applyChannelsConfiguration(ser)
                events.applyConfiguration.clear()

            if ser is not None:
                while True:
                    newMsg = MIDDSParser.decodeMessage(ser)
                    if newMsg is None or 'channel' not in newMsg:
                        break

                    lock.acquire()
                    ch = config.getChannel(newMsg['channel'])
                    if ch is not None:
                        ch.updateValues(newMsg)
                    lock.release()

                for ch in config.channels:
                    msg = ch.generateRecurringMessages()
                    if msg is not None:
                        ser.write(msg)

            time.sleep(5e-3)

        if ser is not None:
            ser.close()
    except KeyboardInterrupt:
        pass

if __name__ == '__main__':
    config = ProgramConfiguration(CONFIG_ROUTE)
    events = GUI2BackendEvents()

    channelsLock = threading.Lock()
    decoderProcess = threading.Thread(target=backendProcess, 
                                      args=(events, channelsLock, config),
                                      daemon=True)
    decoderProcess.start()

    try:
        gui = GUI(channelsLock, events, config)
        gui.run()
    except KeyboardInterrupt:
        pass

    events.stopEvent.set()
    decoderProcess.join()

    config.saveConfig()
    
