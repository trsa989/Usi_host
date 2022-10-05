
In order to help debug, use socat to see DATA traces in USI/TTY and Concentrator/TCP:

1) Start USI socat and usa a pseudoterminal 

	socat -d -x -v  PTY,link=$HOME/dev/vmodem0,raw,echo=0,waitslave /dev/ttyUSB0,echo=0 

2) Start dlmsotcp server application in a dummy port, for example 4159. Use the tty connection to the pseudo terminal creater before:
	
	./dlmsotcp --verbose -v 3 -s 4159 -b 115200 --tty /home/sesteban/dev/vmodem0

3) Finally, start a second socat instance to redirect the Concentrator connection to our dlmsotcp server.
	
	socat -d -d -d -x -v TCP-LISTEN:4059,reuseaddr TCP:localhost:4159


Now, you must have 3 diferent windows displaying all traffic generated between Concentrator, dlmsotco server and Base Node.


