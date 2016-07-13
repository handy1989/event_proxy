import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 3333))
s.send('GET http://192.168.3.57/tmp.10K HTTP/1.1\r\n')
s.send('Host: 192.168.3.57\r\n')
s.send('\r\n')
f = open('xx', 'w')
#print s.recv(10240)
while True:
    n = s.recv(102400)
    print "recv %d bytes" % len(n)
    f.write(n) 
    if len(n) <= 0:
        break
