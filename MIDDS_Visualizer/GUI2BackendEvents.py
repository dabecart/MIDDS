import threading
from datetime import datetime
import time

class GUI2BackendEvents:
    def __init__(self):
        self.stopEvent = threading.Event()
        self.openSerialPort = threading.Event()
        self.closeSerialPort = threading.Event()
        self.startRecording = threading.Event()
        self.stopRecording = threading.Event()
        self.applyChannelsConfiguration = threading.Event()
        self.applySettings = threading.Event()

        self.newMIDDSError = threading.Event()
        self.newMIDDSMessage = threading.Event()

        self.errTitle:              str             = ""
        self.errDate:               datetime|None   = None
        self.errContent:            str             = ""

        self.msgTitle:              str             = ""
        self.msgDate:               datetime|None   = None
        self.msgContent:            str             = ""

        self.deviceConnected:       bool            = False
        self.recording:             bool            = False

        self.lastCommandRequest:    float           = time.perf_counter()

    def raiseError(self, errTitle: str, errContent: str):
        self.errDate = datetime.now()
        self.errTitle = errTitle 
        self.errContent = errContent
        self.newMIDDSError.set()
        
        print(f"ERROR: {self.errTitle}: {self.errContent}")

    def raiseMessage(self, msgTitle: str, msgContent: str):
        self.msgDate = datetime.now()
        self.msgTitle = msgTitle
        self.msgContent = msgContent
        self.newMIDDSMessage.set()
        
        print(f"MESSAGE: {self.msgTitle}: {self.msgContent}")