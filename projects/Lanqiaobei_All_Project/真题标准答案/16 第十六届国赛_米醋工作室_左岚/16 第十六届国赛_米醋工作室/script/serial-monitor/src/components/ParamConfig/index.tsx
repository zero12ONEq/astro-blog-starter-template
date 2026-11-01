// 参数配置表单
import React, { useState, useEffect } from "react";
import {
  Card,
  Form,
  InputNumber,
  Select,
  Button,
  Space,
  message,
  Divider,
  Alert,
  Row,
  Col,
  Descriptions,
  Tag,
  Empty,
} from "antd";
import { SaveOutlined, SyncOutlined, DownloadOutlined } from "@ant-design/icons";
import { useConfigStore } from "../../stores/configStore";
import { useSerialStore } from "../../stores/serialStore";
import { ParamId, ContainerType, ContainerTypeDesc } from "../../types/serial";
import { sendParamConfig, sendQueryCommand, onQueryResponse } from "../../services/tauri-api";

export const ParamConfig: React.FC = () => {
  const { params, deviceParams, syncedToDevice, updateParams, updateDeviceParams, clearDeviceParams, setSyncedToDevice } = useConfigStore();
  const { status, addListener } = useSerialStore();
  const [form] = Form.useForm();
  const [loading, setLoading] = useState(false);
  const [queryLoading, setQueryLoading] = useState(false);

  const isConnected = status === "connected";

  // 监听查询响应
  useEffect(() => {
    if (status !== "connected") return;

    const setupListener = async () => {
      try {
        const unlisten = await onQueryResponse((response) => {
          console.log("查询响应:", response);

          // 解析响应并更新设备参数
          const { param_id, value } = response;
          const numValue = parseFloat(value);

          if (!isNaN(numValue)) {
            updateDeviceParams({ [param_id]: numValue });
          }
        });

        addListener(unlisten);
      } catch (err) {
        console.error("设置查询响应监听失败:", err);
      }
    };

    setupListener();
  }, [status, addListener, updateDeviceParams]);

  // 保存参数到单片机
  const handleSave = async () => {
    try {
      setLoading(true);
      const values = await form.validateFields();

      // 更新本地配置
      updateParams(values);

      // 构造参数数组（只发送有值的参数）
      const paramList = [
        { id: ParamId.H1, value: values.H1.toString() },
        { id: ParamId.H2, value: values.H2.toString() },
        { id: ParamId.F, value: values.F.toString() },
        { id: ParamId.S, value: values.S.toString() },
      ];

      // 根据容器类型添加可选参数
      if (values.r !== undefined && values.r !== null) {
        paramList.push({ id: ParamId.R, value: values.r.toString() });
      }
      if (values.L !== undefined && values.L !== null) {
        paramList.push({ id: ParamId.L, value: values.L.toString() });
      }
      if (values.W !== undefined && values.W !== null) {
        paramList.push({ id: ParamId.W, value: values.W.toString() });
      }
      if (values.H !== undefined && values.H !== null) {
        paramList.push({ id: ParamId.H, value: values.H.toString() });
      }

      // 发送到单片机
      await sendParamConfig(paramList);
      setSyncedToDevice(true);

      message.success("参数配置成功");
    } catch (err) {
      const errMsg = err instanceof Error ? err.message : String(err);
      message.error(`配置失败: ${errMsg}`);
    } finally {
      setLoading(false);
    }
  };

  // 查询单片机当前参数
  const handleQuery = async () => {
    try {
      setQueryLoading(true);
      clearDeviceParams(); // 清空之前的数据

      // 依次查询所有参数
      const paramIds = [
        ParamId.H1,
        ParamId.H2,
        ParamId.F,
        ParamId.S,
        ParamId.R,
        ParamId.L,
        ParamId.W,
        ParamId.H,
      ];

      for (const id of paramIds) {
        await sendQueryCommand(id);
        // 延迟一下，避免发送太快
        await new Promise((resolve) => setTimeout(resolve, 100));
      }

      message.success("参数查询命令已发送，请稍候查看结果");
    } catch (err) {
      const errMsg = err instanceof Error ? err.message : String(err);
      message.error(`查询失败: ${errMsg}`);
    } finally {
      setQueryLoading(false);
    }
  };

  // 重置为默认值
  const handleReset = () => {
    form.setFieldsValue(params);
    message.info("已重置为保存的值");
  };

  return (
    <Card
      title="系统参数配置"
      extra={
        !syncedToDevice && (
          <Alert
            message="参数未同步"
            description="本地参数已修改，请保存到设备"
            type="warning"
            showIcon
            closable
            style={{ marginBottom: 0 }}
          />
        )
      }
    >
      <Row gutter={24}>
        {/* 左侧：参数编辑表单 */}
        <Col xs={24} lg={12}>
          <Form
            form={form}
            layout="vertical"
            initialValues={params}
            onValuesChange={() => setSyncedToDevice(false)}
          >
            <Divider orientation="left">液位参数</Divider>
            <Space style={{ width: "100%" }} size="large">
              <Form.Item
                label="液位上限 (H1)"
                name="H1"
                rules={[
                  { required: true, message: "请输入液位上限" },
                  { type: "number", min: 0, max: 10, message: "范围 0-10m" },
                ]}
              >
                <InputNumber addonAfter="m" style={{ width: 150 }} step={0.1} />
              </Form.Item>

              <Form.Item
                label="液位下限 (H2)"
                name="H2"
                rules={[
                  { required: true, message: "请输入液位下限" },
                  { type: "number", min: 0, max: 10, message: "范围 0-10m" },
                ]}
              >
                <InputNumber addonAfter="m" style={{ width: 150 }} step={0.1} />
              </Form.Item>
            </Space>

            <Divider orientation="left">频率参数</Divider>
            <Form.Item
              label="频率阈值 (F)"
              name="F"
              rules={[
                { required: true, message: "请输入频率阈值" },
                { type: "number", min: 0, max: 10000, message: "范围 0-10000Hz" },
              ]}
            >
              <InputNumber addonAfter="Hz" style={{ width: 200 }} />
            </Form.Item>

            <Divider orientation="left">容器参数</Divider>
            <Form.Item label="容器类型 (S)" name="S" rules={[{ required: true }]}>
              <Select
                style={{ width: 200 }}
                options={[
                  { label: ContainerTypeDesc[ContainerType.SPHERE], value: ContainerType.SPHERE },
                  {
                    label: ContainerTypeDesc[ContainerType.CYLINDER],
                    value: ContainerType.CYLINDER,
                  },
                  { label: ContainerTypeDesc[ContainerType.CUBOID], value: ContainerType.CUBOID },
                ]}
              />
            </Form.Item>

            <Form.Item
              noStyle
              shouldUpdate={(prevValues, currentValues) => prevValues.S !== currentValues.S}
            >
              {({ getFieldValue }) => {
                const containerType = getFieldValue("S");

                return (
                  <Space style={{ width: "100%" }} size="large" wrap>
                    {/* 球体和圆柱需要半径 */}
                    {(containerType === ContainerType.SPHERE ||
                      containerType === ContainerType.CYLINDER) && (
                      <Form.Item
                        label="半径 (r)"
                        name="r"
                        rules={[{ type: "number", min: 0, max: 10 }]}
                      >
                        <InputNumber addonAfter="m" style={{ width: 150 }} step={0.1} />
                      </Form.Item>
                    )}

                    {/* 长方体需要长宽 */}
                    {containerType === ContainerType.CUBOID && (
                      <>
                        <Form.Item
                          label="长度 (L)"
                          name="L"
                          rules={[{ type: "number", min: 0, max: 20 }]}
                        >
                          <InputNumber addonAfter="m" style={{ width: 150 }} step={0.1} />
                        </Form.Item>
                        <Form.Item
                          label="宽度 (W)"
                          name="W"
                          rules={[{ type: "number", min: 0, max: 20 }]}
                        >
                          <InputNumber addonAfter="m" style={{ width: 150 }} step={0.1} />
                        </Form.Item>
                      </>
                    )}

                    {/* 圆柱和长方体需要高度 */}
                    {(containerType === ContainerType.CYLINDER ||
                      containerType === ContainerType.CUBOID) && (
                      <Form.Item
                        label="高度 (H)"
                        name="H"
                        rules={[{ type: "number", min: 0, max: 20 }]}
                      >
                        <InputNumber addonAfter="m" style={{ width: 150 }} step={0.1} />
                      </Form.Item>
                    )}
                  </Space>
                );
              }}
            </Form.Item>

            <Divider />

            <Space>
              <Button
                type="primary"
                icon={<SaveOutlined />}
                onClick={handleSave}
                loading={loading}
                disabled={!isConnected}
              >
                保存到设备
              </Button>

              <Button
                icon={<DownloadOutlined />}
                onClick={handleQuery}
                loading={queryLoading}
                disabled={!isConnected}
              >
                查询设备参数
              </Button>

              <Button icon={<SyncOutlined />} onClick={handleReset}>
                重置
              </Button>
            </Space>

            {!isConnected && (
              <Alert
                message="请先连接串口"
                type="info"
                showIcon
                style={{ marginTop: 16 }}
              />
            )}
          </Form>
        </Col>

        {/* 右侧：设备参数显示 */}
        <Col xs={24} lg={12}>
          <Card
            title="设备当前参数"
            size="small"
            style={{ background: "#1f1f1f", borderColor: "#3E4244" }}
          >
            {deviceParams && Object.keys(deviceParams).length > 0 ? (
              <Descriptions
                bordered
                size="small"
                column={1}
                labelStyle={{ width: 120, background: "#2B2B2B", color: "#A9B7C6" }}
                contentStyle={{ background: "#3C3F41", color: "#FFF" }}
              >
                {deviceParams.H1 !== undefined && (
                  <Descriptions.Item label="液位上限 (H1)">
                    {deviceParams.H1} m
                  </Descriptions.Item>
                )}
                {deviceParams.H2 !== undefined && (
                  <Descriptions.Item label="液位下限 (H2)">
                    {deviceParams.H2} m
                  </Descriptions.Item>
                )}
                {deviceParams.F !== undefined && (
                  <Descriptions.Item label="频率阈值 (F)">
                    {deviceParams.F} Hz
                  </Descriptions.Item>
                )}
                {deviceParams.S !== undefined && (
                  <Descriptions.Item label="容器类型 (S)">
                    <Tag color="blue">{ContainerTypeDesc[deviceParams.S]}</Tag>
                  </Descriptions.Item>
                )}
                {deviceParams.r !== undefined && (
                  <Descriptions.Item label="半径 (r)">
                    {deviceParams.r} m
                  </Descriptions.Item>
                )}
                {deviceParams.L !== undefined && (
                  <Descriptions.Item label="长度 (L)">
                    {deviceParams.L} m
                  </Descriptions.Item>
                )}
                {deviceParams.W !== undefined && (
                  <Descriptions.Item label="宽度 (W)">
                    {deviceParams.W} m
                  </Descriptions.Item>
                )}
                {deviceParams.H !== undefined && (
                  <Descriptions.Item label="高度 (H)">
                    {deviceParams.H} m
                  </Descriptions.Item>
                )}
              </Descriptions>
            ) : (
              <Empty
                image={Empty.PRESENTED_IMAGE_SIMPLE}
                description="暂无设备参数"
                style={{ padding: "40px 0" }}
              >
                <Button
                  type="primary"
                  icon={<DownloadOutlined />}
                  onClick={handleQuery}
                  loading={queryLoading}
                  disabled={!isConnected}
                >
                  点击查询
                </Button>
              </Empty>
            )}
          </Card>
        </Col>
      </Row>
    </Card>
  );
};
