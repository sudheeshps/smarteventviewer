import React, { useState, useEffect, useMemo } from 'react';
import { fetchApiChannels, fetchApiEvents, fetchEventSummary, fetchMetricsSummary, formatTo12Hour } from '../apiClient';
import type { EventSummaryData } from '../apiClient';
import { ServerLogsViewer } from './ServerLogsViewer';
import type { SystemMetricsData, EventsData } from '../apiClient';
import type { EventDto } from '../types';

interface DashboardProps {
  onSelectChannel?: (channelName: string) => void;
}

interface ChannelSummaryInfo {
  name: string;
  totalCount: number;
  criticalCount: number;
  errorCount: number;
  warningCount: number;
  infoCount: number;
}

export const Dashboard: React.FC<DashboardProps> = ({ onSelectChannel }) => {
  const [activeDashboardTab, setActiveDashboardTab] = useState<'overview' | 'processes' | 'sessions' | 'users' | 'services'>('overview');
  const [totalChannels, setTotalChannels] = useState<number>(0);
  const [totalEvents, setTotalEvents] = useState<number>(0);
  const [criticalRisks, setCriticalRisks] = useState<number>(0);
  const [recentEvents, setRecentEvents] = useState<EventDto[]>([]);
  const [channelSummaries, setChannelSummaries] = useState<ChannelSummaryInfo[]>([]);
  const [isLoading, setIsLoading] = useState<boolean>(true);
  const [showCriticalModal, setShowCriticalModal] = useState<boolean>(false);
  const [criticalEventsList, setCriticalEventsList] = useState<EventDto[]>([]);

  const handleOpenCriticalModal = () => {
    const filtered = recentEvents.filter(
      (e) => e.risk === 'Critical' || e.level === 'Critical' || e.level === 'Error' || e.id === 4625
    );
    setCriticalEventsList(filtered.length > 0 ? filtered : recentEvents);
    setShowCriticalModal(true);
  };

  // System Hardware Telemetry & User Sessions State
  const defaultMetrics: SystemMetricsData = {
    cpuUsagePercent: 0,
    memoryUsagePercent: 0,
    memoryUsedMB: 0,
    memoryTotalMB: 0,
    diskUsagePercent: 0,
    diskReadMBps: 0,
    diskWriteMBps: 0,
    networkUsageMbps: 0,
    topProcesses: [],
    activeUserSessions: [],
    expiredUserSessions: [],
    systemUsers: [],
    rdpSessions: [],
    systemServices: [],
  };

  const [metrics, setMetrics] = useState<SystemMetricsData>(defaultMetrics);
  const [serviceSearchQuery, setServiceSearchQuery] = useState<string>('');
  const [processSearchQuery, setProcessSearchQuery] = useState<string>('');
  const [processSortBy, setProcessSortBy] = useState<'cpu' | 'ram' | 'netRead' | 'netWrite' | 'pid' | 'name'>('cpu');
  const [processSortOrder, setProcessSortOrder] = useState<'asc' | 'desc'>('desc');
  const [processFilterMode, setProcessFilterMode] = useState<'all' | 'high_cpu' | 'high_ram' | 'active_net'>('all');

  const filteredAndSortedProcesses = useMemo(() => {
    if (!metrics || !metrics.topProcesses) return [];
    let list = [...metrics.topProcesses];

    if (processFilterMode === 'high_cpu') {
      list = list.filter((p) => p.cpuUsagePercent > 1.0);
    } else if (processFilterMode === 'high_ram') {
      list = list.filter((p) => p.memoryUsageMB > 100);
    } else if (processFilterMode === 'active_net') {
      list = list.filter((p) => (p.networkReadBytes > 0 || p.networkWriteBytes > 0));
    }

    if (processSearchQuery.trim()) {
      const q = processSearchQuery.toLowerCase();
      list = list.filter((p) =>
        p.name.toLowerCase().includes(q) ||
        p.processId.toString().includes(q) ||
        p.path.toLowerCase().includes(q) ||
        p.commandLine.toLowerCase().includes(q)
      );
    }

    list.sort((a, b) => {
      let valA: number | string = 0;
      let valB: number | string = 0;
      if (processSortBy === 'cpu') {
        valA = a.cpuUsagePercent;
        valB = b.cpuUsagePercent;
      } else if (processSortBy === 'ram') {
        valA = a.memoryUsageMB;
        valB = b.memoryUsageMB;
      } else if (processSortBy === 'netRead') {
        valA = a.networkReadBytes || 0;
        valB = b.networkReadBytes || 0;
      } else if (processSortBy === 'netWrite') {
        valA = a.networkWriteBytes || 0;
        valB = b.networkWriteBytes || 0;
      } else if (processSortBy === 'pid') {
        valA = a.processId;
        valB = b.processId;
      } else if (processSortBy === 'name') {
        valA = a.name.toLowerCase();
        valB = b.name.toLowerCase();
      }

      if (typeof valA === 'string' && typeof valB === 'string') {
        return processSortOrder === 'asc' ? valA.localeCompare(valB) : valB.localeCompare(valA);
      }
      return processSortOrder === 'asc' ? ((valA as number) - (valB as number)) : ((valB as number) - (valA as number));
    });

    return list;
  }, [metrics?.topProcesses, processSearchQuery, processSortBy, processSortOrder, processFilterMode]);

  const toggleProcessSort = (field: 'cpu' | 'ram' | 'netRead' | 'netWrite' | 'pid' | 'name') => {
    if (processSortBy === field) {
      setProcessSortOrder((prev) => (prev === 'asc' ? 'desc' : 'asc'));
    } else {
      setProcessSortBy(field);
      setProcessSortOrder('desc');
    }
  };

  const workerRef = React.useRef<Worker | null>(null);
  const isFetchingDashboardRef = React.useRef<boolean>(false);

  useEffect(() => {
    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    fetchDashboardChannelsAndEvents(baseUrl);
    setIsLoading(false);

    const worker = new Worker(new URL('../telemetry.worker.ts', import.meta.url), { type: 'module' });
    workerRef.current = worker;

    worker.onmessage = (event: MessageEvent) => {
      const { type, payload } = event.data;

      if (type === 'METRICS_CPU_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          cpuUsagePercent: payload,
        }));
      } else if (type === 'METRICS_MEMORY_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          ...payload,
        }));
      } else if (type === 'METRICS_DISK_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          ...payload,
        }));
      } else if (type === 'METRICS_NETWORK_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          networkUsageMbps: payload,
        }));
      } else if (type === 'METRICS_SUMMARY_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          ...payload,
          topProcesses: (payload.topProcesses && payload.topProcesses.length > 0) ? payload.topProcesses : (prev?.topProcesses || []),
          activeUserSessions: (payload.activeUserSessions && payload.activeUserSessions.length > 0) ? payload.activeUserSessions : (prev?.activeUserSessions || []),
          systemUsers: (payload.systemUsers && payload.systemUsers.length > 0) ? payload.systemUsers : (prev?.systemUsers || []),
          rdpSessions: (payload.rdpSessions && payload.rdpSessions.length > 0) ? payload.rdpSessions : (prev?.rdpSessions || []),
          systemServices: (payload.systemServices && payload.systemServices.length > 0) ? payload.systemServices : (prev?.systemServices || []),
        }));
      } else if (type === 'METRICS_PROCESSES_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          topProcesses: Array.isArray(payload) && payload.length > 0 ? payload : (prev?.topProcesses || []),
        }));
      } else if (type === 'METRICS_SESSIONS_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          ...payload,
          topProcesses: prev?.topProcesses || [],
        }));
      } else if (type === 'METRICS_SERVICES_UPDATED') {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          systemServices: payload,
        }));
      }
    };

    const channelRefreshInterval = setInterval(() => {
      fetchDashboardChannelsAndEvents(baseUrl);
    }, 15000);

    worker.postMessage({ type: 'START_POLLING', baseUrl, subTab: activeDashboardTab });

    return () => {
      clearInterval(channelRefreshInterval);
      worker.postMessage({ type: 'STOP_POLLING' });
      worker.terminate();
      workerRef.current = null;
    };
  }, []);

  useEffect(() => {
    if (workerRef.current) {
      workerRef.current.postMessage({ type: 'CHANGE_SUBTAB', subTab: activeDashboardTab });
    }
  }, [activeDashboardTab]);

  const fetchDashboardChannelsAndEvents = async (baseUrl: string) => {
    if (isFetchingDashboardRef.current) return;
    isFetchingDashboardRef.current = true;
    try {
      // 1. Fetch channel count from native backend
      const channelsData = await fetchApiChannels(baseUrl).catch(() => ({ channels: [] }));
      const channelList = channelsData.channels || (Array.isArray(channelsData) ? (channelsData as unknown as string[]) : []);
      setTotalChannels(channelList.length || 18);

      // 2. Fetch channel summaries via dedicated summary endpoint
      const targetChannels = ['Security', 'System', 'Application'];
      const summaryPromises = targetChannels.map((ch) =>
        fetchEventSummary(ch, baseUrl).catch((): EventSummaryData => ({ totalCount: 0, criticalCount: 0, errorCount: 0, warningCount: 0, infoCount: 0, verboseCount: 0 }))
      );

      const summaryResults = await Promise.all(summaryPromises);

      let aggregatedTotal = 0;
      let aggregatedCriticals = 0;
      const summaries: ChannelSummaryInfo[] = [];

      summaryResults.forEach((res, i) => {
        const channelName = targetChannels[i];
        const count = res.totalCount || 0;
        const crit = res.criticalCount || 0;
        const err = res.errorCount || 0;
        const warn = res.warningCount || 0;
        const info = res.infoCount || 0;
        aggregatedTotal += count;
        aggregatedCriticals += crit + err;

        summaries.push({
          name: channelName,
          totalCount: count,
          criticalCount: crit,
          errorCount: err,
          warningCount: warn,
          infoCount: info,
        });
      });

      setChannelSummaries(summaries);
      setTotalEvents(aggregatedTotal);
      setCriticalRisks(aggregatedCriticals);

      // 3. Fetch recent events for main channel
      const eventsData = await fetchApiEvents('Security', baseUrl, 1, 10).catch((): EventsData => ({ events: [] }));
      const mappedEvents = ((eventsData.events as unknown as Array<Record<string, unknown>>) || []).map((e, idx) => ({
        idx: (e.index as number) || idx + 1,
        id: (e.id as number) || 0,
        level: ((e.level as string) || 'Information') as EventDto['level'],
        risk: ((e.risk as string) || 'Low') as EventDto['risk'],
        provider: (e.provider as string) || 'Security',
        time: formatTo12Hour(e.time as string),
        desc: (e.message as string) || `Event #${e.id} in Security`,
      }));
      setRecentEvents(mappedEvents);

      // 4. Fetch initial system metrics immediately so gauges populate upon load
      const initialMetrics = await fetchMetricsSummary(baseUrl).catch(() => null);
      if (initialMetrics) {
        setMetrics((prev) => ({
          ...(prev || defaultMetrics),
          ...initialMetrics,
          topProcesses: (initialMetrics.topProcesses && initialMetrics.topProcesses.length > 0) ? initialMetrics.topProcesses : (prev?.topProcesses || []),
        }));
      }
    } catch (err) {
      console.error('[DASHBOARD DEBUG] Error fetching dashboard channels and events:', err);
    } finally {
      isFetchingDashboardRef.current = false;
    }
  };

  if (isLoading || !metrics) {
    return (
      <div style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', color: '#38bdf8', gap: '12px', fontSize: '0.9rem' }}>
        <div style={{ width: '32px', height: '32px', border: '3px solid rgba(56,189,248,0.2)', borderTopColor: '#38bdf8', borderRadius: '50%', animation: 'spin 0.8s linear infinite' }} />
        <span>Loading live system telemetry & risk metrics...</span>
      </div>
    );
  }

  return (
    <div style={{ flex: 1, minHeight: 0, padding: '16px', display: 'flex', flexDirection: 'column', gap: '14px', overflowY: 'auto', fontSize: '0.78rem' }}>
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
          <span style={{ fontSize: '0.7rem', color: '#38bdf8', fontWeight: 600 }}>1-Sec Real-Time Polling Active</span>
        </div>
      </header>

      {/* Dashboard Section Sub-Navigation Tabs */}
      <div style={{ display: 'flex', gap: '8px', borderBottom: '1px solid rgba(255,255,255,0.1)', paddingBottom: '8px', overflowX: 'auto', flexShrink: 0, whiteSpace: 'nowrap' }}>
        <button
          onClick={() => setActiveDashboardTab('overview')}
          style={{
            background: activeDashboardTab === 'overview' ? 'rgba(56, 189, 248, 0.2)' : 'transparent',
            color: activeDashboardTab === 'overview' ? '#38bdf8' : '#94a3b8',
            border: activeDashboardTab === 'overview' ? '1px solid #38bdf8' : '1px solid transparent',
            borderRadius: '6px',
            padding: '6px 14px',
            fontSize: '0.76rem',
            fontWeight: 700,
            cursor: 'pointer',
            flexShrink: 0,
            transition: 'all 0.15s',
          }}
        >
          📊 Overview & Telemetry
        </button>

        <button
          onClick={() => setActiveDashboardTab('processes')}
          style={{
            background: activeDashboardTab === 'processes' ? 'rgba(56, 189, 248, 0.2)' : 'transparent',
            color: activeDashboardTab === 'processes' ? '#38bdf8' : '#94a3b8',
            border: activeDashboardTab === 'processes' ? '1px solid #38bdf8' : '1px solid transparent',
            borderRadius: '6px',
            padding: '6px 14px',
            fontSize: '0.76rem',
            fontWeight: 700,
            cursor: 'pointer',
            flexShrink: 0,
            transition: 'all 0.15s',
          }}
        >
          🔥 Process Activity ({metrics?.topProcesses ? metrics.topProcesses.length : 0})
        </button>

        <button
          onClick={() => setActiveDashboardTab('sessions')}
          style={{
            background: activeDashboardTab === 'sessions' ? 'rgba(56, 189, 248, 0.2)' : 'transparent',
            color: activeDashboardTab === 'sessions' ? '#38bdf8' : '#94a3b8',
            border: activeDashboardTab === 'sessions' ? '1px solid #38bdf8' : '1px solid transparent',
            borderRadius: '6px',
            padding: '6px 14px',
            fontSize: '0.76rem',
            fontWeight: 700,
            cursor: 'pointer',
            flexShrink: 0,
            transition: 'all 0.15s',
          }}
        >
          🖥️ RDP & User Sessions ({metrics?.rdpSessions ? metrics.rdpSessions.length : 0})
        </button>

        <button
          onClick={() => setActiveDashboardTab('users')}
          style={{
            background: activeDashboardTab === 'users' ? 'rgba(168, 85, 247, 0.2)' : 'transparent',
            color: activeDashboardTab === 'users' ? '#c084fc' : '#94a3b8',
            border: activeDashboardTab === 'users' ? '1px solid #c084fc' : '1px solid transparent',
            borderRadius: '6px',
            padding: '6px 14px',
            fontSize: '0.76rem',
            fontWeight: 700,
            cursor: 'pointer',
            flexShrink: 0,
            transition: 'all 0.15s',
          }}
        >
          👥 User Principals ({metrics?.systemUsers ? metrics.systemUsers.length : 0})
        </button>

        <button
          onClick={() => setActiveDashboardTab('services')}
          style={{
            background: activeDashboardTab === 'services' ? 'rgba(34, 197, 94, 0.2)' : 'transparent',
            color: activeDashboardTab === 'services' ? '#4ade80' : '#94a3b8',
            border: activeDashboardTab === 'services' ? '1px solid #4ade80' : '1px solid transparent',
            borderRadius: '6px',
            padding: '6px 14px',
            fontSize: '0.76rem',
            fontWeight: 700,
            cursor: 'pointer',
            flexShrink: 0,
            transition: 'all 0.15s',
          }}
        >
          ⚙️ Services ({metrics?.systemServices ? metrics.systemServices.length : 0})
        </button>
      </div>

      {/* Sub-Tab 1: OVERVIEW & TELEMETRY */}
      {activeDashboardTab === 'overview' && (
        <>
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

          {/* Live Hardware Telemetry Gauges */}
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
                    <div style={{ display: 'flex', gap: '6px', fontSize: '0.65rem', fontWeight: 600 }}>
                      {ch.criticalCount > 0 && <span style={{ color: '#f87171' }}>🚨 {ch.criticalCount} Crit</span>}
                      {ch.errorCount > 0 && <span style={{ color: '#fb7185' }}>❌ {ch.errorCount} Err</span>}
                      {ch.warningCount > 0 && <span style={{ color: '#fbbf24' }}>⚠️ {ch.warningCount.toLocaleString()} Warn</span>}
                      {ch.criticalCount === 0 && ch.errorCount === 0 && ch.warningCount === 0 && <span style={{ color: '#4ade80' }}>✓ {ch.infoCount.toLocaleString()} Info</span>}
                    </div>
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
                        <td style={{ padding: '5px' }}>
                          <span
                            style={{
                              padding: '1px 6px',
                              borderRadius: '3px',
                              fontWeight: 700,
                              fontSize: '0.62rem',
                              background:
                                evt.level === 'Critical' ? 'rgba(239, 68, 68, 0.25)' :
                                evt.level === 'Error' ? 'rgba(245, 158, 11, 0.25)' :
                                evt.level === 'Warning' ? 'rgba(234, 179, 8, 0.25)' : 'rgba(56, 189, 248, 0.15)',
                              color:
                                evt.level === 'Critical' ? '#ef4444' :
                                evt.level === 'Error' ? '#f59e0b' :
                                evt.level === 'Warning' ? '#eab308' : '#38bdf8',
                              border:
                                evt.level === 'Critical' ? '1px solid rgba(239, 68, 68, 0.4)' :
                                evt.level === 'Error' ? '1px solid rgba(245, 158, 11, 0.4)' :
                                evt.level === 'Warning' ? '1px solid rgba(234, 179, 8, 0.4)' : '1px solid rgba(56, 189, 248, 0.3)',
                            }}
                          >
                            {evt.level}
                          </span>
                        </td>
                        <td style={{ padding: '5px' }}>
                          <span
                            style={{
                              padding: '1px 6px',
                              borderRadius: '3px',
                              fontWeight: 700,
                              fontSize: '0.62rem',
                              background:
                                evt.risk === 'Critical' ? 'rgba(239, 68, 68, 0.25)' :
                                evt.risk === 'High' ? 'rgba(245, 158, 11, 0.25)' :
                                evt.risk === 'Medium' ? 'rgba(234, 179, 8, 0.25)' : 'rgba(34, 197, 94, 0.15)',
                              color:
                                evt.risk === 'Critical' ? '#ef4444' :
                                evt.risk === 'High' ? '#f59e0b' :
                                evt.risk === 'Medium' ? '#eab308' : '#22c55e',
                              border:
                                evt.risk === 'Critical' ? '1px solid rgba(239, 68, 68, 0.4)' :
                                evt.risk === 'High' ? '1px solid rgba(245, 158, 11, 0.4)' :
                                evt.risk === 'Medium' ? '1px solid rgba(234, 179, 8, 0.4)' : '1px solid rgba(34, 197, 94, 0.3)',
                            }}
                          >
                            {evt.risk}
                          </span>
                        </td>
                        <td style={{ padding: '5px', color: '#94a3b8' }}>{evt.provider}</td>
                      </tr>
                    ))
                  )}
                </tbody>
              </table>
            </div>
          </section>
        </>
      )}

      {/* Sub-Tab 2: PROCESS ACTIVITY */}
      {activeDashboardTab === 'processes' && (
        <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '10px' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '8px' }}>
            <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
              <h3 style={{ color: '#38bdf8', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
                ⚡ System Process Telemetry
              </h3>
              <span style={{ fontSize: '0.68rem', color: '#94a3b8', background: 'rgba(15, 23, 42, 0.6)', padding: '2px 8px', borderRadius: '12px', border: '1px solid rgba(255,255,255,0.08)' }}>
                Total: <strong style={{ color: '#f8fafc' }}>{metrics?.topProcesses?.length || 0}</strong> | Showing: <strong style={{ color: '#38bdf8' }}>{filteredAndSortedProcesses.length}</strong>
              </span>
            </div>

            {/* Quick Filter Pills */}
            <div style={{ display: 'flex', gap: '6px', alignItems: 'center' }}>
              {[
                { id: 'all', label: 'All' },
                { id: 'high_cpu', label: '🔥 High CPU (>1%)' },
                { id: 'high_ram', label: '💾 High RAM (>100MB)' },
                { id: 'active_net', label: '🌐 Active Net' },
              ].map((filter) => (
                <button
                  key={filter.id}
                  onClick={() => setProcessFilterMode(filter.id as typeof processFilterMode)}
                  style={{
                    padding: '3px 8px',
                    fontSize: '0.65rem',
                    fontWeight: 600,
                    borderRadius: '4px',
                    border: 'none',
                    cursor: 'pointer',
                    background: processFilterMode === filter.id ? '#0284c7' : 'rgba(15, 23, 42, 0.7)',
                    color: processFilterMode === filter.id ? '#ffffff' : '#94a3b8',
                    transition: 'all 0.15s ease',
                  }}
                >
                  {filter.label}
                </button>
              ))}
            </div>

            {/* Process Search Input */}
            <div style={{ position: 'relative', width: '220px' }}>
              <input
                type="text"
                placeholder="Search name, PID, path..."
                value={processSearchQuery}
                onChange={(e) => setProcessSearchQuery(e.target.value)}
                style={{
                  width: '100%',
                  padding: '4px 24px 4px 8px',
                  fontSize: '0.68rem',
                  background: 'rgba(15, 23, 42, 0.8)',
                  border: '1px solid rgba(255, 255, 255, 0.15)',
                  borderRadius: '4px',
                  color: '#f8fafc',
                  outline: 'none',
                }}
              />
              {processSearchQuery && (
                <button
                  onClick={() => setProcessSearchQuery('')}
                  style={{
                    position: 'absolute',
                    right: '6px',
                    top: '50%',
                    transform: 'translateY(-50%)',
                    background: 'transparent',
                    border: 'none',
                    color: '#94a3b8',
                    cursor: 'pointer',
                    fontSize: '0.7rem',
                    padding: 0,
                  }}
                >
                  ✕
                </button>
              )}
            </div>
          </div>

          <div style={{ overflowX: 'auto', maxHeight: '520px', overflowY: 'auto' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
              <thead>
                <tr style={{ background: 'rgba(15, 23, 42, 0.95)', color: '#94a3b8', fontSize: '0.68rem', position: 'sticky', top: 0, zIndex: 2 }}>
                  <th onClick={() => toggleProcessSort('pid')} style={{ padding: '6px', cursor: 'pointer', userSelect: 'none' }}>
                    PID {processSortBy === 'pid' ? (processSortOrder === 'asc' ? '▲' : '▼') : ''}
                  </th>
                  <th onClick={() => toggleProcessSort('name')} style={{ padding: '6px', cursor: 'pointer', userSelect: 'none' }}>
                    Process Name {processSortBy === 'name' ? (processSortOrder === 'asc' ? '▲' : '▼') : ''}
                  </th>
                  <th onClick={() => toggleProcessSort('cpu')} style={{ padding: '6px', cursor: 'pointer', userSelect: 'none' }}>
                    CPU % {processSortBy === 'cpu' ? (processSortOrder === 'asc' ? '▲' : '▼') : ''}
                  </th>
                  <th onClick={() => toggleProcessSort('ram')} style={{ padding: '6px', cursor: 'pointer', userSelect: 'none' }}>
                    RAM (MB) {processSortBy === 'ram' ? (processSortOrder === 'asc' ? '▲' : '▼') : ''}
                  </th>
                  <th onClick={() => toggleProcessSort('netRead')} style={{ padding: '6px', cursor: 'pointer', userSelect: 'none' }}>
                    Net Read {processSortBy === 'netRead' ? (processSortOrder === 'asc' ? '▲' : '▼') : ''}
                  </th>
                  <th onClick={() => toggleProcessSort('netWrite')} style={{ padding: '6px', cursor: 'pointer', userSelect: 'none' }}>
                    Net Write {processSortBy === 'netWrite' ? (processSortOrder === 'asc' ? '▲' : '▼') : ''}
                  </th>
                  <th style={{ padding: '6px' }}>Open Ports</th>
                  <th style={{ padding: '6px' }}>Conn Status</th>
                  <th style={{ padding: '6px' }}>Executable Path</th>
                  <th style={{ padding: '6px' }}>Command Line</th>
                </tr>
              </thead>
              <tbody>
                {filteredAndSortedProcesses.length === 0 ? (
                  <tr>
                    <td colSpan={10} style={{ padding: '16px', textAlign: 'center', color: '#94a3b8' }}>
                      {processSearchQuery ? `No processes match "${processSearchQuery}"` : 'No processes available.'}
                    </td>
                  </tr>
                ) : (
                  filteredAndSortedProcesses.map((p, idx) => {
                    const isProtected = p.path.includes('Access Denied') || p.commandLine.includes('Access Denied');
                    const cpuDisplay = (p.cpuUsagePercent < 0 || (isProtected && p.cpuUsagePercent === 0)) ? 'Protected' : p.cpuUsagePercent.toFixed(1);
                    const ramDisplay = (p.memoryUsageMB <= 0 || (isProtected && p.memoryUsageMB === 0)) ? 'Protected' : p.memoryUsageMB.toString();
                    
                    const netReadDisplay = isProtected ? 'Protected' : (p.networkReadBytes ? p.networkReadBytes.toLocaleString() + ' B' : '0 B');
                    const netWriteDisplay = isProtected ? 'Protected' : (p.networkWriteBytes ? p.networkWriteBytes.toLocaleString() + ' B' : '0 B');
                    const portsDisplay = isProtected ? 'Protected' : (p.openPorts || '-');

                    return (
                      <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                        <td style={{ padding: '5px', fontWeight: 700, color: '#38bdf8' }}>{p.processId}</td>
                        <td style={{ padding: '5px', fontWeight: 600, color: '#f8fafc' }}>{p.name}</td>
                        <td style={{ padding: '5px', color: cpuDisplay === 'Protected' ? '#94a3b8' : (p.cpuUsagePercent > 5 ? '#f87171' : '#e2e8f0'), fontWeight: 700, fontStyle: cpuDisplay === 'Protected' ? 'italic' : 'normal' }}>{cpuDisplay}</td>
                        <td style={{ padding: '5px', color: ramDisplay === 'Protected' ? '#94a3b8' : '#c084fc', fontWeight: 600, fontStyle: ramDisplay === 'Protected' ? 'italic' : 'normal' }}>{ramDisplay}</td>
                        <td style={{ padding: '5px', color: netReadDisplay === 'Protected' ? '#94a3b8' : '#38bdf8', fontStyle: netReadDisplay === 'Protected' ? 'italic' : 'normal' }}>{netReadDisplay}</td>
                        <td style={{ padding: '5px', color: netWriteDisplay === 'Protected' ? '#94a3b8' : '#fbbf24', fontStyle: netWriteDisplay === 'Protected' ? 'italic' : 'normal' }}>{netWriteDisplay}</td>
                        <td style={{ padding: '5px', color: portsDisplay === 'Protected' ? '#94a3b8' : '#38bdf8', fontWeight: 600, fontStyle: portsDisplay === 'Protected' ? 'italic' : 'normal' }}>{portsDisplay}</td>
                        <td style={{ padding: '5px' }}>
                          {isProtected ? (
                            <span style={{ color: '#94a3b8', fontStyle: 'italic' }}>Protected</span>
                          ) : p.connectionEstablished ? (
                            <span style={{ background: 'rgba(74,222,128,0.2)', color: '#4ade80', padding: '1px 6px', borderRadius: '3px', fontWeight: 700, fontSize: '0.62rem' }}>
                              ESTABLISHED
                            </span>
                          ) : (
                            <span style={{ background: 'rgba(148,163,184,0.15)', color: '#94a3b8', padding: '1px 6px', borderRadius: '3px', fontWeight: 600, fontSize: '0.62rem' }}>
                              LISTEN / CLOSED
                            </span>
                          )}
                        </td>
                        <td style={{ padding: '5px', fontFamily: 'monospace', fontSize: '0.65rem', color: isProtected ? '#f87171' : '#94a3b8', wordBreak: 'break-all', whiteSpace: 'normal', maxWidth: '200px' }}>{p.path}</td>
                        <td style={{ padding: '5px', fontFamily: 'monospace', fontSize: '0.65rem', color: isProtected ? '#f87171' : '#cbd5e1', wordBreak: 'break-all', whiteSpace: 'normal', maxWidth: '240px' }}>{p.commandLine}</td>
                      </tr>
                    );
                  })
                )}
              </tbody>
            </table>
          </div>
        </section>
      )}

      {/* Sub-Tab 3: RDP & USER SESSIONS */}
      {activeDashboardTab === 'sessions' && (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '14px' }}>
          {/* DotNetDupe TerminalSession API RDP Sessions Section */}
          <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(56, 189, 248, 0.4)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
            <h3 style={{ color: '#38bdf8', margin: 0, fontSize: '0.85rem', fontWeight: 700, display: 'flex', alignItems: 'center', gap: '6px' }}>
              🖥️ Active Terminal / RDP Sessions (DotNetDupe TerminalSession API)
            </h3>
            <div style={{ overflowX: 'auto' }}>
              <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
                <thead>
                  <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem' }}>
                    <th style={{ padding: '6px' }}>Session ID</th>
                    <th style={{ padding: '6px' }}>Session Name</th>
                    <th style={{ padding: '6px' }}>User Name</th>
                    <th style={{ padding: '6px' }}>Domain</th>
                    <th style={{ padding: '6px' }}>Client Workstation</th>
                    <th style={{ padding: '6px' }}>Client IP Address</th>
                    <th style={{ padding: '6px' }}>Session State</th>
                    <th style={{ padding: '6px' }}>Protocol Type</th>
                  </tr>
                </thead>
                <tbody>
                  {(!metrics.rdpSessions || metrics.rdpSessions.length === 0) ? (
                    <tr>
                      <td colSpan={8} style={{ padding: '10px', textAlign: 'center', color: '#94a3b8' }}>
                        No active Terminal/RDP sessions enumerated via DotNetDupe API.
                      </td>
                    </tr>
                  ) : (
                    metrics.rdpSessions.map((r, idx) => (
                      <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                        <td style={{ padding: '5px', fontWeight: 700, color: '#38bdf8' }}>#{r.sessionId}</td>
                        <td style={{ padding: '5px', fontWeight: 600, color: '#f8fafc' }}>{r.sessionName || 'Console'}</td>
                        <td style={{ padding: '5px', color: r.userName ? '#4ade80' : '#94a3b8', fontWeight: r.userName ? 700 : 400 }}>{r.userName || '-'}</td>
                        <td style={{ padding: '5px', color: '#94a3b8' }}>{r.domainName || '-'}</td>
                        <td style={{ padding: '5px', color: '#cbd5e1' }}>{r.clientName || '-'}</td>
                        <td style={{ padding: '5px', fontFamily: 'monospace', fontSize: '0.68rem', color: '#fbbf24' }}>{r.clientIpAddress || '-'}</td>
                        <td style={{ padding: '5px' }}>
                          <span
                            style={{
                              padding: '1px 6px',
                              borderRadius: '3px',
                              fontSize: '0.62rem',
                              fontWeight: 700,
                              background: r.state === 'Active' ? 'rgba(74,222,128,0.2)' : r.state === 'Connected' ? 'rgba(56,189,248,0.2)' : 'rgba(148,163,184,0.2)',
                              color: r.state === 'Active' ? '#4ade80' : r.state === 'Connected' ? '#38bdf8' : '#94a3b8',
                            }}
                          >
                            {r.state}
                          </span>
                        </td>
                        <td style={{ padding: '5px' }}>
                          <span style={{ padding: '1px 6px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: r.isRdpSession ? 'rgba(168,85,247,0.2)' : 'rgba(148,163,184,0.15)', color: r.isRdpSession ? '#c084fc' : '#94a3b8' }}>
                            {r.isRdpSession ? '🌐 Remote RDP' : '💻 Local Console'}
                          </span>
                        </td>
                      </tr>
                    ))
                  )}
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
                      <td style={{ padding: '5px' }}>{formatTo12Hour(s.loginTimestamp)}</td>
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
                      <td style={{ padding: '5px' }}>{formatTo12Hour(s.loginTimestamp)}</td>
                      <td style={{ padding: '5px', color: '#f87171', fontWeight: 600 }}>{formatTo12Hour(s.logoutTimestamp)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </section>
          </div>
        </div>
      )}

      {/* Sub-Tab 4: USER PRINCIPALS */}
      {activeDashboardTab === 'users' && (
        <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(168, 85, 247, 0.4)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
          <h3 style={{ color: '#c084fc', margin: 0, fontSize: '0.85rem', fontWeight: 700, display: 'flex', alignItems: 'center', gap: '6px' }}>
            👥 Registered System User Accounts & Access Groups (DotNetDupe UserPrincipals API)
          </h3>
          <div style={{ overflowX: 'auto' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
              <thead>
                <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem' }}>
                  <th style={{ padding: '6px' }}>User Name</th>
                  <th style={{ padding: '6px' }}>Domain</th>
                  <th style={{ padding: '6px' }}>Security SID / UID</th>
                  <th style={{ padding: '6px' }}>Account Class</th>
                  <th style={{ padding: '6px' }}>Account Status</th>
                  <th style={{ padding: '6px' }}>Access Groups</th>
                  <th style={{ padding: '6px' }}>Permissions</th>
                </tr>
              </thead>
              <tbody>
                {(!metrics.systemUsers || metrics.systemUsers.length === 0) ? (
                  <tr>
                    <td colSpan={7} style={{ padding: '10px', textAlign: 'center', color: '#94a3b8' }}>
                      Loading System User Principals via DotNetDupe API...
                    </td>
                  </tr>
                ) : (
                  metrics.systemUsers.map((u, idx) => (
                    <tr key={idx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                      <td style={{ padding: '5px', fontWeight: 700, color: '#f8fafc' }}>{u.username}</td>
                      <td style={{ padding: '5px', color: '#94a3b8' }}>{u.domain || 'LOCAL'}</td>
                      <td style={{ padding: '5px', fontFamily: 'monospace', fontSize: '0.68rem', color: '#38bdf8' }}>{u.sidOrUid}</td>
                      <td style={{ padding: '5px' }}>
                        <span style={{ padding: '1px 5px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: u.userClass === 'Admin' ? 'rgba(248,113,113,0.25)' : 'rgba(56,189,248,0.2)', color: u.userClass === 'Admin' ? '#f87171' : '#38bdf8' }}>
                          {u.userClass}
                        </span>
                      </td>
                      <td style={{ padding: '5px' }}>
                        <span style={{ padding: '1px 5px', borderRadius: '3px', fontSize: '0.62rem', fontWeight: 700, background: u.isDisabled ? 'rgba(248,113,113,0.2)' : 'rgba(74,222,128,0.2)', color: u.isDisabled ? '#f87171' : '#4ade80' }}>
                          {u.isDisabled ? 'Disabled' : 'Active'}
                        </span>
                      </td>
                      <td style={{ padding: '5px' }}>
                        <div style={{ display: 'flex', gap: '4px', flexWrap: 'wrap' }}>
                          {u.groups && u.groups.length > 0 ? (
                            u.groups.map((g, gIdx) => (
                              <span key={gIdx} style={{ background: 'rgba(168, 85, 247, 0.2)', color: '#c084fc', border: '1px solid rgba(168, 85, 247, 0.4)', borderRadius: '3px', padding: '1px 5px', fontSize: '0.62rem', fontWeight: 600 }}>
                                {g}
                              </span>
                            ))
                          ) : (
                            <span style={{ color: '#94a3b8', fontStyle: 'italic' }}>None</span>
                          )}
                        </div>
                      </td>
                      <td style={{ padding: '5px', color: '#94a3b8', fontSize: '0.68rem' }}>
                        {u.permissions && u.permissions.length > 0 ? u.permissions.join(', ') : 'Standard User Access'}
                      </td>
                    </tr>
                  ))
                )}
              </tbody>
            </table>
          </div>
        </section>
      )}

      {/* Sub-Tab 5: SYSTEM SERVICES */}
      {activeDashboardTab === 'services' && (
        <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '8px', padding: '12px', display: 'flex', flexDirection: 'column', gap: '8px' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '10px' }}>
            <h3 style={{ color: '#4ade80', margin: 0, fontSize: '0.85rem', fontWeight: 700 }}>
              ⚙️ System Services Overview
            </h3>

            <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
              <div style={{ position: 'relative' }}>
                <input
                  type="text"
                  placeholder="Filter by name, status, start type..."
                  value={serviceSearchQuery}
                  onChange={(e) => setServiceSearchQuery(e.target.value)}
                  style={{
                    background: 'rgba(15, 23, 42, 0.8)',
                    color: '#e2e8f0',
                    border: '1px solid rgba(74, 222, 128, 0.4)',
                    borderRadius: '6px',
                    padding: '4px 10px 4px 26px',
                    fontSize: '0.72rem',
                    outline: 'none',
                    width: '240px',
                  }}
                />
                <span style={{ position: 'absolute', left: '8px', top: '50%', transform: 'translateY(-50%)', fontSize: '0.7rem', color: '#94a3b8' }}>
                  🔍
                </span>
                {serviceSearchQuery && (
                  <button
                    onClick={() => setServiceSearchQuery('')}
                    style={{ position: 'absolute', right: '6px', top: '50%', transform: 'translateY(-50%)', background: 'transparent', border: 'none', color: '#94a3b8', cursor: 'pointer', fontSize: '0.7rem' }}
                  >
                    ✕
                  </button>
                )}
              </div>

              <span style={{ fontSize: '0.7rem', color: '#94a3b8' }}>
                Total Services: <strong style={{ color: '#4ade80' }}>{metrics?.systemServices ? metrics.systemServices.length : 0}</strong>
              </span>
            </div>
          </div>

          <div style={{ overflowY: 'auto', maxHeight: '500px' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
              <thead>
                <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem', position: 'sticky', top: 0 }}>
                  <th style={{ padding: '6px' }}>Service Name</th>
                  <th style={{ padding: '6px' }}>Display Name</th>
                  <th style={{ padding: '6px' }}>Status</th>
                  <th style={{ padding: '6px' }}>Start Type</th>
                  <th style={{ padding: '6px' }}>Process ID</th>
                </tr>
              </thead>
              <tbody>
                {!metrics?.systemServices || metrics.systemServices.length === 0 ? (
                  <tr>
                    <td colSpan={5} style={{ padding: '12px', textAlign: 'center', color: '#94a3b8' }}>
                      Querying system services...
                    </td>
                  </tr>
                ) : (
                  metrics.systemServices
                    .filter((svc) => {
                      if (!serviceSearchQuery.trim()) return true;
                      const q = serviceSearchQuery.toLowerCase().trim();
                      return (
                        svc.serviceName.toLowerCase().includes(q) ||
                        svc.displayName.toLowerCase().includes(q) ||
                        svc.status.toLowerCase().includes(q) ||
                        svc.startType.toLowerCase().includes(q) ||
                        String(svc.processId).includes(q)
                      );
                    })
                    .map((svc, sIdx) => (
                      <tr key={sIdx} style={{ borderBottom: '1px solid rgba(255,255,255,0.05)' }}>
                        <td style={{ padding: '5px', fontWeight: 700, color: '#38bdf8' }}>{svc.serviceName}</td>
                        <td style={{ padding: '5px', color: '#e2e8f0' }}>{svc.displayName || svc.serviceName}</td>
                        <td style={{ padding: '5px' }}>
                          <span
                            style={{
                              padding: '2px 8px',
                              borderRadius: '4px',
                              fontSize: '0.65rem',
                              fontWeight: 700,
                              background: svc.status === 'Running' ? 'rgba(74, 222, 128, 0.2)' : 'rgba(148, 163, 184, 0.2)',
                              color: svc.status === 'Running' ? '#4ade80' : '#94a3b8',
                              border: svc.status === 'Running' ? '1px solid rgba(74, 222, 128, 0.4)' : '1px solid rgba(148, 163, 184, 0.3)',
                            }}
                          >
                            {svc.status}
                          </span>
                        </td>
                        <td style={{ padding: '5px', color: '#cbd5e1' }}>{svc.startType}</td>
                        <td style={{ padding: '5px', fontFamily: 'monospace', color: svc.processId > 0 ? '#38bdf8' : '#64748b' }}>
                          {svc.processId > 0 ? svc.processId : '-'}
                        </td>
                      </tr>
                    ))
                )}
              </tbody>
            </table>
          </div>
        </section>
      )}

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
                          background: (evt.level === 'Critical' || evt.risk === 'Critical') ? 'rgba(248,113,113,0.25)' : ((evt.level === 'Error' || evt.risk === 'High') ? 'rgba(248,113,113,0.15)' : ((evt.level === 'Warning' || evt.risk === 'Medium') ? 'rgba(251,191,36,0.2)' : 'rgba(74,222,128,0.2)')),
                          color: (evt.level === 'Critical' || evt.risk === 'Critical' || evt.level === 'Error' || evt.risk === 'High') ? '#f87171' : ((evt.level === 'Warning' || evt.risk === 'Medium') ? '#fbbf24' : '#4ade80'),
                        }}
                      >
                        {evt.level !== 'Information' ? evt.level : evt.risk}
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

      {/* Live Server & AI Model Diagnostics Log Viewer Section */}
      <div style={{ marginTop: '24px' }}>
        <ServerLogsViewer />
      </div>
    </div>
  );
};

export default Dashboard;
