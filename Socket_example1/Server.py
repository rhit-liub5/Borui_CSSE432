import socket
import sys
import threading

def server_thread(comm_socket, client_num):
    while True:
        incoming_data = comm_socket.recv(1024).decode() ##接收数据
        
        if not incoming_data:
            break
        print("Received from client: ", str(client_num),str(incoming_data))
        outgoing_data = str(incoming_data).upper() ##获取用户输入作为响应
        comm_socket.send(outgoing_data.encode()) ##发送数据
    comm_socket.close() ##关闭通信套接字

def server_program():
    host = socket.gethostname() ##获取本地主机名
    #host_ip = socket.gethostbyname(host) ##获取本地主机IP地址
    
    name, aliases, ip_addresses = socket.gethostbyname_ex(host) ##获取本地主机的所有IP地址
    
    print("Host Name: ", str(name), "Aliases: ", str(aliases), "IP Addresses: ", str(ip_addresses))
    
    host_ip = ''
    i = 0
    while host_ip == '':
        if(address.startswith('127.0.0.1')):
            host_ip = ip_addresses[i]
        i += 1
    
    
    if(len(sys.argv) != 2):
        print("Usage: python Socket_example.py <port>")
        sys.exit()
    port = int(sys.argv[1]) ##获取端口号
    
    print("Host Name: ", str(host), "IP: ", str(host_ip), "Port: ", str(port))
    
    server_listening_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) ##创建套接字
    
    server_listening_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) ##设置套接字选项，允许重用地址
    
    server_listening_socket.bind(('', port)) ##绑定套接字到地址和端口
    
    server_listening_socket.listen(5) ##开始监听连接，参数为最大连接数
    
    client_num = 1
    
    while True:
        server_comm_socket, address = server_listening_socket.accept() ##接受连接
        
        print("Connection from: ", str(client_num), str(address))
        
        t= threading.Thread(target=server_thread, args=(server_comm_socket, client_num)) ##创建线程处理客户端连接
        t.start()
        client_num += 1

if __name__ == '__main__':
    server_program()