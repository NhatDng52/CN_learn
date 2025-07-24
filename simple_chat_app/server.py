import socket
import threading
import json

HOST = '127.0.0.1'
PORT = 12345

# protocol format
# {
#     "username": ____,
#     "command": ____,
#     "other name ": ____ or [],
#     "message": ____,
#     "timestamp": ____, 
#                 }
OnlineUsers = set()
nicknames = {}
chat_history = {}
def register_nickname(client_socket, nickname):
    old_nickname = nicknames.get(client_socket, None)
    nicknames[client_socket] = nickname
    return f"Nickname changed from {old_nickname} to {nickname}" if old_nickname else f"Nickname set to {nickname}\n"

def handle_client_commands(command):
    if command['command'] == 'fetch onl users':
        return json.dumps({
            "users": list(nicknames.values())
        })
    elif command['command'] == 'send_message':
        # Handle sending messages
        to = command['other name'] 
        target_sockets = [client for client, nickname in nicknames.items() if nickname in to]
        if not target_sockets:
            return "No users found with the specified nickname."
        message = command['message']
        for client in target_sockets:
            send_data = {
                "username": command['username'],
                "message": message,
            }
            client.send(json.dumps(send_data).encode('utf-8'))
            return "Message sent."
    elif command['command'] == 'rename':
        new_nickname = command['new_nickname']
        return register_nickname(command['username'], new_nickname)
    else:
        return "Unknown command."

def handle_client(client_socket, addr):
    try:
        username = client_socket.recv(1024).decode('utf-8')
        register_nickname(client_socket, username)
        print(f"[+] Client {addr} connected as {username}.")
        
        while True:
            msg = client_socket.recv(1024)
            if not msg:
                break
            data = json.loads(msg.decode('utf-8'))
            response = handle_client_commands(data)
            if response:
                client_socket.send(response.encode('utf-8'))
    except Exception as err:
        print(f"[!] Error with client {addr}: {err}")
    finally:
        print(f"[-] Client {addr} disconnected.")
        if client_socket in OnlineUsers:
            OnlineUsers.remove(client_socket)
        client_socket.close()


def start_server():
    global server
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()
    print(f"[+] Server is listening on {HOST}:{PORT}")

    try:
        while True:
            client_socket, addr = server.accept()
            OnlineUsers.add(client_socket)
            thread = threading.Thread(target=handle_client, args=(client_socket, addr))
            thread.start()
    except KeyboardInterrupt:
        print("\n[!] KeyboardInterrupt received. Shutting down server...")
    finally:
        for client in OnlineUsers:
            client.close()
        server.close()
        print("[+] Server closed cleanly.")


if __name__ == "__main__":
    start_server()
