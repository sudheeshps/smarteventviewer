import React, { useState, useEffect, useRef } from 'react';
import { fetchApiServerLogs } from '../apiClient';

interface ServerLogsViewerProps {
  onClose?: () => void;
}

export const ServerLogsViewer: React.FC<ServerLogsViewerProps> = ({ onClose }) => {
  const [logs, setLogs] = useState<string[]>([]);
  const [autoRefresh, setAutoRefresh] = useState<boolean>(true);
  const [filterText, setFilterText] = useState<string>('');
  const logContainerRef = useRef<HTMLDivElement>(null);

  const fetchLogs = async () => {
    try {
      const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
      const data = await fetchApiServerLogs(baseUrl);
      if (data.logs) {
        setLogs(data.logs);
      }
    } catch (e) {
      console.error('Error fetching server logs:', e);
    }
  };

  useEffect(() => {
    fetchLogs();
    if (!autoRefresh) return;
    const interval = setInterval(fetchLogs, 2000);
    return () => clearInterval(interval);
  }, [autoRefresh]);

  useEffect(() => {
    if (logContainerRef.current) {
      logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
    }
  }, [logs]);

  const filteredLogs = logs.filter(line => line.toLowerCase().includes(filterText.toLowerCase()));

  return (
    <div style={{
      background: '#0f172a',
      borderRadius: '8px',
      border: '1px solid #334155',
      padding: '16px',
      display: 'flex',
      flexDirection: 'column',
      gap: '12px',
      height: '100%',
      minHeight: '400px',
      color: '#f8fafc',
      fontFamily: 'Consolas, Monaco, "Andale Mono", "Ubuntu Mono", monospace'
    }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '1px solid #334155', paddingBottom: '10px' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
          <span style={{ fontSize: '1.2rem' }}>📜</span>
          <h3 style={{ margin: 0, fontSize: '1.1rem', color: '#38bdf8' }}>Server Diagnostics & Model Logs</h3>
          <span style={{ fontSize: '0.75rem', background: '#1e293b', padding: '2px 8px', borderRadius: '12px', color: '#94a3b8' }}>
            {logs.length} entries
          </span>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
          <input
            type="text"
            placeholder="Filter logs..."
            value={filterText}
            onChange={(e) => setFilterText(e.target.value)}
            style={{
              background: '#1e293b',
              border: '1px solid #475569',
              borderRadius: '4px',
              color: '#f8fafc',
              padding: '4px 8px',
              fontSize: '0.8rem'
            }}
          />
          <label style={{ fontSize: '0.8rem', color: '#94a3b8', display: 'flex', alignItems: 'center', gap: '4px', cursor: 'pointer' }}>
            <input
              type="checkbox"
              checked={autoRefresh}
              onChange={(e) => setAutoRefresh(e.target.checked)}
            />
            Auto-refresh (2s)
          </label>
          <button
            onClick={fetchLogs}
            style={{
              background: '#0284c7',
              border: 'none',
              borderRadius: '4px',
              color: '#ffffff',
              padding: '4px 10px',
              fontSize: '0.8rem',
              cursor: 'pointer'
            }}
          >
            🔄 Refresh
          </button>
          {onClose && (
            <button
              onClick={onClose}
              style={{
                background: '#dc2626',
                border: 'none',
                borderRadius: '4px',
                color: '#ffffff',
                padding: '4px 10px',
                fontSize: '0.8rem',
                cursor: 'pointer'
              }}
            >
              ✖ Close
            </button>
          )}
        </div>
      </div>

      <div
        ref={logContainerRef}
        style={{
          flex: 1,
          background: '#020617',
          borderRadius: '6px',
          border: '1px solid #1e293b',
          padding: '12px',
          overflowY: 'auto',
          maxHeight: '600px',
          fontSize: '0.82rem',
          lineHeight: '1.5'
        }}
      >
        {filteredLogs.length === 0 ? (
          <div style={{ color: '#64748b', fontStyle: 'italic' }}>
            No server log entries recorded yet.
          </div>
        ) : (
          filteredLogs.map((logLine, idx) => {
            let color = '#cbd5e1';
            if (logLine.includes('[AI_ENGINE]') || logLine.includes('LLAMA')) color = '#38bdf8';
            else if (logLine.includes('[WARNING]') || logLine.includes('WARN')) color = '#fbbf24';
            else if (logLine.includes('[ERROR]') || logLine.includes('FAIL')) color = '#f87171';
            else if (logLine.includes('[SERVER]')) color = '#a7f3d0';

            return (
              <div key={idx} style={{ color, whiteSpace: 'pre-wrap', wordBreak: 'break-word', borderBottom: '1px solid rgba(255,255,255,0.03)', padding: '2px 0' }}>
                {logLine}
              </div>
            );
          })
        )}
      </div>
    </div>
  );
};
