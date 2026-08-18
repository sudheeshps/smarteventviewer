export { formatUtcToLocal, formatTo12Hour } from './utils/timeUtils';
import { formatTo12Hour } from './utils/timeUtils';
import type { MultiChannelAnomaliesDto, EventDto } from './types';

export interface ChannelData {
  channels?: string[];
}

export interface EventSummaryData {
  channel?: string;
  totalCount?: number;
  criticalCount?: number;
  errorCount?: number;
  warningCount?: number;
  infoCount?: number;
  verboseCount?: number;
}

export interface EventsData {
  channel?: string;
  totalCount?: number;
  criticalCount?: number;
  errorCount?: number;
  warningCount?: number;
  infoCount?: number;
  verboseCount?: number;
  page?: number;
  pageSize?: number;
  totalPages?: number;
  events?: Array<{
    index: number;
    id: number;
    level: string;
    risk: string;
    provider: string;
    time: string;
    message: string;
  }>;
}

export async function fetchEventSummary(channel: string, baseUrl: string = ''): Promise<EventSummaryData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/events/summary?channel=${encodeURIComponent(channel)}`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  return {
    channel: data.channel || data.Channel || channel,
    totalCount: data.totalCount ?? data.TotalCount ?? 0,
    criticalCount: data.criticalCount ?? data.CriticalCount ?? 0,
    errorCount: data.errorCount ?? data.ErrorCount ?? 0,
    warningCount: data.warningCount ?? data.WarningCount ?? 0,
    infoCount: data.infoCount ?? data.InfoCount ?? 0,
    verboseCount: data.verboseCount ?? data.VerboseCount ?? 0,
  };
}

export interface AnalyzeData {
  taskId?: string;
  status?: string;
  progressMessage?: string;
  downloadProgress?: number;
  downloadRateBytesPerSec?: number;
  downloadedBytes?: number;
  totalBytes?: number;
  channel?: string;
  query?: string;
  analysis?: string;
  eventsAnalyzed?: number;
}

export interface ProcessResourceData {
  processId: number;
  name: string;
  path: string;
  commandLine: string;
  cpuUsagePercent: number;
  memoryUsageMB: number;
  networkReadBytes: number;
  networkWriteBytes: number;
  openPorts: string;
  connectionEstablished: boolean;
}

export interface UserSessionData {
  username: string;
  privilege: string;
  loginTimestamp: string;
  logoutTimestamp: string;
  isActive: boolean;
}

export interface UserPrincipalData {
  username: string;
  domain: string;
  sidOrUid: string;
  userClass: string;
  isDisabled: boolean;
  isAccountLocked: boolean;
  groups: string[];
  permissions: string[];
}

export interface RdpSessionData {
  sessionId: number;
  sessionName: string;
  userName: string;
  domainName: string;
  clientName: string;
  clientIpAddress: string;
  state: string;
  isRdpSession: boolean;
}

export interface ServiceInfoData {
  serviceName: string;
  displayName: string;
  status: string;
  startType: string;
  processId: number;
}

export interface SystemMetricsData {
  cpuUsagePercent: number;
  memoryUsagePercent: number;
  memoryUsedMB: number;
  memoryTotalMB: number;
  diskUsagePercent: number;
  diskReadMBps?: number;
  diskWriteMBps?: number;
  networkUsageMbps: number;
  topProcesses: ProcessResourceData[];
  activeUserSessions: UserSessionData[];
  expiredUserSessions: UserSessionData[];
  systemUsers?: UserPrincipalData[];
  rdpSessions?: RdpSessionData[];
  systemServices?: ServiceInfoData[];
}

export interface ProcessAnomalyData {
  process: ProcessResourceData;
  reason: string;
  risk: string;
}

export interface SessionAnomalyData {
  session: RdpSessionData;
  reason: string;
  risk: string;
}

export interface UserAnomalyData {
  user: UserPrincipalData;
  reason: string;
  risk: string;
}

export interface ServiceAnomalyData {
  service: ServiceInfoData;
  reason: string;
  risk: string;
}

export interface TelemetryPostureReportData {
  flaggedProcesses: ProcessAnomalyData[];
  suspiciousSessions: SessionAnomalyData[];
  flaggedUsers: UserAnomalyData[];
  suspiciousServices: ServiceAnomalyData[];
  threatScore: number;
  overallRisk: 'CRITICAL' | 'HIGH' | 'MEDIUM' | 'LOW';
}

export async function fetchApiChannels(baseUrl: string = ''): Promise<ChannelData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/channels`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  return {
    channels: data.channels || data.Channels || []
  };
}

