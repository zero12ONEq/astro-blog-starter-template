// 串口通信模块
pub mod manager;
pub mod parser;
pub mod protocol;

// 导出公共类型和函数
pub use manager::SerialManager;
pub use parser::{parse_alarm, parse_param_response, parse_query_response, AlarmData, ParamResponse, QueryResponse};
pub use protocol::{build_param_command, build_query_command, build_time_sync_command, SystemParam};
