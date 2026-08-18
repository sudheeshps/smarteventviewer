import React, { useState, useEffect, useRef } from 'react';
import { fetchApiServerLogs, fetchApiLogFormat } from '../apiClient';
import type { LogColumnFormat, LogRecordData } from '../apiClient';
import { formatUtcToLocal } from '../utils/timeUtils';

interface ServerLogsViewerProps {
  onClose?: () => void;
}

export const ServerLogsViewer: React.FC<ServerLogsViewerProps> = ({ onClose }) => {
  const [columns, setColumns] = useState<LogColumnFormat[]>([]);
  const [records, setRecords] = useState<LogRecordData[]>([]);
  const [autoRefresh, setAutoRefresh] = useState<boolean>(true);
  const [filterText, setFilterText] = useState<string>('');
  const logContainerRef = useRef<HTMLDivElement>(null);

  const fetchFormat = async () => {
    try {
      const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
      const data = await fetchApiLogFormat(baseUrl);
      if (data.columns && data.columns.length > 0) {
        setColumns(data.columns);
      } else {
        setColumns([
          { key: 'timestamp', headerName: 'Timestamp', type: 'timestamp', widthPx: 180 },
          { key: 'level', headerName: 'Level', type: 'level', widthPx: 80 },
          { key: 'processId', headerName: 'Process ID', type: 'number', widthPx: 90 },
          { key: 'threadId', headerName: 'Thread ID', type: 'number', widthPx: 90 },
          { key: 'category', headerName: 'Category', type: 'string', widthPx: 140 },
          { key: 'message', headerName: 'Message', type: 'string', widthPx: 450 }
        ]);
      }
    } catch {
      setColumns([
        { key: 'timestamp', headerName: 'Timestamp', type: 'timestamp', widthPx: 180 },
        { key: 'level', headerName: 'Level', type: 'level', widthPx: 80 },
        { key: 'processId', headerName: 'Process ID', type: 'number', widthPx: 90 },
        { key: 'threadId', headerName: 'Thread ID', type: 'number', widthPx: 90 },
        { key: 'category', headerName: 'Category', type: 'string', widthPx: 140 },
        { key: 'message', headerName: 'Message', type: 'string', widthPx: 450 }
      ]);
    }
  };

  const fetchLogs = async () => {
    try {
      const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
      const data = await fetchApiServerLogs(baseUrl);
      if (data.records) {
        setRecords(data.records);
      }
    } catch (e) {
      console.error('Error fetching server logs:', e);
    }
  };

  useEffect(() => {
    fetchFormat();
    fetchLogs();
    if (!autoRefresh) return;
    const interval = setInterval(fetchLogs, 2000);
    return () => clearInterval(interval);
  }, [autoRefresh]);

  useEffect(() => {
    if (logContainerRef.current) {
      logContainerRef.current.scrollTop = 0;
    }
  }, [records]);

  const filteredRecords = records.filter(rec =>
    rec.message.toLowerCase().includes(filterText.toLowerCase()) ||
    rec.category.toLowerCase().includes(filterText.toLowerCase()) ||
    rec.level.toLowerCase().includes(filterText.toLowerCase())
  );

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
      fontFamily: 'Inter, system-ui, sans-serif'
    }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '1px solid #334155', paddingBottom: '10px' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
          <span style={{ fontSize: '1.2rem' }}>📜</span>
          <h3 style={{ margin: 0, fontSize: '1.1rem', color: '#38bdf8' }}>Server Diagnostics & Model Logs</h3>
          <span style={{ fontSize: '0.75rem', background: '#1e293b', padding: '2px 8px', borderRadius: '12px', color: '#94a3b8' }}>
            {records.length} entries
          </span>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
          <input
            type="text"
            placeholder="Filter log records..."
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
          fontSize: '0.78rem',
        }}
      >
        <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left' }}>
          <thead>
            <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', borderBottom: '1px solid #334155' }}>
              {columns.map((col) => (
                <th key={col.key} style={{ padding: '8px', minWidth: `${col.widthPx}px` }}>
                  {col.headerName}
                </th>
              ))}
            </tr>
          </thead>
          <tbody>
            {filteredRecords.length === 0 ? (
              <tr>
                <td colSpan={columns.length || 4} style={{ padding: '16px', color: '#64748b', fontStyle: 'italic', textAlign: 'center' }}>
                  No server log entries recorded yet.
                </td>
              </tr>
            ) : (
              filteredRecords.map((rec, idx) => {
                const lvlColor =
                  rec.level === 'ERROR' ? '#f87171' :
                  rec.level === 'WARN' ? '#fbbf24' :
                  rec.level === 'DEBUG' ? '#a7f3d0' : '#38bdf8';

                return (
                  <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.03)' }}>
                    {columns.map((col) => {
                      const recObj = rec as unknown as Record<string, unknown>;
                      const rawVal = recObj[col.key] ?? recObj[col.key.toLowerCase()] ?? recObj[col.key.toUpperCase()];
                      let cellVal = (rawVal !== undefined && rawVal !== null && rawVal !== '') ? String(rawVal) : '-';
                      if (col.key === 'timestamp' || col.type === 'timestamp') {
                        cellVal = (!rawVal || rawVal === '' || rawVal === '-') ? '-' : formatUtcToLocal(rawVal as string);
                      }

                      return (
                        <td key={col.key} style={{ padding: '6px 8px', color: col.key === 'level' ? lvlColor : '#e2e8f0', whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}>
                          {cellVal}
                        </td>
                      );
                    })}
                  </tr>
                );
              })
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
};
