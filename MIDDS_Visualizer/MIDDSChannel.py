import dataclasses
from datetime import datetime
from collections import deque
from typing import Final, get_origin
from MIDDSParser import MIDDSParser
import ast

@dataclasses.dataclass
class MIDDSChannel:
    MAX_POINTS: Final[int]                      = 200
    TIMEOUT_UNTIL_UNKNOWN_LEVEL_s: Final[float] = 5

    name:           str                 = ""
    number:         int                 = 0
    mode:           str                 = "DS"
    signal:         str                 = "T" # T (TTL) or L (LVDS)
    modeSettings:   dict[str, any]      = dataclasses.field(default_factory = dict)
    toRecord:       bool                = False
    
    _channelLevel:          str         = "?"
    lastLevelUpdate:        datetime    = datetime.now()
    _frequency:             float       = 0.0
    lastFrequencyUpdate:    datetime    = datetime.now()

    levels:         deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS))
    freqs:          deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS))
    freqsUpdates:   deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS))

    # During configuration, if MIDDS returns an error, the channel will be set as badly configured.
    wellConfigured: bool                = True

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
        dt = datetime.now() - self.lastLevelUpdate
        if dt.total_seconds() > MIDDSChannel.TIMEOUT_UNTIL_UNKNOWN_LEVEL_s:
            return f"{self._frequency:.4g}"
        else:
            return self._frequency
    
    @frequency.setter
    def frequency(self, val: float):
        self.lastFrequencyUpdate = datetime.now()
        self._frequency = val
        
        self.freqs.append(self._frequency)
        self.freqsUpdates.append(self.lastFrequencyUpdate)

    @property
    def signalType(self) -> str:
        if self.signal == "T":
            return "TTL"
        elif self.signal == "L":
            return "LVDS"
        else:
            return "Invalid signal"

    def generateRecurringMessages(self) -> bytes|None:
        if self.mode == "IN":
            return b''.join([
                MIDDSParser.encodeInput(self.number, 0, 0),
                MIDDSParser.encodeFrequency(self.number, 0)
            ])
        
        return None

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
                self.frequency = values.get("frequency", -1.0)

    def filterModeSettings(self):
        # The channel options always start with the channel mode code ("IN", "OU"...).
        toRemoveKeys: list[str] = []
        for key in self.modeSettings.keys():
            if not key.startswith(self.mode):
                toRemoveKeys.append(key)
        
        for key in toRemoveKeys:
            del self.modeSettings[key]

    def toDict(self) -> dict[str,any]:
        self.filterModeSettings()
        toSaveDict = dataclasses.asdict(self)

        # Remove the fields that must not be saved on the configuration file.
        del toSaveDict['MAX_POINTS']
        del toSaveDict['TIMEOUT_UNTIL_UNKNOWN_LEVEL_s']
        del toSaveDict['_channelLevel']
        del toSaveDict['_frequency']
        del toSaveDict['lastLevelUpdate']
        del toSaveDict['lastFrequencyUpdate']
        del toSaveDict['levels']
        del toSaveDict['freqs']
        del toSaveDict['freqsUpdates']
        del toSaveDict['wellConfigured']
        return toSaveDict
    
    def getChannelOptionsForChecklist(self) -> list[str]:
        return [keyName for keyName, value in self.modeSettings.items() if value]

    @classmethod
    def cast_dict_to_dataclass(cls, data):
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
    #   Name of option         Description                  Default value
    INOptions = {
        "INPlotFreqInGraph" : ("Add to frequency graph",    False),
    }

    @staticmethod
    def getGUIOptionsForMode(mode: str) -> list[dict[str,any]]:
        if mode == "IN":
            return [{"label": val[0], "value" : key} for key, val in MIDDSChannelOptions.INOptions.items()]
        
        return []
    
    @staticmethod
    def getDefaultChannelOptionsForMode(mode: str) -> dict[str,any]:
        if mode == "IN":
            return {key: val[1] for key, val in MIDDSChannelOptions.INOptions.items()}
    
        return {}