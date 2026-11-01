// 数据监控卡片
import React from "react";
import { Card, Row, Col, Statistic, Progress } from "antd";
import {
  DashboardOutlined,
  FireOutlined,
  DatabaseOutlined,
  LineChartOutlined,
  ThunderboltOutlined,
} from "@ant-design/icons";
import { useDataStore } from "../../stores/dataStore";

export const DataMonitor: React.FC = () => {
  const { current, calculated } = useDataStore();

  return (
    <Row gutter={[16, 16]}>
      {/* 液位监测 */}
      <Col xs={24} sm={12} lg={6}>
        <Card>
          <Statistic
            title="液位高度"
            value={current.liquidHeight ?? "--"}
            suffix="cm"
            prefix={<DashboardOutlined />}
            valueStyle={{ color: "#3f8600" }}
          />
          {calculated.volume !== null && (
            <div style={{ marginTop: 16 }}>
              <div style={{ marginBottom: 8 }}>
                体积: {calculated.volume.toFixed(3)} m³
              </div>
              {calculated.remainingSpace !== null && (
                <>
                  <div style={{ marginBottom: 8 }}>
                    剩余空间: {calculated.remainingSpace.toFixed(1)}%
                  </div>
                  <Progress
                    percent={100 - calculated.remainingSpace}
                    status={
                      calculated.remainingSpace < 10
                        ? "exception"
                        : calculated.remainingSpace < 30
                        ? "normal"
                        : "success"
                    }
                    showInfo={false}
                  />
                </>
              )}
            </div>
          )}
        </Card>
      </Col>

      {/* 温度监测 */}
      <Col xs={24} sm={12} lg={6}>
        <Card>
          <Statistic
            title="温度"
            value={current.temperature ?? "--"}
            suffix="℃"
            prefix={<FireOutlined />}
            valueStyle={{
              color:
                current.temperature && current.temperature > 30
                  ? "#cf1322"
                  : current.temperature && current.temperature < 0
                  ? "#1890ff"
                  : "#3f8600",
            }}
          />
          <div style={{ marginTop: 16, fontSize: 12, color: "#888" }}>
            传感器: DS18B20
          </div>
        </Card>
      </Col>

      {/* 重量监测 */}
      <Col xs={24} sm={12} lg={6}>
        <Card>
          <Statistic
            title="重量"
            value={current.weight ?? "--"}
            suffix="吨"
            prefix={<DatabaseOutlined />}
            precision={2}
            valueStyle={{ color: "#1890ff" }}
          />
          <div style={{ marginTop: 16, fontSize: 12, color: "#888" }}>
            称重传感器
          </div>
        </Card>
      </Col>

      {/* 频率监测 */}
      <Col xs={24} sm={12} lg={6}>
        <Card>
          <Statistic
            title="振动频率"
            value={current.frequency ?? "--"}
            suffix="Hz"
            prefix={<LineChartOutlined />}
            valueStyle={{ color: "#722ed1" }}
          />
          <div style={{ marginTop: 16, fontSize: 12, color: "#888" }}>
            NE555 传感器
          </div>
        </Card>
      </Col>

      {/* 声速监测 */}
      <Col xs={24} sm={12} lg={6}>
        <Card>
          <Statistic
            title="声速"
            value={calculated.speed ?? "--"}
            suffix="m/s"
            prefix={<ThunderboltOutlined />}
            precision={1}
            valueStyle={{ color: "#fa8c16" }}
          />
          <div style={{ marginTop: 16, fontSize: 12, color: "#888" }}>
            超声波计算值
          </div>
        </Card>
      </Col>
    </Row>
  );
};