export async function fetchApiEvents(channel: string, baseUrl: string = '', page: number = 1, pageSize: number = 20, severity: string = 'ALL'): Promise<EventsData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  let url = `${urlPrefix}/api/events?channel=${encodeURIComponent(channel)}&page=${page}&pageSize=${pageSize}`;
  if (severity && severity !== 'ALL') url += `&level=${encodeURIComponent(severity)}`;
  const resp = await fetch(url);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  const rawEvents = data.events || data.Events || [];
  const events = rawEvents.map((e: Record<string, unknown>, idx: number) => ({
    index: (e.index ?? e.Index ?? idx + 1) as number,
    id: (e.id ?? e.Id ?? 0) as number,
    level: (e.level ?? e.Level ?? 'Information') as string,
    risk: (e.risk ?? e.Risk ?? 'Low') as string,
    provider: (e.provider ?? e.Provider ?? '') as string,
    time: (e.time ?? e.Time ?? '') as string,
    message: (e.message ?? e.Message ?? '') as string,
  }));

  return {
    channel: data.channel || data.Channel || channel,
    totalCount: data.totalCount ?? data.TotalCount ?? events.length,
    criticalCount: data.criticalCount ?? data.CriticalCount ?? 0,
    errorCount: data.errorCount ?? data.ErrorCount ?? 0,
    warningCount: data.warningCount ?? data.WarningCount ?? 0,
    infoCount: data.infoCount ?? data.InfoCount ?? 0,
    verboseCount: data.verboseCount ?? data.VerboseCount ?? 0,
    page: data.page ?? data.Page ?? page,
    pageSize: data.pageSize ?? data.PageSize ?? pageSize,
    totalPages: data.totalPages ?? data.TotalPages ?? 1,
    events
  };
}

export async function fetchCrossChannelAnomalies(limit: number = 15, baseUrl: string = ''): Promise<MultiChannelAnomaliesDto> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/events/anomalies?limit=${limit}`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  const mapEvent = (e: Record<string, unknown>, idx: number): EventDto => ({
    idx: (e.index ?? e.Index ?? idx + 1) as number,
    id: (e.id ?? e.Id ?? 0) as number,
    level: ((e.level ?? e.Level ?? 'Information') as string) as EventDto['level'],
    risk: ((e.risk ?? e.Risk ?? 'Low') as string) as EventDto['risk'],
    provider: (e.provider ?? e.Provider ?? '') as string,
    time: formatTo12Hour((e.time ?? e.Time ?? '') as string),
    desc: (e.message ?? e.Message ?? '') as string,
    xml: (e.rawXml ?? e.RawXml ?? '') as string,
  });

  return {
    securityEvents: ((data.securityEvents || data.SecurityEvents || []) as Array<Record<string, unknown>>).map(mapEvent),
    systemEvents: ((data.systemEvents || data.SystemEvents || []) as Array<Record<string, unknown>>).map(mapEvent),
    applicationEvents: ((data.applicationEvents || data.ApplicationEvents || []) as Array<Record<string, unknown>>).map(mapEvent),
    sysmonEvents: ((data.sysmonEvents || data.SysmonEvents || []) as Array<Record<string, unknown>>).map(mapEvent),
    totalCriticalCount: (data.totalCriticalCount ?? data.TotalCriticalCount ?? 0) as number,
    totalErrorCount: (data.totalErrorCount ?? data.TotalErrorCount ?? 0) as number,
    totalWarningCount: (data.totalWarningCount ?? data.TotalWarningCount ?? 0) as number,
  };
}

export async function fetchApiAnalyze(channel: string, query: string, baseUrl: string = ''): Promise<AnalyzeData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/analyze`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ channel, query }),
  });
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  return {
    taskId: data.taskId || data.TaskId || '',
    status: data.status || data.Status || '',
    progressMessage: data.progressMessage || data.ProgressMessage || '',
    downloadProgress: data.downloadProgress ?? data.DownloadProgress ?? 0,
    downloadRateBytesPerSec: data.downloadRateBytesPerSec ?? data.DownloadRateBytesPerSec ?? 0,
    downloadedBytes: data.downloadedBytes ?? data.DownloadedBytes ?? 0,
    totalBytes: data.totalBytes ?? data.TotalBytes ?? 0,
    channel: data.channel || data.Channel || channel,
    query: data.query || data.Query || query,
    analysis: data.analysis || data.Analysis || '',
    eventsAnalyzed: data.eventsAnalyzed ?? data.EventsAnalyzed ?? 0
  };
}

