// 串口连接状态管理
import { create } from "zustand";
import { SerialConnectionStatus } from "../types/serial";
import type { UnlistenFn } from "@tauri-apps/api/event";

interface SerialStore {
  // 连接状态
  status: SerialConnectionStatus;
  portName: string;
  baudRate: number;
  availablePorts: string[];

  // 事件监听器清理函数
  listeners: UnlistenFn[];

  // 错误信息
  error: string | null;

  // Actions
  setStatus: (status: SerialConnectionStatus) => void;
  setPortName: (portName: string) => void;
  setBaudRate: (baudRate: number) => void;
  setAvailablePorts: (ports: string[]) => void;
  addListener: (listener: UnlistenFn) => void;
  clearListeners: () => void;
  setError: (error: string | null) => void;
  reset: () => void;
}

const initialState = {
  status: SerialConnectionStatus.DISCONNECTED,
  portName: "",
  baudRate: 9600,
  availablePorts: [],
  listeners: [],
  error: null,
};

export const useSerialStore = create<SerialStore>((set, get) => ({
  ...initialState,

  setStatus: (status) => set({ status }),

  setPortName: (portName) => set({ portName }),

  setBaudRate: (baudRate) => set({ baudRate }),

  setAvailablePorts: (availablePorts) => set({ availablePorts }),

  addListener: (listener) => {
    set((state) => ({
      listeners: [...state.listeners, listener],
    }));
  },

  clearListeners: async () => {
    const { listeners } = get();
    // 清理所有事件监听器
    await Promise.all(listeners.map((unlisten) => unlisten()));
    set({ listeners: [] });
  },

  setError: (error) => set({ error }),

  reset: async () => {
    await get().clearListeners();
    set(initialState);
  },
}));
