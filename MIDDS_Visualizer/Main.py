import threading
from MIDDSParser import MIDDSParser
from ProgramConfiguration import ProgramConfiguration
from GUI2BackendEvents import GUI2BackendEvents
from GUI import GUI
import serial
import time
import traceback
from datetime import datetime

CONFIG_ROUTE = "config.ini"

def backendProcess(events: GUI2BackendEvents, lock, config: ProgramConfiguration):
    def applyChannelsConfiguration(ser):
        if ser is None: return 
        
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
            
            while True:
                newMsg = MIDDSParser.decodeMessage(ser, events.recording)
                if newMsg is None:
                    # No messages, continue with the next channel's settings.
                    break

                if newMsg.get("command") == MIDDSParser.COMMS_MSG_ERROR_HEAD:
                    events.raiseError("MIDDS Error", newMsg.get("message", "(undefined)"))
                    break

    def sendSYNC(ser):
        if ser is None: return False

        # Set the time of the MIDDS and the SYNC channels. 
        ser.write(
            MIDDSParser.encodeSettingsSYNC(
                channel   = int(config['SYNC']['Channel']),
                frequency = float(config['SYNC']['Frequency']),
                dutyCycle = float(config['SYNC']['DutyCycle']),
                time=datetime.now()
            )
        )

    try:
        ser = None 
        while not events.stopEvent.is_set():
            if events.closeSerialPort.is_set():
                if ser is not None:
                    ser.write(b"$DISC")
                    time.sleep(0.1)
                    print(ser.readline())

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
                    events.raiseError("Serial port error", str(e))
                    events.openSerialPort.clear()
                    continue

                # Stablish connection.
                ser.write(b"$CONN")

                time.sleep(0.1)

                # Read the connected to PROTO MIDDS message.
                print(ser.readline())

                sendSYNC(ser)

                events.deviceConnected = True

                applyChannelsConfiguration(ser)
                events.openSerialPort.clear()

            if events.applyChannelsConfiguration.is_set():
                applyChannelsConfiguration(ser)
                events.applyChannelsConfiguration.clear()

            if events.applySettings.is_set():
                sendSYNC(ser)
                events.applySettings.clear()

            if events.startRecording.is_set():
                events.recording = True
                MIDDSParser.createNewRecordingFile()
                events.raiseMessage("Started recording", f"The recording will be stored in '{MIDDSParser.recordingFileName}'.")
                events.startRecording.clear()

            if events.stopRecording.is_set():
                events.recording = False
                events.raiseMessage("Stopped recording", f"The recording is saved in '{MIDDSParser.recordingFileName}'.")
                events.stopRecording.clear()

            if events.sendCommandToMIDDS.is_set():
                if ser is not None:
                    ser.write(events.commandContent)
                events.commandContent = b''
                events.sendCommandToMIDDS.clear()

            if ser is not None:
                try:
                    while True:
                        newMsg = MIDDSParser.decodeMessage(ser, events.recording)
                        if newMsg is None:
                            break

                        if 'channel' in newMsg:
                            lock.acquire()
                            ch = config.getChannel(newMsg['channel'])
                            if ch is not None:
                                ch.updateValues(newMsg)
                            lock.release()
                        else:
                            print(newMsg)

                    # Generate the recurring messages with the UPDATE_TIME from the ProgramConfiguration.
                    if (time.perf_counter() - events.lastCommandRequest) > float(config['ProgramConfig']['UPDATE_TIME_s']):
                        events.lastCommandRequest = time.perf_counter()

                        for ch in config.channels:
                            msg = ch.generateRecurringMessages()
                            if len(msg) > 0:
                                ser.write(msg)
                except Exception as e:
                    ser = None
                    events.deviceConnected = False
                    events.raiseError("Serial port error", str(e))
                    print(traceback.format_exc())

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
    
