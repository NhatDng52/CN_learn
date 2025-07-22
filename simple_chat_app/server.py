import socket
import threading
import json

HOST = '127.0.0.1'
PORT = 12345

clients = []
server = None  # để tham chiếu toàn cục


def handle_client(client_socket, addr):
    try:
        username = client_socket.recv(1024).decode('utf-8')
        print(f"[+] Client {addr} connected as {username}.")
        while True:
            msg = client_socket.recv(1024)
            if not msg:
                break
            data = json.loads(msg.decode('utf-8'))
            print(f"[{addr}] {data}")
    except Exception as err:
        print(f"[!] Error with client {addr}: {err}")
    finally:
        print(f"[-] Client {addr} disconnected.")
        if client_socket in clients:
            clients.remove(client_socket)
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
            clients.append(client_socket)
            thread = threading.Thread(target=handle_client, args=(client_socket, addr))
            thread.start()
    except KeyboardInterrupt:
        print("\n[!] KeyboardInterrupt received. Shutting down server...")
    finally:
        for client in clients:
            client.close()
        server.close()
        print("[+] Server closed cleanly.")


if __name__ == "__main__":
    start_server()
