#!/usr/bin/env python3

import sys  # For arg parsing
import socket  # For sockets
import select
import math
import DataMessage 
import Sensor 
import BaseStation



def where(sock, destinationID):
    msg = f"WHERE {destinationID}"
    sock.send(msg.encode())
    response = sock.recv(1024) #wait forever until server responds
    response = response.decode()
    response = response.split(" ")
    return (int(response[2]), int(response[3]))


#return true if l1 is completely contained within l2
def contained(l1, l2):
    for entry in l1:
        if entry not in l2:
            return False

    return True

#return a dictionary mapping of nodeId to the node's position (sensor or base)
def updateposition(sock, s_id, rng, x, y):

    ret = {}
    sendstr = "UPDATEPOSITION " + s_id + " " + str(rng) + " " + str(x) + " " + str(y)
    sock.sendall(sendstr.encode())
    recvstr = sock.recv(1024)
    recvstr = recvstr.decode()
    recvlst = recvstr.split()

    for i in range(2, len(recvlst), 3):
        ret[recvlst[i]] = ( int(recvlst[i+1]), int(recvlst[i+2]) )
    return ret


#Returns ID of closest sensor/base station
def findClosest(reach, dest, xDest, yDest):
    close = 99999.0
    ret = -1
    for i in reach.keys():
        dist = math.sqrt((xDest - reach[i][0])**2 + (yDest - reach[i][1])**2)
        if(dist < close):
            close = dist
            ret = i
            
    return ret

def run_client():
    if len(sys.argv) != 7:
        print("Invalid arguments: python3 hw4_client.py [control address] [control port] [SensorID] [SensorRange] [InitalXPosition] [InitialYPosition]")
        sys.exit(0)

    sensorid = sys.argv[3]
    sensorrange = int(sys.argv[4])
    xpos = int(sys.argv[5])
    ypos = int(sys.argv[6])

    connectsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connectsocket.connect((sys.argv[1], int(sys.argv[2])))

    reachable = updateposition(connectsocket, sensorid, sensorrange, xpos, ypos)
    read = [sys.stdin, connectsocket]
    write = []
    while(1):
        readable = select.select(read, [], [])[0]
        if sys.stdin in readable:
            line = sys.stdin.readline().strip()
            linelst = line.split(" ")
            if linelst[0] == "QUIT":
                connectsocket.close()
                sys.exit()
            if linelst[0] == "MOVE":
                xpos = int(linelst[1])
                ypos = int(linelst[2])
                reachable = updateposition(connectsocket, sensorid, sensorrange, xpos, ypos)
            if linelst[0] == "SENDDATA":
                reachable = updateposition(connectsocket, sensorid, sensorrange, xpos, ypos)
                xDest, yDest = where(connectsocket, linelst[1])
                hops = [sensorid]
                nextID = findClosest(reachable, sensorrange, xDest, yDest)
                msg = DataMessage.DataMessage(sensorid, nextID, linelst[1], 1, hops)
                if nextID == linelst[1]:
                    print("{}: Sent a new message directly to {}".format(sensorid, linelst[1]))
                else:
                    print("{}: Sent a new message bound for {}".format(sensorid, linelst[1]))
                connectsocket.sendall(msg.toString().encode())


            if linelst[0] == "WHERE":
                xDest, yDest = where(connectsocket, linelst[1])
                reachable[linelst[1]] = (xDest, yDest)
            


        #We received something from the Control Server
        if connectsocket in readable:
            msg = connectsocket.recv(1024).decode()
            splitmsg = msg.split(" ")
            
            if splitmsg[0] == "DATAMESSAGE":
                if splitmsg[3] == sensorid:
                    print("{}: Message from {} to {} successfully received".format(sensorid, splitmsg[1], splitmsg[3]))
                
                #this is when we receive a datamessage from the server,
                #So we are not the origin
                else:
                    Datamsg = DataMessageFactory(msg)
                    hopList = Datamsg.hopList
                    
                    #if all reachable sites are within the hopList of the message
                    if contained(reachable, hopList):
                        print("{}: Message from {} to {} could not be delivered.".format(sensorid, splitmsg[1], splitmsg[3]))

                    else:
                        reachable = updateposition(connectsocket, sensorid, sensorrange, xpos, ypos)
                        destX, destY = where(splitmsg[3])

                        #get the sensor to send a data message to the right base station / sensor
                        nextID = getNextStep(reachable, destX, destY)

                        print("{}: Message from {} to {} being forwarded through {}".format(sensorid, splitmsg[1], splitmsg[3], splitmsg[2]))
                        msg = f"DATAMESSAGE {splitmsg[1]} {splitmsg[2]} {splitmsg[3]} {splitmsg[4]} {splitmsg[5]} "
                        connectsocket.send(msg.encode())
                 

if __name__ == '__main__':
    run_client()