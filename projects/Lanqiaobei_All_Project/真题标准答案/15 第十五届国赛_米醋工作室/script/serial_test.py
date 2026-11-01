import sys
import os
from PyQt6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                            QHBoxLayout, QComboBox, QPushButton, QTextEdit, QLabel, QLineEdit, QGridLayout)
from PyQt6.QtCore import QTimer
from PyQt6.QtSerialPort import QSerialPort, QSerialPortInfo  # 添加 QSerialPort
from PyQt6.QtGui import QTextCursor, QIcon  # 添加 QIcon 导入
import pyqtgraph as pg

class SerialAssistant(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("串口助手")
        self.resize(1000, 700)  # 增加窗口默认大小
        
        # 获取正确的路径
        if getattr(sys, 'frozen', False):
            # 如果是打包后的程序，路径是 sys._MEIPASS
            base_path = sys._MEIPASS
        else:
            # 如果是开发时，路径是当前脚本所在的路径
            base_path = os.path.dirname(__file__)

        # 设置图标
        icon_path = os.path.join(base_path, 'lunai.ico')
        if os.path.exists(icon_path):
            self.setWindowIcon(QIcon(icon_path))

        style_file = os.path.join(base_path, 'style.qss')
        # 加载样式表
        try:
            with open(style_file, 'r', encoding='utf-8') as f:
                self.setStyleSheet(f.read())
        except Exception as e:
            print(f"加载样式表失败: {str(e)}")
        
        # 创建主窗口部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QHBoxLayout(central_widget)
        
        # 左侧控制面板
        left_panel = QWidget()
        left_layout = QVBoxLayout(left_panel)
        
        # 数据显示区域
        display_widget = QWidget()
        display_layout = QVBoxLayout(display_widget)
        
        # 接收数据显示
        receive_label = QLabel("接收数据")
        font = receive_label.font()
        font.setBold(True)
        receive_label.setFont(font)
        self.data_display = QTextEdit()
        self.data_display.setReadOnly(True)
        self.data_display.setMinimumHeight(150)
        
        # 发送日志显示
        send_log_label = QLabel("发送日志")
        send_log_label.setFont(font)
        self.send_log_display = QTextEdit()
        self.send_log_display.setReadOnly(True)
        self.send_log_display.setMinimumHeight(100)
        
        # 添加到显示布局
        display_layout.addWidget(receive_label)
        display_layout.addWidget(self.data_display)
        display_layout.addWidget(send_log_label)
        display_layout.addWidget(self.send_log_display)
        
        # 串口设置
        self.port_combo = QComboBox()
        self.refresh_ports()
        
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(['1200', '2400', '4800', '9600', '14400', '19200', 
                                '28800', '38400', '57600', '115200', '230400', 
                                '460800', '921600'])
        self.baud_combo.setCurrentText('115200')  # 设置默认波特率
        
        self.stop_bits_combo = QComboBox()
        self.stop_bits_combo.addItems(['1', '1.5', '2'])
        self.stop_bits_combo.setCurrentText('1')  # 设置默认停止位
        
        self.data_bits_combo = QComboBox()
        self.data_bits_combo.addItems(['5', '6', '7', '8'])
        self.data_bits_combo.setCurrentText('8')
        
        self.parity_combo = QComboBox()
        self.parity_combo.addItems(['NONE', 'ODD', 'EVEN', 'MARK', 'SPACE'])
        self.parity_combo.setCurrentText('NONE')  # 设置默认校验位
        
        # 在数据显示区域之前添加控制按钮和输入框
        control_layout = QHBoxLayout()
        
        # 查询状态按钮
        status_btn = QPushButton("查询状态")
        status_btn.clicked.connect(lambda: self.send_command("?"))
        control_layout.addWidget(status_btn)
        
        # 查询位置按钮
        position_btn = QPushButton("查询位置")
        position_btn.clicked.connect(lambda: self.send_command("#"))
        control_layout.addWidget(position_btn)
        
        # 坐标输入区域
        coord_widget = QWidget()
        coord_layout = QHBoxLayout(coord_widget)
        
        self.x_input = QLineEdit()
        self.x_input.setPlaceholderText("X (0-999)")
        self.y_input = QLineEdit()
        self.y_input.setPlaceholderText("Y (0-999)")
        
        coord_layout.addWidget(QLabel("X:"))
        coord_layout.addWidget(self.x_input)
        coord_layout.addWidget(QLabel("Y:"))
        coord_layout.addWidget(self.y_input)
        
        # 确认按钮
        confirm_btn = QPushButton("确认发送")
        confirm_btn.clicked.connect(self.send_coordinates)
        coord_layout.addWidget(confirm_btn)
        
        # 在坐标输入区域之后，添加状态信息显示区块
        status_group = QWidget()
        status_layout = QGridLayout(status_group)
        
        # 创建状态标签
        self.current_speed = QLabel("速度: --")
        self.current_position = QLabel("当前位置: --")
        self.target_position = QLabel("目标位置: --")
        self.current_status = QLabel("当前状态: --")
        
        # 设置字体加粗
        font = self.current_speed.font()
        font.setBold(True)
        for label in [self.current_speed, self.current_position, 
                     self.target_position, self.current_status]:
            label.setFont(font)
        
        # 添加到网格布局
        status_layout.addWidget(self.current_speed, 0, 0)
        status_layout.addWidget(self.current_position, 0, 1)
        status_layout.addWidget(self.target_position, 1, 0)
        status_layout.addWidget(self.current_status, 1, 1)
        
        # 在数据显示区域之前添加发送消息区域
        send_widget = QWidget()
        send_layout = QHBoxLayout(send_widget)
        
        self.send_input = QLineEdit()
        self.send_input.setPlaceholderText("输入要发送的内容")
        self.send_input.returnPressed.connect(self.send_message)  # 添加回车发送
        send_layout.addWidget(self.send_input)
        
        send_btn = QPushButton("发送")
        send_btn.clicked.connect(self.send_message)
        send_layout.addWidget(send_btn)
        
        # 清空按钮
        clear_btn = QPushButton("清空显示")
        clear_btn.clicked.connect(self.clear_display)
        
        # 添加清除轨迹按钮
        clear_trajectory_btn = QPushButton("清除轨迹")
        clear_trajectory_btn.clicked.connect(self.clear_trajectory)
        
        # 优化左侧面板布局
        left_panel.setMaximumWidth(400)  # 限制左侧面板最大宽度
        
        # 串口设置组
        serial_group = QWidget()
        serial_layout = QGridLayout(serial_group)
        serial_layout.addWidget(QLabel("串口"), 0, 0)
        serial_layout.addWidget(self.port_combo, 0, 1)
        serial_layout.addWidget(QLabel("波特率"), 1, 0)
        serial_layout.addWidget(self.baud_combo, 1, 1)
        serial_layout.addWidget(QLabel("数据位"), 2, 0)
        serial_layout.addWidget(self.data_bits_combo, 2, 1)
        serial_layout.addWidget(QLabel("停止位"), 3, 0)
        serial_layout.addWidget(self.stop_bits_combo, 3, 1)
        serial_layout.addWidget(QLabel("校验位"), 4, 0)
        serial_layout.addWidget(self.parity_combo, 4, 1)
        
        # 创建连接按钮和刷新按钮（在串口设置之后，布局之前）
        self.connect_button = QPushButton("连接")
        self.connect_button.clicked.connect(self.toggle_connection)
        
        refresh_button = QPushButton("刷新串口")
        refresh_button.clicked.connect(self.refresh_ports)
        
        # 连接和刷新按钮放在一行
        conn_layout = QHBoxLayout()
        conn_layout.addWidget(self.connect_button)
        conn_layout.addWidget(refresh_button)
        
        # 修改左侧面板布局顺序
        left_layout.addWidget(serial_group)
        left_layout.addLayout(conn_layout)
        left_layout.addLayout(control_layout)
        left_layout.addWidget(coord_widget)
        left_layout.addWidget(status_group)
        left_layout.addWidget(send_widget)
        left_layout.addWidget(display_widget)
        
        # 将清空按钮和清除轨迹按钮放在一行
        clear_btns_layout = QHBoxLayout()
        clear_btns_layout.addWidget(clear_btn)
        clear_btns_layout.addWidget(clear_trajectory_btn)
        left_layout.addLayout(clear_btns_layout)
        
        # 右侧绘图区域
        self.plot_widget = pg.PlotWidget()
        self.plot_widget.setBackground('w')
        self.plot_widget.showGrid(x=True, y=True)
        self.plot_widget.setLabel('left', 'y')
        self.plot_widget.setLabel('bottom', 'x')
        
        # 添加左右面板到主布局
        layout.addWidget(left_panel, stretch=1)
        layout.addWidget(self.plot_widget, stretch=2)
        
        # 使用 QSerialPort 替代 pyserial
        self.serial_port = QSerialPort()
        self.serial_port.readyRead.connect(self.read_data)  # 数据到达时自动触发
        self.connected = False
        
        # 修改绘图数据存储结构
        self.x_data = []
        self.y_data = []
        self.target_x = None  # 添加目标点坐标存储
        self.target_y = None
        
        # 设置对象名称以便应用样式
        left_panel.setObjectName("left_panel")
        serial_group.setObjectName("serial_group")
        status_group.setObjectName("status_group")
        coord_widget.setObjectName("coord_widget")
        self.current_speed.setObjectName("status_label")
        self.current_position.setObjectName("status_label")
        self.target_position.setObjectName("status_label")
        self.current_status.setObjectName("status_label")
        
    def refresh_ports(self):
        """刷新可用串口列表"""
        self.port_combo.clear()
        ports = [port.portName() for port in QSerialPortInfo.availablePorts()]
        self.port_combo.addItems(ports)
    
    def toggle_connection(self):
        """连接或断开串口"""
        if not self.connected:
            try:
                self.serial_port.setPortName(self.port_combo.currentText())
                self.serial_port.setBaudRate(int(self.baud_combo.currentText()))
                
                # 数据位映射
                data_bits_map = {
                    '5': QSerialPort.DataBits.Data5,
                    '6': QSerialPort.DataBits.Data6,
                    '7': QSerialPort.DataBits.Data7,
                    '8': QSerialPort.DataBits.Data8
                }
                self.serial_port.setDataBits(data_bits_map[self.data_bits_combo.currentText()])
                
                # 停止位映射
                stop_bits_map = {
                    '1': QSerialPort.StopBits.OneStop,
                    '1.5': QSerialPort.StopBits.OneAndHalfStop,
                    '2': QSerialPort.StopBits.TwoStop
                }
                self.serial_port.setStopBits(stop_bits_map[self.stop_bits_combo.currentText()])
                
                # 校验位映射
                parity_map = {
                    'NONE': QSerialPort.Parity.NoParity,
                    'ODD': QSerialPort.Parity.OddParity,
                    'EVEN': QSerialPort.Parity.EvenParity,
                    'MARK': QSerialPort.Parity.MarkParity,
                    'SPACE': QSerialPort.Parity.SpaceParity
                }
                self.serial_port.setParity(parity_map[self.parity_combo.currentText()])
                
                if self.serial_port.open(QSerialPort.OpenModeFlag.ReadWrite):
                    self.connected = True
                    self.connect_button.setText("断开")
                    self.send_log_display.append(f"[系统] 串口已连接: {self.port_combo.currentText()}")
                    self.send_log_display.moveCursor(QTextCursor.MoveOperation.End)
                else:
                    raise Exception("无法打开串口")
                    
            except Exception as e:
                self.send_log_display.append(f"[错误] 连接失败: {str(e)}")
                self.send_log_display.moveCursor(QTextCursor.MoveOperation.End)
        else:
            try:
                self.serial_port.close()
                self.connected = False
                self.connect_button.setText("连接")
                self.update_status(speed="--", position="--", target="--", status="--")
                self.send_log_display.append("[系统] 串口已断开")
                self.send_log_display.moveCursor(QTextCursor.MoveOperation.End)
            except Exception as e:
                self.send_log_display.append(f"[错误] 断开失败: {str(e)}")
                self.send_log_display.moveCursor(QTextCursor.MoveOperation.End)
    
    def read_data(self):
        """读取串口数据"""
        if self.serial_port.isOpen():
            data = self.serial_port.readAll()
            if data:
                try:
                    text = bytes(data).decode().strip()
                    lines = text.split('\n')
                    
                    for line in lines:
                        line = line.strip()
                        if not line:  # 跳过空行
                            continue
                            
                        # 在接收数据前添加标识符
                        self.data_display.append(f"[接收] {line}")
                        self.data_display.moveCursor(QTextCursor.MoveOperation.End)
                        
                        # 解析位置数据 (x,y)
                        if line.startswith('(') and line.endswith(')'):
                            try:
                                pos_data = line.strip('()')
                                x, y = map(float, pos_data.split(','))
                                
                                self.x_data.append(x)
                                self.y_data.append(y)
                                
                                self.update_plot()
                                self.update_status(position=line)
                            except Exception:
                                pass
                                
                        # 解析工作模式
                        elif line in ['Idle', 'Busy', 'Wait']:
                            self.update_status(status=line)
                            
                        # 解析速度数据
                        elif line.startswith('Speed:'):
                            try:
                                speed = float(line.split(':')[1])
                                self.update_status(speed=f"{speed:.1f}")
                            except Exception:
                                pass
                                
                except Exception as e:
                    self.data_display.append(f"[错误] 数据解析错误: {str(e)}")
                    self.data_display.moveCursor(QTextCursor.MoveOperation.End)
    
    def update_plot(self):
        """更新绘图"""
        self.plot_widget.clear()
        
        # 绘制运动轨迹，使用更细的线条
        if len(self.x_data) > 1:
            self.plot_widget.plot(self.x_data, self.y_data, pen=pg.mkPen('b', width=1))
        
        # 绘制当前位置点（绿色）
        if self.x_data and self.y_data:
            self.plot_widget.plot([self.x_data[-1]], [self.y_data[-1]], 
                                pen=None, symbol='o', symbolSize=8, 
                                symbolBrush='g', symbolPen=None)
        
        # 绘制目标点（红色）
        if self.target_x is not None and self.target_y is not None:
            self.plot_widget.plot([self.target_x], [self.target_y], 
                                pen=None, symbol='o', symbolSize=8, 
                                symbolBrush='r', symbolPen=None)

    def send_command(self, cmd):
        """发送命令到串口"""
        if self.serial_port.isOpen():
            try:
                self.serial_port.write(cmd.encode())
                self.send_log_display.append(f"[发送] {cmd}")
                self.send_log_display.moveCursor(QTextCursor.MoveOperation.End)
            except Exception as e:
                self.send_log_display.append(f"[错误] 发送失败: {str(e)}")
                self.send_log_display.moveCursor(QTextCursor.MoveOperation.End)
        else:
            self.send_log_display.append("[错误] 串口未连接")
            self.send_log_display.moveCursor(QTextCursor.MoveOperation.End)

    def send_coordinates(self):
        """发送坐标数据"""
        try:
            x = int(self.x_input.text())
            y = int(self.y_input.text())
            
            if not (0 <= x <= 999 and 0 <= y <= 999):
                self.send_log_display.append("坐标范围必须在0-999之间")
                return
                
            cmd = f"({x},{y})"
            self.send_command(cmd)
            self.update_status(target=cmd)
            
            # 更新目标点坐标
            self.target_x = x
            self.target_y = y
            self.update_plot()
            
        except ValueError:
            self.send_log_display.append("请输入有效的数字")

    def send_message(self):
        """发送输入框中的消息"""
        message = self.send_input.text()
        if message:
            self.send_command(message)
            self.send_input.clear()
    
    def clear_display(self):
        """清空显示区域和轨迹"""
        self.data_display.clear()
        self.send_log_display.clear()
        # 清除轨迹数据
        self.x_data = []
        self.y_data = []
        self.update_plot()

    def clear_trajectory(self):
        """只清除轨迹数据"""
        self.x_data = []
        self.y_data = []
        self.update_plot()

    def update_status(self, speed=None, position=None, target=None, status=None):
        """更新状态信息"""
        if speed is not None:
            self.current_speed.setText(f"速度: {speed}")
        if position is not None:
            self.current_position.setText(f"当前位置: {position}")
        if target is not None:
            self.target_position.setText(f"目标位置: {target}")
        if status is not None:
            self.current_status.setText(f"当前状态: {status}")

    def closeEvent(self, event):
        """窗口关闭事件"""
        if self.serial_port.isOpen():
            self.serial_port.close()
        event.accept()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    
    # 设置应用程序图标
    if getattr(sys, 'frozen', False):
        base_path = sys._MEIPASS
    else:
        base_path = os.path.dirname(__file__)
    
    icon_path = os.path.join(base_path, 'lunai.ico')
    if os.path.exists(icon_path):
        app.setWindowIcon(QIcon(icon_path))
    
    window = SerialAssistant()
    window.show()
    sys.exit(app.exec())
