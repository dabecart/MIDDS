import serial
import time
import traceback
from datetime import datetime
from GUI2BackendEvents import GUI2BackendEvents
from ProgramConfiguration import ProgramConfiguration
from MIDDSParser import MIDDSParser

class MIDDSMaster:
    def __init__(self, events: GUI2BackendEvents, lock, config: ProgramConfiguration):
        self.ser:       serial.Serial|None      = None
        self.events:    GUI2BackendEvents       = events
        self.lock                               = lock
        self.config:    ProgramConfiguration    = config
        self.decoderMIDDS:   MIDDSParser             = MIDDSParser()

    def applyChannelsConfiguration(self):
        if self.ser is None: return 
        
        try: 
            # Set channel configuration.
            for ch in self.config.channels:
                self.ser.write(
                    MIDDSParser.encodeSettingsChannel(
                        channel = ch.number,
                        mode    = ch.mode,
                        signal  = ch.signal
                    )
                )
                
                time.sleep(5e-3) #Await a response from MIDDS (if any). 
                
                while True:
                    newMsg = self.decoderMIDDS.decodeMessage(self.ser.read(self.ser.in_waiting), 
                                                             self.events.recording)
                    if newMsg is None:
                        # No messages, continue with the next channel's settings.
                        break

                    if newMsg.get("command") == MIDDSParser.COMMS_MSG_ERROR_HEAD:
                        self.events.raiseError("MIDDS Error", newMsg.get("message", "(undefined)"))
                        break
        except Exception as e:
            self.ser = None
            self.events.deviceConnected = False
            self.events.raiseError("Error applying configuration", str(e))
            print(traceback.format_exc())

    def sendSYNC(self):
        if self.ser is None: return

        # Set the time of the MIDDS and the SYNC channels. 
        self.ser.write(
            MIDDSParser.encodeSettingsSYNC(
                channel   = int(self.config['SYNC']['Channel']),
                frequency = float(self.config['SYNC']['Frequency']),
                dutyCycle = float(self.config['SYNC']['DutyCycle']),
                time=datetime.now()
            )
        )

    def handleCloseSerialPortEvent(self):
        if not self.events.closeSerialPort.is_set(): return
        
        if self.ser is not None:
            self.ser.write(b"$DISC")
            time.sleep(0.1)
            print(self.ser.readline())

            self.ser.close()
            self.ser = None
        self.events.deviceConnected = False
        self.events.closeSerialPort.clear()

    def handleOpenSerialPortEvent(self):
        if not self.events.openSerialPort.is_set(): return

        try:
            self.ser = serial.Serial(port = self.config['ProgramConfig']['SERIAL_PORT'], 
                                        baudrate = 2000000, 
                                        timeout = 0.001)
        except Exception as e:
            self.ser = None
            self.events.deviceConnected = False
            self.events.raiseError("Serial port error", str(e))
            self.events.openSerialPort.clear()
            return

        # Stablish connection.
        self.ser.write(b"$CONN")

        time.sleep(0.1)

        # Read the connected to PROTO MIDDS message.
        print(self.ser.readline())

        self.sendSYNC()

        self.events.deviceConnected = True

        self.applyChannelsConfiguration()
        self.sendSYNC()
        self.events.openSerialPort.clear()

    def handleApplyChannelsConfigurationEvent(self):
        if not self. events.applyChannelsConfiguration.is_set(): return

        self.applyChannelsConfiguration()
        self.events.applyChannelsConfiguration.clear()

    def handleApplySettingsEvent(self):
        if not self.events.applySettings.is_set(): return
        
        self.sendSYNC()
        self.events.applySettings.clear()

    def handleStartRecordingEvent(self):
        if not self.events.startRecording.is_set(): return
        
        self.events.recording = True
        self.decoderMIDDS.createNewRecordingFile()
        self.events.raiseMessage("Started recording", f"The recording will be stored in '{self.decoderMIDDS}'.")
        self.events.startRecording.clear()

    def handleStopRecordingEvent(self):
        if not self.events.stopRecording.is_set(): return

        self.events.recording = False
        self.events.raiseMessage("Stopped recording", f"The recording is saved in '{MIDDSParser.recordingFileName}'.")
        self.events.stopRecording.clear()

    def handleSendCommandToMIDDSEvent(self):
        if not self.events.sendCommandToMIDDS.is_set(): return

        if self.ser is not None:
            self.ser.write(self.events.commandContent)
        self.events.commandContent = b''
        self.events.sendCommandToMIDDS.clear()

    def communicationsHandling(self):
        if self.ser is None: return

        try:
            while True:
                newMsg = self.decoderMIDDS.decodeMessage(self.ser.read(self.ser.in_waiting),
                                                         self.events.recording)
                if newMsg is None:
                    break

                if 'channel' in newMsg:
                    self.lock.acquire()
                    ch = self.config.getChannel(newMsg['channel'])
                    if ch is not None:
                        ch.updateValues(newMsg)
                    self.lock.release()
                else:
                    print(newMsg)

            # Generate the recurring messages with the UPDATE_TIME from the ProgramConfiguration.
            if (time.perf_counter() - self.events.lastCommandRequest) > float(self.config['ProgramConfig']['UPDATE_TIME_s']):
                self.events.lastCommandRequest = time.perf_counter()

                for ch in self.config.channels:
                    msg = ch.generateRecurringMessages()
                    if len(msg) > 0:
                        self.ser.write(msg)
        except Exception as e:
            self.ser = None
            self.events.deviceConnected = False
            self.events.raiseError("Serial port error", str(e))
            print(traceback.format_exc())

    def backendProcess(self):
        try:
            while not self.events.stopEvent.is_set():
                self.handleCloseSerialPortEvent()

                self.handleOpenSerialPortEvent()

                self.handleApplyChannelsConfigurationEvent()

                self.handleApplySettingsEvent()

                self.handleStopRecordingEvent()

                self.handleStartRecordingEvent()

                self.handleSendCommandToMIDDSEvent()

                self.communicationsHandling()

                # Some time is needed to be left for the server to be updated.
                time.sleep(5e-3)
        except KeyboardInterrupt:
            pass

        if self.ser is not None:
            self.ser.close()
