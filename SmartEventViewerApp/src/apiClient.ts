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

export async function fetchApiEvents(channel: string, baseUrl: string = '', page: number = 1, pageSize: number = 20): Promise<EventsData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/events?channel=${encodeURIComponent(channel)}&page=${page}&pageSize=${pageSize}`);
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

export interface ServerLogsData {
  logs?: string[];
}

export async function fetchApiServerLogs(baseUrl: string = ''): Promise<ServerLogsData> {
  const urlPrefix = baseUrl ? baseUrl : (window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080');
  const resp = await fetch(`${urlPrefix}/api/logs`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  const data = await resp.json();
  return {
    logs: data.logs || data.Logs || []
  };
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

