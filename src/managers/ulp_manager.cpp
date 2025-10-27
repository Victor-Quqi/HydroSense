/**
 * @file ulp_manager.cpp
 * @brief ULP管理器实现
 * @details 大致职责是在主CPU进入休眠前，加载并启动ULP程序；在主CPU被唤醒后，解析ULP的唤醒原因
 */

#include "ulp_manager.h"