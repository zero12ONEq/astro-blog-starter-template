// Tauri后端API调用封装
import { invoke } from "@tauri-apps/api/tauri";
import { listen, UnlistenFn } from "@tauri-apps/api/event";
import type {
  SystemParam,
  ParamResponse,
  QueryResponse,
  AlarmData,
} from "../types/serial";

// ==================== 串口基础操作 ====================

/**
 * 列出可用串口
 */
export async function listPorts(): Promise<string[]> {
  return await invoke<string[]>("list_ports");
}

/**
 * 连接串口
 * @param portName 串口名称 (如 "COM3" 或 "/dev/ttyUSB0")
 * @param baudRate 波特率 (默认 9600)
 */
export async function connectSerial(
  portName: string,
  baudRate: number = 9600
): Promise<void> {
  await invoke("connect_serial", { portName, baudRate });
}

/**
 * 断开串口连接
 */
export async function disconnectSerial(): Promise<void> {
  await invoke("disconnect_serial");
}

/**
 * 检查串口是否已连接
 */
export async function isSerialConnected(): Promise<boolean> {
  return await invoke<boolean>("is_serial_connected");
}

// ==================== 串口数据发送 ====================

/**
 * 发送参数配置命令
 * @param params 参数数组
 * @returns 发送的命令字符串
 */
export async function sendParamConfig(
  params: SystemParam[]
): Promise<string> {
  return await invoke<string>("send_param_config", { params });
}

/**
 * 发送参数查询命令
 * @param paramId 参数ID (如 "F", "H1")
 * @returns 发送的命令字符串
 */
export async function sendQueryCommand(paramId: string): Promise<string> {
  return await invoke<string>("send_query_command", { paramId });
}

/**
 * 发送时间同步命令
 * @returns 发送的命令字符串
 */
export async function sendTimeSync(): Promise<string> {
  return await invoke<string>("send_time_sync");
}

/**
 * 发送原始命令
 * @param command 命令字符串
 * @returns 发送的字节数
 */
export async function sendRawCommand(command: string): Promise<number> {
  return await invoke<number>("send_raw_command", { command });
}

// ==================== 异步串口读取器 ====================

/**
 * 启动异步串口读取器
 */
export async function startSerialReader(): Promise<void> {
  await invoke("start_serial_reader");
}

/**
 * 停止异步串口读取器
 */
export async function stopSerialReader(): Promise<void> {
  await invoke("stop_serial_reader");
}

/**
 * 检查读取器是否正在运行
 */
export async function isReaderRunning(): Promise<boolean> {
  return await invoke<boolean>("is_reader_running");
}

// ==================== 事件监听 ====================

/**
 * 监听告警事件
 * @param callback 回调函数
 * @returns 取消监听函数
 */
export async function onAlarm(
  callback: (alarm: AlarmData) => void
): Promise<UnlistenFn> {
  return await listen<AlarmData>("serial:alarm", (event) => {
    callback(event.payload);
  });
}

/**
 * 监听参数查询响应事件
 * @param callback 回调函数
 * @returns 取消监听函数
 */
export async function onQueryResponse(
  callback: (response: QueryResponse) => void
): Promise<UnlistenFn> {
  return await listen<QueryResponse>("serial:query_response", (event) => {
    callback(event.payload);
  });
}

/**
 * 监听参数配置响应事件
 * @param callback 回调函数
 * @returns 取消监听函数
 */
export async function onParamResponse(
  callback: (response: ParamResponse) => void
): Promise<UnlistenFn> {
  return await listen<ParamResponse>("serial:param_response", (event) => {
    callback(event.payload);
  });
}

/**
 * 监听原始数据事件
 * @param callback 回调函数
 * @returns 取消监听函数
 */
export async function onRawData(
  callback: (data: string) => void
): Promise<UnlistenFn> {
  return await listen<string>("serial:raw_data", (event) => {
    callback(event.payload);
  });
}

/**
 * 监听串口错误事件
 * @param callback 回调函数
 * @returns 取消监听函数
 */
export async function onSerialError(
  callback: (error: string) => void
): Promise<UnlistenFn> {
  return await listen<string>("serial:error", (event) => {
    callback(event.payload);
  });
}

// ==================== 工具函数 ====================

/**
 * 完整的连接流程(连接 + 启动读取器)
 * @param portName 串口名称
 * @param baudRate 波特率
 */
export async function connectAndStartReader(
  portName: string,
  baudRate: number = 9600
): Promise<void> {
  await connectSerial(portName, baudRate);
  await startSerialReader();
}

/**
 * 完整的断开流程(停止读取器 + 断开连接)
 */
export async function stopReaderAndDisconnect(): Promise<void> {
  const running = await isReaderRunning();
  if (running) {
    await stopSerialReader();
  }
  await disconnectSerial();
}
