// JetBrains Darcula 主题配色
export const JetBrainsTheme = {
  // 背景色
  background: {
    primary: "#2B2B2B",      // 主背景
    secondary: "#3C3F41",    // 次级背景（侧边栏、工具窗口）
    elevated: "#313335",     // 浮层背景
    hover: "#4B4D4F",        // 悬停状态
    selected: "#4A88C7",     // 选中状态
  },

  // 边框色
  border: {
    default: "#323232",      // 默认边框
    strong: "#5A5D60",       // 强边框
    divider: "#3E4244",      // 分割线
  },

  // 文本色
  text: {
    primary: "#A9B7C6",      // 主文本
    secondary: "#8A8A8A",    // 次级文本
    disabled: "#5F6163",     // 禁用文本
    white: "#FFFFFF",        // 纯白文本
  },

  // 强调色
  accent: {
    blue: "#4A9EFF",         // 蓝色（链接、按钮）
    green: "#6A9955",        // 绿色（成功）
    orange: "#CC7832",       // 橙色（警告）
    red: "#FF6B68",          // 红色（错误）
    purple: "#9876AA",       // 紫色
    yellow: "#FFC66D",       // 黄色
  },

  // 组件色
  component: {
    buttonPrimary: "#365880",    // 主按钮
    buttonPrimaryHover: "#4A88C7", // 主按钮悬停
    buttonSecondary: "#3C3F41",   // 次级按钮
    inputBg: "#45494A",           // 输入框背景
    inputBorder: "#6B6B6B",       // 输入框边框
    scrollbar: "#595959",         // 滚动条
    scrollbarHover: "#6F6F6F",    // 滚动条悬停
  },

  // 状态色
  status: {
    success: "#499C54",      // 成功
    warning: "#F6A52D",      // 警告
    error: "#E63946",        // 错误
    info: "#548AF7",         // 信息
  },

  // 图表色
  chart: {
    line1: "#4A9EFF",        // 曲线1 - 蓝色
    line2: "#6A9955",        // 曲线2 - 绿色
    line3: "#CC7832",        // 曲线3 - 橙色
    line4: "#9876AA",        // 曲线4 - 紫色
    grid: "#424242",         // 网格线
    axis: "#8A8A8A",         // 坐标轴
  },
};

// Ant Design 主题 token 配置
export const antdThemeConfig = {
  token: {
    colorPrimary: JetBrainsTheme.accent.blue,
    colorSuccess: JetBrainsTheme.status.success,
    colorWarning: JetBrainsTheme.status.warning,
    colorError: JetBrainsTheme.status.error,
    colorInfo: JetBrainsTheme.status.info,
    colorBgBase: JetBrainsTheme.background.primary,
    colorBgContainer: JetBrainsTheme.background.secondary,
    colorBorder: JetBrainsTheme.border.default,
    colorText: JetBrainsTheme.text.primary,
    colorTextSecondary: JetBrainsTheme.text.secondary,
    colorTextDisabled: JetBrainsTheme.text.disabled,
    borderRadius: 4,
    fontSize: 13,
    controlHeight: 28,
  },
  components: {
    Layout: {
      headerBg: JetBrainsTheme.background.secondary,
      siderBg: JetBrainsTheme.background.secondary,
      bodyBg: JetBrainsTheme.background.primary,
      footerBg: JetBrainsTheme.background.secondary,
    },
    Menu: {
      itemBg: "transparent",
      itemSelectedBg: JetBrainsTheme.background.selected,
      itemHoverBg: JetBrainsTheme.background.hover,
      itemColor: JetBrainsTheme.text.primary,
      itemSelectedColor: JetBrainsTheme.text.white,
    },
    Card: {
      colorBgContainer: JetBrainsTheme.background.secondary,
      colorBorderSecondary: JetBrainsTheme.border.default,
    },
    Input: {
      colorBgContainer: JetBrainsTheme.component.inputBg,
      colorBorder: JetBrainsTheme.component.inputBorder,
      activeBorderColor: JetBrainsTheme.accent.blue,
    },
    Button: {
      primaryColor: JetBrainsTheme.text.white,
      defaultBg: JetBrainsTheme.component.buttonSecondary,
      defaultBorderColor: JetBrainsTheme.border.strong,
    },
    Table: {
      headerBg: JetBrainsTheme.background.secondary,
      rowHoverBg: JetBrainsTheme.background.hover,
    },
    Tabs: {
      cardBg: JetBrainsTheme.background.secondary,
      itemColor: JetBrainsTheme.text.secondary,
      itemSelectedColor: JetBrainsTheme.text.white,
      itemHoverColor: JetBrainsTheme.text.primary,
    },
  },
};
