#pragma once

// 声明一个外部全局变量，用于表示cache是否申请过下线
// 是否正在申请（管理员每次按下Ctrl+\都会触发使此变量置为true）
// 在心跳线程里每次发送心跳前会检查此变量是否为true，发送一次下线申请包后，此标志变量会重新置false
extern bool offline_applying;

// 是否申请过下线，这个变量在正式下线前会检查此变量，防止cache在没有主动给申请过下线的情况下收到OFFLINEACK下线确认数据包
extern bool offline_applied;   // 是否提交过申请（正式下线会检查管理员是否触发过下线信号）