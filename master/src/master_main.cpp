#include <stdlib.h>

#include <iostream>
#include <unistd.h>

#include "MasterServer.h"
#include "cmcdata.pb.h"
#include "HashSlot.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "usage:" << argv[0] << " <port of server>\n";
        return 0;
    }

    /* ---------------------全局变量区--------------------- */

    const u_int16_t master_port = atoi(argv[1]);  // 本机port
    if (master_port < MIN_PORT || master_port > MAX_PORT) {
        std::cout << "port error\n";
        exit(-1);
    }
    // 新建一个MasterServer对象，并设置线程池大小
    MasterServer master_server(10);
    master_server.init();
    usleep(1000);   // 1ms
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Master started..." << std::endl;
    // 设置master_port端口，准备监听
    if (!master_server.bindAndListen(master_port)) {
        std::cout << "listen error\n";
        exit(-1);
    }
    std::cout << "start listening on port: " << master_port << std::endl;
    std::cout << "-->" << std::endl;

    /* -------- 服务器启动成功，开始服务，while(1)循环监听 -------- */
    if (!master_server.startService()) {
        std::cout << "start service error\n";
        exit(-1);
    }
    
    return 0;
}