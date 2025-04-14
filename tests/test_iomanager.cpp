#include "sylar/iomanager.hpp"
#include "sylar/log.hpp"
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

void write_cb(int connect_fd, sockaddr_in client_sin){
    write(connect_fd, "ok\n", 3);
}

void read_cb(int connect_fd, sockaddr_in client_sin){
    char buffer[256] = {0};
    char str[INET_ADDRSTRLEN];
    int ret = read(connect_fd, buffer, sizeof(buffer));
    SYLAR_LOG_DEBUG(g_logger) << inet_ntop(AF_INET, &client_sin.sin_addr, str, sizeof(str)) << ": " << buffer;

    if(ret > 0){
        sylar::IOManager::GetThis()->addEvent(connect_fd, sylar::IOManager::READ, [connect_fd, client_sin](){
            read_cb(connect_fd, client_sin);
        });
        sylar::IOManager::GetThis()->addEvent(connect_fd, sylar::IOManager::WRITE, [connect_fd, client_sin](){
            write_cb(connect_fd, client_sin);
        });
    } else if(ret == 0){
        SYLAR_LOG_DEBUG(g_logger) << "disconnected: " << inet_ntop(AF_INET, &client_sin.sin_addr, str, sizeof(str));
        close(connect_fd);
    } else {
        SYLAR_LOG_DEBUG(g_logger) << "read error";
    }
}

void listen_cb(int listen_fd){
    sockaddr_in client_sin;
    socklen_t sock_addr_len = INET_ADDRSTRLEN;
    int connect_fd = accept(listen_fd, (sockaddr*)&client_sin, &sock_addr_len);
    char str[INET_ADDRSTRLEN];
    SYLAR_LOG_DEBUG(g_logger) << "connect from ip: " << inet_ntop(AF_INET, &client_sin.sin_addr, str, sizeof(str));

    SYLAR_LOG_DEBUG(g_logger) << "connect_fd: " << connect_fd;
    sylar::IOManager::GetThis()->addEvent(connect_fd, sylar::IOManager::READ, [connect_fd, client_sin](){
        read_cb(connect_fd, client_sin);
    });

    sylar::IOManager::GetThis()->addEvent(listen_fd, sylar::IOManager::READ, [listen_fd](){
        listen_cb(listen_fd);
    });
}

int main(){
    sylar::IOManager io_manager(5, "iomanager");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(7777);
    bind(listen_fd, (sockaddr*)&sin, sizeof(sin));
    listen(listen_fd, 1024);

    sylar::IOManager::GetThis()->addEvent(listen_fd, sylar::IOManager::READ, [listen_fd](){
        listen_cb(listen_fd);
    });

    while(1);
}