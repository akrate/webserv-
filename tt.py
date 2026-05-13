python3 -c "
import socket, threading, time

def connect():
    s = socket.socket()
    s.connect(('127.0.0.1', 1025))
    s.sendall(b'GET / HTTP/1.1\r\nHost: localhost:1025\r\nConnection: keep-alive\r\n\r\n')
    time.sleep(3)   # hold connection open 3 seconds
    s.close()

threads = [threading.Thread(target=connect) for _ in range(6)]
for t in threads:
    t.start()
for t in threads:
    t.join()
"
