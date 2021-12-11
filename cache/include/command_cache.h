#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "HashSlot.h"
#include "cmcdata.pb.h"

using namespace std;

CMCData MakeCommandData(const ::CommandInfo_CmdType value, const string& param1, const string& param2);
bool SendCommandData(const CMCData& send_data, const char* dst_ip, u_int16_t dst_port, CMCData& recv_data);