export async function fetchApiAnalyzeStatus(taskId: string, baseUrl: string = ''): Promise<AnalyzeData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/analyze/status?taskId=${encodeURIComponent(taskId)}`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  return {
    taskId: data.taskId || data.TaskId || taskId,
    status: data.status || data.Status || '',
    progressMessage: data.progressMessage || data.ProgressMessage || '',
    downloadProgress: data.downloadProgress ?? data.DownloadProgress ?? 0,
    downloadRateBytesPerSec: data.downloadRateBytesPerSec ?? data.DownloadRateBytesPerSec ?? 0,
    downloadedBytes: data.downloadedBytes ?? data.DownloadedBytes ?? 0,
    totalBytes: data.totalBytes ?? data.TotalBytes ?? 0,
    channel: data.channel || data.Channel || '',
    query: data.query || data.Query || '',
    analysis: data.analysis || data.Analysis || '',
    eventsAnalyzed: data.eventsAnalyzed ?? data.EventsAnalyzed ?? 0
  };
}

export async function fetchMetricsSummary(baseUrl: string = ''): Promise<Partial<SystemMetricsData>> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/summary`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();
  return {
    cpuUsagePercent: (data.cpuUsagePercent ?? data.CpuUsagePercent ?? 0) as number,
    memoryUsagePercent: (data.memoryUsagePercent ?? data.MemoryUsagePercent ?? 0) as number,
    memoryUsedMB: (data.memoryUsedMB ?? data.MemoryUsedMB ?? 0) as number,
    memoryTotalMB: (data.memoryTotalMB ?? data.MemoryTotalMB ?? 0) as number,
    diskUsagePercent: (data.diskUsagePercent ?? data.DiskUsagePercent ?? 0) as number,
    diskReadMBps: (data.diskReadMBps ?? data.DiskReadMBps ?? 0) as number,
    diskWriteMBps: (data.diskWriteMBps ?? data.DiskWriteMBps ?? 0) as number,
    networkUsageMbps: (data.networkUsageMbps ?? data.NetworkUsageMbps ?? 0) as number,
  };
}

export async function fetchMetricsCpu(baseUrl: string = ''): Promise<number> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/cpu`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();
  return (data.cpuUsagePercent ?? data.CpuUsagePercent ?? 0) as number;
}

export async function fetchMetricsMemory(baseUrl: string = ''): Promise<{ memoryUsagePercent: number; memoryTotalMB: number; memoryUsedMB: number }> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/memory`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();
  return {
    memoryUsagePercent: (data.memoryUsagePercent ?? data.MemoryUsagePercent ?? 0) as number,
    memoryTotalMB: (data.memoryTotalMB ?? data.MemoryTotalMB ?? 0) as number,
    memoryUsedMB: (data.memoryUsedMB ?? data.MemoryUsedMB ?? 0) as number,
  };
}

