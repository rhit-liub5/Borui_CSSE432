import socket
import sys

def client_program():
    if(len(sys.argv) != 3):
        print("Usage: python Socket_example.py <host> <port>")
        sys.exit()
    
    server_port = int(sys.argv[2]) ##获取端口号
    server_addr = (sys.argv[1], server_port) ##获取服务器地址和端口号
    
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) ##创建套接字
    client_socket.connect(server_addr) ##连接到服务器
    message = input("Enter message to send to server: ") ##获取用户输入
    while message.lower().strip() != ';;;':
        client_socket.send(message.encode()) ##发送数据
        data = client_socket.recv(1024).decode() ##接收数据
        print("Received from server: ", str(data)) ##打印服务器响应
        message = input("Enter message to send to server: ") ##获取用户输入
    client_socket.close() ##关闭套接字
if __name__ == '__main__':
    print("started")
    client_program()