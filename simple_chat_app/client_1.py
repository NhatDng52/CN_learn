import socket
import threading
import json
SEVER_HOST = '127.0.0.1'
SERVER_PORT = 12345
USER_NAME = 'client_1'

def main():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((SEVER_HOST, SERVER_PORT))
    print(" Đã kết nối đến server!")
    client.send(USER_NAME.encode('utf-8'))
    while True:
        try:
            msg = input()
            msg = {
    "username": "client_1",
    "command": "send_message",
    "to": "server",
    "message": msg,
    "timestamp": "123", 
                }
            client.send(json.dumps(msg).encode('utf-8'))
        except KeyboardInterrupt:
            print("\n Đã thoát.")
            client.close()
            break

if __name__ == "__main__":
    main()