export async function fetchMetricsDisk(baseUrl: string = ''): Promise<{ diskReadMBps: number; diskWriteMBps: number }> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/disk`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();
  return {
    diskReadMBps: (data.diskReadMBps ?? data.DiskReadMBps ?? 0) as number,
    diskWriteMBps: (data.diskWriteMBps ?? data.DiskWriteMBps ?? 0) as number,
  };
}

export async function fetchMetricsNetwork(baseUrl: string = ''): Promise<number> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/network`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();
  return (data.networkUsageMbps ?? data.NetworkUsageMbps ?? 0) as number;
}

export async function fetchMetricsProcesses(baseUrl: string = ''): Promise<ProcessResourceData[]> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/processes`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();
  const rawTopProcesses = data.topProcesses || data.TopProcesses || [];
  return rawTopProcesses.map((p: Record<string, unknown>) => ({
    processId: (p.processId ?? p.ProcessId ?? 0) as number,
    name: (p.name ?? p.Name ?? '') as string,
    path: (p.path ?? p.Path ?? '') as string,
    commandLine: (p.commandLine ?? p.CommandLine ?? '') as string,
    cpuUsagePercent: (p.cpuUsagePercent ?? p.CpuUsagePercent ?? 0) as number,
    memoryUsageMB: (p.memoryUsageMB ?? p.MemoryUsageMB ?? 0) as number,
    networkReadBytes: (p.networkReadBytes ?? p.NetworkReadBytes ?? 0) as number,
    networkWriteBytes: (p.networkWriteBytes ?? p.NetworkWriteBytes ?? 0) as number,
    openPorts: (p.openPorts ?? p.OpenPorts ?? '-') as string,
    connectionEstablished: (p.connectionEstablished ?? p.ConnectionEstablished ?? false) as boolean,
  }));
}

export async function fetchMetricsSessions(baseUrl: string = ''): Promise<{ activeUserSessions: UserSessionData[]; expiredUserSessions: UserSessionData[]; systemUsers: UserPrincipalData[]; rdpSessions: RdpSessionData[] }> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/sessions`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();

  const rawActiveSessions = data.activeUserSessions || data.ActiveUserSessions || [];
  const activeUserSessions = rawActiveSessions.map((s: Record<string, unknown>) => ({
    username: (s.username ?? s.Username ?? '') as string,
    privilege: (s.privilege ?? s.Privilege ?? '') as string,
    loginTimestamp: (s.loginTimestamp ?? s.LoginTimestamp ?? '') as string,
    logoutTimestamp: (s.logoutTimestamp ?? s.LogoutTimestamp ?? '') as string,
    isActive: (s.isActive ?? s.IsActive ?? true) as boolean,
  }));

  const rawExpiredSessions = data.expiredUserSessions || data.ExpiredUserSessions || [];
  const expiredUserSessions = rawExpiredSessions.map((s: Record<string, unknown>) => ({
    username: (s.username ?? s.Username ?? '') as string,
    privilege: (s.privilege ?? s.Privilege ?? '') as string,
    loginTimestamp: (s.loginTimestamp ?? s.LoginTimestamp ?? '') as string,
    logoutTimestamp: (s.logoutTimestamp ?? s.LogoutTimestamp ?? '') as string,
    isActive: (s.isActive ?? s.IsActive ?? false) as boolean,
  }));

  const rawSystemUsers = data.systemUsers || data.SystemUsers || [];
  const systemUsers = rawSystemUsers.map((u: Record<string, unknown>) => ({
    username: (u.username ?? u.Username ?? '') as string,
    domain: (u.domain ?? u.Domain ?? '') as string,
    sidOrUid: (u.sidOrUid ?? u.SidOrUid ?? '') as string,
    userClass: (u.userClass ?? u.UserClass ?? 'Normal') as string,
    isDisabled: (u.isDisabled ?? u.IsDisabled ?? false) as boolean,
    isAccountLocked: (u.isAccountLocked ?? u.IsAccountLocked ?? false) as boolean,
    groups: (u.groups ?? u.Groups ?? []) as string[],
    permissions: (u.permissions ?? u.Permissions ?? []) as string[],
  }));

  const rawRdpSessions = data.rdpSessions || data.RdpSessions || [];
  const rdpSessions = rawRdpSessions.map((r: Record<string, unknown>) => ({
    sessionId: (r.sessionId ?? r.SessionId ?? 0) as number,
    sessionName: (r.sessionName ?? r.SessionName ?? '') as string,
    userName: (r.userName ?? r.UserName ?? '') as string,
    domainName: (r.domainName ?? r.DomainName ?? '') as string,
    clientName: (r.clientName ?? r.ClientName ?? '') as string,
    clientIpAddress: (r.clientIpAddress ?? r.ClientIpAddress ?? '') as string,
    state: (r.state ?? r.State ?? 'Unknown') as string,
    isRdpSession: (r.isRdpSession ?? r.IsRdpSession ?? false) as boolean,
  }));

  return { activeUserSessions, expiredUserSessions, systemUsers, rdpSessions };
}

