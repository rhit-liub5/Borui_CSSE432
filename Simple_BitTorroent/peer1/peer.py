import socket
import os
import sys
import threading


TRACKER_HOST = "127.0.0.1"
TRACKER_PORT = 9000

def recv_all(conn):
    data = b""
    while True:
        chunk = conn.recv(1)
        if not chunk:
            break
        data += chunk
        if chunk == b"\n":
            break
    return data.decode().strip()

def find_files(folder):
    filenames = []

    if not os.path.exists(folder):
        os.makedirs(folder)

    for name in os.listdir(folder):
        path = os.path.join(folder, name)

        if os.path.isfile(path):
            filenames.append(name)

    return filenames

def send_register(peer_id, peer_ip, peer_port, shared_folder):
    filenames = find_files(shared_folder)

    if len(filenames) == 0:
        print("No files to register.")
        return

    command = f"REGISTER {peer_id} {peer_ip} {peer_port} " + " ".join(filenames) + "\n"
    ## with 的好处是：用完之后会自动关闭 socket。
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TRACKER_HOST, TRACKER_PORT))
        s.sendall(command.encode())

        response = s.recv(1024).decode().strip()
        print("Tracker response:", response)

def client(conn, addr, shared_folder):
    print("Peer connected:", addr)

    try:
        command = recv_all(conn)
        print("Received command:", command)

        parts = command.split()

        if len(parts) != 2 or parts[0] != "GET":
            conn.sendall(b"ERROR Invalid command\n")
            return

        filename = parts[1]
        file_path = os.path.join(shared_folder, filename)

        if not os.path.isfile(file_path):
            conn.sendall(b"ERROR File not found\n")
            return

        filesize = os.path.getsize(file_path)

        header = f"OK {filesize}\n"
        conn.sendall(header.encode())

        with open(file_path, "rb") as f:
            while True:
                data = f.read(4096)

                if not data:
                    break

                conn.sendall(data)

        print("Sent file:", filename)

    finally:
        conn.close()
        print("Peer disconnected:", addr)

def server(peer_ip, peer_port, shared_folder):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server_socket.bind((peer_ip, peer_port))
    server_socket.listen()

    print(f"Peer server listening on {peer_ip}:{peer_port}")

    while True:
        conn, addr = server_socket.accept()

        client_thread = threading.Thread(
            target=client,
            args=(conn, addr, shared_folder)
        )

        client_thread.start()

def main():
    if len(sys.argv) != 4:
        print("Usage: python peer.py <peer_id> <peer_port> <shared_folder>")
        return

    peer_id = sys.argv[1]
    peer_ip = "127.0.0.1"
    peer_port = int(sys.argv[2])
    shared_folder = sys.argv[3]

    print("Peer ID:", peer_id)
    print("Peer IP:", peer_ip)
    print("Peer Port:", peer_port)
    print("Shared folder:", shared_folder)

    send_register(peer_id, peer_ip, peer_port, shared_folder)
    server(peer_ip, peer_port, shared_folder)


if __name__ == "__main__":
    main()