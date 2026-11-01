// 串口配置面板
import React, { useEffect, useState } from "react";
import { Card, Select, Button, Space, Typography, Badge, message } from "antd";
import {
  ApiOutlined,
  DisconnectOutlined,
  ReloadOutlined,
} from "@ant-design/icons";
import { useSerialStore } from "../../stores/serialStore";
import { SerialConnectionStatus, BAUD_RATES } from "../../types/serial";
import {
  listPorts,
  connectAndStartReader,
  stopReaderAndDisconnect,
} from "../../services/tauri-api";

const { Text } = Typography;

export const SerialConfig: React.FC = () => {
  const {
    status,
    portName,
    baudRate,
    availablePorts,
    error,
    setStatus,
    setPortName,
    setBaudRate,
    setAvailablePorts,
    setError,
  } = useSerialStore();

  const [loading, setLoading] = useState(false);

  // 刷新串口列表
  const refreshPorts = async () => {
    try {
      setLoading(true);
      const ports = await listPorts();
      setAvailablePorts(ports);

      if (ports.length === 0) {
        message.warning("未检测到可用串口");
      } else {
        message.success(`检测到 ${ports.length} 个串口`);
        // 自动选择第一个
        if (!portName && ports.length > 0) {
          setPortName(ports[0]);
        }
      }
    } catch (err) {
      const errMsg = err instanceof Error ? err.message : String(err);
      message.error(`刷新串口列表失败: ${errMsg}`);
      setError(errMsg);
    } finally {
      setLoading(false);
    }
  };

  // 连接串口
  const handleConnect = async () => {
    if (!portName) {
      message.warning("请选择串口");
      return;
    }

    try {
      setLoading(true);
      setStatus(SerialConnectionStatus.CONNECTING);
      setError(null);

      await connectAndStartReader(portName, baudRate);

      setStatus(SerialConnectionStatus.CONNECTED);
      message.success(`串口 ${portName} 连接成功`);
    } catch (err) {
      const errMsg = err instanceof Error ? err.message : String(err);
      setStatus(SerialConnectionStatus.ERROR);
      setError(errMsg);
      message.error(`连接失败: ${errMsg}`);
    } finally {
      setLoading(false);
    }
  };

  // 断开串口
  const handleDisconnect = async () => {
    try {
      setLoading(true);
      await stopReaderAndDisconnect();
      setStatus(SerialConnectionStatus.DISCONNECTED);
      message.info("串口已断开");
    } catch (err) {
      const errMsg = err instanceof Error ? err.message : String(err);
      message.error(`断开失败: ${errMsg}`);
    } finally {
      setLoading(false);
    }
  };

  // 组件挂载时刷新串口列表
  useEffect(() => {
    refreshPorts();
  }, []);

  const isConnected = status === SerialConnectionStatus.CONNECTED;
  const isConnecting = status === SerialConnectionStatus.CONNECTING;

  return (
    <Card
      title="串口配置"
      extra={
        <Badge
          status={
            isConnected ? "success" : status === SerialConnectionStatus.ERROR ? "error" : "default"
          }
          text={
            isConnected
              ? "已连接"
              : isConnecting
              ? "连接中..."
              : status === SerialConnectionStatus.ERROR
              ? "连接错误"
              : "未连接"
          }
        />
      }
    >
      <Space direction="vertical" style={{ width: "100%" }} size="middle">
        {/* 串口选择 */}
        <Space.Compact style={{ width: "100%" }}>
          <Select
            style={{ flex: 1 }}
            placeholder="选择串口"
            value={portName || undefined}
            onChange={setPortName}
            disabled={isConnected || loading}
            options={availablePorts.map((port) => ({
              label: port,
              value: port,
            }))}
          />
          <Button
            icon={<ReloadOutlined />}
            onClick={refreshPorts}
            loading={loading}
            disabled={isConnected}
          >
            刷新
          </Button>
        </Space.Compact>

        {/* 波特率选择 */}
        <Select
          style={{ width: "100%" }}
          placeholder="波特率"
          value={baudRate}
          onChange={setBaudRate}
          disabled={isConnected || loading}
          options={BAUD_RATES.map((rate) => ({
            label: `${rate} bps`,
            value: rate,
          }))}
        />

        {/* 连接/断开按钮 */}
        {isConnected ? (
          <Button
            type="default"
            danger
            block
            icon={<DisconnectOutlined />}
            onClick={handleDisconnect}
            loading={loading}
          >
            断开连接
          </Button>
        ) : (
          <Button
            type="primary"
            block
            icon={<ApiOutlined />}
            onClick={handleConnect}
            loading={isConnecting || loading}
            disabled={!portName}
          >
            连接串口
          </Button>
        )}

        {/* 错误信息 */}
        {error && (
          <Text type="danger" style={{ fontSize: 12 }}>
            {error}
          </Text>
        )}
      </Space>
    </Card>
  );
};
