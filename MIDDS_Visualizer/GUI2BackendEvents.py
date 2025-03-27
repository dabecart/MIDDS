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
        self.applyConfiguration = threading.Event()
        self.newMIDDSError = threading.Event()

        self.errTitle:              str             = ""
        self.errDate:               datetime|None   = None
        self.errContent:            str             = ""

        self.deviceConnected:       bool            = False
        self.recording:             bool            = False

        self.lastCommandRequest:    float           = time.perf_counter()

    def setError(self, errTitle: str, errContent: str):
        self.errDate = datetime.now()
        self.errTitle = errTitle
        self.errContent = errContent
        self.newMIDDSError.set()
        
        print(f"{self.errTitle}: {self.errContent}")