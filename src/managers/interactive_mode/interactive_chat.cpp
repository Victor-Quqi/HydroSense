/**
 * @file interactive_chat.cpp
 * @brief Chat interface implementation
 */

#include "interactive_chat.h"
#include "interactive_common.h"

void interactive_chat_enter(void) {
    LOG_DEBUG("Interactive", "Entered STATE_CHAT");
    LOG_INFO("Interactive", "TODO: P1.6 implement chat interface logic");
}

interactive_state_t interactive_chat_handle(interactive_state_t* state) {
    // TODO: P1.6 实现聊天界面逻辑
    // - 显示LLM回复和选项
    // - 单击选择，双击返回

    // Temporary: double click to return
    if (input_manager_get_button_double_clicked()) {
        LOG_INFO("Interactive", "Returning to main menu from CHAT");
        interactive_switch_state(STATE_MAIN_MENU, state);
    }

    return *state;
}
