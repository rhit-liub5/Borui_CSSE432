# protocol.py

def register(peer_id, ip, port, filenames):
    """
    REGISTER peer1 127.0.0.1 5001 hello.txt test.txt
    """
    return f"REGISTER {peer_id} {ip} {port} " + " ".join(filenames) + "\n"


def query(filename):
    """
    QUERY hello.txt
    """
    return f"QUERY {filename}\n"


def update(peer_id, ip, port, filename):
    """
    UPDATE peer2 127.0.0.1 5002 hello.txt
    """
    return f"UPDATE {peer_id} {ip} {port} {filename}\n"


def get(filename):
    """
    GET hello.txt
    """
    return f"GET {filename}\n"


def found(peer_id, ip, port):
    """
    FOUND peer1 127.0.0.1 5001
    """
    return f"FOUND {peer_id} {ip} {port}\n"


def ok_register():
    return "OK REGISTERED\n"


def ok_update():
    return "OK UPDATED\n"


def ok_file(filesize):
    """
    OK 128
    """
    return f"OK {filesize}\n"


def not_found():
    return "NOT_FOUND\n"


def end():
    return "END\n"


def error(message):
    return f"ERROR {message}\n"