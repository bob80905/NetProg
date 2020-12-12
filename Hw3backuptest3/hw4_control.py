#!/usr/bin/env python3

import sys  # For arg parsing
import socket  # For sockets
import select
import BaseStation
import DataMessage
import Sensor


def send(t, msg):
    t[0].sendall(msg.encode('utf-8'))

#return the straight line distance between two points
def euclideanDistance(x1, y1, x2, y2):
    return (((x2 - x1)**2) + ((y2 - y1)**2))**.5


'''
Arguments:
startSite, the current site, base or sensor
destX, the x coordinate of the destination
destY, the y coordinate of the destination
allBases, a list of all Base objects recognized by the control server
hopList, a list of base station ID strings that have already received this message

returns None if there is no valid next Base Station to send to
Otherwise, return the next closest Base station object to send the message to.
'''
def getNextClosestBaseStation(startSite, destX, destY, allBases, hopList):
    sendableList = []
    if startSite.type == "BASE":
        if startSite.baseID == "CONTROL":
            sendableList = allBases
        #get all Base stations that are in range, and have not already received this message.
        else:
            sendableList = [x for x in allBases if x.baseID not in hopList and x.baseID in startSite.listOfLinks]
    else:
        for base in allBases:
            if euclideanDistance(startSite.xPos, startSite.yPos, base.xPos, base.yPos) <= startSite.range and base.baseID not in hopList:
                sendableList.append(base)

    if len(sendableList) == 0:
        return None

    minimumDist = euclideanDistance(sendableList[0].xPos, sendableList[0].yPos, destX, destY)
    closestBase = sendableList[0]

    for base in sendableList:
        dist = euclideanDistance(base.xPos, base.yPos, destX, destY)
        if dist < minimumDist:
            minimumDist = dist
            closestBase = base

    return closestBase

def getNextClosestSensor(startSite, destX, destY, allSensors, hopList):
    sendableList = []
    if startSite.type == "SENSOR":
        for sensor in allSensors:
            if sensor.baseID not in hopList:
                if euclideanDistance(sensor.xPos, sensor.yPos, startSite.xPos, startSite.yPos) <= startSite.range:
                    if euclideanDistance(sensor.xPos, sensor.yPos, startSite.xPos, startSite.yPos) <= sensor.range:
                        sendableList.append(sensor)

    else:
        for sensor in allSensors:
            if sensor.baseID not in hopList:
                if euclideanDistance(sensor.xPos, sensor.yPos, startSite.xPos, startSite.yPos) <= sensor.range:
                    sendableList.append(sensor)

    if len(sendableList) == 0:
        return None

    minimumDist = euclideanDistance(sendableList[0].xPos, sendableList[0].yPos, destX, destY)
    closestSensor = sendableList[0]

    for sensor in sendableList:
        dist = euclideanDistance(sensor.xPos, sensor.yPos, destX, destY)
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
        data = line.strip("\n").split(" ")
        listOfLinks = data[4:]
        allBases.append(BaseStation.BaseStation(data[0], int(data[1]), int(data[2]), int(data[3]), listOfLinks))

    return allBases
        


# For WHERE command
def locateSensor(idname, baseList, sensorList):
    for base in baseList:
        if idname == base.baseID:
            return base
    for sensor in sensorList:
        if idname == sensor.baseID:
            return sensor

# For REACHABLE, create list of reachable sensor and base ids [ID] [XPosition] [YPosition]
def reachableList(sensor, baseList , sensorList):
    count = 0
    inrange = ""
    for base in baseList:
        dist = euclideanDistance(sensor.xPos, sensor.yPos, base.xPos, base.yPos)
        if dist <= sensor.range:
            inrange += str(base.baseID) + " " + str(base.xPos) + " " + str(base.yPos) + " "
            count += 1
    for s in sensorList:
        dist = euclideanDistance(sensor.xPos, sensor.yPos, s.xPos, s.yPos)
        if dist <= sensor.range:
            inrange += str(s.baseID) + " " + str(s.xPos) + " " + str(s.yPos) + " "
            count += 1
    return (count, inrange)

