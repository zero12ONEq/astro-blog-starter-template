// Dashboard主页 - JetBrains风格
import React, { useEffect, useState } from "react";
import { Layout, Menu, Space, Badge, Typography } from "antd";
import {
  DashboardOutlined,
  SettingOutlined,
  WarningOutlined,
  LineChartOutlined,
  ApiOutlined,
  CheckCircleOutlined,
  CloseCircleOutlined,
} from "@ant-design/icons";
import { SerialConfig } from "../../components/SerialConfig";
import { DataMonitor } from "../../components/DataMonitor";
import { ParamConfig } from "../../components/ParamConfig";
import { AlarmPanel } from "../../components/AlarmPanel";
import {
  LiquidHeightChart,
  TemperatureChart,
  WeightChart,
  FrequencyChart,
} from "../../components/ChartView";
import { useSerialStore } from "../../stores/serialStore";
import { useDataStore } from "../../stores/dataStore";
import { useAlarmStore } from "../../stores/alarmStore";
import {
  onAlarm,
  onQueryResponse,
  onParamResponse,
  onRawData,
  onSerialError,
} from "../../services/tauri-api";
import { JetBrainsTheme } from "../../styles/theme";
import "./Dashboard.css";

const { Sider, Content, Footer } = Layout;
const { Text } = Typography;

type ViewType = "monitor" | "config" | "alarm" | "charts";

