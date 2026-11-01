// 实时传感器数据管理
import { create } from "zustand";

/** 传感器数据点 */
export interface SensorData {
  timestamp: number; // Unix时间戳(毫秒)
  liquidHeight: number | null; // 液位高度(cm)
  temperature: number | null; // 温度(℃)
  weight: number | null; // 重量(吨)
  frequency: number | null; // 频率(Hz)
}

/** 计算数据 */
export interface CalculatedData {
  volume: number | null; // 体积(m³)
  remainingSpace: number | null; // 剩余空间(%)
  speed: number | null; // 声速(m/s)
}

/** 历史数据点（包含传感器数据和计算数据）*/
export interface HistoryDataPoint extends SensorData, CalculatedData {}

interface DataStore {
  // 最新数据
  current: SensorData;
  calculated: CalculatedData;

  // 历史数据(用于图表,最多保留1000个点) - 现在包含计算数据
  history: HistoryDataPoint[];
  maxHistoryLength: number;

  // Actions
  updateLiquidHeight: (height: number) => void;
  updateTemperature: (temp: number) => void;
  updateWeight: (weight: number) => void;
  updateFrequency: (freq: number) => void;
  updateCalculated: (data: Partial<CalculatedData>) => void;
  addHistoryPoint: (data: SensorData) => void;
  clearHistory: () => void;
  reset: () => void;
}

const initialCurrent: SensorData = {
  timestamp: Date.now(),
  liquidHeight: null,
  temperature: null,
  weight: null,
  frequency: null,
};

const initialCalculated: CalculatedData = {
  volume: null,
  remainingSpace: null,
  speed: null,
};

export const useDataStore = create<DataStore>((set) => ({
  current: initialCurrent,
  calculated: initialCalculated,
  history: [],
  maxHistoryLength: 1000,

  updateLiquidHeight: (liquidHeight) => {
    set((state) => ({
      current: {
        ...state.current,
        liquidHeight,
        timestamp: Date.now(),
      },
    }));
  },

  updateTemperature: (temperature) => {
    set((state) => ({
      current: {
        ...state.current,
        temperature,
        timestamp: Date.now(),
      },
    }));
  },

  updateWeight: (weight) => {
    set((state) => ({
      current: {
        ...state.current,
        weight,
        timestamp: Date.now(),
      },
    }));
  },

  updateFrequency: (frequency) => {
    set((state) => ({
      current: {
        ...state.current,
        frequency,
        timestamp: Date.now(),
      },
    }));
  },

  updateCalculated: (data) => {
    set((state) => ({
      calculated: {
        ...state.calculated,
        ...data,
      },
    }));
  },

  addHistoryPoint: (data) => {
    set((state) => {
      // 合并传感器数据和计算数据，优先使用传入的data，其次使用state.current中的值
      const fullData: HistoryDataPoint = {
        timestamp: data.timestamp,
        liquidHeight: data.liquidHeight ?? state.current.liquidHeight,
        temperature: data.temperature ?? state.current.temperature,
        weight: data.weight ?? state.current.weight,
        frequency: data.frequency ?? state.current.frequency,
        volume: state.calculated.volume,
        remainingSpace: state.calculated.remainingSpace,
        speed: state.calculated.speed,
      };

      const newHistory = [...state.history, fullData];
      // 保持最大长度限制
      if (newHistory.length > state.maxHistoryLength) {
        newHistory.shift();
      }
      return { history: newHistory };
    });
  },

  clearHistory: () => set({ history: [] }),

  reset: () => {
    set({
      current: initialCurrent,
      calculated: initialCalculated,
      history: [],
    });
  },
}));