export async function fetchApiMetrics(baseUrl: string = ''): Promise<SystemMetricsData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();

  const rawTopProcesses = data.topProcesses || data.TopProcesses || [];
  const topProcesses = rawTopProcesses.map((p: Record<string, unknown>) => ({
    processId: (p.processId ?? p.ProcessId ?? 0) as number,
    name: (p.name ?? p.Name ?? '') as string,
    path: (p.path ?? p.Path ?? '') as string,
    commandLine: (p.commandLine ?? p.CommandLine ?? '') as string,
    cpuUsagePercent: (p.cpuUsagePercent ?? p.CpuUsagePercent ?? 0) as number,
    memoryUsageMB: (p.memoryUsageMB ?? p.MemoryUsageMB ?? 0) as number,
    networkReadBytes: (p.networkReadBytes ?? p.NetworkReadBytes ?? 0) as number,
    networkWriteBytes: (p.networkWriteBytes ?? p.NetworkWriteBytes ?? 0) as number,
    openPorts: (p.openPorts ?? p.OpenPorts ?? '-') as string,
    connectionEstablished: (p.connectionEstablished ?? p.ConnectionEstablished ?? false) as boolean,
  }));

  const rawActiveSessions = data.activeUserSessions || data.ActiveUserSessions || [];
  const activeUserSessions = rawActiveSessions.map((s: Record<string, unknown>) => ({
    username: (s.username ?? s.Username ?? '') as string,
    privilege: (s.privilege ?? s.Privilege ?? '') as string,
    loginTimestamp: (s.loginTimestamp ?? s.LoginTimestamp ?? '') as string,
    logoutTimestamp: (s.logoutTimestamp ?? s.LogoutTimestamp ?? '') as string,
    isActive: (s.isActive ?? s.IsActive ?? true) as boolean,
  }));

  const rawExpiredSessions = data.expiredUserSessions || data.ExpiredUserSessions || [];
  const expiredUserSessions = rawExpiredSessions.map((s: Record<string, unknown>) => ({
    username: (s.username ?? s.Username ?? '') as string,
    privilege: (s.privilege ?? s.Privilege ?? '') as string,
    loginTimestamp: (s.loginTimestamp ?? s.LoginTimestamp ?? '') as string,
    logoutTimestamp: (s.logoutTimestamp ?? s.LogoutTimestamp ?? '') as string,
    isActive: (s.isActive ?? s.IsActive ?? false) as boolean,
  }));

  const rawSystemUsers = data.systemUsers || data.SystemUsers || [];
  const systemUsers = rawSystemUsers.map((u: Record<string, unknown>) => ({
    username: (u.username ?? u.Username ?? '') as string,
    domain: (u.domain ?? u.Domain ?? '') as string,
    sidOrUid: (u.sidOrUid ?? u.SidOrUid ?? '') as string,
    userClass: (u.userClass ?? u.UserClass ?? 'Normal') as string,
    isDisabled: (u.isDisabled ?? u.IsDisabled ?? false) as boolean,
    isAccountLocked: (u.isAccountLocked ?? u.IsAccountLocked ?? false) as boolean,
    groups: (u.groups ?? u.Groups ?? []) as string[],
    permissions: (u.permissions ?? u.Permissions ?? []) as string[],
  }));

  const rawRdpSessions = data.rdpSessions || data.RdpSessions || [];
  const rdpSessions = rawRdpSessions.map((r: Record<string, unknown>) => ({
    sessionId: (r.sessionId ?? r.SessionId ?? 0) as number,
    sessionName: (r.sessionName ?? r.SessionName ?? '') as string,
    userName: (r.userName ?? r.UserName ?? '') as string,
    domainName: (r.domainName ?? r.DomainName ?? '') as string,
    clientName: (r.clientName ?? r.ClientName ?? '') as string,
    clientIpAddress: (r.clientIpAddress ?? r.ClientIpAddress ?? '') as string,
    state: (r.state ?? r.State ?? 'Unknown') as string,
    isRdpSession: (r.isRdpSession ?? r.IsRdpSession ?? false) as boolean,
  }));

  return {
    cpuUsagePercent: (data.cpuUsagePercent ?? data.CpuUsagePercent ?? 0) as number,
    memoryUsagePercent: (data.memoryUsagePercent ?? data.MemoryUsagePercent ?? 0) as number,
    memoryUsedMB: (data.memoryUsedMB ?? data.MemoryUsedMB ?? 0) as number,
    memoryTotalMB: (data.memoryTotalMB ?? data.MemoryTotalMB ?? 0) as number,
    diskUsagePercent: (data.diskUsagePercent ?? data.DiskUsagePercent ?? 0) as number,
    diskReadMBps: (data.diskReadMBps ?? data.DiskReadMBps ?? 0) as number,
    diskWriteMBps: (data.diskWriteMBps ?? data.DiskWriteMBps ?? 0) as number,
    networkUsageMbps: (data.networkUsageMbps ?? data.NetworkUsageMbps ?? 0) as number,
    topProcesses,
    activeUserSessions,
    expiredUserSessions,
    systemUsers,
    rdpSessions,
  };
}

