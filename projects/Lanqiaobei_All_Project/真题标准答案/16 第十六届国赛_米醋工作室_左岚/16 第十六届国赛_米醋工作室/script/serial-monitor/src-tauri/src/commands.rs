// Tauri命令定义
use crate::serial::{
    build_param_command, build_query_command, build_time_sync_command, parse_alarm,
    parse_param_response, parse_query_response, AlarmData, ParamResponse, QueryResponse,
    SerialManager, SystemParam,
};
use std::sync::{Arc, Mutex};
use tauri::State;

/// 全局串口管理器状态
pub struct SerialState {
    pub manager: Arc<Mutex<SerialManager>>,
}

impl SerialState {
    pub fn new() -> Self {
        Self {
            manager: Arc::new(Mutex::new(SerialManager::new())),
        }
    }
}

/// 列出可用串口
#[tauri::command]
pub fn list_ports() -> Result<Vec<String>, String> {
    SerialManager::list_ports()
}

/// 连接串口
#[tauri::command]
pub fn connect_serial(
    state: State<SerialState>,
    port_name: String,
    baud_rate: u32,
) -> Result<(), String> {
    let mut manager = state
        .manager
        .lock()
        .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

    manager.connect(port_name, baud_rate)
}

/// 断开串口连接
#[tauri::command]
pub fn disconnect_serial(state: State<SerialState>) -> Result<(), String> {
    let mut manager = state
        .manager
        .lock()
        .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

    manager.disconnect()
}

/// 检查串口连接状态
#[tauri::command]
pub fn is_serial_connected(state: State<SerialState>) -> Result<bool, String> {
    let manager = state
        .manager
        .lock()
        .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

    Ok(manager.is_connected())
}

/// 发送参数配置命令
#[tauri::command]
pub fn send_param_config(
    state: State<SerialState>,
    params: Vec<SystemParam>,
) -> Result<String, String> {
    let mut manager = state
        .manager
        .lock()
        .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

    // 构造命令
    let command = build_param_command(&params);

    // 发送命令
    let bytes = command.as_bytes();
    manager.send(bytes)?;

    Ok(command)
}

/// 发送参数查询命令
#[tauri::command]
pub fn send_query_command(state: State<SerialState>, param_id: String) -> Result<String, String> {
    let mut manager = state
        .manager
        .lock()
        .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

    // 构造查询命令
    let command = build_query_command(&param_id);

    // 发送命令
    let bytes = command.as_bytes();
    manager.send(bytes)?;

    Ok(command)
}

/// 发送时间同步命令
#[tauri::command]
pub fn send_time_sync(state: State<SerialState>) -> Result<String, String> {
    let mut manager = state
        .manager
        .lock()
        .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

    // 构造时间同步命令
    let command = build_time_sync_command();

    // 发送命令
    let bytes = command.as_bytes();
    manager.send(bytes)?;

    Ok(command)
}

/// 发送原始命令
#[tauri::command]
pub fn send_raw_command(state: State<SerialState>, command: String) -> Result<usize, String> {
    let mut manager = state
        .manager
        .lock()
        .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

    let bytes = command.as_bytes();
    manager.send(bytes)
}

// ==================== 异步串口读取器 ====================

use std::io::Read;
use std::sync::atomic::{AtomicBool, Ordering};
use tauri::{AppHandle, Manager as TauriManager};
use tokio::task;

/// 全局读取器运行状态
pub struct ReaderState {
    pub running: Arc<AtomicBool>,
}

impl ReaderState {
    pub fn new() -> Self {
        Self {
            running: Arc::new(AtomicBool::new(false)),
        }
    }
}

