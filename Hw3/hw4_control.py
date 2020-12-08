#!/usr/bin/env python3

import sys  # For arg parsing
import socket  # For sockets
import select


#return the straight line distance between two points
def euclideanDistance(x1, y1, x2, y2):
    return (((x2 - x1)**2) + ((y2 - y1)**2))**.5


'''
Arguments:
curBase, the current Base object
destX, the x coordinate of the destination
destY, the y coordinate of the destination
allBases, a list of all Base objects recognized by the control server
hopList, a list of base station ID strings that have already received this message

returns None if there is no valid next Base Station to send to
Otherwise, return the next closest Base station object to send the message to.
'''
def getNextClosestBaseStation(curBase, destX, destY, allBases, hopList):
    sendableList = []
    if curBase.baseID == "CONTROL":
        sendableList = allBases
    #get all Base stations that are in range, and have not already received this message.
    else:
        sendableList = [x for x in allBases if x.baseID not in hopList and x.baseID in curBase.listOfLinks]
    if len(sendableList) == 0:
        return None

    minimumDist = euclideanDistance(sendableList[0].xPos, sendableList[0].yPos, destX, destY)
    closestBaseID = sendableList[0].baseID

    for base in sendableList:
        dist = euclideanDistance(base.xPos, base.yPos, destX, destY) < minimumDist:
        if dist < minimumDist:
            minimumDist = dist
            closestBase = base

    return closestBase

def getNextClosestSensor(curBase, destX, destY, allSensors, hopList):
    sendableList = [x for x in allSensors if x.id not in hopList and euclideanDistance(x.xPos, x.yPos, curBase.xPos, curBase.yPos) <= x.range]
    if len(sendableList) == 0:
        return None

    minimumDist = euclideanDistance(sendableList[0].xPos, sendableList[0].yPos, destX, destY)
    closestSensor = sendableList[0]

    for sensor in sendableList:
        dist = euclideanDistance(sensor.xPos, sensor.yPos, destX, destY) < minimumDist:
        if dist < minimumDist:
            minimumDist = dist
            closestSensor = base

    return closestSensor

# The base station file contains the following information in each row (ending in a newline character \n), with each field separated by a space:
# [BaseID] [XPos] [YPos] [NumLinks] [ListOfLinks] 
    
#return a list of Base objects, constructed by the input file    
def loadBases(filename):
    f = open(filename, 'r')
    lines = f.readlines()
    f.close()
    
    allBases = []
    for line in lines:
        data = line.split(" ")
        listOfLinks = data[4:]
        allBases.append(BaseStation(data[0], int(data[1]), int(data[2]), int(data[3]), listOfLinks))

    return allBases
        


# For WHERE command
def locateSensor(id, baseList):
    for base in baseList:
        if id == base.baseID:
            return base
    for sensor in sensorList:
        if id == sensor.id:
            return sensor

# For REACHABLE, create list of reachable sensor and base ids [ID] [XPosition] [YPosition]
def reachableList(sensor):
    count = 0
    inrange = ""
    for base in baseList:
        dist = euclideanDistance(sensor.xPos, sensor.yPos, base.xPos, base.yPos)
        if dist <= sensor.range:
            inrange += base.baseID + " " + base.xPos + " " + base.yPos + " "
            count += 1
    for s in sensorList:
        dist = euclideanDistance(sensor.xPos, sensor.yPos, s.xPos, s.yPos)
        if dist <= sensor.range:
            inrange += s.id + " " + s.xPos + " " + s.yPos + " "
            count += 1
    return (count, reachableList)