export async function fetchMetricsServices(baseUrl: string = ''): Promise<ServiceInfoData[]> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/services`);
  if (!resp.ok) throw new Error(`HTTP Error: ${resp.status}`);
  const data = await resp.json();
  const rawServices = data.services || data.Services || [];
  return rawServices.map((s: Record<string, unknown>) => ({
    serviceName: (s.serviceName ?? s.ServiceName ?? '') as string,
    displayName: (s.displayName ?? s.DisplayName ?? '') as string,
    status: (s.status ?? s.Status ?? 'Stopped') as string,
    startType: (s.startType ?? s.StartType ?? 'Manual') as string,
    processId: (s.processId ?? s.ProcessId ?? 0) as number,
  }));
}

export interface LogColumnFormat {
  key: string;
  headerName: string;
  type: string;
  widthPx: number;
}

export interface LogFormatData {
  columns: LogColumnFormat[];
}

export interface LogRecordData {
  timestamp: string;
  level: string;
  processId?: string;
  threadId?: string;
  category: string;
  message: string;
}

export interface ServerLogsData {
  records: LogRecordData[];
  logs?: string[];
}

export async function fetchApiLogFormat(baseUrl: string = ''): Promise<LogFormatData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/logs/format`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  const rawCols = data.columns || data.Columns || [];
  return {
    columns: rawCols.map((c: Record<string, unknown>) => ({
      key: (c.key ?? c.Key ?? '') as string,
      headerName: (c.headerName ?? c.HeaderName ?? '') as string,
      type: (c.type ?? c.Type ?? 'string') as string,
      widthPx: (c.widthPx ?? c.WidthPx ?? 150) as number,
    }))
  };
}

