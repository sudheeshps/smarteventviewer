import { useState, useEffect } from 'react';
import { Sidebar } from './components/Sidebar';
import { Dashboard } from './components/Dashboard';
import { EventsExplorer } from './components/EventsExplorer';
import { RiskCenter } from './components/RiskCenter';
import { fetchApiChannels } from './apiClient';

export function App() {
  const [activeTab, setActiveTab] = useState<'dashboard' | 'events' | 'riskcenter'>('events');
  const [currentChannel, setCurrentChannel] = useState<string>('Security');
  const [windowsLogs, setWindowsLogs] = useState<string[]>(['Security', 'System', 'Application', 'Setup', 'ForwardedEvents']);
  const [appServicesLogs, setAppServicesLogs] = useState<string[]>([
    'Microsoft-Windows-PowerShell/Operational',
    'Microsoft-Windows-TaskScheduler/Operational',
    'Microsoft-Windows-TerminalServices-LocalSessionManager/Operational',
    'Microsoft-Windows-TerminalServices-RemoteConnectionManager/Operational',
    'Microsoft-Windows-Sysmon/Operational',
    'Microsoft-Windows-WindowsUpdateClient/Operational',
    'Microsoft-Windows-Diagnostics-Performance/Operational',
    'Microsoft-Windows-CodeIntegrity/Operational',
    'Microsoft-Windows-SMBServer/Operational',
    'Microsoft-Windows-WMI-Activity/Operational',
    'Key Management Service',
    'Hardware Events',
    'Internet Explorer',
  ]);

  useEffect(() => {
    fetchChannels();
  }, []);

  const fetchChannels = async () => {
    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    console.log(`[UI-APP DEBUG] Attempting to fetch channels via apiClient from: ${baseUrl}`);
    try {
      const data = await fetchApiChannels(baseUrl);
      console.log(`[UI-APP DEBUG] Received channels payload via apiClient:`, data);
      if (data && data.channels) {
        const winLogs = data.channels.filter((c: string) =>
          ['Security', 'System', 'Application', 'Setup', 'ForwardedEvents'].includes(c)
        );
        const appLogs = data.channels.filter((c: string) => !winLogs.includes(c));
        setWindowsLogs(winLogs);
        setAppServicesLogs(appLogs);
      } else if (Array.isArray(data)) {
        console.log(`[UI-APP DEBUG] Received raw array of channels:`, data);
        const winLogs = (data as unknown as string[]).filter((c: string) =>
          ['Security', 'System', 'Application', 'Setup', 'ForwardedEvents'].includes(c)
        );
        const appLogs = (data as unknown as string[]).filter((c: string) => !winLogs.includes(c));
        setWindowsLogs(winLogs);
        setAppServicesLogs(appLogs);
      }
    } catch (err) {
      console.error(`[UI-APP DEBUG] Error fetching channels via apiClient from ${baseUrl}:`, err);
    }
  };

  return (
    <div style={{ display: 'flex', height: '100vh', width: '100vw', backgroundColor: '#0f172a', color: '#f8fafc', overflow: 'hidden' }}>
      <Sidebar
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        currentChannel={currentChannel}
        onSelectChannel={(ch) => setCurrentChannel(ch)}
        windowsLogs={windowsLogs}
        appServicesLogs={appServicesLogs}
      />
      <main style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
        {activeTab === 'dashboard' && <Dashboard />}
        {activeTab === 'events' && <EventsExplorer channelName={currentChannel} />}
        {activeTab === 'riskcenter' && <RiskCenter />}
      </main>
    </div>
  );
}

export default App;
