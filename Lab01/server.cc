#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;
string mode;
int port;

int main(int argc, char *argv[]){
    if (argc != 3){
        cerr << "Usage: ./server -t <port-number>" <<endl;
        return 1;
    }

    mode = argv[1];
    if (mode != "-t" || mode != "-u"){
        cerr<<"You need a right model -t or -u\n";
    }
    
    port = atoi(argv[2]);
    if (port<0 || port > 65535){
        cerr<<"bad port number\n";
    }
    
}