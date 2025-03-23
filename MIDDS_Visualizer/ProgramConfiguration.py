import configparser
from MIDDSChannel import MIDDSChannel
import os.path

class ProgramConfiguration:
    CHANNEL_COUNT = 14

    def __init__(self, configRoute):
        self.config = configparser.ConfigParser()
        self.config.optionxform = str

        # Test if it's an absolute path.
        if os.path.isfile(configRoute):
            self.configPath = configRoute
            self.parseConfiguration_()
        else:
            # Test if it's a relative path.
            dirname = os.path.dirname(__file__)
            configWithRelativePath = os.path.join(dirname, configRoute)
            self.configPath = configWithRelativePath

            if os.path.isfile(configWithRelativePath):
                # If the config file exists, parse it.
                self.parseConfiguration_()
            else:
                # First run program settings
                self.config['ProgramConfig'] = {
                    'CHANNEL_COUNT' :   ProgramConfiguration.CHANNEL_COUNT,
                    'SERIAL_PORT'   :   "",
                }
                self.channels: list[MIDDSChannel] = [
                    MIDDSChannel(number=i) for i in range(ProgramConfiguration.CHANNEL_COUNT)]
        
    def parseConfiguration_(self):
        self.config.read(self.configPath)

        # Read the general Program configuration.
        if 'ProgramConfig' in self.config:
            programConfig = self.config['ProgramConfig']
            if 'CHANNEL_COUNT' not in programConfig:
                programConfig['CHANNEL_COUNT'] = str(ProgramConfiguration.CHANNEL_COUNT)
            if 'SERIAL_PORT' not in programConfig:
                programConfig['SERIAL_PORT'] = ""

        # Read the channels' configuration.
        self.channels: list[MIDDSChannel] = []
        for channelNumber in range(ProgramConfiguration.CHANNEL_COUNT):
            channelConfigSectionName = f"Channel{channelNumber:02d}"
            if channelConfigSectionName in self.config:
                self.channels.append(MIDDSChannel.cast_dict_to_dataclass(self.config[channelConfigSectionName]))
            else:
                self.channels.append(MIDDSChannel(number=channelNumber))

    def getChannel(self, channelNumber: int) -> MIDDSChannel|None:
        if(channelNumber >= len(self.channels)):
            return None
        return self.channels[channelNumber] 

    def saveConfig(self):
        for channel in self.channels:
            self.config[f"Channel{channel.number:02d}"] = channel.toDict()
        
        with open(self.configPath, 'w') as configfile:
            self.config.write(configfile)

    # Dictionary function calls.
    def __getitem__(self, key):
        return self.config[key]  # Get item like a dictionary

    def __setitem__(self, key, value):
        self.config[key] = value  # Set item like a dictionary

    def __delitem__(self, key):
        del self.config[key]  # Delete item like a dictionary

    def __contains__(self, key):
        return key in self.config  # Enable `in` keyword

    def __repr__(self):
        return repr(self.config)  # Pretty print