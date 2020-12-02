#!/usr/bin/env python3

import sys  # For arg parsing
import socket  # For sockets
import select

def updateposition(sock, s_id, rng, x, y):
    sendstr = "UPDATEPOSITION " + s_id + " " + rng + " " + x + " " + y
    sock.sendall(sendstr.encode('utf-8'))
    recvstr = sock.recv(1024)
    recvlst = recvstr.split()
    reach = []
    for i in range(2, len(recvlst), 3):
        reach.append((recvlst[i], recvlst[i+1], recvlst[i+2]))

    return reach

def findClosest(reach, dest):



def run_client():
    if len(sys.argv != 7):
        print("Invalid arguments: python3 hw4_client.py [control address] [control port] [SensorID] [SensorRange] [InitalXPosition] [InitialYPosition]")
        sys.exit(0)

    sensorid = sys.argv[3]
    sensorrange = sys.argv[4]
    xpos = sys.argv[5]
    ypos = sys.argv[6]

    connectsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connectsocket.connect((sys.argv[1], sys.argv[2]))

    reachable = updateposition(connectsocket, sensorid, sensorrange, xpos, ypos)
    read = [sys.stdin, connectsocket]
    write = []
    while(1):
        readable, writeable, exception = select.select(read, write, read)
        if sys.stdin in readable:
            line = sys.stdin.read()
            linelst = line.split()
            if linelst[0] == "QUIT":
                connectsocket.close()
                sys.exit()
            if linelst[0] == "MOVE":
                xpos = linelst[1]
                ypos = linelst[2]
                reachable = updateposition(connectsocket, sensorid, sensorrange, xpos, ypos)
            if linelst[0] == "SENDDATA":
                reachable = updateposition(connectsocket, sensorid, sensorrange, xpos, ypos)
                hops = [sensorid]
                msg = DataMessage(sensorid, linelst[1], linelst[1], 1, hops)

        if connectsocket in readable:

if __name__ == '__main__':
    run_client()
