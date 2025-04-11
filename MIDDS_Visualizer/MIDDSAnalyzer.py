from MIDDSParser import MIDDSParser
from tqdm import tqdm
import pickle
import os
from collections import defaultdict
import numpy as np
from numba import njit

class MIDDSAnalyzer:
    def __init__(self, fileRoute: str):
        self.fileRoute = fileRoute
        self.parserMIDDS = MIDDSParser()
        self.msgList = []

        # Data stored per channel.
        self.data = defaultdict(dict)
        # TODO: freqData, inputData...

        self.loadMessagesFromFile_()
        self.processMonitorMessages()

    def processMonitorMessages(self):
        monitorPerChannel = defaultdict(list)
        for msg in self.msgList:
            if msg.get("command") == MIDDSParser.COMMS_MSG_MONITOR_HEAD:
                monitorPerChannel[msg["channel"]].extend(msg["samples"])

        for ch in monitorPerChannel:
            print(f"Channel {ch}: size {len(monitorPerChannel[ch])}")
            monitorData = np.array(monitorPerChannel[ch], dtype=np.uint64)
            edgeTimestamps_ns, edges, periods, periodsTime, highDeltas, highDeltasTime, lowDeltas, lowDeltasTime = self.calculateChannelPeriods_(monitorData)
            frequencies = self.calculateFrequencies_(periods)
            self.data[ch] = {
                "timestamp":        edgeTimestamps_ns,
                "edge":             edges,
                "period":           periods,
                "periodTime":       periodsTime,
                "highDelta":        highDeltas,
                "highDeltaTime":    highDeltasTime,
                "lowDelta":         lowDeltas,
                "lowDeltaTime":     lowDeltasTime,
                "frequency":        frequencies,
                "frequencyTime":    periodsTime
            }

    @staticmethod
    @njit
    def calculateFrequencies_(periods: np.ndarray):
        return np.float64(1.0) / periods.astype(np.float64)

    @staticmethod
    @njit
    def calculateChannelPeriods_(monitorEdges: np.ndarray):
        # Initial setup of arrays. They will be latter trimmed.
        edgeTimestamps_ns   = np.zeros(len(monitorEdges), dtype=np.uint64)
        # True if rising edge.
        edges               = np.zeros(len(monitorEdges), dtype=np.bool)

        # Periods (Y axis) and periodsTime (X axis).
        periods         = np.zeros(len(monitorEdges), dtype=np.uint64)
        periodsTime     = np.zeros(len(monitorEdges), dtype=np.uint64)

        highDeltas      = np.zeros(len(monitorEdges), dtype=np.uint64)
        highDeltasTime  = np.zeros(len(monitorEdges), dtype=np.uint64)

        lowDeltas       = np.zeros(len(monitorEdges), dtype=np.uint64)
        lowDeltasTime   = np.zeros(len(monitorEdges), dtype=np.uint64)

        periodsIndex    = 0
        highDeltasIndex = 0
        lowDeltasIndex  = 0        

        for i, sample in enumerate(monitorEdges[1:]):
            edgeTimestamps_ns[i] = sample >> 1
            edges[i] = (sample & 0x01) == 1

            if i < 1: continue

            delta = edgeTimestamps_ns[i] - edgeTimestamps_ns[i-1]
            if edges[i] == edges[i-1]:
                # This is the period.
                periods[periodsIndex] = delta
                periodsTime[periodsIndex] = edges[i]
                periodsIndex += 1
            else:
                # Edge variation. Deltas can be calculated...
                if edges[i]:
                    # Last edge was rising, therefore, low delta.
                    lowDeltas[lowDeltasIndex] = delta
                    lowDeltasTime[lowDeltasIndex] = edgeTimestamps_ns[i]
                    lowDeltasIndex += 1
                else:
                    highDeltas[highDeltasIndex] = delta
                    highDeltasTime[highDeltasIndex] = edgeTimestamps_ns[i]
                    highDeltasIndex += 1
                
                # Periods get calculated by the sum of the two previous edges.
                periods[periodsIndex] = highDeltas[highDeltasIndex-1] + lowDeltas[lowDeltasIndex-1]
                periodsTime[periodsIndex] = edgeTimestamps_ns[i]
                periodsIndex += 1

        # Trim the arrays.
        periods         = periods[:periodsIndex]
        periodsTime     = periodsTime[:periodsIndex]

        highDeltas      = highDeltas[:highDeltasIndex]
        highDeltasTime  = highDeltasTime[:highDeltasIndex]

        lowDeltas       = lowDeltas[:lowDeltasIndex]
        lowDeltasTime   = lowDeltasTime[:lowDeltasIndex]

        return edgeTimestamps_ns, edges, periods, periodsTime, highDeltas, highDeltasTime, lowDeltas, lowDeltasTime

    def getFileName(self):
        return os.path.basename(self.fileRoute)

    def dumpMsgListToFile(self, fileRoute: str):
        with open(fileRoute, 'wb') as f:
            pickle.dump(self.msgList, f)
        print(f"Saved processed file to {fileRoute}.")

    def loadMessagesFromFile_(self):
        # Get extension of the file.
        _, extension = os.path.splitext(self.fileRoute)
        if extension == ".pmf":
            # If the file is an already processed file...
            with open(self.fileRoute, 'rb') as f:
                self.msgList = pickle.load(f)
        else:
            try:
                file = open(self.fileRoute, "rb")
                fileContent = file.read()
            except:
                raise Exception("File does not exist")
            
            for i in tqdm(range(len(fileContent))):
                decodedMsg = self.parserMIDDS.decodeMessage(fileContent[i:i+1])
                if decodedMsg is None: continue

                self.msgList.append(decodedMsg)

        print(f"Loaded {len(self.msgList)} messages.")