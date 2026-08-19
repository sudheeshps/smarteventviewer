import React, { useState, useEffect } from 'react';
import type { EventDto, MultiChannelAnomaliesDto } from '../types';
import {
  fetchCrossChannelAnomalies,
  fetchPostureReport,
  fetchApiAnalyze,
  fetchApiAnalyzeStatus,
  formatTo12Hour,
} from '../apiClient';
import type {
  TelemetryPostureReportData,
  ProcessAnomalyData,
  SessionAnomalyData,
  UserAnomalyData,
  ServiceAnomalyData,
} from '../apiClient';

interface RiskCenterProps {
  onSelectChannel?: (channelName: string) => void;
}

export const RiskCenter: React.FC<RiskCenterProps> = ({ onSelectChannel }) => {
  const [activeSubtab, setActiveSubtab] = useState<'events' | 'processes' | 'sessions' | 'users' | 'ai'>('events');
  const [anomalies, setAnomalies] = useState<MultiChannelAnomaliesDto>({
    securityEvents: [],
    systemEvents: [],
    applicationEvents: [],
    sysmonEvents: [],
    totalCriticalCount: 0,
    totalErrorCount: 0,
    totalWarningCount: 0,
  });

  const [posture, setPosture] = useState<TelemetryPostureReportData>({
    flaggedProcesses: [],
    suspiciousSessions: [],
    flaggedUsers: [],
    suspiciousServices: [],
    threatScore: 0,
    overallRisk: 'LOW',
  });

  const [isLoading, setIsLoading] = useState<boolean>(true);
  const [selectedChannelFilter, setSelectedChannelFilter] = useState<'ALL' | 'Security' | 'System' | 'Application' | 'Sysmon'>('ALL');

  // AI SIEM Analysis State
  const [aiQuery, setAiQuery] = useState<string>('Full-Spectrum Host Threat & SIEM Assessment');
  const [isAnalyzing, setIsAnalyzing] = useState<boolean>(false);
  const [aiProgressMessage, setAiProgressMessage] = useState<string>('');
  const [aiReport, setAiReport] = useState<string>('');
  const [copiedReport, setCopiedReport] = useState<boolean>(false);

  useEffect(() => {
    loadAllThreatData();
  }, []);

  const loadAllThreatData = async () => {
    setIsLoading(true);
    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    try {
      const [anomData, postData] = await Promise.all([
        fetchCrossChannelAnomalies(20, baseUrl).catch(() => ({
          securityEvents: [],
          systemEvents: [],
          applicationEvents: [],
          sysmonEvents: [],
          totalCriticalCount: 0,
          totalErrorCount: 0,
          totalWarningCount: 0,
        })),
        fetchPostureReport(baseUrl).catch(() => ({
          flaggedProcesses: [],
          suspiciousSessions: [],
          flaggedUsers: [],
          suspiciousServices: [],
          threatScore: 0,
          overallRisk: 'LOW' as const,
        })),
      ]);

      setAnomalies(anomData);
      setPosture(postData);
    } catch (err) {
      console.error('[RISK_CENTER] Failed to fetch cross-channel threat data:', err);
    } finally {
      setIsLoading(false);
    }
  };

  const handleRunAiAnalysis = async (queryToRun?: string) => {
    const q = queryToRun || aiQuery || 'Full-Spectrum Host Threat & SIEM Assessment';
    setIsAnalyzing(true);
    setAiProgressMessage('Enqueuing full-spectrum SIEM cross-correlation task...');
    setAiReport('');
    setActiveSubtab('ai');

    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    try {
      const enqueueRes = await fetchApiAnalyze('All Channels (SIEM)', q, baseUrl);
      const taskId = enqueueRes.taskId;

      if (!taskId) {
        setAiReport(enqueueRes.analysis || 'Analysis completed.');
        setIsAnalyzing(false);
        return;
      }

      // Poll task status
      let pollCount = 0;
      const pollInterval = setInterval(async () => {
        pollCount++;
        try {
          const statusRes = await fetchApiAnalyzeStatus(taskId, baseUrl);
          setAiProgressMessage(statusRes.progressMessage || 'Synthesizing MITRE ATT&CK mappings and telemetry...');

          if (statusRes.status === 'COMPLETED') {
            clearInterval(pollInterval);
            setAiReport(statusRes.analysis || '');
            setIsAnalyzing(false);
          } else if (statusRes.status === 'FAILED' || pollCount > 30) {
            clearInterval(pollInterval);
            setAiReport(statusRes.analysis || 'Inference completed or timed out.');
            setIsAnalyzing(false);
          }
        } catch {
          clearInterval(pollInterval);
          setIsAnalyzing(false);
        }
      }, 600);
    } catch (err) {
      setAiReport(`Failed to initiate AI SIEM investigation: ${err}`);
      setIsAnalyzing(false);
    }
  };

  const handleCopyReport = () => {
    if (!aiReport) return;
    navigator.clipboard.writeText(aiReport);
    setCopiedReport(true);
    setTimeout(() => setCopiedReport(false), 2000);
  };

  const allEventsCombined: Array<EventDto & { channelTag: string }> = [
    ...anomalies.securityEvents.map((e) => ({ ...e, time: formatTo12Hour(e.time), channelTag: 'Security' })),
    ...anomalies.systemEvents.map((e) => ({ ...e, time: formatTo12Hour(e.time), channelTag: 'System' })),
    ...anomalies.applicationEvents.map((e) => ({ ...e, time: formatTo12Hour(e.time), channelTag: 'Application' })),
    ...anomalies.sysmonEvents.map((e) => ({ ...e, time: formatTo12Hour(e.time), channelTag: 'Sysmon' })),
  ];

  const filteredEvents = selectedChannelFilter === 'ALL'
    ? allEventsCombined
    : allEventsCombined.filter((e) => e.channelTag === selectedChannelFilter);

  const getRiskColor = (risk: string) => {
    switch (risk.toUpperCase()) {
      case 'CRITICAL': return '#ef4444';
      case 'HIGH': return '#f97316';
      case 'MEDIUM': return '#eab308';
      default: return '#22c55e';
    }
  };

  const threatScoreColor = posture.threatScore >= 70 ? '#ef4444' : posture.threatScore >= 40 ? '#f97316' : posture.threatScore >= 15 ? '#eab308' : '#22c55e';

  return (
    <div style={{ flex: 1, padding: '16px', display: 'flex', flexDirection: 'column', gap: '14px', overflowY: 'auto', fontSize: '0.8rem', background: '#0b1120', color: '#f8fafc' }}>
      {/* Header & SIEM Posture Banner */}
      <header style={{
        background: 'linear-gradient(135deg, rgba(30, 41, 59, 0.9) 0%, rgba(15, 23, 42, 0.95) 100%)',
        padding: '14px 18px',
        borderRadius: '10px',
        border: '1px solid rgba(255, 255, 255, 0.1)',
        display: 'flex',
        flexWrap: 'wrap',
        justifyContent: 'space-between',
        alignItems: 'center',
        gap: '14px',
        boxShadow: '0 4px 16px rgba(0, 0, 0, 0.4)',
      }}>
        <div style={{ display: 'flex', flexDirection: 'column', gap: '6px', minWidth: '280px', flex: '1 1 auto' }}>
          <div style={{ display: 'flex', alignItems: 'center', flexWrap: 'wrap', gap: '10px' }}>
            <h1 style={{ fontSize: '1.2rem', color: '#f87171', margin: 0, fontWeight: 800, letterSpacing: '-0.02em' }}>
              🚨 SIEM Threat Intelligence & AI Analysis Operations
            </h1>
            <span style={{
              background: `rgba(${posture.overallRisk === 'CRITICAL' ? '239,68,68' : posture.overallRisk === 'HIGH' ? '249,115,22' : '34,197,94'}, 0.15)`,
              color: threatScoreColor,
              padding: '3px 10px',
              borderRadius: '6px',
              fontWeight: 800,
              fontSize: '0.72rem',
              border: `1px solid ${threatScoreColor}`,
              display: 'inline-flex',
              alignItems: 'center',
              gap: '6px',
              letterSpacing: '0.02em',
              whiteSpace: 'nowrap',
              flexShrink: 0,
            }}>
              <span style={{
                width: '6px',
                height: '6px',
                borderRadius: '50%',
                backgroundColor: threatScoreColor,
                display: 'inline-block',
                boxShadow: `0 0 6px ${threatScoreColor}`,
              }} />
              POSTURE: {posture.overallRisk} ({posture.threatScore}/100)
            </span>
          </div>
          <p style={{ fontSize: '0.75rem', color: '#94a3b8', margin: 0, lineHeight: '1.4' }}>
            Cross-channel event aggregation (Security, System, App, Sysmon) correlated with live kernel runtime telemetry & Local Llama AI.
          </p>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px', flexShrink: 0, flexWrap: 'wrap' }}>
          <button
            onClick={() => handleRunAiAnalysis('Full-Spectrum Host Threat & SIEM Assessment')}
            disabled={isAnalyzing}
            style={{
              background: 'linear-gradient(135deg, #f43f5e 0%, #e11d48 100%)',
              color: '#ffffff',
              border: 'none',
              padding: '7px 14px',
              borderRadius: '6px',
              cursor: isAnalyzing ? 'not-allowed' : 'pointer',
              fontWeight: 700,
              fontSize: '0.75rem',
              display: 'flex',
              alignItems: 'center',
              gap: '6px',
              boxShadow: '0 2px 8px rgba(225, 29, 72, 0.4)',
              whiteSpace: 'nowrap',
              opacity: isAnalyzing ? 0.7 : 1,
            }}
          >
            {isAnalyzing ? '⚡ Analyzing Host...' : '🚀 Run Full SIEM AI Assessment'}
          </button>
          <button
            onClick={loadAllThreatData}
            style={{
              background: 'rgba(30, 41, 59, 0.8)',
              color: '#38bdf8',
              border: '1px solid rgba(56, 189, 248, 0.4)',
              padding: '7px 12px',
              borderRadius: '6px',
              cursor: 'pointer',
              fontWeight: 700,
              fontSize: '0.75rem',
              whiteSpace: 'nowrap',
            }}
          >
            {isLoading ? 'Scanning...' : '🔄 Refresh Feeds'}
          </button>
        </div>
      </header>

      {/* 4 Core Threat Metric Cards */}
      <section style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '10px' }}>
        <div
          onClick={() => setActiveSubtab('events')}
          style={{
            background: activeSubtab === 'events' ? 'rgba(239, 68, 68, 0.15)' : 'rgba(30, 41, 59, 0.7)',
            border: `1px solid ${activeSubtab === 'events' ? '#f87171' : 'rgba(255,255,255,0.08)'}`,
            borderRadius: '8px',
            padding: '12px',
            cursor: 'pointer',
          }}
        >
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', fontWeight: 600 }}>Cross-Channel Anomalies</div>
          <div style={{ fontSize: '1.4rem', fontWeight: 800, color: '#f87171', margin: '4px 0' }}>
            {allEventsCombined.length}
          </div>
          <div style={{ fontSize: '0.68rem', color: '#cbd5e1' }}>
            {anomalies.totalCriticalCount} Crit | {anomalies.totalErrorCount} Err | {anomalies.totalWarningCount} Warn
          </div>
        </div>

        <div
          onClick={() => setActiveSubtab('processes')}
          style={{
            background: activeSubtab === 'processes' ? 'rgba(249, 115, 22, 0.15)' : 'rgba(30, 41, 59, 0.7)',
            border: `1px solid ${activeSubtab === 'processes' ? '#fb923c' : 'rgba(255,255,255,0.08)'}`,
            borderRadius: '8px',
            padding: '12px',
            cursor: 'pointer',
          }}
        >
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', fontWeight: 600 }}>Flagged Suspicious Processes</div>
          <div style={{ fontSize: '1.4rem', fontWeight: 800, color: '#fb923c', margin: '4px 0' }}>
            {posture.flaggedProcesses.length}
          </div>
          <div style={{ fontSize: '0.68rem', color: '#cbd5e1' }}>
            LOLBins, Temp binaries, Network sockets
          </div>
        </div>

        <div
          onClick={() => setActiveSubtab('sessions')}
          style={{
            background: activeSubtab === 'sessions' ? 'rgba(56, 189, 248, 0.15)' : 'rgba(30, 41, 59, 0.7)',
            border: `1px solid ${activeSubtab === 'sessions' ? '#38bdf8' : 'rgba(255,255,255,0.08)'}`,
            borderRadius: '8px',
            padding: '12px',
            cursor: 'pointer',
          }}
        >
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', fontWeight: 600 }}>Suspicious RDP Sessions</div>
          <div style={{ fontSize: '1.4rem', fontWeight: 800, color: '#38bdf8', margin: '4px 0' }}>
            {posture.suspiciousSessions.length}
          </div>
          <div style={{ fontSize: '0.68rem', color: '#cbd5e1' }}>
            External public IPs & Shadow logins
          </div>
        </div>

        <div
          onClick={() => setActiveSubtab('users')}
          style={{
            background: activeSubtab === 'users' ? 'rgba(168, 85, 247, 0.15)' : 'rgba(30, 41, 59, 0.7)',
            border: `1px solid ${activeSubtab === 'users' ? '#c084fc' : 'rgba(255,255,255,0.08)'}`,
            borderRadius: '8px',
            padding: '12px',
            cursor: 'pointer',
          }}
        >
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', fontWeight: 600 }}>User & Service Alerts</div>
          <div style={{ fontSize: '1.4rem', fontWeight: 800, color: '#c084fc', margin: '4px 0' }}>
            {posture.flaggedUsers.length + posture.suspiciousServices.length}
          </div>
          <div style={{ fontSize: '0.68rem', color: '#cbd5e1' }}>
            {posture.flaggedUsers.length} Principals | {posture.suspiciousServices.length} Services
          </div>
        </div>
      </section>

      {/* Subtab Navigation */}
      <div style={{ display: 'flex', gap: '6px', borderBottom: '1px solid rgba(255, 255, 255, 0.1)', paddingBottom: '4px' }}>
        <button
          onClick={() => setActiveSubtab('events')}
          style={{
            background: activeSubtab === 'events' ? '#38bdf8' : 'transparent',
            color: activeSubtab === 'events' ? '#0f172a' : '#94a3b8',
            border: 'none',
            padding: '6px 12px',
            borderRadius: '4px',
            cursor: 'pointer',
            fontWeight: 700,
            fontSize: '0.75rem',
          }}
        >
          🚨 Cross-Channel Anomaly Feed ({allEventsCombined.length})
        </button>
        <button
          onClick={() => setActiveSubtab('processes')}
          style={{
            background: activeSubtab === 'processes' ? '#38bdf8' : 'transparent',
            color: activeSubtab === 'processes' ? '#0f172a' : '#94a3b8',
            border: 'none',
            padding: '6px 12px',
            borderRadius: '4px',
            cursor: 'pointer',
            fontWeight: 700,
            fontSize: '0.75rem',
          }}
        >
          ⚡ Flagged Processes ({posture.flaggedProcesses.length})
        </button>
        <button
          onClick={() => setActiveSubtab('sessions')}
          style={{
            background: activeSubtab === 'sessions' ? '#38bdf8' : 'transparent',
            color: activeSubtab === 'sessions' ? '#0f172a' : '#94a3b8',
            border: 'none',
            padding: '6px 12px',
            borderRadius: '4px',
            cursor: 'pointer',
            fontWeight: 700,
            fontSize: '0.75rem',
          }}
        >
          🖥️ Suspicious RDP ({posture.suspiciousSessions.length})
        </button>
        <button
          onClick={() => setActiveSubtab('users')}
          style={{
            background: activeSubtab === 'users' ? '#38bdf8' : 'transparent',
            color: activeSubtab === 'users' ? '#0f172a' : '#94a3b8',
            border: 'none',
            padding: '6px 12px',
            borderRadius: '4px',
            cursor: 'pointer',
            fontWeight: 700,
            fontSize: '0.75rem',
          }}
        >
          👥 Users & Services ({posture.flaggedUsers.length + posture.suspiciousServices.length})
        </button>
        <button
          onClick={() => setActiveSubtab('ai')}
          style={{
            background: activeSubtab === 'ai' ? '#f43f5e' : 'transparent',
            color: activeSubtab === 'ai' ? '#ffffff' : '#f43f5e',
            border: '1px solid #f43f5e',
            padding: '6px 12px',
            borderRadius: '4px',
            cursor: 'pointer',
            fontWeight: 700,
            fontSize: '0.75rem',
            marginLeft: 'auto',
          }}
        >
          🤖 SIEM AI Intelligence Copilot
        </button>
      </div>

      {/* SUBTAB 1: Cross-Channel Event Anomalies Feed */}
      {activeSubtab === 'events' && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
          {/* Channel Filter Pills */}
          <div style={{ display: 'flex', gap: '6px', alignItems: 'center' }}>
            <span style={{ fontSize: '0.7rem', color: '#94a3b8', fontWeight: 600 }}>Filter Channel:</span>
            {(['ALL', 'Security', 'System', 'Application', 'Sysmon'] as const).map((ch) => (
              <button
                key={ch}
                onClick={() => setSelectedChannelFilter(ch)}
                style={{
                  background: selectedChannelFilter === ch ? 'rgba(56, 189, 248, 0.25)' : 'rgba(30, 41, 59, 0.6)',
                  color: selectedChannelFilter === ch ? '#38bdf8' : '#94a3b8',
                  border: `1px solid ${selectedChannelFilter === ch ? '#38bdf8' : 'rgba(255,255,255,0.08)'}`,
                  padding: '3px 8px',
                  borderRadius: '4px',
                  fontSize: '0.68rem',
                  fontWeight: 600,
                  cursor: 'pointer',
                }}
              >
                {ch === 'ALL' ? '🌐 All Channels' : ch}
              </button>
            ))}
          </div>

          <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', overflow: 'hidden' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.73rem' }}>
              <thead>
                <tr style={{ background: 'rgba(15, 23, 42, 0.95)', color: '#94a3b8', fontSize: '0.68rem', borderBottom: '1px solid rgba(255,255,255,0.1)' }}>
                  <th style={{ padding: '8px' }}>Channel</th>
                  <th style={{ padding: '8px' }}>Event ID</th>
                  <th style={{ padding: '8px' }}>Risk / Level</th>
                  <th style={{ padding: '8px' }}>Provider</th>
                  <th style={{ padding: '8px' }}>Timestamp</th>
                  <th style={{ padding: '8px' }}>Description</th>
                </tr>
              </thead>
              <tbody>
                {filteredEvents.length === 0 ? (
                  <tr>
                    <td colSpan={6} style={{ padding: '16px', textAlign: 'center', color: '#94a3b8' }}>
                      {isLoading ? 'Ingesting cross-channel event stream...' : 'No critical anomalies identified in the selected filter.'}
                    </td>
                  </tr>
                ) : (
                  filteredEvents.map((evt, i) => (
                    <tr key={i} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)', background: i % 2 === 0 ? 'transparent' : 'rgba(15,23,42,0.3)' }}>
                      <td style={{ padding: '6px 8px' }}>
                        <span
                          onClick={() => onSelectChannel && onSelectChannel(evt.channelTag === 'Sysmon' ? 'Microsoft-Windows-Sysmon/Operational' : evt.channelTag)}
                          style={{
                            padding: '1px 6px',
                            borderRadius: '3px',
                            fontSize: '0.64rem',
                            fontWeight: 700,
                            background: evt.channelTag === 'Security' ? 'rgba(239,68,68,0.2)' : evt.channelTag === 'Sysmon' ? 'rgba(168,85,247,0.2)' : 'rgba(56,189,248,0.2)',
                            color: evt.channelTag === 'Security' ? '#f87171' : evt.channelTag === 'Sysmon' ? '#c084fc' : '#38bdf8',
                            cursor: 'pointer',
                          }}
                        >
                          {evt.channelTag}
                        </span>
                      </td>
                      <td style={{ padding: '6px 8px', fontWeight: 700, color: '#38bdf8' }}>#{evt.id}</td>
                      <td style={{ padding: '6px 8px' }}>
                        <span style={{
                          padding: '1px 5px',
                          borderRadius: '3px',
                          fontSize: '0.62rem',
                          fontWeight: 700,
                          background: `rgba(${evt.risk === 'Critical' || evt.level === 'Critical' ? '239,68,68' : '234,179,8'}, 0.2)`,
                          color: getRiskColor(evt.risk || evt.level),
                        }}>
                          {evt.risk || evt.level}
                        </span>
                      </td>
                      <td style={{ padding: '6px 8px', color: '#94a3b8' }}>{evt.provider}</td>
                      <td style={{ padding: '6px 8px', color: '#94a3b8', whiteSpace: 'nowrap' }}>{evt.time}</td>
                      <td style={{ padding: '6px 8px', color: '#e2e8f0', maxWidth: '380px', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                        {evt.desc}
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* SUBTAB 2: Flagged Running Processes */}
      {activeSubtab === 'processes' && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
          <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', overflow: 'hidden' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.73rem' }}>
              <thead>
                <tr style={{ background: 'rgba(15, 23, 42, 0.95)', color: '#94a3b8', fontSize: '0.68rem', borderBottom: '1px solid rgba(255,255,255,0.1)' }}>
                  <th style={{ padding: '8px' }}>PID</th>
                  <th style={{ padding: '8px' }}>Process Name</th>
                  <th style={{ padding: '8px' }}>Threat Severity</th>
                  <th style={{ padding: '8px' }}>Flagged Reason</th>
                  <th style={{ padding: '8px' }}>Command Line / Path</th>
                  <th style={{ padding: '8px' }}>RAM / CPU</th>
                </tr>
              </thead>
              <tbody>
                {posture.flaggedProcesses.length === 0 ? (
                  <tr>
                    <td colSpan={6} style={{ padding: '16px', textAlign: 'center', color: '#94a3b8' }}>
                      ✅ No anomalous or LOLBin processes currently running outside security baseline.
                    </td>
                  </tr>
                ) : (
                  posture.flaggedProcesses.map((p: ProcessAnomalyData, i: number) => (
                    <tr key={i} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                      <td style={{ padding: '6px 8px', fontWeight: 700, color: '#38bdf8' }}>{p.process.processId}</td>
                      <td style={{ padding: '6px 8px', fontWeight: 600 }}>{p.process.name}</td>
                      <td style={{ padding: '6px 8px' }}>
                        <span style={{ padding: '1px 5px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: 'rgba(249,115,22,0.2)', color: '#fb923c' }}>
                          {p.risk}
                        </span>
                      </td>
                      <td style={{ padding: '6px 8px', color: '#f87171', fontWeight: 600 }}>{p.reason}</td>
                      <td style={{ padding: '6px 8px', color: '#cbd5e1', maxWidth: '320px', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                        <code>{p.process.commandLine || p.process.path}</code>
                      </td>
                      <td style={{ padding: '6px 8px', color: '#94a3b8' }}>
                        {p.process.memoryUsageMB} MB / {p.process.cpuUsagePercent.toFixed(1)}%
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* SUBTAB 3: Suspicious RDP & Sessions */}
      {activeSubtab === 'sessions' && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
          <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', overflow: 'hidden' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.73rem' }}>
              <thead>
                <tr style={{ background: 'rgba(15, 23, 42, 0.95)', color: '#94a3b8', fontSize: '0.68rem', borderBottom: '1px solid rgba(255,255,255,0.1)' }}>
                  <th style={{ padding: '8px' }}>Session ID</th>
                  <th style={{ padding: '8px' }}>User Account</th>
                  <th style={{ padding: '8px' }}>Risk Level</th>
                  <th style={{ padding: '8px' }}>Detected Threat Reason</th>
                  <th style={{ padding: '8px' }}>Remote Client IP</th>
                  <th style={{ padding: '8px' }}>Session State</th>
                </tr>
              </thead>
              <tbody>
                {posture.suspiciousSessions.length === 0 ? (
                  <tr>
                    <td colSpan={6} style={{ padding: '16px', textAlign: 'center', color: '#94a3b8' }}>
                      ✅ No external IP logins, shadow sessions, or unauthorized RDP connections detected.
                    </td>
                  </tr>
                ) : (
                  posture.suspiciousSessions.map((s: SessionAnomalyData, i: number) => (
                    <tr key={i} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                      <td style={{ padding: '6px 8px', fontWeight: 700, color: '#38bdf8' }}>#{s.session.sessionId}</td>
                      <td style={{ padding: '6px 8px', fontWeight: 600 }}>{s.session.userName}</td>
                      <td style={{ padding: '6px 8px' }}>
                        <span style={{ padding: '1px 5px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: 'rgba(239,68,68,0.2)', color: '#f87171' }}>
                          {s.risk}
                        </span>
                      </td>
                      <td style={{ padding: '6px 8px', color: '#f87171', fontWeight: 600 }}>{s.reason}</td>
                      <td style={{ padding: '6px 8px', color: '#38bdf8', fontWeight: 700 }}>{s.session.clientIpAddress}</td>
                      <td style={{ padding: '6px 8px', color: '#94a3b8' }}>{s.session.state}</td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* SUBTAB 4: Users & Services */}
      {activeSubtab === 'users' && (
        <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '12px' }}>
          {/* Flagged Users */}
          <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px' }}>
            <h3 style={{ margin: '0 0 10px 0', fontSize: '0.85rem', color: '#c084fc', fontWeight: 700 }}>
              👥 User Principal & Privilege Alerts ({posture.flaggedUsers.length})
            </h3>
            {posture.flaggedUsers.length === 0 ? (
              <p style={{ color: '#94a3b8', fontSize: '0.75rem' }}>✅ All local accounts match security baseline.</p>
            ) : (
              <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
                {posture.flaggedUsers.map((u: UserAnomalyData, i: number) => (
                  <div key={i} style={{ background: 'rgba(15, 23, 42, 0.6)', padding: '8px 10px', borderRadius: '6px', border: '1px solid rgba(192, 132, 252, 0.3)' }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                      <span style={{ fontWeight: 700, color: '#f8fafc' }}>{u.user.username} ({u.user.userClass})</span>
                      <span style={{ padding: '1px 5px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: 'rgba(234,179,8,0.2)', color: '#fbbf24' }}>
                        {u.risk}
                      </span>
                    </div>
                    <div style={{ fontSize: '0.7rem', color: '#f87171', marginTop: '3px' }}>{u.reason}</div>
                  </div>
                ))}
              </div>
            )}
          </div>

          {/* Flagged Services */}
          <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px' }}>
            <h3 style={{ margin: '0 0 10px 0', fontSize: '0.85rem', color: '#38bdf8', fontWeight: 700 }}>
              ⚙️ Suspicious & Non-Windows Services ({posture.suspiciousServices.length})
            </h3>
            {posture.suspiciousServices.length === 0 ? (
              <p style={{ color: '#94a3b8', fontSize: '0.75rem' }}>✅ All background services operating within system baseline.</p>
            ) : (
              <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', maxHeight: '300px', overflowY: 'auto' }}>
                {posture.suspiciousServices.map((srv: ServiceAnomalyData, i: number) => (
                  <div key={i} style={{ background: 'rgba(15, 23, 42, 0.6)', padding: '8px 10px', borderRadius: '6px', border: '1px solid rgba(56, 189, 248, 0.3)' }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                      <span style={{ fontWeight: 700, color: '#f8fafc' }}>{srv.service.serviceName} ({srv.service.displayName})</span>
                      <span style={{ padding: '1px 5px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: 'rgba(56,189,248,0.2)', color: '#38bdf8' }}>
                        {srv.risk}
                      </span>
                    </div>
                    <div style={{ fontSize: '0.7rem', color: '#94a3b8', marginTop: '3px' }}>{srv.reason} (Status: {srv.service.status})</div>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      )}

      {/* SUBTAB 5: AI SIEM Intelligence Copilot */}
      {activeSubtab === 'ai' && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
          {/* Query Bar */}
          <div style={{ background: 'rgba(30, 41, 59, 0.8)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
            <div style={{ display: 'flex', gap: '8px' }}>
              <input
                type="text"
                value={aiQuery}
                onChange={(e) => setAiQuery(e.target.value)}
                placeholder="Ask SIEM Copilot (e.g. Investigate credential theft, lateral movement, or unauthorized admin logins)..."
                style={{
                  flex: 1,
                  background: 'rgba(15, 23, 42, 0.9)',
                  border: '1px solid rgba(56, 189, 248, 0.4)',
                  color: '#f8fafc',
                  padding: '8px 12px',
                  borderRadius: '6px',
                  fontSize: '0.78rem',
                }}
                onKeyDown={(e) => {
                  if (e.key === 'Enter' && !isAnalyzing) handleRunAiAnalysis();
                }}
              />
              <button
                onClick={() => handleRunAiAnalysis()}
                disabled={isAnalyzing}
                style={{
                  background: '#f43f5e',
                  color: '#ffffff',
                  border: 'none',
                  padding: '8px 16px',
                  borderRadius: '6px',
                  cursor: 'pointer',
                  fontWeight: 700,
                  fontSize: '0.75rem',
                }}
              >
                {isAnalyzing ? '⚡ Correlating...' : '🚀 Investigate'}
              </button>
            </div>

            {/* Preset Query Badges */}
            <div style={{ display: 'flex', gap: '6px', flexWrap: 'wrap' }}>
              <span style={{ fontSize: '0.68rem', color: '#94a3b8', alignSelf: 'center' }}>Quick Investigations:</span>
              {[
                'Full-Spectrum Host Threat & SIEM Assessment',
                'Investigate Encoded LOLBins & Script Execution',
                'Check External RDP Logins & Remote Lateral Movement',
                'Audit Unauthorized Local Administrators & Service Persistence',
              ].map((queryPreset) => (
                <button
                  key={queryPreset}
                  onClick={() => {
                    setAiQuery(queryPreset);
                    handleRunAiAnalysis(queryPreset);
                  }}
                  style={{
                    background: 'rgba(15, 23, 42, 0.8)',
                    color: '#38bdf8',
                    border: '1px solid rgba(56, 189, 248, 0.25)',
                    padding: '3px 8px',
                    borderRadius: '4px',
                    fontSize: '0.66rem',
                    cursor: 'pointer',
                  }}
                >
                  {queryPreset}
                </button>
              ))}
            </div>
          </div>

          {/* Progress / Output Display */}
          {isAnalyzing && (
            <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #f43f5e', borderRadius: '8px', padding: '16px', textAlign: 'center' }}>
              <div style={{ fontSize: '0.85rem', color: '#f43f5e', fontWeight: 700, marginBottom: '6px' }}>
                ⚡ SIEM Correlation & AI Inference in Progress
              </div>
              <div style={{ fontSize: '0.75rem', color: '#94a3b8' }}>{aiProgressMessage}</div>
            </div>
          )}

          {aiReport && !isAnalyzing && (
            <div style={{ background: 'rgba(15, 23, 42, 0.95)', border: '1px solid rgba(56, 189, 248, 0.3)', borderRadius: '8px', padding: '16px', position: 'relative' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '1px solid rgba(255,255,255,0.1)', paddingBottom: '8px', marginBottom: '12px' }}>
                <span style={{ fontSize: '0.8rem', fontWeight: 700, color: '#38bdf8' }}>📄 SIEM AI Threat Intelligence Report</span>
                <button
                  onClick={handleCopyReport}
                  style={{
                    background: copiedReport ? '#22c55e' : 'rgba(30, 41, 59, 0.8)',
                    color: copiedReport ? '#0f172a' : '#94a3b8',
                    border: '1px solid rgba(255,255,255,0.2)',
                    padding: '4px 10px',
                    borderRadius: '4px',
                    fontSize: '0.7rem',
                    cursor: 'pointer',
                    fontWeight: 600,
                  }}
                >
                  {copiedReport ? '✓ Copied!' : '📋 Copy Report'}
                </button>
              </div>
              <div style={{ whiteSpace: 'pre-wrap', lineHeight: '1.6', fontSize: '0.76rem', color: '#e2e8f0', fontFamily: 'system-ui, -apple-system, sans-serif' }}>
                {aiReport}
              </div>
            </div>
          )}
        </div>
      )}
    </div>
  );
};

export default RiskCenter;
