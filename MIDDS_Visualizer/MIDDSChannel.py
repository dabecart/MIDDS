import dataclasses
from datetime import datetime
from collections import deque
from typing import Final, get_origin
from MIDDSParser import MIDDSParser
import ast

@dataclasses.dataclass
class MIDDSChannel:
    MAX_POINTS:                     Final[int]   = dataclasses.field(default=1000   , metadata={"export": False})
    MAX_DELTA_POINTS:               Final[int]   = dataclasses.field(default=10000  , metadata={"export": False})
    TIMEOUT_UNTIL_UNKNOWN_LEVEL_s:  Final[float] = dataclasses.field(default=5.0    , metadata={"export": False})
    MIN_TIME_BETWEEN_PLOT_MEAS_s:   Final[float] = dataclasses.field(default=0.2    , metadata={"export": False})

    # Signal: T (TTL) or L (LVDS)
    name:           str                 = dataclasses.field(default=""              , metadata={"export": True})
    number:         int                 = dataclasses.field(default=0               , metadata={"export": True})
    mode:           str                 = dataclasses.field(default="DS"            , metadata={"export": True})
    signal:         str                 = dataclasses.field(default="T"             , metadata={"export": True})
    modeSettings:   dict[str, any]      = dataclasses.field(default_factory = dict  , metadata={"export": True})
    toRecord:       bool                = dataclasses.field(default=False           , metadata={"export": True})
    
    _channelLevel:          str         = dataclasses.field(default="?"             , metadata={"export": False})
    lastLevelUpdate:        datetime    = dataclasses.field(default=datetime.now()  , metadata={"export": False})
    _frequency:             float       = dataclasses.field(default=-1.0            , metadata={"export": False})
    _dutyCycle:             float       = dataclasses.field(default=-1.0            , metadata={"export": False})
    lastFrequencyUpdate:    datetime    = dataclasses.field(default=datetime.now()  , metadata={"export": False})

    # For input channels...
    levels:         deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS), metadata={"export": False})
    freqs:          deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS), metadata={"export": False})
    dutyCycles:     deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS), metadata={"export": False})
    freqsUpdates:   deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS), metadata={"export": False})

    # For monitoring channels...
    samples:                deque       = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS), metadata={"export": False})
    # deltas is a deque of tuples: (deltaTime: float, isHighEdge: bool) 
    # Whilst monitoring both edges, if isHighEdge then deltaTime is measured from a low period on the wave. 
    # If monitoring only high or only low, then deltaTime is always the period of the wave.
    deltas:                 deque       = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_DELTA_POINTS), metadata={"export": False})
    lastSampleForDelta:     int         = dataclasses.field(default = -1, metadata={"export": False})
    _risingDelta:           float       = dataclasses.field(default = -1.0, metadata={"export": False})
    _fallingDelta:          float       = dataclasses.field(default = -1.0, metadata={"export": False})

    @property
    def channelLevel(self) -> str:
        dt = datetime.now() - self.lastLevelUpdate
        if dt.total_seconds() > MIDDSChannel.TIMEOUT_UNTIL_UNKNOWN_LEVEL_s:
            return "?"
        else:
            return self._channelLevel
    
    @channelLevel.setter
    def channelLevel(self, val: str):
        self.lastLevelUpdate = datetime.now()
        self._channelLevel = val

    @property
    def frequency(self) -> float:
        dt = datetime.now() - self.lastFrequencyUpdate
        if dt.total_seconds() > MIDDSChannel.TIMEOUT_UNTIL_UNKNOWN_LEVEL_s:
            return "?"
        else:
            return f"{self._frequency:.9f}"
    
    @property
    def dutyCycle(self) -> float:
        dt = datetime.now() - self.lastFrequencyUpdate
        if dt.total_seconds() > MIDDSChannel.TIMEOUT_UNTIL_UNKNOWN_LEVEL_s:
            return "?"
        else:
            return f"{self._dutyCycle:.9f}"

    def setFrequencyAndDutyCycle(self, freq: float, dutyCycle: float, time: datetime):
        self._frequency = freq
        self._dutyCycle = dutyCycle
        # Use the now datetime so that if the MIDDS is set to an older UNIX base time it is still
        # shown on the interface.
        self.lastFrequencyUpdate = datetime.now()
        
        if len(self.freqsUpdates) > 1:
            dt = time - self.freqsUpdates[-1]
            # Give time between points to plot. If not given, high frequency signals will appear 
            # "shorter" than low frequency signals. The latter will expand more on the horizontal,
            # leaving the fast frequencies on a narrower space.
            if dt.total_seconds() < MIDDSChannel.MIN_TIME_BETWEEN_PLOT_MEAS_s:
                return

        self.freqs.append(self._frequency)
        self.dutyCycles.append(self._dutyCycle)
        self.freqsUpdates.append(time)

    @property
    def signalType(self) -> str:
        if self.signal == "T":
            return "TTL"
        elif self.signal == "L":
            return "LVDS"
        else:
            return "Invalid signal"

    """
    If working as a Monitor Rising Edges, this value is the period of the wave.
    If working as Monitor Both Edges, this value is the time length of the low section of the wave.
    """
    @property
    def risingDelta(self) -> str:
        return f'{self._risingDelta*1e9 : .1f} ns'

    """
    If working as a Monitor Falling Edges, this value is the period of the wave.
    If working as Monitor Both Edges, this value is the time length of the high section of the wave.
    """
    @property
    def fallingDelta(self) -> str:
        return f'{self._fallingDelta*1e9 : .1f} ns'

    def generateRecurringMessages(self) -> bytes:
        msg: bytes = b''
        if self.mode == "IN":
            if self.modeSettings.get("INRequestLevel", False):
                msg += MIDDSParser.encodeInput(self.number, 0, 0)
            if self.modeSettings.get("INRequestFrequency", False):
                msg += MIDDSParser.encodeFrequency(self.number, 0)
        elif self.mode == "OU":
            if self.modeSettings.get("OUAutoRequestLevel", False):
                msg += MIDDSParser.encodeInput(self.number, 0, 0)
        return msg

    def updateValues(self, values: dict[str,any]):
        msgCommand = values.get("command", None)
        if msgCommand is None: return

        if self.mode == "IN":
            if msgCommand == MIDDSParser.COMMS_MSG_INPUT_HEAD:
                read = values.get("readValue", "?")
                if read == 0:
                    self.channelLevel = "LOW"
                elif read == 1:
                    self.channelLevel = "HIGH"
                else:
                    self.channelLevel = "?"
            elif msgCommand == MIDDSParser.COMMS_MSG_FREQ_HEAD:
                freq = values.get("frequency", -1.0)
                duty = values.get("dutyCycle", -1.0)
                time = values.get("time", datetime.now())
                self.setFrequencyAndDutyCycle(freq, duty, time)
        elif self.mode == "OU":
            if msgCommand == MIDDSParser.COMMS_MSG_INPUT_HEAD:
                read = values.get("readValue", "?")
                if read == 0:
                    self.channelLevel = "LOW"
                elif read == 1:
                    self.channelLevel = "HIGH"
                else:
                    self.channelLevel = "?"
        elif self.mode in ("MR", "MF", "MB"):
            readSamples: list[int] = values.get("samples", None)
            if readSamples is None: 
                return

            self.samples.extend(readSamples)
            # if len(self.freqsUpdates) > 0 and len(self.samples) > 0:
            #     dt = datetime.fromtimestamp((self.samples[-1] >> 1) / 1e9) - self.freqsUpdates[-1]
            #     if dt.total_seconds() < MIDDSChannel.MIN_TIME_BETWEEN_PLOT_MEAS_s:
            #         return
                
            self.calculateChannelFrequencyFromTimestamps()
            self.calculateDeltas(readSamples)

    def calculateDeltas(self, currentSamples: list[int]):
        if len(currentSamples) <= 0:
            return
        
        if self.lastSampleForDelta != -1:
            currentSamples = [self.lastSampleForDelta, ] + currentSamples
            
        self.lastSampleForDelta = currentSamples[-1]
        
        if len(currentSamples) < 2:
            return

        for s0, s1 in zip(currentSamples[:-1], currentSamples[1:]):
            delta: float = ((s1 >> 1) - (s0 >> 1)) / 1e9
            self.deltas.append((delta, (s1 & 0x01) == 1)) 

        if self.mode == "MB":
            if len(self.deltas) < 2:
                return
            
            if (self.lastSampleForDelta & 0x01) == 1:
                # Last sample was rising. 
                self._risingDelta = self.deltas[-2][0]
                self._fallingDelta = self.deltas[-1][0]
            else:
                # Last sample was falling. 
                self._fallingDelta = self.deltas[-2][0]
                self._risingDelta = self.deltas[-1][0]

            # self.setFrequencyAndDutyCycle(
            #     freq = 1.0/(self._risingDelta + self._fallingDelta),
            #     dutyCycle = self._risingDelta/(self._risingDelta + self._fallingDelta)*100.0,
            #     time = datetime.fromtimestamp((self.lastSampleForDelta >> 1) / 1e9)
            # )
        elif self.mode == "MR":
            self._risingDelta = self.deltas[-1][0]
            self._fallingDelta = -1.0
        elif self.mode == "MF":
            self._risingDelta = -1.0
            self._fallingDelta = self.deltas[-1][0]
        else:
            self._risingDelta = -1.0
            self._fallingDelta = -1.0

    def calculateChannelFrequencyFromTimestamps(self):
        if len(self.samples) < 5: return

        sampleList: tuple[int] = tuple(self.samples)
        
        firstRising: bool = True
        periodSum_ns: float = 0.0
        risedTimeSum_ns: float = 0.0
        cycleCount: int = 0
        previousRisingTime_ns: int = 0

        lastWasRising = False
        for i, sample in enumerate(sampleList):
            isRising:       bool = (sample & 0x01) == 1
            timestamp_ns:   int  = sample >> 1

            if not isRising:
                # Continue until a rising edge is found.
                if firstRising: 
                    continue

                # Break the loop if the last edge is a falling edge. The calculations must end on a 
                # rising edge.
                if i == (len(sampleList) - 1): 
                    break

            if isRising:
                currentDelta = timestamp_ns - previousRisingTime_ns
                if not firstRising:
                    periodSum_ns += currentDelta
                    cycleCount += 1
                previousRisingTime_ns = timestamp_ns
                firstRising = False

                if lastWasRising: 
                    print(f"Freq error H Ch {self.number}")
                    self.samples.clear()
                    return
                lastWasRising = True
            else:
                risedTimeSum_ns += timestamp_ns - previousRisingTime_ns
                if not lastWasRising: 
                    print(f"Freq Error L Ch {self.number}")
                    self.samples.clear()
                    return
                lastWasRising = False

        if cycleCount > 0:
            # Set the frequency, duty cycle and time. Time will be the last time of the timestamp.
            dutyCycle = -1.0
            if self.mode == "MB":
                # Duty cycle can only be calculated with both edges.
                dutyCycle = risedTimeSum_ns * 100.0 / periodSum_ns

            self.setFrequencyAndDutyCycle(
                freq = cycleCount/periodSum_ns*1e9,
                dutyCycle = dutyCycle,
                time = datetime.fromtimestamp((sampleList[-1] >> 1) / 1e9)
            )
            
    def filterModeSettings(self):
        # The channel options always start with the channel mode code ("IN", "OU"...).
        toRemoveKeys: list[str] = []
        for key in self.modeSettings.keys():
            if not key.startswith(self.mode):
                toRemoveKeys.append(key)
        
        for key in toRemoveKeys:
            del self.modeSettings[key]

    def clearValues(self):
        self.levels.clear()
        self.freqs.clear()
        self.dutyCycles.clear()
        self.freqsUpdates.clear()
        self.samples.clear()
        self.deltas.clear()
        self.lastSampleForDelta = -1

        self._channelLevel = "?"
        self._frequency = -1.0
        self._dutyCycle = -1.0

    def copyFrom(self, other):
        self.name           = str(other.name)           
        self.number         = int(other.number)
        self.mode           = str(other.mode)
        self.signal         = str(other.signal)
        self.modeSettings   = dict(other.modeSettings)
        self.toRecord       = bool(other.toRecord)

    def toDict(self) -> dict[str,any]:
        self.filterModeSettings()
        return {k: v for k, v in dataclasses.asdict(self).items() if self.__dataclass_fields__[k].metadata.get("export", True)}

    def getChannelOptionsForChecklist(self) -> list[str]:
        return [keyName for keyName, value in self.modeSettings.items() if value]

    @classmethod
    def castDictToDataclass(cls, data):
        casted_data = {}
        for field in dataclasses.fields(cls):
            field_type = field.type
            if field.name in data:
                if field_type == int:
                    casted_data[field.name] = int(data[field.name])
                elif field_type == float:
                    casted_data[field.name] = float(data[field.name])
                elif field_type == bool:
                    casted_data[field.name] = data[field.name].lower() in ("true", "1", "yes")
                elif field_type == datetime:
                    casted_data[field.name] = datetime.strptime(data[field.name], "%Y-%m-%d %H:%M:%S")
                elif get_origin(field_type) == dict:
                    casted_data[field.name] = ast.literal_eval(data[field.name])
                else:
                    casted_data[field.name] = data[field.name]
        return cls(**casted_data)
    
