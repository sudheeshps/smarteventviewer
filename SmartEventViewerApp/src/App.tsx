import { useState, useEffect } from 'react';
import { Sidebar } from './components/Sidebar';
import { Dashboard } from './components/Dashboard';
import { EventsExplorer } from './components/EventsExplorer';
import { RiskCenter } from './components/RiskCenter';
import { ServerLogsViewer } from './components/ServerLogsViewer';
import { fetchApiChannels, fetchApiAnalyze, fetchApiAnalyzeStatus } from './apiClient';

export function App() {
  const [activeTab, setActiveTab] = useState<'dashboard' | 'events' | 'riskcenter' | 'serverlogs'>('dashboard');
  const [currentChannel, setCurrentChannel] = useState<string>('Security');
  const [sidebarWidth, setSidebarWidth] = useState<number>(300);
  const [isResizing, setIsResizing] = useState<boolean>(false);
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

  const handleMouseDown = () => {
    setIsResizing(true);
  };

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (!isResizing) return;
      const newWidth = Math.max(180, Math.min(650, e.clientX));
      setSidebarWidth(newWidth);
    };

    const handleMouseUp = () => {
      setIsResizing(false);
    };

    if (isResizing) {
      window.addEventListener('mousemove', handleMouseMove);
      window.addEventListener('mouseup', handleMouseUp);
    }
    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };
  }, [isResizing]);

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

  const [showChatPanel, setShowChatPanel] = useState<boolean>(false);
  const [isChatCollapsed, setIsChatCollapsed] = useState<boolean>(false);
  const [chatHistory, setChatHistory] = useState<Array<{ sender: 'user' | 'assistant'; text: string; channel?: string }>>([]);

  const [isChatAnalyzing, setIsChatAnalyzing] = useState<boolean>(false);
  const [chatProgressMessage, setChatProgressMessage] = useState<string>('Initializing RAG analysis...');
  const [chatDownloadProgress, setChatDownloadProgress] = useState<number>(0);

  return (
    <div style={{ display: 'flex', height: '100vh', width: '100vw', backgroundColor: '#0f172a', color: '#f8fafc', overflow: 'hidden', userSelect: isResizing ? 'none' : 'auto' }}>
      <div style={{ width: `${sidebarWidth}px`, display: 'flex', flexShrink: 0 }}>
        <Sidebar
          activeTab={activeTab}
          setActiveTab={setActiveTab}
          currentChannel={currentChannel}
          onSelectChannel={(ch) => setCurrentChannel(ch)}
          windowsLogs={windowsLogs}
          appServicesLogs={appServicesLogs}
        />
      </div>

      {/* Resizable Vertical Splitter */}
      <div
        onMouseDown={handleMouseDown}
        style={{
          width: '6px',
          cursor: 'col-resize',
          background: isResizing ? '#38bdf8' : 'rgba(255, 255, 255, 0.08)',
          transition: 'background 0.15s',
          zIndex: 10,
        }}
        title="Drag to resize channel tree panel"
      />

      <main style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden', minHeight: 0, position: 'relative' }}>
        <div style={{ display: activeTab === 'dashboard' ? 'flex' : 'none', flex: 1, flexDirection: 'column', minHeight: 0 }}>
          <Dashboard
            onSelectChannel={(ch) => {
              setCurrentChannel(ch);
              setActiveTab('events');
            }}
          />
        </div>

        <div style={{ display: activeTab === 'events' ? 'flex' : 'none', flex: 1, flexDirection: 'column', minHeight: 0 }}>
          <EventsExplorer
            channelName={currentChannel}
            onOpenChat={(initialQuery, responseText) => {
              setShowChatPanel(true);
              setIsChatCollapsed(false);
              const isProgressUpdate = responseText.startsWith('⏳ ');
              if (isProgressUpdate) {
                setIsChatAnalyzing(true);
                const cleanMsg = responseText.replace('⏳ ', '');
                setChatProgressMessage(cleanMsg);
                setChatHistory((prev) => {
                  const hasUserMsg = prev.some((m) => m.sender === 'user' && m.text === initialQuery);
                  if (hasUserMsg) {
                    return [...prev]; // Return new array reference to guarantee React state re-render
                  }
                  return [...prev, { sender: 'user' as const, text: initialQuery, channel: currentChannel }];
                });
              } else {
                setIsChatAnalyzing(false);
                setChatHistory((prev) => {
                  const hasUserMsg = prev.some((m) => m.sender === 'user' && m.text === initialQuery);
                  const userEntry = { sender: 'user' as const, text: initialQuery, channel: currentChannel };
                  const assistantEntry = { sender: 'assistant' as const, text: responseText, channel: currentChannel };
                  const newHistory = hasUserMsg ? prev : [...prev, userEntry];
                  return [...newHistory, assistantEntry];
                });
              }
            }}
          />
        </div>

        <div style={{ display: activeTab === 'riskcenter' ? 'flex' : 'none', flex: 1, flexDirection: 'column', minHeight: 0 }}>
          <RiskCenter
            onSelectChannel={(ch) => {
              setCurrentChannel(ch);
              setActiveTab('events');
            }}
          />
        </div>

        <div style={{ display: activeTab === 'serverlogs' ? 'flex' : 'none', flex: 1, flexDirection: 'column', minHeight: 0 }}>
          <ServerLogsViewer />
        </div>

        {/* Floating Chat Drawer Toggle Button when collapsed or hidden */}
        {(!showChatPanel || isChatCollapsed) && (
          <button
            onClick={() => {
              setShowChatPanel(true);
              setIsChatCollapsed(false);
            }}
            style={{
              position: 'absolute',
              bottom: '20px',
              right: '20px',
              background: '#0284c7',
              color: '#ffffff',
              border: '1px solid #38bdf8',
              borderRadius: '24px',
              padding: '10px 18px',
              fontSize: '0.8rem',
              fontWeight: 700,
              cursor: 'pointer',
              boxShadow: '0 4px 14px rgba(0,0,0,0.5)',
              display: 'flex',
              alignItems: 'center',
              gap: '8px',
              zIndex: 40,
              transition: 'transform 0.15s, background 0.15s',
            }}
            title="Expand AI Threat Security Analyst Chat Drawer"
          >
            <span>🤖 AI Security Analyst</span>
            {chatHistory.length > 0 && (
              <span style={{ background: '#38bdf8', color: '#0f172a', borderRadius: '10px', padding: '1px 6px', fontSize: '0.65rem' }}>
                {chatHistory.length}
              </span>
            )}
          </button>
        )}
      </main>

      {/* Right-Side RAG AI Threat Assistant Chat Drawer */}
      {showChatPanel && !isChatCollapsed && (
        <aside style={{ width: '420px', background: '#0f172a', borderLeft: '1px solid #38bdf8', display: 'flex', flexDirection: 'column', zIndex: 30, boxShadow: '-4px 0 20px rgba(0,0,0,0.5)' }}>
          {/* Chat Drawer Header */}
          <div style={{ background: 'rgba(30, 41, 59, 0.9)', padding: '10px 14px', borderBottom: '1px solid rgba(255,255,255,0.1)', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <div style={{ fontWeight: 700, fontSize: '0.85rem', color: '#38bdf8', display: 'flex', alignItems: 'center', gap: '6px' }}>
              🤖 AI Threat Security Analyst (RAG Engine)
            </div>
            <div style={{ display: 'flex', gap: '8px', alignItems: 'center' }}>
              <button
                onClick={() => setIsChatCollapsed(true)}
                style={{ background: 'transparent', border: 'none', color: '#94a3b8', fontSize: '1rem', cursor: 'pointer', fontWeight: 700 }}
                title="Collapse Chat Pane"
              >
                ──
              </button>
              <button
                onClick={() => setShowChatPanel(false)}
                style={{ background: 'transparent', border: 'none', color: '#94a3b8', fontSize: '1.1rem', cursor: 'pointer', fontWeight: 700 }}
                title="Close Chat Drawer"
              >
                ✕
              </button>
            </div>
          </div>

          {/* Chat Conversation Thread */}
          <div style={{ flex: 1, padding: '12px', overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: '10px' }}>
            {chatHistory.length === 0 ? (
              <div style={{ color: '#94a3b8', fontStyle: 'italic', fontSize: '0.75rem', textAlign: 'center', marginTop: '20px' }}>
                Ask any question or click "Analyze Events" to start RAG security analysis...
              </div>
            ) : (
              chatHistory.map((msg, idx) => (
                <div
                  key={idx}
                  style={{
                    alignSelf: msg.sender === 'user' ? 'flex-end' : 'flex-start',
                    maxWidth: '90%',
                    background: msg.sender === 'user' ? '#0369a1' : 'rgba(30, 41, 59, 0.9)',
                    color: '#f8fafc',
                    borderRadius: '8px',
                    padding: '8px 12px',
                    fontSize: '0.75rem',
                    border: msg.sender === 'user' ? '1px solid #38bdf8' : '1px solid rgba(255,255,255,0.1)',
                  }}
                >
                  <div style={{ fontSize: '0.65rem', color: msg.sender === 'user' ? '#7dd3fc' : '#38bdf8', fontWeight: 700, marginBottom: '4px' }}>
                    {msg.sender === 'user' ? '👤 User Query' : '🤖 AI Security Response'}
                  </div>
                  <div style={{ whiteSpace: 'pre-wrap', lineHeight: '1.4' }}>{msg.text}</div>
                </div>
              ))
            )}
            {isChatAnalyzing && (
              <div
                style={{
                  alignSelf: 'flex-start',
                  background: 'linear-gradient(135deg, rgba(30, 41, 59, 0.95), rgba(15, 23, 42, 0.95))',
                  color: '#38bdf8',
                  borderRadius: '10px',
                  padding: '10px 14px',
                  fontSize: '0.75rem',
                  border: '1px solid #38bdf8',
                  boxShadow: '0 4px 14px rgba(56, 189, 248, 0.25)',
                  display: 'flex',
                  flexDirection: 'column',
                  gap: '8px',
                  minWidth: '260px',
                  animation: 'pulseGlow 2s ease-in-out infinite',
                }}
              >
                <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
                  <div
                    style={{
                      width: '18px',
                      height: '18px',
                      border: '2px solid rgba(56, 189, 248, 0.2)',
                      borderTop: '2px solid #38bdf8',
                      borderRight: '2px solid #38bdf8',
                      borderRadius: '50%',
                      animation: 'spin 0.7s cubic-bezier(0.4, 0, 0.2, 1) infinite',
                      flexShrink: 0,
                    }}
                  />
                  <div style={{ display: 'flex', flexDirection: 'column', gap: '2px' }}>
                    <div style={{ fontWeight: 700, fontSize: '0.78rem', color: '#38bdf8', display: 'flex', alignItems: 'center', gap: '6px' }}>
                      <span>{chatDownloadProgress > 0 ? '📥 Downloading AI Model Weights...' : '⚡ RAG AI Engine Analyzing Logs...'}</span>
                    </div>
                    <div style={{ fontSize: '0.68rem', color: '#94a3b8', fontStyle: 'italic' }}>
                      {chatProgressMessage === 'Task enqueued for processing.' ? 'Evaluating event logs & scanning vectors...' : chatProgressMessage}
                    </div>
                  </div>
                </div>

                {chatDownloadProgress > 0 && chatDownloadProgress <= 100 && (
                  <div style={{ width: '100%', marginTop: '4px' }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.65rem', color: '#38bdf8', marginBottom: '3px', fontWeight: 600 }}>
                      <span>📥 Downloading GGUF Model Weights</span>
                      <span>{Math.round(chatDownloadProgress)}%</span>
                    </div>
                    <div style={{ width: '100%', height: '6px', background: 'rgba(15, 23, 42, 0.8)', borderRadius: '3px', overflow: 'hidden', border: '1px solid rgba(56, 189, 248, 0.3)' }}>
                      <div
                        style={{
                          width: `${Math.min(100, Math.max(0, chatDownloadProgress))}%`,
                          height: '100%',
                          background: 'linear-gradient(90deg, #38bdf8, #818cf8)',
                          borderRadius: '3px',
                          transition: 'width 0.2s ease-out',
                        }}
                      />
                    </div>
                  </div>
                )}
              </div>
            )}
          </div>

          {/* Chat Follow-up Input Box */}
          <div style={{ padding: '10px', background: 'rgba(30, 41, 59, 0.9)', borderTop: '1px solid rgba(255,255,255,0.1)', display: 'flex', gap: '6px' }}>
            <input
              type="text"
              disabled={isChatAnalyzing}
              placeholder={isChatAnalyzing ? chatProgressMessage : 'Ask a question (context & history preserved)...'}
              onKeyDown={async (e) => {
                if (e.key === 'Enter' && e.currentTarget.value.trim() && !isChatAnalyzing) {
                  const queryText = e.currentTarget.value.trim();
                  e.currentTarget.value = '';
                  let finalResult = 'Analysis complete.';
                  setChatHistory((prev) => [...prev, { sender: 'user', text: queryText }]);
                  setIsChatAnalyzing(true);
                  setChatProgressMessage('Enqueued for analysis...');
                  setChatDownloadProgress(0);
                  
                  setTimeout(async () => {
                    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
                    try {
                      const enqueueRes = await fetchApiAnalyze(currentChannel || 'ALL', queryText, baseUrl);
                      const taskId = enqueueRes.taskId;
                      if (!taskId) {
                        setChatHistory((prev) => [...prev, { sender: 'assistant', text: enqueueRes.analysis || 'Error initiating task.' }]);
                      } else {
                        let isCompleted = false;
                        let statusRes = enqueueRes;
                        while (!isCompleted) {
                          await new Promise((resolve) => setTimeout(resolve, 300));
                          statusRes = await fetchApiAnalyzeStatus(taskId, baseUrl);
                          if (statusRes.progressMessage) {
                            setChatProgressMessage(statusRes.progressMessage);
                          }
                          if (statusRes.downloadProgress !== undefined) {
                            setChatDownloadProgress(statusRes.downloadProgress);
                          }
                          if (statusRes.status === 'COMPLETED' || statusRes.status === 'FAILED') {
                            isCompleted = true;
                          }
                        }
                        finalResult = (statusRes.analysis && statusRes.analysis.trim().length > 0) ? statusRes.analysis : 'Analysis complete (no anomalies flagged).';
                        setChatHistory((prev) => [...prev, { sender: 'assistant', text: finalResult }]);
                      }
                    } catch (err) {
                      setChatHistory((prev) => [...prev, { sender: 'assistant', text: 'Error connecting to RAG engine.' }]);
                    } finally {
                      setIsChatAnalyzing(false);
                      setChatDownloadProgress(0);
                    }
                  }, 0);
                }
              }}
              style={{
                flex: 1,
                background: isChatAnalyzing ? '#1e293b' : '#0f172a',
                color: isChatAnalyzing ? '#94a3b8' : '#f8fafc',
                border: '1px solid rgba(255,255,255,0.1)',
                borderRadius: '4px',
                padding: '6px 10px',
                fontSize: '0.72rem',
                outline: 'none',
                cursor: isChatAnalyzing ? 'not-allowed' : 'text',
              }}
            />
          </div>
        </aside>
      )}
    </div>
  );
}

export default App;
