// 串口协议解析器
use serde::{Deserialize, Serialize};

/// 参数配置响应类型
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", content = "message")]
pub enum ParamResponse {
    Ok,
    Error(String),
    Warn(String),
}

/// 参数查询响应
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct QueryResponse {
    pub param_id: String,
    pub value: String,
}

/// 异常上报数据
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AlarmData {
    pub error_code: u16,
    pub timestamp: String, // 格式: HHMMSS
    pub value: f32,
}

/// 解析参数配置响应
/// 格式: OK, ERROR, WARN
pub fn parse_param_response(data: &str) -> Result<ParamResponse, String> {
    let trimmed = data.trim();

    if trimmed == "OK" {
        Ok(ParamResponse::Ok)
    } else if trimmed.starts_with("ERROR") {
        let msg = trimmed.strip_prefix("ERROR").unwrap_or("").trim();
        Ok(ParamResponse::Error(msg.to_string()))
    } else if trimmed.starts_with("WARN") {
        let msg = trimmed.strip_prefix("WARN").unwrap_or("").trim();
        Ok(ParamResponse::Warn(msg.to_string()))
    } else {
        Err(format!("未知的参数响应格式: {}", trimmed))
    }
}

/// 解析参数查询响应
/// 格式: (F,1000) 或 (H1,2.5)
pub fn parse_query_response(data: &str) -> Result<QueryResponse, String> {
    let trimmed = data.trim();

    // 检查格式: (参数ID,值)
    if !trimmed.starts_with('(') || !trimmed.ends_with(')') {
        return Err(format!("查询响应格式错误: {}", trimmed));
    }

    // 去掉括号
    let content = &trimmed[1..trimmed.len() - 1];

    // 按逗号分割
    let parts: Vec<&str> = content.split(',').collect();
    if parts.len() != 2 {
        return Err(format!("查询响应字段数量错误: {}", trimmed));
    }

    Ok(QueryResponse {
        param_id: parts[0].trim().to_string(),
        value: parts[1].trim().to_string(),
    })
}

/// 解析异常上报数据
/// 格式: [500,080002,2.13]
pub fn parse_alarm(data: &str) -> Result<AlarmData, String> {
    let trimmed = data.trim();

    // 检查格式: [错误码,时间戳,数值]
    if !trimmed.starts_with('[') || !trimmed.ends_with(']') {
        return Err(format!("告警格式错误: {}", trimmed));
    }

    // 去掉方括号
    let content = &trimmed[1..trimmed.len() - 1];

    // 按逗号分割
    let parts: Vec<&str> = content.split(',').collect();
    if parts.len() != 3 {
        return Err(format!("告警字段数量错误: {}", trimmed));
    }

    // 解析错误码
    let error_code = parts[0]
        .trim()
        .parse::<u16>()
        .map_err(|e| format!("解析错误码失败: {}", e))?;

    // 解析时间戳
    let timestamp = parts[1].trim().to_string();

    // 解析数值
    let value = parts[2]
        .trim()
        .parse::<f32>()
        .map_err(|e| format!("解析告警数值失败: {}", e))?;

    Ok(AlarmData {
        error_code,
        timestamp,
        value,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_param_response() {
        assert!(matches!(
            parse_param_response("OK").unwrap(),
            ParamResponse::Ok
        ));
        assert!(matches!(
            parse_param_response("ERROR Invalid parameter").unwrap(),
            ParamResponse::Error(_)
        ));
    }

    #[test]
    fn test_parse_query_response() {
        let result = parse_query_response("(F,1000)").unwrap();
        assert_eq!(result.param_id, "F");
        assert_eq!(result.value, "1000");
    }

    #[test]
    fn test_parse_alarm() {
        let result = parse_alarm("[500,080002,2.13]").unwrap();
        assert_eq!(result.error_code, 500);
        assert_eq!(result.timestamp, "080002");
        assert_eq!(result.value, 2.13);
    }
}
