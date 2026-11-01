// 数据库模块
use rusqlite::{Connection, Result};
use serde::{Deserialize, Serialize};
use std::path::PathBuf;
use tauri::AppHandle;

/// 传感器数据记录
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SensorRecord {
    pub id: Option<i64>,
    pub timestamp: String,
    pub liquid_height: Option<f64>,
    pub temperature: Option<i32>,
    pub weight: Option<f64>,
    pub frequency: Option<i32>,
    pub volume: Option<f64>,
    pub remaining_space: Option<f64>,
}

/// 告警日志记录
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AlarmRecord {
    pub id: Option<i64>,
    pub timestamp: String,
    pub error_code: u16,
    pub error_value: f32,
    pub description: String,
}

/// 初始化数据库
pub fn init_database(app_handle: &AppHandle) -> Result<()> {
    let db_path = get_database_path(app_handle)?;
    let conn = Connection::open(&db_path)?;

    // 创建传感器数据表
    conn.execute(
        "CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            liquid_height REAL,
            temperature INTEGER,
            weight REAL,
            frequency INTEGER,
            volume REAL,
            remaining_space REAL
        )",
        [],
    )?;

    // 创建告警日志表
    conn.execute(
        "CREATE TABLE IF NOT EXISTS alarm_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            error_code INTEGER NOT NULL,
            error_value REAL NOT NULL,
            description TEXT NOT NULL
        )",
        [],
    )?;

    // 创建索引
    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_sensor_timestamp ON sensor_data(timestamp)",
        [],
    )?;

    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_alarm_timestamp ON alarm_logs(timestamp)",
        [],
    )?;

    Ok(())
}

/// 获取数据库路径
fn get_database_path(app_handle: &AppHandle) -> Result<PathBuf> {
    let app_dir = app_handle
        .path_resolver()
        .app_data_dir()
        .ok_or_else(|| rusqlite::Error::InvalidPath("无法获取应用数据目录".into()))?;

    // 确保目录存在
    std::fs::create_dir_all(&app_dir)
        .map_err(|e| rusqlite::Error::InvalidPath(format!("创建目录失败: {}", e).into()))?;

    Ok(app_dir.join("serial_monitor.db"))
}

/// 获取数据库连接
pub fn get_connection(app_handle: &AppHandle) -> Result<Connection> {
    let db_path = get_database_path(app_handle)?;
    Connection::open(db_path)
}

/// 插入传感器数据
pub fn insert_sensor_data(
    conn: &Connection,
    record: &SensorRecord,
) -> Result<i64> {
    conn.execute(
        "INSERT INTO sensor_data
         (timestamp, liquid_height, temperature, weight, frequency, volume, remaining_space)
         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)",
        rusqlite::params![
            record.timestamp,
            record.liquid_height,
            record.temperature,
            record.weight,
            record.frequency,
            record.volume,
            record.remaining_space,
        ],
    )?;

    Ok(conn.last_insert_rowid())
}

/// 插入告警日志
pub fn insert_alarm_log(
    conn: &Connection,
    record: &AlarmRecord,
) -> Result<i64> {
    conn.execute(
        "INSERT INTO alarm_logs (timestamp, error_code, error_value, description)
         VALUES (?1, ?2, ?3, ?4)",
        rusqlite::params![
            record.timestamp,
            record.error_code,
            record.error_value,
            record.description,
        ],
    )?;

    Ok(conn.last_insert_rowid())
}

/// 查询传感器数据(按时间范围)
pub fn query_sensor_data(
    conn: &Connection,
    start_time: &str,
    end_time: &str,
    limit: Option<usize>,
) -> Result<Vec<SensorRecord>> {
    let sql = if let Some(limit_val) = limit {
        format!(
            "SELECT * FROM sensor_data
             WHERE timestamp BETWEEN ?1 AND ?2
             ORDER BY timestamp DESC
             LIMIT {}",
            limit_val
        )
    } else {
        "SELECT * FROM sensor_data
         WHERE timestamp BETWEEN ?1 AND ?2
         ORDER BY timestamp DESC"
            .to_string()
    };

    let mut stmt = conn.prepare(&sql)?;
    let records = stmt
        .query_map([start_time, end_time], |row| {
            Ok(SensorRecord {
                id: Some(row.get(0)?),
                timestamp: row.get(1)?,
                liquid_height: row.get(2)?,
                temperature: row.get(3)?,
                weight: row.get(4)?,
                frequency: row.get(5)?,
                volume: row.get(6)?,
                remaining_space: row.get(7)?,
            })
        })?
        .collect::<Result<Vec<_>>>()?;

    Ok(records)
}

/// 查询告警日志(按时间范围)
pub fn query_alarm_logs(
    conn: &Connection,
    start_time: &str,
    end_time: &str,
    limit: Option<usize>,
) -> Result<Vec<AlarmRecord>> {
    let sql = if let Some(limit_val) = limit {
        format!(
            "SELECT * FROM alarm_logs
             WHERE timestamp BETWEEN ?1 AND ?2
             ORDER BY timestamp DESC
             LIMIT {}",
            limit_val
        )
    } else {
        "SELECT * FROM alarm_logs
         WHERE timestamp BETWEEN ?1 AND ?2
         ORDER BY timestamp DESC"
            .to_string()
    };

    let mut stmt = conn.prepare(&sql)?;
    let records = stmt
        .query_map([start_time, end_time], |row| {
            Ok(AlarmRecord {
                id: Some(row.get(0)?),
                timestamp: row.get(1)?,
                error_code: row.get(2)?,
                error_value: row.get(3)?,
                description: row.get(4)?,
            })
        })?
        .collect::<Result<Vec<_>>>()?;

    Ok(records)
}
