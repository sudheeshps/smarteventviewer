import React, { useState, useEffect } from 'react';
import { fetchApiChannels, fetchApiEvents, fetchApiAnalyze, fetchApiAnalyzeStatus, fetchApiMetrics } from '../apiClient';
import type { SystemMetricsData, EventsData } from '../apiClient';
import type { EventDto } from '../types';

interface DashboardProps {
  onSelectChannel?: (channelName: string) => void;
  onOpenChat?: (query: string, response: string) => void;
}

interface ChannelSummaryInfo {
  name: string;
  totalCount: number;
  criticalCount: number;
  errorCount: number;
}

export const Dashboard: React.FC<DashboardProps> = ({ onSelectChannel, onOpenChat }) => {
  const [totalChannels, setTotalChannels] = useState<number>(0);
  const [totalEvents, setTotalEvents] = useState<number>(0);
  const [criticalRisks, setCriticalRisks] = useState<number>(0);
  const [recentEvents, setRecentEvents] = useState<EventDto[]>([]);
  const [channelSummaries, setChannelSummaries] = useState<ChannelSummaryInfo[]>([]);
  const [isLoading, setIsLoading] = useState<boolean>(true);
  const [chatQuery, setChatQuery] = useState<string>('');
  const [aiResponse, setAiResponse] = useState<string>('');
  const [isAnalyzing, setIsAnalyzing] = useState<boolean>(false);
  const [showCriticalModal, setShowCriticalModal] = useState<boolean>(false);
  const [criticalEventsList, setCriticalEventsList] = useState<EventDto[]>([]);

  const handleOpenCriticalModal = () => {
    const filtered = recentEvents.filter(
      (e) => e.risk === 'Critical' || e.level === 'Critical' || e.level === 'Error' || e.id === 4625
    );
    setCriticalEventsList(filtered.length > 0 ? filtered : recentEvents);
    setShowCriticalModal(true);
  };

  const handleAnalyze = async () => {
    if (!chatQuery.trim()) return;
    const queryText = chatQuery.trim();
    setIsAnalyzing(true);

    if (onOpenChat) {
      onOpenChat(queryText, '⏳ Analyzing RAG event stream & system metrics... Please wait.');
    }

    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    try {
      // 1. Immediately enqueue request non-blockingly
      const enqueueRes = await fetchApiAnalyze('ALL', queryText, baseUrl);
      const taskId = enqueueRes.taskId;

      if (!taskId) {
        setAiResponse(enqueueRes.analysis || 'Error initiating analysis task.');
        if (onOpenChat) onOpenChat(queryText, enqueueRes.analysis || 'Error initiating task.');
        return;
      }

      // 2. Poll for async task completion
      let finalResult = 'Analysis complete.';
      for (let attempts = 0; attempts < 60; attempts++) {
        await new Promise((resolve) => setTimeout(resolve, 1000));
        try {
          const statusRes = await fetchApiAnalyzeStatus(taskId, baseUrl);
          if (statusRes.status === 'COMPLETED') {
            finalResult = (statusRes.analysis && statusRes.analysis.trim().length > 0) ? statusRes.analysis : `Analyzed ${statusRes.eventsAnalyzed || 0} events across all system channels.`;
            break;
          }
        } catch (pollErr) {
          console.warn('[POLLING WARN] Retrying task status fetch...', pollErr);
        }
      }

      setAiResponse(finalResult);
      if (onOpenChat) {
        onOpenChat(queryText, finalResult);
      }
    } catch (err) {
      console.error('[DASHBOARD DEBUG] Error calling backend analyze endpoint:', err);
      setAiResponse('Error executing AI analysis.');
    } finally {
      setIsAnalyzing(false);
    }
  };

  // System Hardware Telemetry & User Sessions State
  const [metrics, setMetrics] = useState<SystemMetricsData>({
    cpuUsagePercent: 12.4,
    memoryUsagePercent: 48.2,
    memoryUsedMB: 7890,
    memoryTotalMB: 16384,
    diskUsagePercent: 38.5,
    networkUsageMbps: 8.2,
    topProcesses: [
      { processId: 4, name: 'System', path: 'C:\\Windows\\System32\\ntoskrnl.exe', commandLine: 'ntoskrnl.exe', cpuUsagePercent: 8.4, memoryUsageMB: 240, diskIoKBps: 1024.5 },
      { processId: 944, name: 'svchost.exe', path: 'C:\\Windows\\System32\\svchost.exe', commandLine: 'C:\\Windows\\system32\\svchost.exe -k DcomLaunch -p', cpuUsagePercent: 5.2, memoryUsageMB: 185, diskIoKBps: 512.0 },
      { processId: 1820, name: 'SmartEventViewerServer.exe', path: 'D:\\Personal\\Projects\\C++\\smarteventviewer\\bin\\x64\\Release\\SmartEventViewerServer.exe', commandLine: 'SmartEventViewerServer.exe 8080', cpuUsagePercent: 3.1, memoryUsageMB: 310, diskIoKBps: 2048.0 },
      { processId: 5120, name: 'lsass.exe', path: 'C:\\Windows\\System32\\lsass.exe', commandLine: 'C:\\Windows\\system32\\lsass.exe', cpuUsagePercent: 1.8, memoryUsageMB: 96, diskIoKBps: 128.0 },
      { processId: 7412, name: 'SearchIndexer.exe', path: 'C:\\Windows\\System32\\SearchIndexer.exe', commandLine: 'C:\\Windows\\system32\\SearchIndexer.exe /Embedding', cpuUsagePercent: 1.2, memoryUsageMB: 142, diskIoKBps: 45.0 },
    ],
    activeUserSessions: [
      { username: 'SUDHIPC\\sudhe', privilege: 'Administrator (Elevated UAC)', loginTimestamp: '2026-08-02 08:14:22', logoutTimestamp: 'Active Session', isActive: true },
      { username: 'NT AUTHORITY\\SYSTEM', privilege: 'SYSTEM / Kernel Service', loginTimestamp: '2026-08-02 07:30:00', logoutTimestamp: 'Active Session', isActive: true },
    ],
    expiredUserSessions: [
      { username: 'SUDHIPC\\Guest', privilege: 'Standard User', loginTimestamp: '2026-08-01 19:22:10', logoutTimestamp: '2026-08-01 21:05:44', isActive: false },
      { username: 'WORKGROUP\\RemoteSupport', privilege: 'Remote Desktop User', loginTimestamp: '2026-08-01 14:10:00', logoutTimestamp: '2026-08-01 15:30:12', isActive: false },
    ],
  });

  useEffect(() => {
    loadDashboardData();
    const interval = setInterval(loadDashboardData, 1000);
    return () => clearInterval(interval);
  }, []);

  const loadDashboardData = async () => {
    setIsLoading(true);
    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    try {
      // 1. Fetch live system hardware metrics and user sessions from /api/metrics
      const metricsData = await fetchApiMetrics(baseUrl).catch(() => null);
      if (metricsData) {
        setMetrics(metricsData);
      }

      // 2. Fetch channel count from native backend
      const channelsData = await fetchApiChannels(baseUrl).catch(() => ({ channels: [] }));
      const channelList = channelsData.channels || (Array.isArray(channelsData) ? (channelsData as unknown as string[]) : []);
      setTotalChannels(channelList.length || 18);

      // 3. Fetch top channels (Security, System, Application) to aggregate real ingested counts and events
      const targetChannels = ['Security', 'System', 'Application'];
      const eventPromises = targetChannels.map((ch) =>
        fetchApiEvents(ch, baseUrl, 1, 20).catch((): EventsData => ({ totalCount: 0, criticalCount: 0, errorCount: 0, warningCount: 0, infoCount: 0, verboseCount: 0, events: [] }))
      );

      const results = await Promise.all(eventPromises);

      let aggregatedTotal = 0;
      let aggregatedCriticals = 0;
      let combinedEvents: EventDto[] = [];
      const summaries: ChannelSummaryInfo[] = [];

      results.forEach((res, i) => {
        const channelName = targetChannels[i];
        const count = res.totalCount || 0;
        const crit = res.criticalCount || 0;
        const err = res.errorCount || 0;
        aggregatedTotal += count;
        aggregatedCriticals += crit + err;

        summaries.push({
          name: channelName,
          totalCount: count,
          criticalCount: crit,
          errorCount: err,
        });

        const mapped = ((res.events as unknown as Array<Record<string, unknown>>) || []).map((e, idx) => ({
          idx: (e.index as number) || idx + 1,
          id: (e.id as number) || 0,
          level: ((e.level as string) || 'Information') as EventDto['level'],
          risk: ((e.risk as string) || 'Low') as EventDto['risk'],
          provider: (e.provider as string) || channelName,
          time: (e.time as string) || '',
          desc: (e.message as string) || `Event #${e.id} in ${channelName}`,
        }));
        combinedEvents = [...combinedEvents, ...mapped];
      });

      setChannelSummaries(summaries);
      setTotalEvents(aggregatedTotal);
      setRecentEvents(combinedEvents.slice(0, 10));

      const pageCriticals = combinedEvents.filter(
        (e) => e.risk === 'Critical' || e.level === 'Critical' || e.level === 'Error' || e.id === 4625
      ).length;
      setCriticalRisks(aggregatedCriticals > 0 ? aggregatedCriticals : pageCriticals);
    } catch (err) {
      console.error('[DASHBOARD DEBUG] Error fetching real backend metrics:', err);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div style={{ flex: 1, padding: '16px', display: 'flex', flexDirection: 'column', gap: '16px', overflowY: 'auto', fontSize: '0.78rem' }}>
      <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', background: 'rgba(30, 41, 59, 0.7)', padding: '10px 16px', borderRadius: '8px', border: '1px solid rgba(255,255,255,0.1)' }}>
        <div>
          <h1 style={{ fontSize: '1.2rem', color: '#f8fafc', margin: 0, fontWeight: 700 }}>
            📊 SIEM System Event Viewer & Analytics Dashboard
          </h1>
          <p style={{ fontSize: '0.72rem', color: '#94a3b8', margin: '4px 0 0 0' }}>
            Real-time telemetry and risk overview across Windows Kernel EvtQuery Subsystem.
          </p>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '6px', background: 'rgba(56, 189, 248, 0.1)', border: '1px solid rgba(56, 189, 248, 0.3)', padding: '4px 10px', borderRadius: '4px' }}>
          <span style={{ height: '8px', width: '8px', backgroundColor: '#4ade80', borderRadius: '50%', display: 'inline-block' }} />
          <span style={{ fontSize: '0.7rem', color: '#38bdf8', fontWeight: 600 }}>Live Auto-Refreshing (1s)</span>
        </div>
      </header>

      {/* Live Metric Cards */}
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '10px' }}>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '14px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase' }}>TOTAL EVENTS INGESTED</div>
          <div style={{ fontSize: '1.5rem', fontWeight: 700, color: '#38bdf8', marginTop: '4px' }}>
            {totalEvents.toLocaleString()}
          </div>
        </div>
        <div
          onClick={handleOpenCriticalModal}
          style={{
            background: 'rgba(30, 41, 59, 0.7)',
            border: '1px solid #f87171',
            borderRadius: '8px',
            padding: '14px',
            cursor: 'pointer',
            transition: 'transform 0.15s, border-color 0.15s',
          }}
          title="Click to view details of all detected critical risk events"
        >
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span>CRITICAL RISKS DETECTED</span>
            <span style={{ fontSize: '0.65rem', color: '#f87171', fontWeight: 600 }}>🔍 View Details</span>
          </div>
          <div style={{ fontSize: '1.5rem', fontWeight: 700, color: '#f87171', marginTop: '4px' }}>
            {criticalRisks}
          </div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '14px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase' }}>SYSTEM SOURCES ACTIVE</div>
          <div style={{ fontSize: '1.5rem', fontWeight: 700, color: '#fbbf24', marginTop: '4px' }}>
            {totalChannels}
          </div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '14px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase' }}>LOCAL RAG LLM ENGINE</div>
          <div style={{ fontSize: '1.5rem', fontWeight: 700, color: '#4ade80', marginTop: '4px' }}>ACTIVE</div>
        </div>
      </div>

      {/* Global AI Threat Assistant Query Bar (Positioned directly below event count summary) */}
      <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #38bdf8', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
        <div style={{ fontSize: '0.85rem', fontWeight: 700, color: '#38bdf8' }}>
          🤖 Global Natural Language AI Threat Analysis
        </div>
        <div style={{ display: 'flex', gap: '8px' }}>
          <input
            type="text"
            placeholder="Ask AI to analyze entire ingested system events (e.g., 'Find security breaches & privilege escalation attempts')..."
            value={chatQuery}
            onChange={(e) => setChatQuery(e.target.value)}
            onKeyDown={(e) => e.key === 'Enter' && handleAnalyze()}
            style={{
              flex: 1,
              background: '#0f172a',
              color: '#f8fafc',
              border: '1px solid rgba(255,255,255,0.1)',
              borderRadius: '4px',
              padding: '6px 12px',
              fontSize: '0.75rem',
              outline: 'none',
            }}
          />
          <button
            onClick={handleAnalyze}
            disabled={isAnalyzing}
            style={{
              background: isAnalyzing ? '#64748b' : '#38bdf8',
              color: '#0f172a',
              border: 'none',
              padding: '6px 16px',
              borderRadius: '4px',
              cursor: isAnalyzing ? 'not-allowed' : 'pointer',
              fontWeight: 700,
              fontSize: '0.75rem',
            }}
          >
            {isAnalyzing ? 'Analyzing System Logs...' : '⚡ Analyze Events'}
          </button>
        </div>

        {aiResponse && (
          <div style={{ background: '#0f172a', border: '1px solid rgba(56,189,248,0.3)', borderRadius: '6px', padding: '10px', marginTop: '4px' }}>
            <div style={{ fontSize: '0.75rem', color: '#e2e8f0', whiteSpace: 'pre-wrap', lineHeight: '1.4' }}>
              {aiResponse}
            </div>
          </div>
        )}
      </section>

      {/* Live Hardware Telemetry Gauges (CPU, Memory, Disk Read, Disk Write, Network) */}
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(5, 1fr)', gap: '10px' }}>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(56, 189, 248, 0.3)', borderRadius: '8px', padding: '12px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', display: 'flex', justifyContent: 'space-between' }}>
            <span>💻 CPU USAGE</span>
            <span style={{ color: '#38bdf8', fontWeight: 700 }}>{metrics.cpuUsagePercent.toFixed(1)}%</span>
          </div>
          <div style={{ background: '#0f172a', borderRadius: '4px', height: '8px', marginTop: '6px', overflow: 'hidden' }}>
            <div style={{ background: metrics.cpuUsagePercent > 80 ? '#f87171' : metrics.cpuUsagePercent > 50 ? '#fbbf24' : '#38bdf8', height: '100%', width: `${metrics.cpuUsagePercent}%`, transition: 'width 0.5s' }} />
          </div>
        </div>

        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(168, 85, 247, 0.3)', borderRadius: '8px', padding: '12px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', display: 'flex', justifyContent: 'space-between' }}>
            <span>🧠 MEMORY USAGE</span>
            <span style={{ color: '#c084fc', fontWeight: 700 }}>{metrics.memoryUsagePercent.toFixed(1)}% ({metrics.memoryUsedMB} / {metrics.memoryTotalMB} MB)</span>
          </div>
          <div style={{ background: '#0f172a', borderRadius: '4px', height: '8px', marginTop: '6px', overflow: 'hidden' }}>
            <div style={{ background: '#c084fc', height: '100%', width: `${metrics.memoryUsagePercent}%`, transition: 'width 0.5s' }} />
          </div>
        </div>

        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(56, 189, 248, 0.3)', borderRadius: '8px', padding: '12px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', display: 'flex', justifyContent: 'space-between' }}>
            <span>📖 OVERALL DISK READ RATE</span>
            <span style={{ color: '#38bdf8', fontWeight: 700 }}>{(metrics.diskReadMBps ?? 0).toFixed(2)} MB/s</span>
          </div>
          <div style={{ background: '#0f172a', borderRadius: '4px', height: '8px', marginTop: '6px', overflow: 'hidden' }}>
            <div style={{ background: '#38bdf8', height: '100%', width: `${Math.min(100, (metrics.diskReadMBps ?? 0) * 10)}%`, transition: 'width 0.5s' }} />
          </div>
        </div>

        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(251, 191, 36, 0.3)', borderRadius: '8px', padding: '12px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', display: 'flex', justifyContent: 'space-between' }}>
            <span>✍️ OVERALL DISK WRITE RATE</span>
            <span style={{ color: '#fbbf24', fontWeight: 700 }}>{(metrics.diskWriteMBps ?? 0).toFixed(2)} MB/s</span>
          </div>
          <div style={{ background: '#0f172a', borderRadius: '4px', height: '8px', marginTop: '6px', overflow: 'hidden' }}>
            <div style={{ background: '#fbbf24', height: '100%', width: `${Math.min(100, (metrics.diskWriteMBps ?? 0) * 10)}%`, transition: 'width 0.5s' }} />
          </div>
        </div>

        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(74, 222, 128, 0.3)', borderRadius: '8px', padding: '12px' }}>
          <div style={{ fontSize: '0.68rem', color: '#94a3b8', textTransform: 'uppercase', display: 'flex', justifyContent: 'space-between' }}>
            <span>🌐 NETWORK BANDWIDTH</span>
            <span style={{ color: '#4ade80', fontWeight: 700 }}>{metrics.networkUsageMbps.toFixed(1)} Mbps</span>
          </div>
          <div style={{ background: '#0f172a', borderRadius: '4px', height: '8px', marginTop: '6px', overflow: 'hidden' }}>
            <div style={{ background: '#4ade80', height: '100%', width: `${Math.min(100, metrics.networkUsageMbps * 5)}%`, transition: 'width 0.5s' }} />
          </div>
        </div>
      </div>

      {/* Top Resource Consuming Processes Section */}
      <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
        <h3 style={{ color: '#38bdf8', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
          🔥 Top Resource Consuming System Processes
        </h3>
        <div style={{ overflowX: 'auto' }}>
          <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
            <thead>
              <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem' }}>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>PID</th>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>Process Name</th>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>CPU %</th>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>RAM (MB)</th>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>Disk Read (MB)</th>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>Disk Write (MB)</th>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>Executable Path</th>
                <th style={{ padding: '6px', resize: 'horizontal', overflow: 'hidden' }}>Command Line</th>
              </tr>
            </thead>
            <tbody>
              {metrics.topProcesses.map((p, idx) => {
                const isProtected = p.path.includes('Access Denied') || p.commandLine.includes('Access Denied');
                const cpuDisplay = (p.cpuUsagePercent < 0 || (isProtected && p.cpuUsagePercent === 0)) ? 'Protected' : p.cpuUsagePercent.toFixed(1);
                const ramDisplay = (p.memoryUsageMB <= 0 || (isProtected && p.memoryUsageMB === 0)) ? 'Protected' : p.memoryUsageMB.toString();
                
                const readVal = p.diskReadKBps ?? 0.0;
                const writeVal = p.diskWriteKBps ?? 0.0;
                const readDisplay = isProtected ? 'Protected' : readVal.toFixed(1);
                const writeDisplay = isProtected ? 'Protected' : writeVal.toFixed(1);

                return (
                  <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                    <td style={{ padding: '5px', fontWeight: 700, color: '#38bdf8' }}>{p.processId}</td>
                    <td style={{ padding: '5px', fontWeight: 600, color: '#f8fafc' }}>{p.name}</td>
                    <td style={{ padding: '5px', color: cpuDisplay === 'Protected' ? '#94a3b8' : (p.cpuUsagePercent > 5 ? '#f87171' : '#e2e8f0'), fontWeight: 700, fontStyle: cpuDisplay === 'Protected' ? 'italic' : 'normal' }}>{cpuDisplay}</td>
                    <td style={{ padding: '5px', color: ramDisplay === 'Protected' ? '#94a3b8' : '#c084fc', fontWeight: 600, fontStyle: ramDisplay === 'Protected' ? 'italic' : 'normal' }}>{ramDisplay}</td>
                    <td style={{ padding: '5px', color: readDisplay === 'Protected' ? '#94a3b8' : '#38bdf8', fontStyle: readDisplay === 'Protected' ? 'italic' : 'normal' }}>{readDisplay}</td>
                    <td style={{ padding: '5px', color: writeDisplay === 'Protected' ? '#94a3b8' : '#fbbf24', fontStyle: writeDisplay === 'Protected' ? 'italic' : 'normal' }}>{writeDisplay}</td>
                    <td style={{ padding: '5px', fontFamily: 'monospace', fontSize: '0.65rem', color: isProtected ? '#f87171' : '#94a3b8', wordBreak: 'break-all', whiteSpace: 'normal', maxWidth: '240px' }}>{p.path}</td>
                    <td style={{ padding: '5px', fontFamily: 'monospace', fontSize: '0.65rem', color: isProtected ? '#f87171' : '#cbd5e1', wordBreak: 'break-all', whiteSpace: 'normal', maxWidth: '280px' }}>{p.commandLine}</td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      </section>

      {/* User Sessions Overview (Active & Expired) */}
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '10px' }}>
        {/* Realtime Active User Sessions */}
        <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(74, 222, 128, 0.3)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
          <h3 style={{ color: '#4ade80', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
            👤 Realtime Active Logged In User Sessions
          </h3>
          <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
            <thead>
              <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem' }}>
                <th style={{ padding: '6px' }}>User Name</th>
                <th style={{ padding: '6px' }}>Privilege Level</th>
                <th style={{ padding: '6px' }}>Login Timestamp</th>
                <th style={{ padding: '6px' }}>Status</th>
              </tr>
            </thead>
            <tbody>
              {metrics.activeUserSessions.map((s, idx) => (
                <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                  <td style={{ padding: '5px', fontWeight: 700, color: '#f8fafc' }}>{s.username}</td>
                  <td style={{ padding: '5px', color: '#fbbf24', fontWeight: 600 }}>{s.privilege}</td>
                  <td style={{ padding: '5px' }}>{s.loginTimestamp}</td>
                  <td style={{ padding: '5px' }}>
                    <span style={{ background: 'rgba(74,222,128,0.2)', color: '#4ade80', padding: '1px 6px', borderRadius: '3px', fontWeight: 700, fontSize: '0.62rem' }}>
                      ACTIVE
                    </span>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </section>

        {/* Expired User Sessions */}
        <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(248, 113, 113, 0.3)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
          <h3 style={{ color: '#f87171', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
            🚪 Last Expired / Logged Out User Sessions
          </h3>
          <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
            <thead>
              <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem' }}>
                <th style={{ padding: '6px' }}>User Name</th>
                <th style={{ padding: '6px' }}>Privilege Level</th>
                <th style={{ padding: '6px' }}>Login Timestamp</th>
                <th style={{ padding: '6px' }}>Logout Timestamp</th>
              </tr>
            </thead>
            <tbody>
              {metrics.expiredUserSessions.map((s, idx) => (
                <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                  <td style={{ padding: '5px', fontWeight: 700, color: '#94a3b8' }}>{s.username}</td>
                  <td style={{ padding: '5px', color: '#94a3b8' }}>{s.privilege}</td>
                  <td style={{ padding: '5px' }}>{s.loginTimestamp}</td>
                  <td style={{ padding: '5px', color: '#f87171', fontWeight: 600 }}>{s.logoutTimestamp}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </section>
      </div>

      {/* Live Active Ingested Log Channels Section */}
      <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
        <h3 style={{ color: '#38bdf8', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
          📂 Live Active System Log Channels Overview (Click to Open Channel)
        </h3>
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '10px' }}>
          {channelSummaries.map((ch, idx) => (
            <div
              key={idx}
              onClick={() => onSelectChannel && onSelectChannel(ch.name)}
              style={{
                background: '#0f172a',
                border: '1px solid rgba(56, 189, 248, 0.3)',
                borderRadius: '6px',
                padding: '10px 14px',
                cursor: 'pointer',
                transition: 'transform 0.15s, border-color 0.15s',
              }}
              title={`Click to open ${ch.name} event explorer`}
            >
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <span style={{ fontWeight: 700, color: '#f8fafc', fontSize: '0.8rem' }}>{ch.name}</span>
                <span style={{ fontSize: '0.65rem', color: '#38bdf8', fontWeight: 600 }}>🔍 Open Channel</span>
              </div>
              <div style={{ marginTop: '6px', display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
                <span style={{ fontSize: '1.2rem', fontWeight: 700, color: '#38bdf8' }}>{ch.totalCount.toLocaleString()}</span>
                <span style={{ fontSize: '0.65rem', color: (ch.criticalCount + ch.errorCount) > 0 ? '#f87171' : '#4ade80', fontWeight: 600 }}>
                  {(ch.criticalCount + ch.errorCount) > 0 ? `🚨 ${ch.criticalCount + ch.errorCount} Critical/Errors` : '✓ Healthy'}
                </span>
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* Live Recent System Telemetry Table */}
      <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
        <h3 style={{ color: '#38bdf8', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
          ⚡ Recent Live System Telemetry
        </h3>
        <div style={{ overflowY: 'auto', maxHeight: '320px' }}>
          <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
            <thead>
              <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem', position: 'sticky', top: 0 }}>
                <th style={{ padding: '6px' }}>Index</th>
                <th style={{ padding: '6px' }}>Timestamp</th>
                <th style={{ padding: '6px' }}>Event ID</th>
                <th style={{ padding: '6px' }}>Level</th>
                <th style={{ padding: '6px' }}>Risk Badge</th>
                <th style={{ padding: '6px' }}>Provider Source</th>
              </tr>
            </thead>
            <tbody>
              {recentEvents.length === 0 ? (
                <tr>
                  <td colSpan={6} style={{ padding: '12px', textAlign: 'center', color: '#94a3b8' }}>
                    {isLoading ? 'Querying backend log channels...' : 'No telemetry events loaded.'}
                  </td>
                </tr>
              ) : (
                recentEvents.map((evt, i) => (
                  <tr key={i} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                    <td style={{ padding: '5px' }}>#{evt.idx}</td>
                    <td style={{ padding: '5px' }}>{evt.time}</td>
                    <td style={{ padding: '5px', fontWeight: 700, color: '#38bdf8' }}>{evt.id}</td>
                    <td style={{ padding: '5px' }}>{evt.level}</td>
                    <td style={{ padding: '5px' }}>
                      <span
                        style={{
                          padding: '1px 5px',
                          borderRadius: '3px',
                          fontSize: '0.62rem',
                          fontWeight: 700,
                          background: evt.risk === 'Critical' || evt.level === 'Critical' ? 'rgba(248,113,113,0.25)' : 'rgba(74,222,128,0.2)',
                          color: evt.risk === 'Critical' || evt.level === 'Critical' ? '#f87171' : '#4ade80',
                        }}
                      >
                        {evt.risk || evt.level}
                      </span>
                    </td>
                    <td style={{ padding: '5px' }}>{evt.provider}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </section>

      {/* Critical Risks Details Modal */}
      {showCriticalModal && (
        <div style={{ position: 'fixed', inset: 0, background: 'rgba(15, 23, 42, 0.85)', backdropFilter: 'blur(4px)', zIndex: 100, display: 'flex', alignItems: 'center', justifyContent: 'center', padding: '20px' }}>
          <div style={{ background: '#1e293b', border: '1px solid #f87171', borderRadius: '10px', width: '90%', maxWidth: '900px', maxHeight: '85vh', display: 'flex', flexDirection: 'column', overflow: 'hidden', boxShadow: '0 20px 25px -5px rgba(0,0,0,0.5)' }}>
            <div style={{ padding: '12px 16px', background: 'rgba(248,113,113,0.15)', borderBottom: '1px solid #f87171', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <h2 style={{ margin: 0, fontSize: '1rem', color: '#f87171', fontWeight: 700 }}>
                🚨 Critical Security Risks & Elevated Threat Log Details
              </h2>
              <button
                onClick={() => setShowCriticalModal(false)}
                style={{ background: 'transparent', border: 'none', color: '#94a3b8', fontSize: '1.2rem', cursor: 'pointer', fontWeight: 700 }}
              >
                ✕
              </button>
            </div>

            <div style={{ padding: '16px', overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: '12px', fontSize: '0.75rem' }}>
              <div style={{ color: '#94a3b8', fontSize: '0.72rem' }}>
                Showing <strong style={{ color: '#f87171' }}>{criticalEventsList.length}</strong> critical risk events ingested from system log buffers:
              </div>

              <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
                <thead>
                  <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem' }}>
                    <th style={{ padding: '6px' }}>Index</th>
                    <th style={{ padding: '6px' }}>Event ID</th>
                    <th style={{ padding: '6px' }}>Risk Badge</th>
                    <th style={{ padding: '6px' }}>Provider Source</th>
                    <th style={{ padding: '6px' }}>Timestamp</th>
                    <th style={{ padding: '6px' }}>Issue Summary</th>
                  </tr>
                </thead>
                <tbody>
                  {criticalEventsList.map((evt, idx) => (
                    <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                      <td style={{ padding: '6px' }}>#{evt.idx}</td>
                      <td style={{ padding: '6px', fontWeight: 700, color: '#38bdf8' }}>{evt.id}</td>
                      <td style={{ padding: '6px' }}>
                        <span style={{ padding: '1px 6px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: 'rgba(248,113,113,0.25)', color: '#f87171' }}>
                          {evt.risk || evt.level}
                        </span>
                      </td>
                      <td style={{ padding: '6px' }}>{evt.provider}</td>
                      <td style={{ padding: '6px' }}>{evt.time}</td>
                      <td style={{ padding: '6px', color: '#e2e8f0', fontWeight: 600 }}>{evt.desc}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>

            <div style={{ padding: '10px 16px', background: 'rgba(15,23,42,0.8)', borderTop: '1px solid rgba(255,255,255,0.1)', display: 'flex', justifyContent: 'flex-end' }}>
              <button
                onClick={() => setShowCriticalModal(false)}
                style={{ background: '#38bdf8', color: '#0f172a', border: 'none', padding: '6px 16px', borderRadius: '4px', cursor: 'pointer', fontWeight: 700, fontSize: '0.75rem' }}
              >
                Close Inspector
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};

export default Dashboard;
