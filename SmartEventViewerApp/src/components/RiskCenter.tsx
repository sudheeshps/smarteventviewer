import React, { useState, useEffect } from 'react';
import type { EventDto } from '../types';
import { fetchApiEvents, formatTo12Hour } from '../apiClient';

interface RiskCenterProps {
  onSelectChannel?: (channelName: string) => void;
}

export const RiskCenter: React.FC<RiskCenterProps> = ({ onSelectChannel }) => {
  const [highRiskEvents, setHighRiskEvents] = useState<EventDto[]>([]);
  const [securityTotal, setSecurityTotal] = useState<number>(0);
  const [sysmonTotal, setSysmonTotal] = useState<number>(0);
  const [systemTotal, setSystemTotal] = useState<number>(0);
  const [totalScanned, setTotalScanned] = useState<number>(0);
  const [isLoading, setIsLoading] = useState<boolean>(true);

  useEffect(() => {
    loadRiskData();
  }, []);

  const loadRiskData = async () => {
    setIsLoading(true);
    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    try {
      // Fetch real security, sysmon, and system events from native backend
      const [secData, sysmonData, sysData] = await Promise.all([
        fetchApiEvents('Security', baseUrl, 1, 50).catch(() => ({ events: [], totalCount: 0 })),
        fetchApiEvents('Microsoft-Windows-Sysmon/Operational', baseUrl, 1, 50).catch(() => ({ events: [], totalCount: 0 })),
        fetchApiEvents('System', baseUrl, 1, 50).catch(() => ({ events: [], totalCount: 0 })),
      ]);

      const mapEvents = (raw: unknown[], channel: string): EventDto[] =>
        ((raw as Array<Record<string, unknown>>) || []).map((e, i) => ({
          idx: (e.index as number) || i + 1,
          id: (e.id as number) || 0,
          level: ((e.level as string) || 'Information') as EventDto['level'],
          risk: ((e.risk as string) || 'Low') as EventDto['risk'],
          provider: (e.provider as string) || channel,
          time: formatTo12Hour(e.time as string),
          desc: (e.message as string) || `Event #${e.id} in ${channel}`,
        }));

      const secMapped = mapEvents(secData.events as unknown[], 'Security');
      const sysmonMapped = mapEvents(sysmonData.events as unknown[], 'Microsoft-Windows-Sysmon/Operational');
      const sysMapped = mapEvents(sysData.events as unknown[], 'System');

      setSecurityTotal(secData.totalCount || secMapped.length);
      setSysmonTotal(sysmonData.totalCount || sysmonMapped.length);
      setSystemTotal(sysData.totalCount || sysMapped.length);

      const allCombined = [...secMapped, ...sysmonMapped, ...sysMapped];
      const highRisk = allCombined.filter(
        (e) => e.risk === 'Critical' || e.risk === 'High' || e.level === 'Critical' || e.level === 'Error' || e.id === 4625 || e.id === 4624 || e.id === 7045
      );

      setHighRiskEvents(highRisk.length > 0 ? highRisk : allCombined.slice(0, 10));
      setTotalScanned((secData.totalCount || 0) + (sysmonData.totalCount || 0) + (sysData.totalCount || 0));
    } catch (err) {
      console.error('[RISK-CENTER DEBUG] Error loading real backend security events:', err);
    } finally {
      setIsLoading(false);
    }
  };

  const criticalCount = highRiskEvents.filter((e) => e.risk === 'Critical' || e.level === 'Critical').length;
  const highCount = highRiskEvents.filter((e) => e.risk === 'High' || e.level === 'Error').length;

  return (
    <div style={{ flex: 1, padding: '16px', display: 'flex', flexDirection: 'column', gap: '12px', overflowY: 'auto', fontSize: '0.78rem' }}>
      <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', background: 'rgba(30, 41, 59, 0.7)', padding: '10px 16px', borderRadius: '8px', border: '1px solid rgba(255,255,255,0.1)' }}>
        <div>
          <h1 style={{ fontSize: '1.1rem', color: '#f87171', margin: 0, fontWeight: 700 }}>
            🚨 SIEM Security Risk Center & Threat Operations
          </h1>
          <p style={{ fontSize: '0.72rem', color: '#94a3b8', margin: '4px 0 0 0' }}>
            Real-time Threat Intelligence & Anomaly Scoring backed by native Windows Event Logs.
          </p>
        </div>
        <button
          onClick={loadRiskData}
          style={{ background: '#f87171', color: '#0f172a', border: 'none', padding: '6px 12px', borderRadius: '4px', cursor: 'pointer', fontWeight: 700, fontSize: '0.72rem' }}
        >
          {isLoading ? 'Scanning Backend...' : '🔄 Refresh Risk Data'}
        </button>
      </header>

      {/* Real Threat Summary Metrics */}
      <section style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '8px' }}>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #f87171', borderRadius: '6px', padding: '10px' }}>
          <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Critical Threat Incidents</span>
          <div style={{ fontSize: '1.2rem', fontWeight: 700, color: '#f87171' }}>{criticalCount}</div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #fbbf24', borderRadius: '6px', padding: '10px' }}>
          <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>High Risk Detection</span>
          <div style={{ fontSize: '1.2rem', fontWeight: 700, color: '#fbbf24' }}>{highCount}</div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #38bdf8', borderRadius: '6px', padding: '10px' }}>
          <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Total Scanned Events</span>
          <div style={{ fontSize: '1.2rem', fontWeight: 700, color: '#38bdf8' }}>{totalScanned.toLocaleString()}</div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #4ade80', borderRadius: '6px', padding: '10px' }}>
          <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Monitored Security Channels</span>
          <div style={{ fontSize: '1.2rem', fontWeight: 700, color: '#4ade80' }}>3 Live Feeds</div>
        </div>
      </section>

      {/* Real High Severity Breaches Table */}
      <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
        <h3 style={{ color: '#f87171', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
          🔥 Active Security Anomalies & Elevated Risk Logs
        </h3>
        <div style={{ overflowY: 'auto', maxHeight: '300px' }}>
          <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
            <thead>
              <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem', position: 'sticky', top: 0 }}>
                <th style={{ padding: '6px' }}>Index</th>
                <th style={{ padding: '6px' }}>Event ID</th>
                <th style={{ padding: '6px' }}>Risk Severity</th>
                <th style={{ padding: '6px' }}>Provider Source</th>
                <th style={{ padding: '6px' }}>Timestamp</th>
                <th style={{ padding: '6px' }}>Description</th>
              </tr>
            </thead>
            <tbody>
              {highRiskEvents.length === 0 ? (
                <tr>
                  <td colSpan={6} style={{ padding: '12px', textAlign: 'center', color: '#94a3b8' }}>
                    {isLoading ? 'Loading real security logs from backend...' : 'No critical anomalies detected in current event buffer.'}
                  </td>
                </tr>
              ) : (
                highRiskEvents.map((evt, i) => (
                  <tr key={i} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                    <td style={{ padding: '5px' }}>#{evt.idx}</td>
                    <td style={{ padding: '5px', fontWeight: 700, color: '#38bdf8' }}>{evt.id}</td>
                    <td style={{ padding: '5px' }}>
                      <span
                        style={{
                          padding: '1px 6px',
                          borderRadius: '3px',
                          fontSize: '0.62rem',
                          fontWeight: 700,
                          background: evt.risk === 'Critical' || evt.level === 'Critical' ? 'rgba(248,113,113,0.25)' : 'rgba(251,191,36,0.25)',
                          color: evt.risk === 'Critical' || evt.level === 'Critical' ? '#f87171' : '#fbbf24',
                        }}
                      >
                        {evt.risk || evt.level}
                      </span>
                    </td>
                    <td style={{ padding: '5px' }}>{evt.provider}</td>
                    <td style={{ padding: '5px' }}>{evt.time}</td>
                    <td style={{ padding: '5px', color: '#e2e8f0' }}>{evt.desc}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </section>

      {/* Live Channel Feeds Summary */}
      <section style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '8px' }}>
        <div
          onClick={() => onSelectChannel && onSelectChannel('Security')}
          style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(56, 189, 248, 0.3)', borderRadius: '6px', padding: '10px', cursor: 'pointer', transition: 'transform 0.15s' }}
          title="Click to open Security Event Explorer"
        >
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <h4 style={{ color: '#38bdf8', margin: '0 0 4px 0', fontSize: '0.78rem' }}>🛡️ Security Log Channel</h4>
            <span style={{ fontSize: '0.62rem', color: '#38bdf8', fontWeight: 600 }}>🔍 Open</span>
          </div>
          <p style={{ margin: 0, color: '#f8fafc', fontSize: '1rem', fontWeight: 700 }}>{securityTotal.toLocaleString()} <span style={{ fontSize: '0.65rem', color: '#94a3b8', fontWeight: 400 }}>Total Ingested Events</span></p>
        </div>
        <div
          onClick={() => onSelectChannel && onSelectChannel('Microsoft-Windows-Sysmon/Operational')}
          style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(56, 189, 248, 0.3)', borderRadius: '6px', padding: '10px', cursor: 'pointer', transition: 'transform 0.15s' }}
          title="Click to open Sysmon Operational Event Explorer"
        >
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <h4 style={{ color: '#38bdf8', margin: '0 0 4px 0', fontSize: '0.78rem' }}>⚙️ Sysmon Operation Channel</h4>
            <span style={{ fontSize: '0.62rem', color: '#38bdf8', fontWeight: 600 }}>🔍 Open</span>
          </div>
          <p style={{ margin: 0, color: '#f8fafc', fontSize: '1rem', fontWeight: 700 }}>{sysmonTotal.toLocaleString()} <span style={{ fontSize: '0.65rem', color: '#94a3b8', fontWeight: 400 }}>Total System Monitor Logs</span></p>
        </div>
        <div
          onClick={() => onSelectChannel && onSelectChannel('System')}
          style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(56, 189, 248, 0.3)', borderRadius: '6px', padding: '10px', cursor: 'pointer', transition: 'transform 0.15s' }}
          title="Click to open System Event Explorer"
        >
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <h4 style={{ color: '#38bdf8', margin: '0 0 4px 0', fontSize: '0.78rem' }}>🖥️ System Log Channel</h4>
            <span style={{ fontSize: '0.62rem', color: '#38bdf8', fontWeight: 600 }}>🔍 Open</span>
          </div>
          <p style={{ margin: 0, color: '#f8fafc', fontSize: '1rem', fontWeight: 700 }}>{systemTotal.toLocaleString()} <span style={{ fontSize: '0.65rem', color: '#94a3b8', fontWeight: 400 }}>Total Service & Kernel Logs</span></p>
        </div>
      </section>
    </div>
  );
};

export default RiskCenter;