#recursively handle intermediate base station communications
#basecases: delivering to a base destination, sending a message to a sensor, or being unable to send a message
def recursiveSend(origin, start, end, hopList, baseList, sensorList, connections):
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
                print("{}: Sent a new message directly to {}.".format("CONTROL", end))
                return
                
            else:
                print("{}: Sent a new message bound for {}.".format("CONTROL", end))
                recursiveSend(origin, toSendBaseID, end, ["CONTROL"], baseList, sensorList, connections)
                

        elif end in [x.baseID for x in sensorList]:
            sensor = [s for s in sensorList if s.baseID == end]
            toSendBase = getNextClosestBaseStation(control, sensor.xPos, sensor.yPos, baseList, hopList)
            if toSendBase == None:
                print("Unable to deliver message")
                return

            toSendBaseID = toSendBase.baseID
            
            # #Shouldnt be possible, nextID will never be a sensor
            # if toSendBaseID == end: #end is a sensor id
            #     print("{}: Sent a new message directly to {}".format("CONTROL", end))
            #     return
            
            # else:
            print("{}: Sent a new message bound for {}.".format("CONTROL", end))
            recursiveSend(origin, toSendBaseID, end, ["CONTROL"], baseList, sensorList, connections)
        
    else:
        startSite = None
        for base in baseList:
            if base.baseID == start:
                startSite = base
        
        if startSite == None:
            for sensor in sensorList:
                if sensor.baseID == start:
                    startSite = sensor
            
        destination = ""
        if end in [x.baseID for x in baseList]:
            destination = [base for base in baseList if base.baseID == end][0]
        else:
            destination = [sensor for sensor in sensorList if sensor.baseID == end][0]


        toSendBase = getNextClosestBaseStation(startSite, destination.xPos, destination.yPos, baseList, hopList)
        toSendSensor = getNextClosestSensor(startSite, destination.xPos, destination.yPos, sensorList, hopList)
        
        if toSendBase == None and toSendSensor == None:
            print("{}: Message from {} to {} could not be delivered.".format(start, origin, end))
            return
        
        #if exactly one (Base or Sensor) is non-None
        if (toSendBase != None or toSendSensor != None) and (not (toSendBase != None and toSendSensor != None)):
            if toSendBase != None:
                if start == end:
                    print("{}: Message from {} to {} successfully received.".format(start, origin, end))
                    
                    return
                
                else:
                    print("{}: Message from {} to {} being forwarded through {}".format(start, origin, end, start))
                    hopList.append(start)
                    recursiveSend(origin, toSendBase.baseID, end, hopList, baseList, sensorList, connections)
            else:
                if toSendSensor.baseID == end:
                    if origin == start:
                        print("{}: Sent a new message directly to {}.".format(start, end))
                    else:
                        print("{}: Message from {} to {} being forwarded through {}".format(start, origin, end, start))
                    s = connections[toSendSensor.baseID]
                    msg = f'{origin} {toSendSensor.baseID} {end} {len(hopList)} {hopList}'
                    send(msg, s)
                    
                    return
                else:
                    if origin == start:
                        print("{}: Sent a new message bound for {}.".format(start, end))
                    else:
                        print("{}: Message from {} to {} being forwarded through {}".format(start, origin, end, start))
                    
                    s = connections[toSendSensor.baseID]
                    msg = f'{origin} {toSendSensor.baseID} {end} {len(hopList)} {hopList}'
                    send(msg, s)
                    return
        
        else:
            #the current base may either send to another base or another sensor

            isSensor = False
            #find whats closer to the destination: sensor or base?
            if euclideanDistance(toSendBase.xPos, toSendBase.yPos, destination.xPos, destination.yPos) > \
                euclideanDistance(toSendSensor.xPos, toSendSensor.yPos, destination.xPos, destination.yPos):
                #base is further away than sensor, so send to sensor
                isSensor = True

            if isSensor: #if we should send to a sensor (because its closer to destination)
                if toSendSensor.baseID == end:
                    if origin == start:
                        print("{}: Sent a new message directly to {}.".format(start, end))
                    else:
                        print("{}: Message from {} to {} being forwarded through {}".format(start, origin, end, start))
                   
                    s = connections[toSendSensor.baseID]
                    msg = f'{origin} {toSendSensor.baseID} {end} {len(hopList)} {hopList}'
                    send(s, msg)
                    return
                else:

                    if origin == start:
                        print("{}: Sent a new message bound for {}.".format(start, end))
                    else:
                        print("{}: Message from {} to {} being forwarded through {}".format(start, origin, end, start))
                   
                    s = connections[toSendSensor.baseID]
                    msg = f'DATAMESSAGE {origin} {toSendSensor.baseID} {end} {len(hopList)} {hopList}'
                    send(s, msg)
                    return

            else:
                
                if start == end:
                    print("{}: Message from {} to {} successfully received.".format(start, origin, end))
                    # if origin == start:
                    #     print("{}: Sent a new message directly to {}.".format(start, end))
                    # else:
                    #     print("{}: Message from {} to {} being forwarded through {}".format(start, origin, end, start))
                   
                    
                    return
                else:

                    if origin == start:
                        print("{}: Sent a new message bound for {}.".format(start, end))
                    else:
                        print("{}: Message from {} to {} being forwarded through {}".format(start, origin, end, start))
                   
                    hopList.append(start)
                    recursiveSend(origin, toSendBase.baseID, end, hopList, baseList, sensorList, connections)
                            

