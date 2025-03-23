import dataclasses
from datetime import datetime
from collections import deque
from typing import Final
from MIDDSParser import MIDDSParser

@dataclasses.dataclass
class MIDDSChannel:
    MAX_POINTS: Final[int] = 50
    TIMEOUT_UNTIL_UNKNOWN_LEVEL_s: Final[float] = 5

    name:           str                 = ""
    number:         int                 = 0
    mode:           str                 = "DS"
    signal:         str                 = "T" # T (TTL) or L (LVDS)
    modeSettings:   dict[str, any]      = dataclasses.field(default_factory = dict[str,any])
    toRecord:       bool                = False
    
    _channelLevel:  str                = "?"
    frequency:      float               = 0.0
    lastUpdate:     datetime            = datetime.now()

    levels:         deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS))
    freqs:          deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS))
    freqsUpdates:   deque               = dataclasses.field(default_factory = lambda: deque(maxlen = MIDDSChannel.MAX_POINTS))

    # During configuration, if MIDDS returns an error, the channel will be set as badly configured.
    wellConfigured: bool                = True

    @property
    def channelLevel(self) -> str:
        dt = datetime.now() - self.lastUpdate
        if dt.total_seconds() > MIDDSChannel.TIMEOUT_UNTIL_UNKNOWN_LEVEL_s:
            return "?"
        else:
            return self._channelLevel
    
    @channelLevel.setter
    def channelLevel(self, val: str):
        self.lastUpdate = datetime.now()
        self._channelLevel = val

    def generateRecurringMessages(self) -> bytes|None:
        if self.mode == "IN":
            return MIDDSParser.encodeInput(self.number, 0, 0)
        
        return None

    def updateValues(self, values: dict[str,any]):
        if self.mode == "IN":
            read = values.get("readValue", "?")
            if read == 0:
                self.channelLevel = "LOW"
            elif read == 1:
                self.channelLevel = "HIGH"
            else:
                self.channelLevel = "?"

    def toDict(self) -> dict[str,any]:
        toSaveDict = dataclasses.asdict(self)

        # Remove the fields that must not be saved on the configuration file.
        del toSaveDict['_channelLevel']
        del toSaveDict['frequency']
        del toSaveDict['lastUpdate']
        del toSaveDict['levels']
        del toSaveDict['freqs']
        del toSaveDict['freqsUpdates']
        del toSaveDict['wellConfigured']
        return toSaveDict

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
                else:
                    casted_data[field.name] = data[field.name]
        return cls(**casted_data)