class DataMessage:
    def __init__(self, originID = "", nextID = "", destinationID = "", hopListLength = 0, hopList = []):
        self.type = "DATAMESSAGE"
        self.originID = originID
        self.nextID = nextID
        self.destinationID = destinationID
        self.hopListLength = hopListLength
        self.hopList = hopList

    
    def update(self, toSend):
        self.hopListLength += 1
        self.hopList.append(middleman)
        self.nextID = toSend

    def toString(self):
        returnString = "{} {} {} {} {} {}".format(self.type, self.originID, self.nextID, self.destinationID, self.hopListLength, self.hopList)
        return returnString

    # Will not send where message has been to avoid cycles
    def canSend(self, destination):
        if destination not in self.hopList:
            return True
        return False


#given the output on toString, produce a DataMessage object
def DataMessageFactory(datastr):
    data = datastr.split(" ", 5)
    length = int(data[4])
    realHopList = eval(data[5])
    # i = 0
    # print(data)
    # while i < length:
    #     if i == 0:
    #         realHopList.append(data[5][1])
    #     else:
    #         realHopList.append(data[5+i][0])
    #     i+=1

    return DataMessage(data[1], data[2], data[3], length, realHopList)
    
