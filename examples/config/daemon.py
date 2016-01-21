#! /usr/bin/python

import os, sys

#print "daemon.py start"

if not os.environ.has_key('POWERRT_MACHINE'):
    print 'POWERRT_MACHINE is not set'
    sys.exit()

machineName = os.environ['POWERRT_MACHINE'];

machine = __import__(machineName,fromlist=[''])

def calcTree( myRank, nidMap, nodesPerBoard ):

	routerID = myRank/nodesPerBoard
	#print "calcTree( {0} {1} {2})".format( myRank, len(nidMap), routerID )

	ret = '' 

	if len(nidMap) <= nodesPerBoard:
		return ""

	if 0 == routerID:
		otherHostPort = 16000
		#print 'len nidMap {0} nodesPerBoard {1}'.format( len(nidMap), nodesPerBoard) 
		numLinks = len(nidMap) / nodesPerBoard 
		if len(nidMap) % nodesPerBoard: 
			numLinks += 1

		for i in xrange( 1, numLinks ):
			myListenPort = 16000 + i
			other = i * nodesPerBoard
			otherHost = nidMap[other]
			tmp = ' --rtr.routerInfo={0}:{1}:{2}:{3}:{4}:{5}'.\
					format( i, myListenPort, otherHost, otherHostPort, i, i)
			ret += tmp
	else:
		
		myListenPort = 16000
		otherHost = nidMap[0]
		otherHostPort = 16000 + routerID 
	
		ret += ' --rtr.routerInfo={0}:{1}:{2}:{3}'.\
					format( 0, myListenPort, otherHost, otherHostPort )

	#print 'calcTree(',myRank,')', ret

	return ret

def calcBoardRoot( myRank, numRanks, nodesPerBoard, boardsPerCab ):

	cab = (myRank / (nodesPerBoard * boardsPerCab ) )

	board = (myRank / nodesPerBoard ) % boardsPerCab 

	#print "{0} cab={1} board={2}".format(myRank,cab,board)

	tmp = 'plat.cab{0}.board{1}'.format(cab,board)
	#print 'calcBoardRoot rank={0} {1}'.format( myRank, tmp)
	return tmp 

def calcNodeRoot( myRank, numRanks, nodesPerBoard, boardsPerCab ):
	ret = calcBoardRoot( myRank, numRanks, nodesPerBoard, boardsPerCab ) 

	node = myRank % nodesPerBoard 

	tmp = ret + '.node{0}'.format(node)

	#print 'calcNodedRoot rank={0} {1}'.format(myRank, tmp)
	return tmp 


def configRtr( myRank, nidMap, config, routeFile, nodesPerBoard ):
	tmp = ""

	if not myRank % nodesPerBoard:
		if 0 == myRank % nodesPerBoard:
			tmp += ' --rtr.clientPort=15000'
		tmp += ' --rtr.serverPort=15001'
		tmp += ' --rtr.routerType=tree'
		tmp += calcTree( myRank, nidMap, nodesPerBoard  ) 
		tmp += ' --rtr.routerId=' + str( myRank/nodesPerBoard )
		tmp += ' --rtr.routeTable=' + routeFile
		tmp += ' --rtr.pwrApiConfig=' + config


	return tmp

def configBoard( myRank, nidMap, config, nodesPerBoard, boardsPerCab  ):
	tmp = ""
	if not myRank % nodesPerBoard:
		tmp += ' --srvr0.name=srv0'	
		tmp += ' --srvr0.rtrHost=' + nidMap[ myRank/nodesPerBoard * nodesPerBoard  ]
		tmp += ' --srvr0.rtrPort=15001'
		tmp += ' --srvr0.pwrApiConfig=' + config 
		tmp += ' --srvr0.pwrApiRoot=' + calcBoardRoot( myRank, len(nidMap), nodesPerBoard, boardsPerCab )
		tmp += ' --srvr0.pwrApiServer=' + nidMap[ myRank/nodesPerBoard * nodesPerBoard]
		tmp += ' --srvr0.pwrApiServerPort=15000'

	#print 'configBoard(',myRank,')',tmp 

	return tmp


def configNode( myRank, nidMap, config, nodesPerBoard, boardsPerCab ):
	tmp = ""
	tmp += ' --srvr.name=srvr'	
	tmp += ' --srvr.rtrHost=' + nidMap[ myRank/nodesPerBoard * nodesPerBoard ]
	tmp += ' --srvr.rtrPort=15001'
	tmp += ' --srvr.pwrApiConfig=' + config
	tmp += ' --srvr.pwrApiRoot=' + calcNodeRoot( myRank, len(nidMap), nodesPerBoard, boardsPerCab )
	#print 'configNode(',myRank,')',tmp 
	return  tmp

def initDaemon( myRank, nidMap, config, routeFile ):
	#print "nidmap={0} myRank={1}".format( nidMap, myRank)

	nodesPerBoard = machine.nodesPerBoard 
	boardsPerCab = machine.boardsPerCab 

	tmp = '/home/mjleven/pwrGIT/working/build/install/bin/pwrdaemon'

	tmp += configRtr( myRank, nidMap, config, routeFile,nodesPerBoard )
	tmp += configNode( myRank, nidMap, config, nodesPerBoard, boardsPerCab )
	tmp += configBoard( myRank, nidMap, config, nodesPerBoard, boardsPerCab )

	#print 'initDaemon(',myRank,')',tmp
	return tmp	


def initClient( myRank, nidMap, config, routeFile, object ):
	tmp  = '/home/mjleven/pwrGIT/working/build/examples/simpleTest '
	tmp += object 
	tmp += ' 0'
	#print tmp
	return tmp

#print "daemon.py end"
