import React, { useState } from 'react';

interface SidebarProps {
  activeTab: 'dashboard' | 'events' | 'riskcenter';
  setActiveTab: (tab: 'dashboard' | 'events' | 'riskcenter') => void;
  currentChannel: string;
  onSelectChannel: (channel: string) => void;
  windowsLogs: string[];
  appServicesLogs: string[];
}

export const Sidebar: React.FC<SidebarProps> = ({
  activeTab,
  setActiveTab,
  currentChannel,
  onSelectChannel,
  windowsLogs,
  appServicesLogs,
}) => {
  const [collapsedFolders, setCollapsedFolders] = useState<Record<string, boolean>>({
    winLogs: false,
    appServices: false,
    microsoft: false,
    windows: false,
  });

  const toggleFolder = (folderKey: string) => {
    setCollapsedFolders((prev) => ({ ...prev, [folderKey]: !prev[folderKey] }));
  };

  const totalCount = windowsLogs.length + appServicesLogs.length;

  return (
    <aside id="sidebar-panel" style={{
      width: '300px',
      minWidth: '200px',
      maxWidth: '600px',
      background: '#1e293b',
      borderRight: '1px solid rgba(255, 255, 255, 0.1)',
      padding: '16px 12px',
      display: 'flex',
      flexDirection: 'column',
      gap: '16px',
      overflowY: 'auto',
    }}>
      <div style={{ fontSize: '1.1rem', fontWeight: 700, color: '#38bdf8', display: 'flex', alignItems: 'center', gap: '8px' }}>
        🛡️ SmartEventViewer
      </div>

      <div style={{ fontSize: '0.75rem', textTransform: 'uppercase', color: '#94a3b8', letterSpacing: '0.04em' }}>
        Navigation
      </div>

      <ul style={{ listStyle: 'none', display: 'flex', flexDirection: 'column', gap: '4px' }}>
        <li
          onClick={() => setActiveTab('dashboard')}
          style={{
            padding: '8px 12px',
            borderRadius: '6px',
            cursor: 'pointer',
            fontWeight: 600,
            background: activeTab === 'dashboard' ? 'rgba(56, 189, 248, 0.15)' : 'transparent',
            color: activeTab === 'dashboard' ? '#38bdf8' : '#94a3b8',
          }}
        >
          📊 SIEM Dashboard & Analytics
        </li>
        <li
          onClick={() => setActiveTab('events')}
          style={{
            padding: '8px 12px',
            borderRadius: '6px',
            cursor: 'pointer',
            fontWeight: 600,
            background: activeTab === 'events' ? 'rgba(56, 189, 248, 0.15)' : 'transparent',
            color: activeTab === 'events' ? '#38bdf8' : '#94a3b8',
          }}
        >
          📁 Events Explorer
        </li>
        <li
          onClick={() => setActiveTab('riskcenter')}
          style={{
            padding: '8px 12px',
            borderRadius: '6px',
            cursor: 'pointer',
            fontWeight: 600,
            background: activeTab === 'riskcenter' ? 'rgba(56, 189, 248, 0.15)' : 'transparent',
            color: activeTab === 'riskcenter' ? '#38bdf8' : '#94a3b8',
          }}
        >
          🚨 SIEM Risk Center
        </li>
      </ul>

      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', fontSize: '0.75rem', color: '#94a3b8', marginTop: '8px' }}>
        <span>Windows Event Logs Tree</span>
        <span style={{ color: '#38bdf8', fontWeight: 700 }}>{totalCount} System Sources</span>
      </div>

      <div style={{ display: 'flex', flexDirection: 'column', gap: '4px', fontSize: '0.8rem' }}>
        {/* Windows Logs Folder */}
        <div>
          <div
            onClick={() => toggleFolder('winLogs')}
            style={{ display: 'flex', alignItems: 'center', gap: '6px', padding: '6px 8px', borderRadius: '6px', cursor: 'pointer', fontWeight: 700 }}
          >
            <span style={{ fontSize: '0.7rem', transform: collapsedFolders.winLogs ? 'rotate(-90deg)' : 'none', transition: 'transform 0.2s' }}>▼</span>
            📁 Windows Logs
          </div>

          {!collapsedFolders.winLogs && (
            <div style={{ paddingLeft: '18px', borderLeft: '1px dashed rgba(255,255,255,0.1)', marginLeft: '10px', display: 'flex', flexDirection: 'column', gap: '2px' }}>
              {windowsLogs.map((ch) => (
                <div
                  key={ch}
                  onClick={() => { setActiveTab('events'); onSelectChannel(ch); }}
                  style={{
                    padding: '6px 10px',
                    borderRadius: '6px',
                    cursor: 'pointer',
                    fontSize: '0.78rem',
                    background: currentChannel === ch && activeTab === 'events' ? 'rgba(56, 189, 248, 0.15)' : 'transparent',
                    color: currentChannel === ch && activeTab === 'events' ? '#38bdf8' : '#94a3b8',
                  }}
                >
                  {ch}
                </div>
              ))}
            </div>
          )}
        </div>

        {/* Applications and Services Logs Folder */}
        <div>
          <div
            onClick={() => toggleFolder('appServices')}
            style={{ display: 'flex', alignItems: 'center', gap: '6px', padding: '6px 8px', borderRadius: '6px', cursor: 'pointer', fontWeight: 700 }}
          >
            <span style={{ fontSize: '0.7rem', transform: collapsedFolders.appServices ? 'rotate(-90deg)' : 'none', transition: 'transform 0.2s' }}>▼</span>
            📁 Applications and Services Logs
          </div>

          {!collapsedFolders.appServices && (
            <div style={{ paddingLeft: '18px', borderLeft: '1px dashed rgba(255,255,255,0.1)', marginLeft: '10px', display: 'flex', flexDirection: 'column', gap: '2px' }}>
              {/* Microsoft Folder */}
              <div
                onClick={() => toggleFolder('microsoft')}
                style={{ display: 'flex', alignItems: 'center', gap: '6px', padding: '4px 6px', borderRadius: '6px', cursor: 'pointer', fontWeight: 700 }}
              >
                <span style={{ fontSize: '0.7rem', transform: collapsedFolders.microsoft ? 'rotate(-90deg)' : 'none' }}>▼</span>
                📁 Microsoft
              </div>

              {!collapsedFolders.microsoft && (
                <div style={{ paddingLeft: '14px', borderLeft: '1px dashed rgba(255,255,255,0.1)', marginLeft: '8px' }}>
                  <div
                    onClick={() => toggleFolder('windows')}
                    style={{ display: 'flex', alignItems: 'center', gap: '6px', padding: '4px 6px', borderRadius: '6px', cursor: 'pointer', fontWeight: 700 }}
                  >
                    <span style={{ fontSize: '0.7rem', transform: collapsedFolders.windows ? 'rotate(-90deg)' : 'none' }}>▼</span>
                    📁 Windows
                  </div>

                  {!collapsedFolders.windows && (
                    <div style={{ paddingLeft: '14px', borderLeft: '1px dashed rgba(255,255,255,0.1)', marginLeft: '8px', display: 'flex', flexDirection: 'column', gap: '2px' }}>
                      {appServicesLogs
                        .filter((ch) => ch.startsWith('Microsoft-Windows-'))
                        .map((ch) => (
                          <div
                            key={ch}
                            onClick={() => { setActiveTab('events'); onSelectChannel(ch); }}
                            style={{
                              padding: '5px 8px',
                              borderRadius: '6px',
                              cursor: 'pointer',
                              fontSize: '0.76rem',
                              whiteSpace: 'nowrap',
                              overflow: 'hidden',
                              textOverflow: 'ellipsis',
                              background: currentChannel === ch && activeTab === 'events' ? 'rgba(56, 189, 248, 0.15)' : 'transparent',
                              color: currentChannel === ch && activeTab === 'events' ? '#38bdf8' : '#94a3b8',
                            }}
                            title={ch}
                          >
                            {ch}
                          </div>
                        ))}
                    </div>
                  )}
                </div>
              )}

              {/* Other Custom Channels */}
              {appServicesLogs
                .filter((ch) => !ch.startsWith('Microsoft-Windows-'))
                .map((ch) => (
                  <div
                    key={ch}
                    onClick={() => { setActiveTab('events'); onSelectChannel(ch); }}
                    style={{
                      padding: '5px 8px',
                      borderRadius: '6px',
                      cursor: 'pointer',
                      fontSize: '0.76rem',
                      background: currentChannel === ch && activeTab === 'events' ? 'rgba(56, 189, 248, 0.15)' : 'transparent',
                      color: currentChannel === ch && activeTab === 'events' ? '#38bdf8' : '#94a3b8',
                    }}
                  >
                    📁 {ch}
                  </div>
                ))}
            </div>
          )}
        </div>
      </div>
    </aside>
  );
};
