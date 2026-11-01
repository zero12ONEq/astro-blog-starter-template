// 系统配置参数管理
import { create } from "zustand";
import { persist } from "zustand/middleware";
import { ParamId, ContainerType } from "../types/serial";

/** 系统参数配置 */
export interface SystemConfig {
  // 液位参数
  H1: number; // 液位上限(m)
  H2: number; // 液位下限(m)

  // 频率参数
  F: number; // 频率阈值(Hz)

  // 容器参数
  S: ContainerType; // 容器类型
  r: number; // 半径(m) - 适用于球体和圆柱
  L: number; // 长度(m) - 适用于长方体
  W: number; // 宽度(m) - 适用于长方体
  H: number; // 高度(m) - 适用于圆柱和长方体
}

/** 标定点 */
export interface CalibrationPoint {
  weight: number; // 标称重量(吨)
  voltage: number; // 对应电压(V)
}

interface ConfigStore {
  // 系统参数（本地编辑）
  params: SystemConfig;

  // 设备参数（从设备查询到的）
  deviceParams: Partial<SystemConfig> | null;

  // 标定数据
  calibration: CalibrationPoint[];

  // 是否已保存到单片机
  syncedToDevice: boolean;

  // Actions
  updateParam: (paramId: ParamId, value: number) => void;
  updateParams: (params: Partial<SystemConfig>) => void;
  updateDeviceParams: (params: Partial<SystemConfig>) => void;
  clearDeviceParams: () => void;
  setCalibration: (points: CalibrationPoint[]) => void;
  addCalibrationPoint: (point: CalibrationPoint) => void;
  setSyncedToDevice: (synced: boolean) => void;
  reset: () => void;
}

const defaultConfig: SystemConfig = {
  H1: 2.0, // 液位上限 2m
  H2: 0.3, // 液位下限 0.3m
  F: 2000, // 频率阈值 2000Hz
  S: ContainerType.CYLINDER, // 默认圆柱体
  r: 1.0, // 半径 1m
  L: 2.0, // 长度 2m
  W: 2.0, // 宽度 2m
  H: 3.0, // 高度 3m
};

const defaultCalibration: CalibrationPoint[] = [
  { weight: 0, voltage: 0 },
  { weight: 5, voltage: 1.25 },
  { weight: 10, voltage: 2.5 },
  { weight: 15, voltage: 3.75 },
  { weight: 20, voltage: 5.0 },
];

export const useConfigStore = create<ConfigStore>()(
  persist(
    (set) => ({
      params: defaultConfig,
      deviceParams: null,
      calibration: defaultCalibration,
      syncedToDevice: false,

      updateParam: (paramId, value) => {
        set((state) => ({
          params: {
            ...state.params,
            [paramId]: value,
          },
          syncedToDevice: false, // 标记为未同步
        }));
      },

      updateParams: (newParams) => {
        set((state) => ({
          params: {
            ...state.params,
            ...newParams,
          },
          syncedToDevice: false,
        }));
      },

      updateDeviceParams: (newParams) => {
        set((state) => ({
          deviceParams: {
            ...state.deviceParams,
            ...newParams,
          },
        }));
      },

      clearDeviceParams: () => {
        set({ deviceParams: null });
      },

      setCalibration: (calibration) => {
        set({ calibration, syncedToDevice: false });
      },

      addCalibrationPoint: (point) => {
        set((state) => ({
          calibration: [...state.calibration, point].sort((a, b) => a.weight - b.weight),
          syncedToDevice: false,
        }));
      },

      setSyncedToDevice: (syncedToDevice) => set({ syncedToDevice }),

      reset: () => {
        set({
          params: defaultConfig,
          deviceParams: null,
          calibration: defaultCalibration,
          syncedToDevice: false,
        });
      },
    }),
    {
      name: "serial-monitor-config", // localStorage key
      partialize: (state) => ({
        params: state.params,
        calibration: state.calibration,
      }),
    }
  )
);
