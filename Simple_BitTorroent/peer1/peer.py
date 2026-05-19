import socket
import os
import sys
import threading
import protocol

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

def query_tracker(filename):
    command = f"QUERY {filename}\n"
    peers = []

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TRACKER_HOST, TRACKER_PORT))
        s.sendall(command.encode())

        while True:
            line = recv_all(s)
            if line == "NOT_FOUND":
                return []
            if line == "END":
                break
            parts = line.split()
            if len(parts) == 4 and parts[0] == "FOUND":
                peer_id = parts[1]
                ip = parts[2]
                port = int(parts[3])
                peers.append((peer_id, ip, port))
    return peers

def download_from_peer(ip, port, filename, shared_folder):
    command = f"GET {filename}\n"

    file_path = os.path.join(shared_folder, filename)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((ip, port))
        s.sendall(command.encode())

        header = recv_all(s)
        parts = header.split()

        if len(parts) == 0 or parts[0] != "OK":
            print("Download failed:", header)
            return False

        filesize = int(parts[1])

        received = 0

        with open(file_path, "wb") as f:
            while received < filesize:
                data = s.recv(4096)

                if not data:
                    break

                f.write(data)
                received += len(data)

        if received == filesize:
            print("Downloaded:", filename)
            return True
        else:
            print("Download incomplete.")
            return False

def send_update(peer_id, peer_ip, peer_port, filename):
    command = f"UPDATE {peer_id} {peer_ip} {peer_port} {filename}\n"

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TRACKER_HOST, TRACKER_PORT))
        s.sendall(command.encode())

        response = s.recv(1024).decode().strip()
        print("Tracker response:", response)

def download_file(filename, shared_folder, peer_id, peer_ip, peer_port):
    peers = query_tracker(filename)

    if len(peers) == 0:
        print("No peers found for file:", filename)
        return

    print("Peers found:")
    for i, peer in enumerate(peers):
        print(i, peer)

    target_peer_id, target_ip, target_port = peers[0]

    print("Downloading from:", target_peer_id, target_ip, target_port)

    success = download_from_peer(target_ip, target_port, filename, shared_folder)

    if success:
        send_update(peer_id, peer_ip, peer_port, filename)

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
    server_thread = threading.Thread(
        target=server,
        args=(peer_ip, peer_port, shared_folder)
    )

    server_thread.start()

    while True:
        filename = input("Enter filename to download, or quit: ")

        if filename == "quit":
            break

        download_file(filename, shared_folder, peer_id, peer_ip, peer_port)


if __name__ == "__main__":
    main()