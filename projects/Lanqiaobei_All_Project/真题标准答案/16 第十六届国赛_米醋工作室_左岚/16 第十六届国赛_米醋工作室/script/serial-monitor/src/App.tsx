import { ConfigProvider, theme } from "antd";
import zhCN from "antd/locale/zh_CN";
import { Dashboard } from "./pages/Dashboard";
import { antdThemeConfig } from "./styles/theme";
import "./App.css";

function App() {
  return (
    <ConfigProvider
      locale={zhCN}
      theme={{
        ...antdThemeConfig,
        algorithm: theme.darkAlgorithm,
      }}
    >
      <Dashboard />
    </ConfigProvider>
  );
}

export default App;