export const Dashboard: React.FC = () => {
  const [currentView, setCurrentView] = useState<ViewType>("monitor");
  const { addListener, status, portName, baudRate } = useSerialStore();
  const { current, updateLiquidHeight, updateTemperature, updateWeight, updateFrequency, addHistoryPoint } = useDataStore();
  const { addAlarm: addAlarmToStore, unacknowledgedCount } = useAlarmStore();

  // 设置事件监听器
  useEffect(() => {
    if (status !== "connected") return;

    const setupListeners = async () => {
      try {
        const unlistenAlarm = await onAlarm((alarm) => {
          addAlarmToStore(alarm);
        });

        const unlistenQuery = await onQueryResponse((response) => {
          console.log("参数查询响应:", response);
        });

        const unlistenParam = await onParamResponse((response) => {
          console.log("参数配置响应:", response);
        });

        const unlistenRaw = await onRawData((data) => {
          parseAndUpdateSensorData(data);
        });

        const unlistenError = await onSerialError((error) => {
          console.error("串口错误:", error);
        });

        addListener(unlistenAlarm);
        addListener(unlistenQuery);
        addListener(unlistenParam);
        addListener(unlistenRaw);
        addListener(unlistenError);
      } catch (err) {
        console.error("设置事件监听器失败:", err);
      }
    };

    setupListeners();
  }, [status]);

  const parseAndUpdateSensorData = (data: string) => {
    const parts = data.split(",");
    parts.forEach((part) => {
      const [key, value] = part.split(":");
      const numValue = parseFloat(value);
      if (isNaN(numValue)) return;

      switch (key.trim()) {
        case "L":
          updateLiquidHeight(numValue);
          break;
        case "T":
          updateTemperature(numValue);
          break;
        case "W":
          updateWeight(numValue);
          break;
        case "F":
          updateFrequency(numValue);
          break;
      }
    });

    addHistoryPoint({ ...current, timestamp: Date.now() });
  };

  const renderContent = () => {
    switch (currentView) {
      case "monitor":
        return (
          <div style={{ padding: 16 }}>
            <DataMonitor />
            <div style={{ marginTop: 16 }}>
              <SerialConfig />
            </div>
          </div>
        );
      case "config":
        return (
          <div style={{ padding: 16 }}>
            <ParamConfig />
          </div>
        );
      case "alarm":
        return (
          <div style={{ padding: 16 }}>
            <AlarmPanel />
          </div>
        );
      case "charts":
        return (
          <div style={{ padding: 16, display: "grid", gridTemplateColumns: "repeat(2, 1fr)", gap: 16 }}>
            <LiquidHeightChart />
            <TemperatureChart />
            <WeightChart />
            <FrequencyChart />
          </div>
        );
      default:
        return null;
    }
  };

  return (
    <Layout style={{ height: "100vh", background: JetBrainsTheme.background.primary }}>
      {/* 左侧边栏 */}
      <Sider
        width={220}
        style={{
          background: JetBrainsTheme.background.secondary,
          borderRight: `1px solid ${JetBrainsTheme.border.default}`,
        }}
      >
        <div
          style={{
            padding: "16px 12px",
            borderBottom: `1px solid ${JetBrainsTheme.border.default}`,
            color: JetBrainsTheme.text.primary,
            fontWeight: 600,
            fontSize: 14,
          }}
        >
          串口监控助手
        </div>

        <Menu
          mode="inline"
          selectedKeys={[currentView]}
          onClick={({ key }) => setCurrentView(key as ViewType)}
          style={{
            background: "transparent",
            border: "none",
            color: JetBrainsTheme.text.primary,
          }}
          items={[
            {
              key: "monitor",
              icon: <DashboardOutlined />,
              label: "实时监控",
            },
            {
              key: "charts",
              icon: <LineChartOutlined />,
              label: "数据曲线",
            },
            {
              key: "config",
              icon: <SettingOutlined />,
              label: "参数配置",
            },
            {
              key: "alarm",
              icon: <WarningOutlined />,
              label: (
                <Space>
                  告警记录
                  {unacknowledgedCount > 0 && (
                    <Badge
                      count={unacknowledgedCount}
                      style={{
                        background: JetBrainsTheme.status.error,
                      }}
                    />
                  )}
                </Space>
              ),
            },
          ]}
        />
      </Sider>

      {/* 主内容区 */}
      <Layout>
        <Content
          style={{
            background: JetBrainsTheme.background.primary,
            overflow: "auto",
          }}
        >
          {renderContent()}
        </Content>

        {/* 底部状态栏 */}
        <Footer
          style={{
            padding: "4px 16px",
            background: JetBrainsTheme.background.secondary,
            borderTop: `1px solid ${JetBrainsTheme.border.default}`,
            display: "flex",
            alignItems: "center",
            justifyContent: "space-between",
            height: 28,
          }}
        >
          <Space size={16} split={<div className="jetbrains-statusbar-separator" />}>
            <Space size={8}>
              <ApiOutlined style={{ fontSize: 12 }} />
              <Text
                style={{
                  fontSize: 12,
                  color: JetBrainsTheme.text.secondary,
                }}
              >
                串口:
              </Text>
              {status === "connected" ? (
                <Space size={4}>
                  <CheckCircleOutlined style={{ color: JetBrainsTheme.status.success }} />
                  <Text
                    style={{
                      fontSize: 12,
                      color: JetBrainsTheme.text.primary,
                    }}
                  >
                    {portName} @ {baudRate}bps
                  </Text>
                </Space>
              ) : (
                <Space size={4}>
                  <CloseCircleOutlined style={{ color: JetBrainsTheme.text.disabled }} />
                  <Text
                    style={{
                      fontSize: 12,
                      color: JetBrainsTheme.text.disabled,
                    }}
                  >
                    未连接
                  </Text>
                </Space>
              )}
            </Space>

            {unacknowledgedCount > 0 && (
              <Space size={4}>
                <WarningOutlined style={{ color: JetBrainsTheme.status.warning }} />
                <Text
                  style={{
                    fontSize: 12,
                    color: JetBrainsTheme.status.warning,
                  }}
                >
                  {unacknowledgedCount} 条未确认告警
                </Text>
              </Space>
            )}
          </Space>

          <Text
            style={{
              fontSize: 11,
              color: JetBrainsTheme.text.disabled,
            }}
          >
            蓝桥杯单片机上位机 v1.0.0
          </Text>
        </Footer>
      </Layout>
    </Layout>
  );
};
