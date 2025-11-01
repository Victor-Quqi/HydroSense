import serial
import argparse
import time
import sys

# 定义信标常量
EOT_BEACON = b'<<EOT>>\n'

def run_test(port, timeout, command, delay):
    """
    执行硬件在环测试。

    :param port: 串口号 (例如 'COM7')
    :param timeout: 超时时间 (秒)
    :param command: 要执行的命令 (字符串)
    :param delay: 发送命令前的延迟时间 (秒)
    """
    try:
        # 配置并打开串口
        ser = serial.Serial(
            port=port,
            baudrate=115200,
            timeout=1  # 设置一个较短的读取超时，以便在循环中检查全局超时
        )
        print(f"--- 串口 '{port}' 已打开 ---")
    except serial.SerialException as e:
        print(f"错误: 无法打开串口 '{port}': {e}", file=sys.stderr)
        sys.exit(1)

    start_time = time.time()

    # 模式判断：主动执行模式 vs 被动观测模式
    is_active_mode = command is not None

    if is_active_mode:
        # --- 主动执行模式 ---
        print(f"模式: 主动执行 (命令: '{command}', 超时: {timeout}s)")
        
        # 可选的初始延迟
        # 增加一个固定的启动延迟，确保设备准备就绪
        # 这个问题在测试中发现：即使有 --delay，也可能因为设备启动慢而错过命令
        print("等待 2s 以确保设备初始化完成...")
        time.sleep(2)

        if delay > 0:
            print(f"执行额外延迟 {delay}s...")
            time.sleep(delay)
            
        # 发送命令
        ser.write((command + '\n').encode('utf-8'))
        print(f">>> {command}")

    else:
        # --- 被动观测模式 ---
        print(f"模式: 被动观测 (超时: {timeout}s)")

    try:
        buffer = b''
        while True:
            # 检查是否超时
            if time.time() - start_time > timeout:
                if is_active_mode:
                    print(f"\n错误: 操作超时 ({timeout}s)，未收到结束信标 '{EOT_BEACON.decode().strip()}'。", file=sys.stderr)
                else:
                    print(f"\n--- 观测结束 (超时: {timeout}s) ---")
                break
            
            # 从串口读取数据
            data = ser.read(ser.in_waiting or 1)
            if data:
                buffer += data
                # 尝试按行处理
                while b'\n' in buffer:
                    line, buffer = buffer.split(b'\n', 1)
                    line += b'\n' # 将换行符加回去
                    
                    # 打印从设备接收到的每一行
                    sys.stdout.write(line.decode('utf-8', errors='ignore'))
                    sys.stdout.flush()

                    # 在主动模式下，检查是否收到了结束信标
                    if is_active_mode and line.strip() == EOT_BEACON.strip():
                        print(f"--- 收到结束信标，测试成功 ---")
                        # 成功接收到信标，立即退出循环
                        return

    except KeyboardInterrupt:
        print("\n--- 用户中断 ---")
    finally:
        if ser.is_open:
            ser.close()
            print(f"--- 串口 '{port}' 已关闭 ---")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='HydroSense HIL 测试脚本',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        '--port',
        default='COM7',
        help='指定串口号 (默认: COM7)'
    )
    parser.add_argument(
        '--timeout',
        type=int,
        default=30,
        help='设置超时时间，单位为秒 (默认: 30s)'
    )
    parser.add_argument(
        '--command',
        type=str,
        help='要发送到设备的命令。\n'
             '如果提供此参数，脚本将进入“主动执行”模式，\n'
             '并在收到 <<EOT>> 信标后立即退出。\n'
             '如果省略此参数，脚本将进入“被动观测”模式，\n'
             '并持续打印串口输出，直到超时。'
    )
    parser.add_argument(
        '--delay',
        type=int,
        default=0,
        help='发送命令前的延迟时间，单位为秒 (默认: 0s)'
    )

    args = parser.parse_args()

    run_test(args.port, args.timeout, args.command, args.delay)