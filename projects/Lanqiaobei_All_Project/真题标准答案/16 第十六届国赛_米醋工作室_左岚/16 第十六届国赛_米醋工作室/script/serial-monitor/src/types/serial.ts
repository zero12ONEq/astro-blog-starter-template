// 串口通信相关类型定义

/** 系统参数 */
export interface SystemParam {
  id: string;
  value: string;
}

/** 参数配置响应类型 */
export type ParamResponse =
  | { type: "Ok" }
  | { type: "Error"; message: string }
  | { type: "Warn"; message: string };

/** 参数查询响应 */
export interface QueryResponse {
  param_id: string;
  value: string;
}

/** 异常上报数据 */
export interface AlarmData {
  error_code: number;
  timestamp: string; // 格式: HHMMSS
  value: number;
}

/** 告警类型枚举 */
export enum AlarmType {
  LIQUID_OVERFLOW = 500, // 液位超上限
  LIQUID_LOW = 501, // 液位低于下限
  FREQ_ABNORMAL = 502, // 频率数值异常
  FREQ_TREND = 503, // 频率趋势异常
  FREQ_VIOLENT = 504, // 频率剧烈变化
}

/** 告警类型描述映射 */
export const AlarmTypeDesc: Record<AlarmType, string> = {
  [AlarmType.LIQUID_OVERFLOW]: "液位超上限",
  [AlarmType.LIQUID_LOW]: "液位低于下限",
  [AlarmType.FREQ_ABNORMAL]: "频率数值异常",
  [AlarmType.FREQ_TREND]: "频率趋势异常",
  [AlarmType.FREQ_VIOLENT]: "频率剧烈变化",
};

/** 告警严重级别 */
export enum AlarmSeverity {
  INFO = "info",
  WARNING = "warning",
  ERROR = "error",
  CRITICAL = "critical",
}

/** 告警严重级别映射 */
export const AlarmSeverityMap: Record<AlarmType, AlarmSeverity> = {
  [AlarmType.LIQUID_OVERFLOW]: AlarmSeverity.CRITICAL,
  [AlarmType.LIQUID_LOW]: AlarmSeverity.CRITICAL,
  [AlarmType.FREQ_ABNORMAL]: AlarmSeverity.WARNING,
  [AlarmType.FREQ_TREND]: AlarmSeverity.INFO,
  [AlarmType.FREQ_VIOLENT]: AlarmSeverity.ERROR,
};

/** 串口连接状态 */
export enum SerialConnectionStatus {
  DISCONNECTED = "disconnected",
  CONNECTING = "connecting",
  CONNECTED = "connected",
  ERROR = "error",
}

/** 串口配置 */
export interface SerialConfig {
  port_name: string;
  baud_rate: number;
}

/** 默认波特率选项 */
export const BAUD_RATES = [9600, 19200, 38400, 57600, 115200] as const;

/** 系统参数ID枚举 */
export enum ParamId {
  H1 = "H1", // 液位上限 (m)
  H2 = "H2", // 液位下限 (m)
  F = "F", // 频率阈值 (Hz)
  S = "S", // 容器类型 (0-球体/1-圆柱/2-长方体)
  R = "r", // 半径 (m, 适用于球体和圆柱)
  L = "L", // 长度 (m, 适用于长方体)
  W = "W", // 宽度 (m, 适用于长方体)
  H = "H", // 高度 (m, 适用于圆柱和长方体)
}

/** 容器类型枚举 */
export enum ContainerType {
  SPHERE = 0, // 球体
  CYLINDER = 1, // 圆柱体
  CUBOID = 2, // 长方体
}

/** 容器类型描述映射 */
export const ContainerTypeDesc: Record<ContainerType, string> = {
  [ContainerType.SPHERE]: "球体",
  [ContainerType.CYLINDER]: "圆柱体",
  [ContainerType.CUBOID]: "长方体",
};
