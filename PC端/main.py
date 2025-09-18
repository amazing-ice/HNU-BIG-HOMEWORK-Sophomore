import tkinter as tk
from tkinter import scrolledtext, font, ttk, simpledialog, messagebox
import serial
import time
import ctypes
import threading
import sys
import queue
import os
import random
from ai import get_llm_response, questions

# --- 配置 ---
SERIAL_PORT = 'COM3'  # ❗❗❗ 请根据您的实际情况修改COM端口号
BAUD_RATE = 9600

# --- STC板发送的命令代码定义 ---
CMD_SET_POMODORO = 0
CMD_UNLOCK = 1
CMD_LOCK = 2
CMD_LED_CONTROL = 3
CMD_UPDATE_TIME = 4

# --- 名言库 ---
QUOTES = [
    "业精于勤，荒于嬉。",
    "志不强者智不达。",
    "此刻的专注，是未来的序章。",
    "每一次专注，都是一次超越。",
    "心无旁骛，万事可成。"
]

class PomodoroApp:
    def __init__(self, root):
        self.root = root
        self.root.title("智能番茄钟联动程序")
        self.root.geometry("500x450")
        self.root.resizable(True, True)
        self.root.minsize(500, 450)

        # 实例变量
        self.ser = None
        self.data_queue = queue.Queue()
        self.pomodoro_thread = None
        self.stop_timer_flag = threading.Event()
        self.lock_window = None
        self.tasks = []  # 存储任务列表
        # 用于更新锁屏窗口时间的变量，必须是实例变量
        self.lock_timer_var = tk.StringVar()

        self.setup_ui()
        self.start_serial_thread()
        self.update_gui()
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

    def setup_ui(self):
        # 图标和样式设置
        try:
            # 尝试在脚本所在目录查找图标
            script_dir = os.path.dirname(os.path.abspath(__file__))
            icon_path = os.path.join(script_dir, "tomato.ico")
            if os.path.exists(icon_path):
                self.root.iconbitmap(icon_path)
        except Exception as e:
            print(f"设置图标失败: {e}")

        style = ttk.Style()
        current_theme = style.theme_use()
        try:
            style.configure(f"{current_theme}.TNotebook.Tab", padding=[10, 5], font=font.Font(family="Microsoft YaHei", size=10))
        except tk.TclError:
            print("警告：无法应用自定义选项卡样式。")

        # --- 选项卡控件 ---
        self.notebook = ttk.Notebook(self.root)
        self.main_frame = tk.Frame(self.notebook)
        self.task_frame = tk.Frame(self.notebook)
        self.log_frame = tk.Frame(self.notebook)
        
        self.notebook.add(self.main_frame, text="主界面")
        self.notebook.add(self.task_frame, text="任务列表")
        self.notebook.add(self.log_frame, text="日志")
        self.notebook.pack(pady=10, padx=10, expand=True, fill='both')

        # --- 主界面元素 ---
        self.status_font = font.Font(family="Microsoft YaHei", size=12)
        self.status_var = tk.StringVar(value="正在初始化...")
        tk.Label(self.main_frame, textvariable=self.status_var, font=self.status_font, fg="gray").pack(pady=10)

        self.timer_font = font.Font(family="Consolas", size=60, weight="bold")
        self.timer_var = tk.StringVar(value="00:25:00")
        tk.Label(self.main_frame, textvariable=self.timer_var, font=self.timer_font).pack(pady=20)
        
        tk.Label(self.main_frame, text="HNU 杜禹寰 向望 谢滨冰", font=("Microsoft YaHei", 10), fg="darkblue").pack(pady=10)

        # --- 任务列表界面元素 ---
        tk.Label(self.task_frame, text="今日任务列表:", font=self.status_font).pack(anchor='w', padx=10, pady=5)
        self.task_listbox = tk.Listbox(self.task_frame, font=("Microsoft YaHei", 11), height=10)
        self.task_listbox.pack(padx=10, pady=5, fill='both', expand=True)

        task_entry_frame = tk.Frame(self.task_frame)
        task_entry_frame.pack(fill='x', padx=10, pady=5)
        self.task_entry = tk.Entry(task_entry_frame, font=("Microsoft YaHei", 11))
        self.task_entry.pack(side='left', fill='x', expand=True)
        self.add_task_button = tk.Button(task_entry_frame, text="添加任务", command=self.add_task)
        self.add_task_button.pack(side='right', padx=5)
        self.delete_task_button = tk.Button(self.task_frame, text="删除选中任务", command=self.delete_task)
        self.delete_task_button.pack(pady=5)

        # --- 日志界面元素 ---
        self.log_area = scrolledtext.ScrolledText(self.log_frame, state='disabled', font=("Microsoft YaHei", 9))
        self.log_area.pack(pady=5, padx=5, expand=True, fill='both')

    def log(self, message):
        timestamp = time.strftime("%H:%M:%S", time.localtime())
        full_message = f"[{timestamp}] {message}\n"
        self.log_area.config(state='normal')
        self.log_area.insert(tk.END, full_message)
        self.log_area.see(tk.END)
        self.log_area.config(state='disabled')

    def start_serial_thread(self):
        self.serial_thread = threading.Thread(target=self.serial_listener, daemon=True)
        self.serial_thread.start()

    def update_gui(self):
        while True:
            try:
                message_type, data = self.data_queue.get_nowait()
                if message_type == "status": 
                    self.status_var.set(data)
                elif message_type == "log": 
                    self.log(data)
                elif message_type == "timer":
                    # 更新主窗口的时间
                    self.timer_var.set(data)
                    # 如果锁定窗口存在，也更新它的时间
                    if self.lock_window and self.lock_window.winfo_exists():
                        self.lock_timer_var.set(data)
                elif message_type == "start_session":
                    # 从安全的主线程调用界面函数
                    self.start_pomodoro_session(data)
            except queue.Empty:
                break
        self.root.after(100, self.update_gui)

    def on_closing(self):
        self.log("程序正在关闭...")
        if self.pomodoro_thread and self.pomodoro_thread.is_alive():
            self.stop_timer_flag.set()
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.root.destroy()

    def serial_listener(self):
        self.data_queue.put(("status", "正在连接串口..."))
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
            self.data_queue.put(("log", f"尝试连接到 {SERIAL_PORT}"))
        except serial.SerialException as e:
            self.data_queue.put(("status", f"错误：无法连接到 {SERIAL_PORT}"))
            self.data_queue.put(("log", f"错误详情: {e}"))
            return
        
        self.data_queue.put(("status", "等待STC板就绪信号(RDY)..."))
        is_ready = False
        handshake_buffer = b''
        timeout = time.time() + 10
        buffer = b'' # 提前定义buffer
        
        while not is_ready and time.time() < timeout+15:
            if self.ser.in_waiting > 0:
                handshake_buffer += self.ser.read(self.ser.in_waiting)
                ready_signal_pos = handshake_buffer.find(b'RDY\n')
                if ready_signal_pos != -1:
                    is_ready = True
                    buffer = handshake_buffer[ready_signal_pos + 4:]
                    self.data_queue.put(("status", "设备已连接 | 待机模式"))
                    self.data_queue.put(("log", "握手成功！系统准备就绪。"))
            time.sleep(0.1)
        
        if not is_ready:
            self.data_queue.put(("status", "错误：连接超时"))
            self.ser.close()
            return

        valid_cmds = [CMD_SET_POMODORO, CMD_UNLOCK, CMD_LOCK, CMD_LED_CONTROL, CMD_UPDATE_TIME]
        while True:
            try:
                if self.ser.in_waiting > 0:
                    buffer += self.ser.read(self.ser.in_waiting)
                
                # 循环处理缓冲区中的所有完整指令包
                while len(buffer) >= 5:
                    if buffer[0] in valid_cmds:
                        self.handle_command_packet(buffer[:5])
                        buffer = buffer[5:]
                    else:
                        break # 如果开头不是有效指令，则跳出循环等待更多数据或文本行
                
                # 处理文本行
                newline_pos = buffer.find(b'\n')
                if newline_pos != -1:
                    text = buffer[:newline_pos+1].decode('utf-8', errors='ignore').strip()
                    if text: self.data_queue.put(("log", f"STC信息: {text}"))
                    buffer = buffer[newline_pos+1:]
                
                time.sleep(0.05)
            except Exception as e:
                self.data_queue.put(("status", "错误：串口连接中断"))
                self.data_queue.put(("log", f"串口线程错误: {e}"))
                if self.ser:
                    self.ser.close()
                break

    def handle_command_packet(self, packet):
        print(packet)
        if len(packet) != 5: return
        cmd, d1, d2, d3 = packet[0], packet[1], packet[2], packet[3]
        self.data_queue.put(("log", f"收到指令 -> 码:{cmd}, 数据:{list(packet[1:])}"))
        
        if cmd == CMD_LOCK or cmd == CMD_UPDATE_TIME:
            self.data_queue.put(("status", "时间设置模式"))
            if cmd == CMD_LOCK: self.data_queue.put(("log", f"进入设置模式: {d1:02d}:{d2:02d}:{d3:02d}"))
            self.timer_var.set(f"{d1:02d}:{d2:02d}:{d3:02d}")
            
        elif cmd == CMD_SET_POMODORO:
            total_seconds = d1 * 3600 + d2 * 60 + d3
            self.data_queue.put(("status", "专注中..."))
            self.data_queue.put(("log", f"确认设置，时长 {d1:02d}:{d2:02d}:{d3:02d}"))
            # 向主线程发送请求，由主线程创建专注窗口
            self.data_queue.put(("start_session", total_seconds))
            
        elif cmd == CMD_UNLOCK:
            self.data_queue.put(("status", "设备已连接 | 待机模式"))
            self.data_queue.put(("log", "收到STC板的停止指令。"))
            if self.pomodoro_thread and self.pomodoro_thread.is_alive():
                self.stop_timer_flag.set()

        elif cmd == CMD_LED_CONTROL:
            if (d1 == 1):
                tq = dict()
                tq['晴'] = '10'
                tq['雨'] = '11'
                tq['雾'] = '12'
                tq['阴'] = '13'
                answer = get_llm_response(questions[d1-1])
                print(answer)
                answer = answer.split((','))
                answer[0] = tq[answer[0]]
                answer = [int(_) for _ in answer]
                stop_command = bytes([CMD_LED_CONTROL] + answer)
                self.ser.write(stop_command)  # 发送指令到STC板
                self.data_queue.put(("log", f"已向STC板发送天气"))
            elif (d1==2):
                month = {'一': 1, '二': 2, '三': 3, '四': 4, '五': 5, '六': 6, '七': 7, '八': 8, '九': 9, '十': 10, '冬': 11, '腊': 12}
                day_shi = {'初': 0, '十': 10, '廿': 20}
                answer = get_llm_response(questions[d1 - 1])
                print(answer)
                t = [CMD_LED_CONTROL, 2, month[answer[0]]]
                if (answer[2:] == "初十"):
                    t.append(10)
                elif (answer[2:]) == "二十":
                    t.append(20)
                elif (answer[2:]) == "三十":
                    t.append(30)
                else:
                    t.append(day_shi[answer[2]] + month[answer[3]])
                t.append(0)
                print(t)
                stop_command = bytes(t)
                self.ser.write(stop_command)  # 发送指令到STC板
                self.data_queue.put(("log", f"已向STC板发送日期"))

    def add_task(self):
        task = self.task_entry.get().strip()
        if task:
            self.tasks.append(task)
            self.task_listbox.insert(tk.END, task)
            self.task_entry.delete(0, tk.END)
            self.log(f"任务已添加: {task}")
        else:
            messagebox.showwarning("提示", "任务内容不能为空！")

    def delete_task(self):
        selected_indices = self.task_listbox.curselection()
        if not selected_indices:
            messagebox.showwarning("提示", "请先选择要删除的任务！")
            return
        
        # 从后往前删除，避免索引错乱
        for i in sorted(selected_indices, reverse=True):
            task = self.tasks.pop(i)
            self.task_listbox.delete(i)
            self.log(f"任务已删除: {task}")

    def start_pomodoro_session(self, total_seconds):
        if self.lock_window and self.lock_window.winfo_exists():
            self.log("已有专注任务在进行中。")
            return

        self.lock_window = tk.Toplevel(self.root)
        self.lock_window.attributes('-fullscreen', True)
        self.lock_window.attributes('-topmost', True)
        self.lock_window.config(bg="white")
        self.lock_window.protocol("WM_DELETE_WINDOW", lambda: None) # 禁用关闭按钮

        self.lock_timer_var.set("") # 清空旧时间
        tk.Label(self.lock_window, textvariable=self.lock_timer_var, font=("Consolas", 90, "bold"), fg="black", bg="white").pack(pady=(80, 20))
        
        task_display_text = "本次任务:\n\n"
        if self.tasks:
            task_display_text += "\n".join(f"- {task}" for task in self.tasks)
        else:
            task_display_text += "- 保持专注！"
        tk.Label(self.lock_window, text=task_display_text, justify='left', font=("Microsoft YaHei", 14), fg="gray", bg="white").pack(pady=20)
        
        tk.Label(self.lock_window, text=random.choice(QUOTES), font=("KaiTi", 16, "italic"), fg="darkblue", bg="white").pack(pady=40)
        
        exit_button = tk.Button(
            self.lock_window,
            text="退出专注模式",
            command=self.force_stop_pomodoro,
            font=("Microsoft YaHei", 14, "bold"),
            bg="#FF6347",  # 番茄红色
            fg="white",
            relief="flat",
            padx=20,
            pady=10,
            cursor="hand2"
        )
        exit_button.pack(side="bottom", pady=40)

        if self.pomodoro_thread and self.pomodoro_thread.is_alive():
            self.stop_timer_flag.set()
            self.pomodoro_thread.join(timeout=1)
            
        self.pomodoro_thread = threading.Thread(target=self.pomodoro_timer_task, args=(total_seconds, self.lock_window), daemon=True)
        self.pomodoro_thread.start()

    def force_stop_pomodoro(self, event=None):
        self.data_queue.put(("log", "用户请求提前结束。"))
        
        if self.pomodoro_thread and self.pomodoro_thread.is_alive():
            self.stop_timer_flag.set()

        if self.ser and self.ser.is_open:
            try:
                stop_command = bytes([CMD_UNLOCK, 0, 0, 0, 0])
                self.ser.write(stop_command)
                self.data_queue.put(("log", f"已向STC板发送停止指令 (CMD={CMD_UNLOCK})。"))
            except Exception as e:
                self.data_queue.put(("log", f"发送停止指令失败: {e}"))
    
    def pomodoro_timer_task(self, total_seconds, lock_window):
        self.stop_timer_flag.clear()
        self.data_queue.put(("log", f"专注开始，总计 {total_seconds} 秒。"))
        seconds_left = total_seconds
        
        while seconds_left >= 0 and not self.stop_timer_flag.is_set():
            hours, minutes, seconds = seconds_left // 3600, (seconds_left % 3600) // 60, seconds_left % 60
            
            time_str = f"{hours:02d}:{minutes:02d}:{seconds:02d}"
            # 只把更新时间的请求放入队列，由主线程处理
            self.data_queue.put(("timer", time_str))
            
            time.sleep(1)
            seconds_left -= 1
        
        if lock_window.winfo_exists():
            lock_window.destroy()
        self.lock_window = None

        if self.stop_timer_flag.is_set():
            self.data_queue.put(("status", "设备已连接 | 待机模式"))
            self.data_queue.put(("log", "专注被手动停止。"))
        else:
            self.data_queue.put(("status", "设备已连接 | 休息一下吧"))
            self.data_queue.put(("log", "专注时间结束！"))
            self.data_queue.put(("timer", "00:00:00"))

if __name__ == '__main__':
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except Exception:
        try:
            ctypes.windll.user32.SetProcessDPIAware()
        except Exception:
            print("警告：无法设置DPI感知。")

    root = tk.Tk()
    app = PomodoroApp(root)
    root.mainloop()