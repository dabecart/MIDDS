from MIDDSParser import MIDDSParser
from tqdm import tqdm

class MIDDSAnalyzer:
    def __init__(self, fileRoute: str):
        self.fileRoute = fileRoute
        self.parserMIDDS = MIDDSParser()

        try:
            file = open(self.fileRoute, "rb")
            fileContent = file.read()
        except:
            raise Exception("File does not exist")
        
        self.msgList = []
        for i in tqdm(range(len(fileContent))):
            decodedMsg = self.parserMIDDS.decodeMessage(fileContent[i:i+1])
            if decodedMsg is None: continue

            self.msgList.append(decodedMsg)

        print(f"Loaded {len(self.msgList)} messages.")