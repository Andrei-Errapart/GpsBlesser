Generate PPS signal according to incoming $GPZDA (not GPGGA) from trimble.

Input 1: Trimble, 38400, 10Hz.
	Sentences: GGGA, VTG , ZDA
	Port: UART0.RX

Output 1: Compass, 9600, 25Hz.
	Sentence: HDG
	Port: UART1.TX

Output 2: GPS + HDG, 38400.
	Sentences: all from GPS + HDG
	Port: UART0.TX

Output 3: PPS, both negative and positive.
	Port: LED-s.

HDG calculation: from GGA.

Control:
	Port: UART3, 38400 baud.

$HEHDT,xx,T*hh
heading, degrees, true
T
