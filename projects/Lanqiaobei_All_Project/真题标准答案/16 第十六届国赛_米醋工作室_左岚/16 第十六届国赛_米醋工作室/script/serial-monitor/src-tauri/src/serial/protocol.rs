// 串口协议构造器
use chrono::Local;
use serde::{Deserialize, Serialize};

/// 系统参数
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SystemParam {
    pub id: String,
    pub value: String,
}

/// 构造参数配置命令
/// 格式: (H1,2.0)(H2,0.3)(F,2000)
pub fn build_param_command(params: &[SystemParam]) -> String {
    params
        .iter()
        .map(|p| format!("({},{})", p.id, p.value))
        .collect::<Vec<String>>()
        .join("")
}

/// 构造单个参数查询命令
/// 格式: (F,?)
pub fn build_query_command(param_id: &str) -> String {
    format!("({},?)", param_id)
}

/// 构造时间同步命令
/// 格式: (T:HHMMSS)
pub fn build_time_sync_command() -> String {
    let now = Local::now();
    format!("(T:{})", now.format("%H%M%S"))
}

/// 构造批量参数查询命令
/// 格式: (F,?)(H1,?)(H2,?)
pub fn build_batch_query_command(param_ids: &[&str]) -> String {
    param_ids
        .iter()
        .map(|id| format!("({},?)", id))
        .collect::<Vec<String>>()
        .join("")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_build_param_command() {
        let params = vec![
            SystemParam {
                id: "H1".to_string(),
                value: "2.0".to_string(),
            },
            SystemParam {
                id: "H2".to_string(),
                value: "0.3".to_string(),
            },
        ];
        let result = build_param_command(&params);
        assert_eq!(result, "(H1,2.0)(H2,0.3)");
    }

    #[test]
    fn test_build_query_command() {
        let result = build_query_command("F");
        assert_eq!(result, "(F,?)");
    }

    #[test]
    fn test_build_batch_query_command() {
        let result = build_batch_query_command(&["F", "H1", "H2"]);
        assert_eq!(result, "(F,?)(H1,?)(H2,?)");
    }
}
