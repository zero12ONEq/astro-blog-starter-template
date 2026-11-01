// 告警信息管理
import { create } from "zustand";
import type { AlarmData } from "../types/serial";
import { AlarmType, AlarmSeverity, AlarmSeverityMap } from "../types/serial";

/** 告警记录(扩展) */
export interface AlarmRecord extends AlarmData {
  id: string; // 唯一ID
  severity: AlarmSeverity; // 严重级别
  receivedAt: number; // 接收时间(Unix时间戳)
  acknowledged: boolean; // 是否已确认
}

interface AlarmStore {
  // 告警列表
  alarms: AlarmRecord[];
  maxAlarmCount: number; // 最大保存数量

  // 未确认告警计数
  unacknowledgedCount: number;

  // 告警音效开关
  soundEnabled: boolean;

  // Actions
  addAlarm: (alarm: AlarmData) => void;
  acknowledgeAlarm: (id: string) => void;
  acknowledgeAll: () => void;
  clearAlarms: () => void;
  removeAlarm: (id: string) => void;
  setSoundEnabled: (enabled: boolean) => void;
  reset: () => void;
}

function createAlarmRecord(alarm: AlarmData): AlarmRecord {
  return {
    ...alarm,
    id: `${Date.now()}-${Math.random()}`,
    severity: AlarmSeverityMap[alarm.error_code as AlarmType] || AlarmSeverity.INFO,
    receivedAt: Date.now(),
    acknowledged: false,
  };
}

export const useAlarmStore = create<AlarmStore>((set, get) => ({
  alarms: [],
  maxAlarmCount: 500,
  unacknowledgedCount: 0,
  soundEnabled: true,

  addAlarm: (alarm) => {
    const record = createAlarmRecord(alarm);

    set((state) => {
      const newAlarms = [record, ...state.alarms];

      // 保持最大数量限制
      if (newAlarms.length > state.maxAlarmCount) {
        newAlarms.pop();
      }

      // 更新未确认计数
      const unacknowledgedCount = newAlarms.filter((a) => !a.acknowledged).length;

      return {
        alarms: newAlarms,
        unacknowledgedCount,
      };
    });

    // 播放告警音效(如果启用)
    if (get().soundEnabled && record.severity === AlarmSeverity.CRITICAL) {
      // TODO: 实现音效播放
      console.log("🔊 告警音效");
    }
  },

  acknowledgeAlarm: (id) => {
    set((state) => {
      const alarms = state.alarms.map((alarm) =>
        alarm.id === id ? { ...alarm, acknowledged: true } : alarm
      );

      const unacknowledgedCount = alarms.filter((a) => !a.acknowledged).length;

      return { alarms, unacknowledgedCount };
    });
  },

  acknowledgeAll: () => {
    set((state) => ({
      alarms: state.alarms.map((alarm) => ({ ...alarm, acknowledged: true })),
      unacknowledgedCount: 0,
    }));
  },

  clearAlarms: () => {
    set({
      alarms: [],
      unacknowledgedCount: 0,
    });
  },

  removeAlarm: (id) => {
    set((state) => {
      const alarms = state.alarms.filter((alarm) => alarm.id !== id);
      const unacknowledgedCount = alarms.filter((a) => !a.acknowledged).length;

      return { alarms, unacknowledgedCount };
    });
  },

  setSoundEnabled: (soundEnabled) => set({ soundEnabled }),

  reset: () => {
    set({
      alarms: [],
      unacknowledgedCount: 0,
    });
  },
}));