/// 启动异步串口读取器
#[tauri::command]
pub async fn start_serial_reader(
    app_handle: AppHandle,
    serial_state: State<'_, SerialState>,
    reader_state: State<'_, ReaderState>,
) -> Result<(), String> {
    // 检查是否已在运行
    if reader_state.running.load(Ordering::SeqCst) {
        return Err("串口读取器已在运行".to_string());
    }

    // 获取串口句柄的克隆
    let port_clone = {
        let manager = serial_state
            .manager
            .lock()
            .map_err(|e| format!("锁定串口管理器失败: {}", e))?;

        manager
            .get_port_clone()
            .ok_or_else(|| "串口未连接".to_string())?
    };

    // 设置运行状态
    reader_state.running.store(true, Ordering::SeqCst);
    let running = reader_state.running.clone();

    // 在tokio异步运行时中启动读取任务
    task::spawn_blocking(move || {
        let mut buffer = vec![0u8; 1024];
        let mut line_buffer = String::new();

        while running.load(Ordering::SeqCst) {
            // 获取串口锁并读取数据
            let read_result = {
                let mut port = match port_clone.lock() {
                    Ok(p) => p,
                    Err(e) => {
                        eprintln!("锁定串口失败: {}", e);
                        break;
                    }
                };

                port.read(&mut buffer)
            };

            match read_result {
                Ok(n) if n > 0 => {
                    // 将读取的字节转换为字符串
                    if let Ok(data_str) = String::from_utf8(buffer[..n].to_vec()) {
                        line_buffer.push_str(&data_str);

                        // 按换行符分割处理完整的行
                        while let Some(pos) = line_buffer.find('\n') {
                            let line = line_buffer[..pos].trim().to_string();
                            line_buffer.drain(..=pos);

                            if !line.is_empty() {
                                // 解析并推送数据
                                process_serial_data(&app_handle, &line);
                            }
                        }
                    }
                }
                Ok(_) => {
                    // 没有数据,短暂休眠避免CPU占用过高
                    std::thread::sleep(std::time::Duration::from_millis(10));
                }
                Err(e) => {
                    eprintln!("读取串口数据失败: {}", e);
                    // 发送错误事件
                    let _ = app_handle.emit_all(
                        "serial:error",
                        format!("读取串口数据失败: {}", e),
                    );
                    break;
                }
            }
        }

        // 读取器停止
        running.store(false, Ordering::SeqCst);
    });

    Ok(())
}

/// 停止异步串口读取器
#[tauri::command]
pub fn stop_serial_reader(reader_state: State<ReaderState>) -> Result<(), String> {
    if !reader_state.running.load(Ordering::SeqCst) {
        return Err("串口读取器未运行".to_string());
    }

    reader_state.running.store(false, Ordering::SeqCst);
    Ok(())
}

/// 检查读取器是否正在运行
#[tauri::command]
pub fn is_reader_running(reader_state: State<ReaderState>) -> bool {
    reader_state.running.load(Ordering::SeqCst)
}

/// 处理串口接收到的数据
fn process_serial_data(app_handle: &AppHandle, data: &str) {
    // 尝试解析为不同类型的数据

    // 1. 检查是否为告警数据: [500,080002,2.13]
    if data.starts_with('[') && data.ends_with(']') {
        if let Ok(alarm) = parse_alarm(data) {
            let _ = app_handle.emit_all("serial:alarm", alarm);
            return;
        }
    }

    // 2. 检查是否为参数查询响应: (F,1000)
    if data.starts_with('(') && data.ends_with(')') && data.contains(',') {
        if let Ok(query_resp) = parse_query_response(data) {
            let _ = app_handle.emit_all("serial:query_response", query_resp);
            return;
        }
    }

    // 3. 检查是否为参数配置响应: OK, ERROR, WARN
    if data == "OK" || data.starts_with("ERROR") || data.starts_with("WARN") {
        if let Ok(param_resp) = parse_param_response(data) {
            let _ = app_handle.emit_all("serial:param_response", param_resp);
            return;
        }
    }

    // 4. 未识别的数据,作为原始数据推送
    let _ = app_handle.emit_all("serial:raw_data", data);
}
