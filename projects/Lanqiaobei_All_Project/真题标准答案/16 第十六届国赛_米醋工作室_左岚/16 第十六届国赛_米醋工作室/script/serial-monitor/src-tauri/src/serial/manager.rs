// 串口管理器
use serialport::{SerialPort, SerialPortInfo};
use std::io::{Read, Write};
use std::sync::{Arc, Mutex};
use std::time::Duration;

/// 串口连接配置
#[derive(Debug, Clone)]
pub struct SerialConfig {
    pub port_name: String,
    pub baud_rate: u32,
}

impl Default for SerialConfig {
    fn default() -> Self {
        Self {
            port_name: String::new(),
            baud_rate: 9600,
        }
    }
}

/// 串口管理器
pub struct SerialManager {
    port: Option<Arc<Mutex<Box<dyn SerialPort>>>>,
    config: SerialConfig,
}

impl SerialManager {
    /// 创建新的串口管理器
    pub fn new() -> Self {
        Self {
            port: None,
            config: SerialConfig::default(),
        }
    }

    /// 列出可用串口
    pub fn list_ports() -> Result<Vec<String>, String> {
        match serialport::available_ports() {
            Ok(ports) => {
                let port_names: Vec<String> = ports.iter().map(|p| p.port_name.clone()).collect();
                Ok(port_names)
            }
            Err(e) => Err(format!("获取串口列表失败: {}", e)),
        }
    }

    /// 连接串口
    pub fn connect(&mut self, port_name: String, baud_rate: u32) -> Result<(), String> {
        // 如果已连接,先断开
        if self.port.is_some() {
            self.disconnect()?;
        }

        // 打开串口
        let port = serialport::new(&port_name, baud_rate)
            .timeout(Duration::from_millis(1000))
            .data_bits(serialport::DataBits::Eight)
            .parity(serialport::Parity::None)
            .stop_bits(serialport::StopBits::One)
            .flow_control(serialport::FlowControl::None)
            .open()
            .map_err(|e| format!("打开串口失败: {}", e))?;

        self.port = Some(Arc::new(Mutex::new(port)));
        self.config = SerialConfig {
            port_name,
            baud_rate,
        };

        Ok(())
    }

    /// 断开串口连接
    pub fn disconnect(&mut self) -> Result<(), String> {
        if self.port.is_some() {
            self.port = None;
            Ok(())
        } else {
            Err("串口未连接".to_string())
        }
    }

    /// 检查是否已连接
    pub fn is_connected(&self) -> bool {
        self.port.is_some()
    }

    /// 发送数据
    pub fn send(&mut self, data: &[u8]) -> Result<usize, String> {
        if let Some(port) = &self.port {
            let mut port = port.lock().map_err(|e| format!("锁定串口失败: {}", e))?;
            port.write(data)
                .map_err(|e| format!("发送数据失败: {}", e))
        } else {
            Err("串口未连接".to_string())
        }
    }

    /// 读取数据
    pub fn read(&mut self, buffer: &mut [u8]) -> Result<usize, String> {
        if let Some(port) = &self.port {
            let mut port = port.lock().map_err(|e| format!("锁定串口失败: {}", e))?;
            port.read(buffer)
                .map_err(|e| format!("读取数据失败: {}", e))
        } else {
            Err("串口未连接".to_string())
        }
    }

    /// 获取串口配置
    pub fn get_config(&self) -> &SerialConfig {
        &self.config
    }

    /// 获取串口句柄的克隆(用于异步读取线程)
    pub fn get_port_clone(&self) -> Option<Arc<Mutex<Box<dyn SerialPort>>>> {
        self.port.clone()
    }
}

impl Default for SerialManager {
    fn default() -> Self {
        Self::new()
    }
}
