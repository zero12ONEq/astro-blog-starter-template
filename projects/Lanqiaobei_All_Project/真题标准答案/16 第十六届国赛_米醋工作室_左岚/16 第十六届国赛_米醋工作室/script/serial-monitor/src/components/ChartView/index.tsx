// 实时曲线图组件
import React, { useMemo } from "react";
import ReactECharts from "echarts-for-react";
import { Card } from "antd";
import dayjs from "dayjs";
import { useDataStore } from "../../stores/dataStore";
import type { EChartsOption } from "echarts";

interface RealtimeChartProps {
  title: string;
  dataKey: "liquidHeight" | "temperature" | "weight" | "frequency" | "speed" | "volume" | "remainingSpace";
  unit: string;
  color?: string;
}

export const RealtimeChart: React.FC<RealtimeChartProps> = ({
  title,
  dataKey,
  unit,
  color = "#5470c6",
}) => {
  const { history } = useDataStore();

  const option: EChartsOption = useMemo(() => {
    // 提取时间和数据
    const times = history.map((item) => dayjs(item.timestamp).format("HH:mm:ss"));
    const values = history.map((item) => item[dataKey]);

    return {
      title: {
        text: title,
        left: "center",
        textStyle: {
          fontSize: 14,
        },
      },
      tooltip: {
        trigger: "axis",
        formatter: (params: any) => {
          const param = params[0];
          return `${param.name}<br/>${param.seriesName}: ${param.value ?? "--"} ${unit}`;
        },
      },
      grid: {
        left: "10%",
        right: "10%",
        bottom: "15%",
        top: "20%",
      },
      xAxis: {
        type: "category",
        data: times,
        boundaryGap: false,
        axisLabel: {
          rotate: 45,
        },
      },
      yAxis: {
        type: "value",
        name: unit,
        axisLabel: {
          formatter: `{value} ${unit}`,
        },
      },
      series: [
        {
          name: title,
          type: "line",
          data: values,
          smooth: true,
          symbol: "none",
          lineStyle: {
            color: color,
            width: 2,
          },
          areaStyle: {
            color: {
              type: "linear",
              x: 0,
              y: 0,
              x2: 0,
              y2: 1,
              colorStops: [
                {
                  offset: 0,
                  color: color + "40",
                },
                {
                  offset: 1,
                  color: color + "10",
                },
              ],
            },
          },
        },
      ],
    };
  }, [history, dataKey, title, unit, color]);

  return (
    <Card bodyStyle={{ padding: 12 }}>
      <ReactECharts option={option} style={{ height: 250 }} />
    </Card>
  );
};

// 预定义的4个图表组件
export const LiquidHeightChart: React.FC = () => (
  <RealtimeChart
    title="液位高度"
    dataKey="liquidHeight"
    unit="cm"
    color="#3f8600"
  />
);

export const TemperatureChart: React.FC = () => (
  <RealtimeChart
    title="温度"
    dataKey="temperature"
    unit="℃"
    color="#cf1322"
  />
);

export const WeightChart: React.FC = () => (
  <RealtimeChart title="重量" dataKey="weight" unit="吨" color="#1890ff" />
);

export const FrequencyChart: React.FC = () => (
  <RealtimeChart
    title="振动频率"
    dataKey="frequency"
    unit="Hz"
    color="#722ed1"
  />
);

export const SpeedChart: React.FC = () => (
  <RealtimeChart
    title="声速"
    dataKey="speed"
    unit="m/s"
    color="#fa8c16"
  />
);

export const VolumeChart: React.FC = () => (
  <RealtimeChart
    title="液体体积"
    dataKey="volume"
    unit="m³"
    color="#13c2c2"
  />
);

export const RemainingSpaceChart: React.FC = () => (
  <RealtimeChart
    title="剩余空间"
    dataKey="remainingSpace"
    unit="%"
    color="#52c41a"
  />
);