def run_server():
    if len(sys.argv) != 3:
        print("Invalid arguments: python3 hw4_control.py [control port] [base station file]")
        sys.exit(0)

    baseList = loadBases(sys.argv[2])
    sensorList = []

    readlist = [sys.stdin]
    listeningsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listeningsocket.bind(('', int(sys.argv[1])))
    listeningsocket.listen(5)
    
    readlist.append(listeningsocket)
    maxFd = len(readlist) + 1

    connections = {}

    while True:
        
        fd = select.select(readlist, [], [])[0]

        
        if sys.stdin in fd:
            # Get user input
            command = sys.stdin.readline()
            command = command.strip("\n").split(" ")

            if command[0] == "SENDDATA":
                originID = command[1]
                destinationID = command[2]
                recursiveSend(originID, "CONTROL", destinationID, ["CONTROL"], baseList, sensorList, connections)
            
            if command[0] == "QUIT":
                listeningsocket.close()
                break

        # New incoming connection
        elif listeningsocket in fd:

            (client_socket, address) = listeningsocket.accept()
            readlist.append(client_socket)

        # Msg from an old connection
        else:
            clientsock = fd[0]
            message = clientsock.recv(1024)
            message = message.decode()
            if message:
                splitmessage = message.split(" ")

                if splitmessage[0] == "WHERE":
                    foundbase = locateSensor(splitmessage[1], baseList, sensorList)
                    msg = f"THERE {foundbase.baseID} {foundbase.xPos} {foundbase.yPos}"
                    clientsock.sendall(msg.encode())
                    # send back THERE
                    
                if splitmessage[0] == "UPDATEPOSITION":
                    connections[splitmessage[1]] = (client_socket, address)
                    # update position if already in list
                    found = False
                    for sensor in sensorList:
                        if sensor.baseID == splitmessage[1]:
                            found = True
                            sensor.xPos = int(splitmessage[3])
                            sensor.yPos = int(splitmessage[4])
                    if not found:
                        # new sensor
                        sensorList.append(Sensor.Sensor(splitmessage[1], int(splitmessage[2]), int(splitmessage[3]), int(splitmessage[4]) ) )
                    # UPDATEPOSITION [SensorID] [SensorRange] [CurrentXPosition] [CurrentYPosition]
                    thisSensor = Sensor.Sensor(splitmessage[1], int(splitmessage[2]), int(splitmessage[3]), int(splitmessage[4]) )
                    (count, reachList) = reachableList(thisSensor, baseList, sensorList)
                    clientsock.sendall(f"REACHABLE {count} {reachList}".encode())
                
                # if receive data message with [NextID], relay
                if splitmessage[0] == "DATAMESSAGE":
                    message = DataMessage.DataMessageFactory(message)
                    
                    
                    recursiveSend(message.originID, message.nextID, message.destinationID, message.hopList, baseList, sensorList, connections)                  
            
            else:
                print("Client disconnected")
                break

    for conn in connections.keys():
        connections[conn][0].close()

if __name__ == '__main__':
    run_server()