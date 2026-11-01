// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod commands;
mod database;
mod serial;

use commands::{ReaderState, SerialState};
use tauri::Manager;

fn main() {
    tauri::Builder::default()
        // 初始化串口管理器状态
        .manage(SerialState::new())
        // 初始化读取器状态
        .manage(ReaderState::new())
        // 注册所有命令
        .invoke_handler(tauri::generate_handler![
            commands::list_ports,
            commands::connect_serial,
            commands::disconnect_serial,
            commands::is_serial_connected,
            commands::send_param_config,
            commands::send_query_command,
            commands::send_time_sync,
            commands::send_raw_command,
            commands::start_serial_reader,
            commands::stop_serial_reader,
            commands::is_reader_running,
        ])
        // 初始化数据库
        .setup(|app| {
            if let Err(e) = database::init_database(&app.handle()) {
                eprintln!("数据库初始化失败: {}", e);
            }
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
