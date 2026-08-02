export interface ChannelData {
  channels?: string[];
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

export interface AnalyzeData {
  taskId?: string;
  status?: string;
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
  diskIoKBps: number;
  diskReadKBps?: number;
  diskWriteKBps?: number;
}

export interface UserSessionData {
  username: string;
  privilege: string;
  loginTimestamp: string;
  logoutTimestamp: string;
  isActive: boolean;
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
}

export async function fetchApiChannels(baseUrl: string = 'http://localhost:8080'): Promise<ChannelData> {
  const resp = await fetch(`${baseUrl}/api/channels`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  return await resp.json();
}

export async function fetchApiEvents(channel: string, baseUrl: string = 'http://localhost:8080', page: number = 1, pageSize: number = 20): Promise<EventsData> {
  const resp = await fetch(`${baseUrl}/api/events?channel=${encodeURIComponent(channel)}&page=${page}&pageSize=${pageSize}`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  return await resp.json();
}

export async function fetchApiAnalyze(channel: string, query: string, baseUrl: string = 'http://localhost:8080'): Promise<AnalyzeData> {
  const resp = await fetch(`${baseUrl}/api/analyze`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ channel, query }),
  });
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  return await resp.json();
}

export async function fetchApiAnalyzeStatus(taskId: string, baseUrl: string = 'http://localhost:8080'): Promise<AnalyzeData> {
  const resp = await fetch(`${baseUrl}/api/analyze/status?taskId=${encodeURIComponent(taskId)}`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  return await resp.json();
}

export async function fetchApiMetrics(baseUrl: string = 'http://localhost:8080'): Promise<SystemMetricsData> {
  const resp = await fetch(`${baseUrl}/api/metrics`);
  if (!resp.ok) {
    throw new Error(`HTTP Error: ${resp.status}`);
  }
  return await resp.json();
}
