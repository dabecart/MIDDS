import threading
from ProgramConfiguration import ProgramConfiguration
from GUI2BackendEvents import GUI2BackendEvents
from GUI import GUI
from MIDDSMaster import MIDDSMaster

CONFIG_ROUTE = "config.ini"

if __name__ == '__main__':
    config = ProgramConfiguration(CONFIG_ROUTE)
    channelsLock = threading.Lock()
    events = GUI2BackendEvents()
    midds = MIDDSMaster(events=events, lock=channelsLock, config=config)

    decoderProcess = threading.Thread(target=midds.backendProcess,
                                      daemon=True)
    decoderProcess.start()

    try:
        gui = GUI(channelsLock, events, config)
        gui.run()
    except KeyboardInterrupt:
        pass

    events.stopEvent.set()
    decoderProcess.join(10)

    config.saveConfig()
    
