import socket
import os
import sys


TRACKER_HOST = "127.0.0.1"
TRACKER_PORT = 9000


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

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TRACKER_HOST, TRACKER_PORT))
        s.sendall(command.encode())

        response = s.recv(1024).decode().strip()
        print("Tracker response:", response)

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


if __name__ == "__main__":
    main()