import socket
import threading

HOST = "127.0.0.1"
PORT = 9000

file_data = {}
lock = threading.Lock()

def recv_all(conn):
    data = b""
    while True:
        is_disconnet = conn.recv(1)
        if not is_disconnet:
            break
        data = data + is_disconnet
        if is_disconnet == b"\n":
            break
    return data.decode().strip()

def print_file_data():
    print("\n===== FILE TABLE =====")
    if not file_data:
        print("(empty)")
    else:
        for filename, peers in file_data.items():
            print(f"{filename}: {peers}")
    print("======================\n")

def pro_register(parts):
    if len(parts) < 5:
        return "ERROR bad REGISTER format"

    peer_id = parts[1]
    ip = parts[2]

    try:
        port = int(parts[3])
    except ValueError:
        return "ERROR Invalid port"

    filenames = parts[4:]

    with lock:
        for filename in filenames:
            if filename not in file_data:
                file_data[filename] = []

            peer_info = (peer_id, ip, port)

            if peer_info not in file_data[filename]:
                file_data[filename].append(peer_info)

        print_file_data()

    return "OK REGISTERED"

def pro_query(parts):
    if len(parts) != 2:
        return "ERROR bad QUERY format"

    filename = parts[1]
    with lock:
        if filename not in file_data:
            return "NOT_FOUND"

        peers = file_data[filename]

        response_lines = []

        for peer_id, ip, port in peers:
            response_lines.append(f"FOUND {peer_id} {ip} {port}")

    response_lines.append("END")

    return "\n".join(response_lines)

def pro_update(parts):
    if len(parts) != 5:
        return "ERROR bad UPDATE format"

    peer_id = parts[1]
    ip = parts[2]

    try:
        port = int(parts[3])
    except ValueError:
        return "ERROR Invalid port"

    filename = parts[4]
    with lock:
        if filename not in file_data:
            file_data[filename] = []

        peer_info = (peer_id, ip, port)

        if peer_info not in file_data[filename]:
            file_data[filename].append(peer_info)

        print_file_data()

    return "OK UPDATED"

def do_client(conn, addr):
    print(f"[CONNECTED] {addr} connected")
    message = recv_all(conn)

    if message:
        print(f"[MESSAGE FROM {addr}] {message}")

        parts = message.split()
        command = parts[0]

        if command == "REGISTER":
            response = pro_register(parts)
        elif command == "QUERY":
            response = pro_query(parts)
        elif command == "UPDATE":
            response = pro_update(parts)
        else:
            response = "ERROR Unknown command"

        conn.sendall((response + "\n").encode())
    conn.close()
    print(f"[DISCONNECTED] {addr} disconnected")

def main():
    print(f"Starting tracker on host:{HOST} port:{PORT}")
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    server_socket.bind((HOST, PORT))
    server_socket.listen()
    print("Tracker is listening for connections...")

    while True:
        conn, addr = server_socket.accept()

        client_thread = threading.Thread(
            target=do_client,
            args=(conn, addr)
        )

        client_thread.start()


if __name__ == "__main__":
    main()