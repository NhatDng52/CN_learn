import socket
import threading
import json
import time
from datetime import datetime
SEVER_HOST = '127.0.0.1'
SERVER_PORT = 12345
USER_NAME = 'client_1'
NICKNAME = 'client_1'

def listen_for_messages(client):
    """Function to listen for messages from the server"""
    while True:
        try:
            response = client.recv(1024).decode('utf-8')
            if response:
                print(f"\n[Server Response]: {response}")
                print("Enter message: ", end="", flush=True)  # Restore input prompt
            else:
                break
        except Exception as e:
            print(f"\n[Error] Connection lost: {e}")
            break

def main():
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((SEVER_HOST, SERVER_PORT))
    print(" Đã kết nối đến server!")
    client.send(USER_NAME.encode('utf-8'))
    
    # Start the listening thread
    listen_thread = threading.Thread(target=listen_for_messages, args=(client,))
    listen_thread.daemon = True  # Thread will close when main program closes
    listen_thread.start()
    
    print("Connected! You can start sending messages. Type 'quit' to exit.")
    
    while True:
        try:
            msg = input("Enter message: ")
            if msg.lower() == 'quit':
                print("\n Đã thoát.")
                break
            elif msg.lower() == 'fetch onl users':    
                msg_data = {
                    "username": NICKNAME,
                    "command": "fetch onl users",
                    "to": "server",
                    "message": msg,
                    "timestamp": datetime.now().isoformat()
                }
            elif msg.lower() == 'send_message':
                to_nickname = input("Enter the nickname of the user to send message: ")
                message = input("Enter your message: ")
                msg_data = {
                    "username": NICKNAME,
                    "command": "send_message",
                    "other name": [to_nickname],
                    "message": message,
                    "timestamp": datetime.now().isoformat()
                }
            elif msg.lower() == 'rename':
                new_nickname = input("Enter your new nickname: ")
                msg_data = {
                    "username": NICKNAME,
                    "command": "rename",
                    "new_nickname": new_nickname,
                    "timestamp": datetime.now().isoformat()
                }
            else:
                print("Unknown command. Please try again.")
                continue
            
            client.send(json.dumps(msg_data).encode('utf-8'))
        except KeyboardInterrupt:
            print("\n Đã thoát.")
            break
    
    client.close()

if __name__ == "__main__":
    main()