export async function fetchApiServerLogs(baseUrl: string = ''): Promise<ServerLogsData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/logs`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  const rawRecords = data.records || data.Records || [];

  if (Array.isArray(rawRecords) && rawRecords.length > 0) {
    const records = rawRecords.map((r: Record<string, unknown>) => ({
      timestamp: (r.timestamp ?? r.Timestamp ?? '') as string,
      level: (r.level ?? r.Level ?? 'INFO') as string,
      processId: (r.processId ?? r.ProcessId ?? '-') as string,
      threadId: (r.threadId ?? r.ThreadId ?? '-') as string,
      category: (r.category ?? r.Category ?? 'SERVER') as string,
      message: (r.message ?? r.Message ?? '') as string,
    }));
    return { records };
  }

  // Fallback if backend returns plain string array in logs / Logs
  const rawLogs = data.logs || data.Logs || (Array.isArray(data) ? data : []);
  const records = rawLogs.map((line: string) => {
    let ts = new Date().toISOString().replace('T', ' ').substring(0, 19);
    let level = 'INFO';
    let processId = '-';
    let threadId = '-';
    let category = 'SERVER';
    let message = line;

    if (typeof line === 'string') {
      const match = line.match(/^(\d{4}[-/]\d{2}[-/]\d{2}\s\d{2}:\d{2}:\d{2})\s*(?:\[(.*?)\])?\s*(.*)$/);
      if (match) {
        ts = match[1] || ts;
        category = match[2] || 'SERVER';
        message = match[3] || line;
      } else {
        const bracketMatch = line.match(/^\[(.*?)\]\s*(.*)$/);
        if (bracketMatch) {
          category = bracketMatch[1] || 'SERVER';
          message = bracketMatch[2] || line;
        }
      }

      const pidMatch = line.match(/\[PID:(\d+)\]/i);
      if (pidMatch) {
        processId = pidMatch[1];
      }

      const tidMatch = line.match(/\[TID:(\d+)\]/i);
      if (tidMatch) {
        threadId = tidMatch[1];
      }

      const upper = line.toUpperCase();
      if (upper.includes('ERROR') || upper.includes('FAIL')) level = 'ERROR';
      else if (upper.includes('WARN')) level = 'WARN';
      else if (upper.includes('AI_ENGINE') || upper.includes('LLM')) level = 'DEBUG';
    }

    return { timestamp: ts, level, processId, threadId, category, message };
  });

  return { records };
}

export function subscribeTelemetryPushStream(
  onMetricsUpdate: (data: SystemMetricsData) => void,
  baseUrl: string = '',
  intervalMs: number = 1000
): () => void {
  let isSubscribed = true;
  const poll = async () => {
    if (!isSubscribed) return;
    try {
      const data = await fetchApiMetrics(baseUrl);
      if (isSubscribed && data) {
        onMetricsUpdate(data);
      }
    } catch {
      // Ignore network blips during push streaming
    }
  };

  poll();
  const timerId = setInterval(poll, intervalMs);
  return () => {
    isSubscribed = false;
    clearInterval(timerId);
  };
}

export async function fetchPostureReport(baseUrl: string = ''): Promise<TelemetryPostureReportData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/metrics/posture`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  const rawProcs = data.flaggedProcesses || data.FlaggedProcesses || [];
  const rawSessions = data.suspiciousSessions || data.SuspiciousSessions || [];
  const rawUsers = data.flaggedUsers || data.FlaggedUsers || [];
  const rawServices = data.suspiciousServices || data.SuspiciousServices || [];

  return {
    flaggedProcesses: rawProcs.map((p: any) => ({
      process: {
        processId: p.process?.processId ?? p.process?.ProcessId ?? 0,
        name: p.process?.name ?? p.process?.Name ?? '',
        path: p.process?.path ?? p.process?.Path ?? '',
        commandLine: p.process?.commandLine ?? p.process?.CommandLine ?? '',
        cpuUsagePercent: p.process?.cpuUsagePercent ?? p.process?.CpuUsagePercent ?? 0,
        memoryUsageMB: p.process?.memoryUsageMB ?? p.process?.MemoryUsageMB ?? 0,
        networkReadBytes: p.process?.networkReadBytes ?? p.process?.NetworkReadBytes ?? 0,
        networkWriteBytes: p.process?.networkWriteBytes ?? p.process?.NetworkWriteBytes ?? 0,
        openPorts: p.process?.openPorts ?? p.process?.OpenPorts ?? '',
        connectionEstablished: p.process?.connectionEstablished ?? p.process?.ConnectionEstablished ?? false,
      },
      reason: p.reason ?? p.Reason ?? '',
      risk: p.risk ?? p.Risk ?? 'Medium',
    })),
    suspiciousSessions: rawSessions.map((s: any) => ({
      session: {
        sessionId: s.session?.sessionId ?? s.session?.SessionId ?? 0,
        sessionName: s.session?.sessionName ?? s.session?.SessionName ?? '',
        userName: s.session?.userName ?? s.session?.UserName ?? '',
        domainName: s.session?.domainName ?? s.session?.DomainName ?? '',
        clientName: s.session?.clientName ?? s.session?.ClientName ?? '',
        clientIpAddress: s.session?.clientIpAddress ?? s.session?.ClientIpAddress ?? '',
        state: s.session?.state ?? s.session?.State ?? '',
        isRdpSession: s.session?.isRdpSession ?? s.session?.IsRdpSession ?? false,
      },
      reason: s.reason ?? s.Reason ?? '',
      risk: s.risk ?? s.Risk ?? 'High',
    })),
    flaggedUsers: rawUsers.map((u: any) => ({
      user: {
        username: u.user?.username ?? u.user?.Username ?? '',
        domain: u.user?.domain ?? u.user?.Domain ?? '',
        sidOrUid: u.user?.sidOrUid ?? u.user?.SidOrUid ?? '',
        userClass: u.user?.userClass ?? u.user?.UserClass ?? '',
        isDisabled: u.user?.isDisabled ?? u.user?.IsDisabled ?? false,
        isAccountLocked: u.user?.isAccountLocked ?? u.user?.IsAccountLocked ?? false,
        groups: u.user?.groups ?? u.user?.Groups ?? [],
        permissions: u.user?.permissions ?? u.user?.Permissions ?? [],
      },
      reason: u.reason ?? u.Reason ?? '',
      risk: u.risk ?? u.Risk ?? 'Medium',
    })),
    suspiciousServices: rawServices.map((srv: any) => ({
      service: {
        serviceName: srv.service?.serviceName ?? srv.service?.ServiceName ?? '',
        displayName: srv.service?.displayName ?? srv.service?.DisplayName ?? '',
        status: srv.service?.status ?? srv.service?.Status ?? '',
        startType: srv.service?.startType ?? srv.service?.StartType ?? '',
        processId: srv.service?.processId ?? srv.service?.ProcessId ?? 0,
      },
      reason: srv.reason ?? srv.Reason ?? '',
      risk: srv.risk ?? srv.Risk ?? 'Low',
    })),
    threatScore: data.threatScore ?? data.ThreatScore ?? 0,
    overallRisk: data.overallRisk || data.OverallRisk || 'LOW',
  };
}