class MIDDSChannelOptions:
    #   Name of the option          Description                 Default value
    INOptions = {
        "INRequestLevel"        : ("Request channel level",     True),
        "INRequestFrequency"    : ("Request frequency",         True),
        "INPlotFreqInGraph"     : ("Add to frequency graph",    False),
        "INPlotDutyCycleInGraph": ("Add to duty cycle graph",   False),
    }

    OUOptions = {
        "OUAutoRequestLevel"    : ("Request level periodically",True),
    }

    MROptions = {
        "MRCalculateFrequency"  : ("Calculate frequency",       True),
        "MRPlotFreqInGraph"     : ("Add to frequency graph",    False),
        "MRPlotDeltasInGraph"   : ("Add to deltas graph",       False),
    }

    MFOptions = {
        "MFCalculateFrequency"  : ("Calculate frequency",       True),
        "MFPlotFreqInGraph"     : ("Add to frequency graph",    False),
        "MFPlotDeltasInGraph"   : ("Add to deltas graph",       False),
    }

    MBOptions = {
        "MBCalculateFrequency"  : ("Calculate frequency",       True),
        "MBPlotFreqInGraph"     : ("Add to frequency graph",    False),
        "MBPlotDutyCycleInGraph": ("Add to duty cycle graph",   False),
        "MBPlotDeltasInGraph"   : ("Add to deltas graph",       False),
    }

    @staticmethod
    def getGUIOptionsForMode(mode: str) -> list[dict[str,any]]:
        options = getattr(MIDDSChannelOptions, f"{mode}Options", None)
        if options is None:
            return []
        return [{"label": val[0], "value": key} for key, val in options.items()]

    @staticmethod
    def getDefaultChannelOptionsForMode(mode: str) -> dict[str,any]:
        options = getattr(MIDDSChannelOptions, f"{mode}Options", None)
        if options is None:
            return {}
        return {key: val[1] for key, val in options.items()}
