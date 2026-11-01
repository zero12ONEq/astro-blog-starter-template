// 告警面板
import React, { useMemo } from "react";
import {
  Card,
  List,
  Tag,
  Button,
  Space,
  Badge,
  Empty,
  Popconfirm,
  Switch,
} from "antd";
import {
  WarningOutlined,
  CheckOutlined,
  DeleteOutlined,
  ClearOutlined,
  SoundOutlined,
} from "@ant-design/icons";
import dayjs from "dayjs";
import { useAlarmStore } from "../../stores/alarmStore";
import type { AlarmRecord } from "../../stores/alarmStore";
import { AlarmSeverity, AlarmTypeDesc, AlarmType } from "../../types/serial";

// 严重级别颜色映射
const SeverityColors: Record<AlarmSeverity, string> = {
  [AlarmSeverity.INFO]: "blue",
  [AlarmSeverity.WARNING]: "orange",
  [AlarmSeverity.ERROR]: "red",
  [AlarmSeverity.CRITICAL]: "red",
};

// 严重级别图标
const SeverityIcons: Record<AlarmSeverity, React.ReactNode> = {
  [AlarmSeverity.INFO]: <WarningOutlined style={{ color: "#1890ff" }} />,
  [AlarmSeverity.WARNING]: <WarningOutlined style={{ color: "#faad14" }} />,
  [AlarmSeverity.ERROR]: <WarningOutlined style={{ color: "#ff4d4f" }} />,
  [AlarmSeverity.CRITICAL]: <WarningOutlined style={{ color: "#ff4d4f" }} />,
};

export const AlarmPanel: React.FC = () => {
  const {
    alarms,
    unacknowledgedCount,
    soundEnabled,
    acknowledgeAlarm,
    acknowledgeAll,
    clearAlarms,
    removeAlarm,
    setSoundEnabled,
  } = useAlarmStore();

  // 按时间排序(最新在前)
  const sortedAlarms = useMemo(() => {
    return [...alarms].sort((a, b) => b.receivedAt - a.receivedAt);
  }, [alarms]);

  return (
    <Card
      title={
        <Space>
          <span>告警记录</span>
          <Badge count={unacknowledgedCount} showZero />
        </Space>
      }
      extra={
        <Space>
          <Switch
            checkedChildren={<SoundOutlined />}
            unCheckedChildren={<SoundOutlined />}
            checked={soundEnabled}
            onChange={setSoundEnabled}
            title="告警音效"
          />
          <Button
            size="small"
            icon={<CheckOutlined />}
            onClick={acknowledgeAll}
            disabled={unacknowledgedCount === 0}
          >
            全部确认
          </Button>
          <Popconfirm
            title="确定清空所有告警吗?"
            onConfirm={clearAlarms}
            disabled={alarms.length === 0}
          >
            <Button
              size="small"
              danger
              icon={<ClearOutlined />}
              disabled={alarms.length === 0}
            >
              清空
            </Button>
          </Popconfirm>
        </Space>
      }
    >
      {alarms.length === 0 ? (
        <Empty description="暂无告警" />
      ) : (
        <List
          dataSource={sortedAlarms}
          renderItem={(alarm: AlarmRecord) => (
            <List.Item
              key={alarm.id}
              style={{
                opacity: alarm.acknowledged ? 0.6 : 1,
                background: alarm.acknowledged ? "transparent" : "#fff7e6",
              }}
              actions={[
                !alarm.acknowledged && (
                  <Button
                    type="text"
                    size="small"
                    icon={<CheckOutlined />}
                    onClick={() => acknowledgeAlarm(alarm.id)}
                  >
                    确认
                  </Button>
                ),
                <Button
                  type="text"
                  size="small"
                  danger
                  icon={<DeleteOutlined />}
                  onClick={() => removeAlarm(alarm.id)}
                />,
              ].filter(Boolean)}
            >
              <List.Item.Meta
                avatar={SeverityIcons[alarm.severity]}
                title={
                  <Space>
                    <Tag color={SeverityColors[alarm.severity]}>
                      {AlarmTypeDesc[alarm.error_code as AlarmType] || `错误码 ${alarm.error_code}`}
                    </Tag>
                    {!alarm.acknowledged && <Badge status="processing" text="未确认" />}
                  </Space>
                }
                description={
                  <Space direction="vertical" size={4}>
                    <div>
                      <strong>数值:</strong> {alarm.value}
                    </div>
                    <div>
                      <strong>时间:</strong> {dayjs(alarm.receivedAt).format("YYYY-MM-DD HH:mm:ss")}
                    </div>
                    <div style={{ color: "#999", fontSize: 12 }}>
                      时间戳: {alarm.timestamp}
                    </div>
                  </Space>
                }
              />
            </List.Item>
          )}
          style={{ maxHeight: 400, overflowY: "auto" }}
        />
      )}
    </Card>
  );
};