#recursively handle intermediate base station communications
#basecases: delivering to a base destination, sending a message to a sensor, or being unable to send a message
def recursiveSend(start, end, hopList, baseList, sensorList):
    if start == "CONTROL":
        #nextID will always be a base station if origin is control
        control = Base("CONTROL", 0, 0, 0, [])

        if end in [x.baseID for x in baseList]:
            destinationBase = [base for base in baseList if base.baseID == end]
            toSendBase = getNextClosestBaseStation(control, destinationBase.xPos, destinationBase.yPos, baseList, hopList)
            
            #possible that no base stations exist?
            if toSendBase == None:
                print("Unable to deliver message")
                return

            toSendBaseID = toSendBase.baseID

            
            if toSendBaseID == end:
                print("[{}]: Sent a new message directly to [{}]".format("CONTROL", end))
                return
                
            else:
                print("[{}]: Sent a new message bound for [{}]".format("CONTROL", end))
                recursiveSend(toSendBaseID, end, ["CONTROL"])

        elif end in [x.id for x in sensorList]:
            sensor = [s for s in sensorList if s.id == end]
            toSendBase = getNextClosestBaseStation(control, sensor.xPos, sensor.yPos, baseList, hopList)
            if toSendBase == None:
                print("Unable to deliver message")
                return

            toSendBaseID = toSendBase.baseID
            
            # #Shouldnt be possible, nextID will never be a sensor
            # if toSendBaseID == end: #end is a sensor id
            #     print("[{}]: Sent a new message directly to [{}]".format("CONTROL", end))
            #     return
            
            # else:
            print("[{}]: Sent a new message bound for [{}]".format("CONTROL", end))
            recursiveSend(toSendBaseID, end, ["CONTROL"])
        
    else:
        #remember, start may not ever be a sensor
        originBase = x for x in baseList if x.baseID == start
        destination = ""
        if end in [x.baseID for x in baseList]:
            destination = [base for base in baseList if base.baseID == end]
        else:
            destination = [sensor for sensor in sensorList if sensor.id == end]

        toSendBase = getNextClosestBaseStation(control, destination.xPos, destination.yPos, baseList, hopList)

        toSendSensor = getNextClosestSensor(originBase, destination.xPos, destination.yPos, sensorList, hopList)

        if toSendBase == None and toSendSensor == None:
            print("Message couldn't be delivered")
            return
        
        #if exactly one is non-None
        if (toSendBase != None or toSendSensor != None) and (not (toSendBase != None and toSendSensor != None)):
            if toSendBase != None:
                if toSendBase.baseID == end:
                    print("[{}]: Sent a new message directly to [{}]".format(originBase.baseID, end))
                    return
                
                else:
                    print("[{}]: Sent a new message bound for [{}]".format(start, end))
                    recursiveSend(toSendBaseID, end, hoplist.append(start))
            else:
                if toSendSensor.id == end:
                    print("[{}]: Sent a new message directly to [{}]".format(originBase.baseID, end))
                    SEND TO SENSOR
                    return
                else:
                    print("[{}]: Sent a new message bound for [{}]".format(start, end))
                    SEND TO SENSOR
                    return
        
        else:
            #the current base may either send to another base or another sensor

            isSensor = False
            #find whats closer to the destination: sensor or base?
            if euclideanDistance(toSendBase.xPos, toSendBase.yPos, destinationBase.xPos, destinationBase.yPos) > \
                euclideanDistance(toSendSensor.xPos, toSendSensor.yPos, destinationBase.xPos, destinationBase.yPos):
                #base is further away than sensor, so send to sensor
                isSensor = True

            if isSensor: #if we should send to a sensor (because its closer to destination)
                if toSendSensor.id == end:
                    print("[{}]: Sent a new message directly to [{}]".format(originBase.baseID, end))
                    SEND TO SENSOR
                    return
                else:
                    print("[{}]: Sent a new message bound for [{}]".format(start, end))
                    SEND TO SENSOR
                    return

            else:
                if toSendBase.baseID == end:
                    print("[{}]: Sent a new message directly to [{}]".format(originBase.baseID, end))
                    return
                else:
                    print("[{}]: Sent a new message bound for [{}]".format(start, end))
                    recursiveSend(toSendBase.baseID, end, hopList.append(toSendBase.baseID), baseList, sensorList )
                    return
        

def run_server():
    if len(sys.argv) != 3:
        print("Invalid arguments: python3 hw4_control.py [control port] [base station file]")
        sys.exit(0)

    baseList = loadBases(sys.argv[2])
    sensorList = []

    readlist = [sys.stdin]
    listeningsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listeningsocket.bind(('', int(sys.arg[1])))
    listeningsocket.listen(5)
    
    readlist.append(listeningsocket)
    maxFd = len(readlist) + 1

    while True:
        
        fd = select.select(readlist, 0, 0)

        
        if sys.stdin in fd:
            # Get user input
            command = sys.stdin.readline()
            command = command.split(" ")

            if command[0] == "SENDDATA":
                originID = command[1]
                destinationID = command[2]

                recursiveSend(originID, destinationID, [originID], baseList, sensorList)
            
            if command[0] == "QUIT":
                listeningsocket.close()
                break

            

        else:

            # Reading data
            while True:
                (client_socket, address) = listeningsocket.accept()
                message = client_socket.recv(1024)
                if message:
                    splitmessage = message.split(" ")

                    if splitmessage[0] == "WHERE":
                        foundbase = locatesensor(message[1], baseList)
                        client_socket.send(f"THERE {foundbase.baseID} {foundbase.xPos} {foundbase.yPos}")
                        # send back THERE
                        
                    if splitmessage[0] == "UPDATEPOSITION":
                        # update position if already in list
                        found = False
                        for sensor in sensorList:
                            if sensor.id == splitmessage[1]:
                                found = True
                                sensor.xPos = splitmessage[3]
                                sensor.yPos = splitmessage[4]
                        if not found:
                            # new sensor
                            sensorList.append(Sensor(splitmessage[1], splitmessage[2], splitmessage[3], splitmessage[4]))
                        (count, reachableList) = reachableList(splitmessage[1])
                        client_socket.send(f"REACHABLE {count} {reachableList}")
                    
                    # if receive data message with [NextID], relay
                    if splitmessage[0] == "DATAMESSAGE":
                        message = DataMessageFactory(message)                  
                
                else:
                    print("Client disconnected")
                    break

        client_socket.close()

if __name__ == '__main__':
    run_server()