// Telemetry Web Worker running in a separate background thread
// Polls active category endpoints at 1.5s intervals and immediately on tab switch.

let socket: WebSocket | null = null;
let reconnectTimer: ReturnType<typeof setTimeout> | null = null;
let heartbeatIntervalId: ReturnType<typeof setInterval> | null = null;
let currentAbortController: AbortController | null = null;
let currentBaseUrl: string = '';
let currentActiveSubTab: string = 'overview';
let pollingIntervalId: ReturnType<typeof setInterval> | null = null;
let isFetching: boolean = false;

self.onmessage = (event: MessageEvent) => {
  const { type, baseUrl, subTab } = event.data;

  if (type === 'START_POLLING') {
    currentBaseUrl = baseUrl || currentBaseUrl;
    currentActiveSubTab = subTab || currentActiveSubTab;
    connectWebSocket();
    fetchActiveSubTab(false);
    if (pollingIntervalId !== null) clearInterval(pollingIntervalId);
    pollingIntervalId = setInterval(() => {
      if (!socket || socket.readyState === WebSocket.CLOSED || socket.readyState === WebSocket.CLOSING) {
        scheduleReconnect();
      }
      fetchActiveSubTab(false);
    }, 2000);
  } else if (type === 'STOP_POLLING') {
    if (pollingIntervalId !== null) {
      clearInterval(pollingIntervalId);
      pollingIntervalId = null;
    }
    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
      reconnectTimer = null;
    }
    if (heartbeatIntervalId !== null) {
      clearInterval(heartbeatIntervalId);
      heartbeatIntervalId = null;
    }
    if (socket) {
      socket.onclose = null;
      socket.onerror = null;
      socket.onmessage = null;
      socket.onopen = null;
      try { socket.close(); } catch {}
      socket = null;
    }
    if (currentAbortController) {
      currentAbortController.abort();
      currentAbortController = null;
    }
    isFetching = false;
  } else if (type === 'CHANGE_SUBTAB') {
    if (currentActiveSubTab !== subTab) {
      currentActiveSubTab = subTab;
      fetchActiveSubTab(true);
    }
  }
};

function getWebSocketUrl(): string {
  if (currentBaseUrl) {
    return currentBaseUrl.replace(/^http/, 'ws') + '/ws/telemetry';
  }
  const origin = self.location.origin;
  if (origin && origin.includes('://')) {
    const normalizedOrigin = origin.replace('localhost', '127.0.0.1');
    return normalizedOrigin.replace(/^http/, 'ws') + '/ws/telemetry';
  }
  return 'ws://127.0.0.1:8080/ws/telemetry';
}

function scheduleReconnect() {
  if (heartbeatIntervalId !== null) {
    clearInterval(heartbeatIntervalId);
    heartbeatIntervalId = null;
  }
  if (socket) {
    socket.onclose = null;
    socket.onerror = null;
    socket.onmessage = null;
    socket.onopen = null;
    try { socket.close(); } catch {}
    socket = null;
  }
  if (!reconnectTimer) {
    reconnectTimer = setTimeout(() => {
      reconnectTimer = null;
      connectWebSocket();
    }, 2000);
  }
}

function connectWebSocket() {
  if (socket) {
    socket.onclose = null;
    socket.onerror = null;
    socket.onmessage = null;
    socket.onopen = null;
    try { socket.close(); } catch {}
    socket = null;
  }
  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
    reconnectTimer = null;
  }
  if (heartbeatIntervalId !== null) {
    clearInterval(heartbeatIntervalId);
    heartbeatIntervalId = null;
  }

  const wsUrl = getWebSocketUrl();

  try {
    socket = new WebSocket(wsUrl);

    socket.onopen = () => {
      console.log('[TelemetryWorker] WebSocket connected:', wsUrl);
      fetchActiveSubTab(true);
      if (heartbeatIntervalId !== null) clearInterval(heartbeatIntervalId);
      heartbeatIntervalId = setInterval(() => {
        if (socket && socket.readyState === WebSocket.OPEN) {
          try {
            socket.send(JSON.stringify({ type: 'PING' }));
          } catch {
            scheduleReconnect();
          }
        } else {
          scheduleReconnect();
        }
      }, 10000);
    };

    socket.onmessage = (event: MessageEvent) => {
      try {
        const msg = JSON.parse(event.data);
        if (msg.type === 'TELEMETRY_UPDATED') {
          handleServerPush(msg.category);
        }
      } catch {
      }
    };

    socket.onerror = () => {
      scheduleReconnect();
    };

    socket.onclose = () => {
      scheduleReconnect();
    };
  } catch {
    scheduleReconnect();
  }
}

function handleServerPush(category: string) {
  if (category === 'llm_analysis') {
    self.postMessage({ type: 'LLM_ANALYSIS_UPDATED' });
  } else if (category === 'summary' || category === 'processes') {
    fetchActiveSubTab(true);
  } else if (category === 'sessions' && (currentActiveSubTab === 'sessions' || currentActiveSubTab === 'users')) {
    fetchActiveSubTab(true);
  } else if (category === 'services' && currentActiveSubTab === 'services') {
    fetchActiveSubTab(true);
  }
}

function mapProcessItem(p: Record<string, unknown>) {
  return {
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
  };
}

async function fetchActiveSubTab(forceAbort: boolean = false) {
  if (isFetching && !forceAbort) {
    return;
  }

  if (forceAbort && currentAbortController) {
    currentAbortController.abort();
    currentAbortController = null;
  }

  isFetching = true;
  currentAbortController = new AbortController();
  const signal = currentAbortController.signal;
  const timeoutId = setTimeout(() => {
    if (currentAbortController) {
      currentAbortController.abort();
    }
  }, 4000);
  const urlPrefix = currentBaseUrl ? currentBaseUrl : (self.location.origin.includes(':') ? self.location.origin : 'http://127.0.0.1:8080');

  try {
    if (currentActiveSubTab === 'overview') {
      const resp = await fetch(`${urlPrefix}/api/metrics/summary`, { signal });
      if (resp.ok) {
        const data = await resp.json();
        const rawProcs = (data.topProcesses || data.TopProcesses || []) as Array<Record<string, unknown>>;
        const mappedProcs = rawProcs.map(mapProcessItem);
        self.postMessage({
          type: 'METRICS_SUMMARY_UPDATED',
          payload: {
            ...data,
            topProcesses: mappedProcs,
          }
        });
      }
    } else if (currentActiveSubTab === 'processes') {
      const resp = await fetch(`${urlPrefix}/api/metrics/processes`, { signal });
      if (resp.ok) {
        const data = await resp.json();
        const rawList = Array.isArray(data) ? data : (data.topProcesses || data.TopProcesses || []);
        const processesList = (rawList as Array<Record<string, unknown>>).map(mapProcessItem);
        self.postMessage({ type: 'METRICS_PROCESSES_UPDATED', payload: processesList });
      }
    } else if (currentActiveSubTab === 'sessions' || currentActiveSubTab === 'users') {
      const resp = await fetch(`${urlPrefix}/api/metrics/sessions`, { signal });
      if (resp.ok) {
        const data = await resp.json();
        const rawActive = (data.activeUserSessions || data.ActiveUserSessions || []) as Array<Record<string, unknown>>;
        const activeUserSessions = rawActive.map(s => ({
          username: (s.username ?? s.Username ?? '') as string,
          privilege: (s.privilege ?? s.Privilege ?? '') as string,
          loginTimestamp: (s.loginTimestamp ?? s.LoginTimestamp ?? '') as string,
          logoutTimestamp: (s.logoutTimestamp ?? s.LogoutTimestamp ?? '') as string,
          isActive: (s.isActive ?? s.IsActive ?? true) as boolean,
        }));

        const rawExpired = (data.expiredUserSessions || data.ExpiredUserSessions || []) as Array<Record<string, unknown>>;
        const expiredUserSessions = rawExpired.map(s => ({
          username: (s.username ?? s.Username ?? '') as string,
          privilege: (s.privilege ?? s.Privilege ?? '') as string,
          loginTimestamp: (s.loginTimestamp ?? s.LoginTimestamp ?? '') as string,
          logoutTimestamp: (s.logoutTimestamp ?? s.LogoutTimestamp ?? '') as string,
          isActive: (s.isActive ?? s.IsActive ?? false) as boolean,
        }));

        const rawUsers = (data.systemUsers || data.SystemUsers || []) as Array<Record<string, unknown>>;
        const systemUsers = rawUsers.map(u => ({
          username: (u.username ?? u.Username ?? '') as string,
          domain: (u.domain ?? u.Domain ?? '') as string,
          sidOrUid: (u.sidOrUid ?? u.SidOrUid ?? '') as string,
          userClass: (u.userClass ?? u.UserClass ?? 'Normal') as string,
          isDisabled: (u.isDisabled ?? u.IsDisabled ?? false) as boolean,
          isAccountLocked: (u.isAccountLocked ?? u.IsAccountLocked ?? false) as boolean,
          groups: (u.groups ?? u.Groups ?? []) as string[],
          permissions: (u.permissions ?? u.Permissions ?? []) as string[],
        }));

        const rawRdp = (data.rdpSessions || data.RdpSessions || []) as Array<Record<string, unknown>>;
        const rdpSessions = rawRdp.map(r => ({
          sessionId: (r.sessionId ?? r.SessionId ?? 0) as number,
          sessionName: (r.sessionName ?? r.SessionName ?? '') as string,
          userName: (r.userName ?? r.UserName ?? '') as string,
          domainName: (r.domainName ?? r.DomainName ?? '') as string,
          clientName: (r.clientName ?? r.ClientName ?? '') as string,
          clientIpAddress: (r.clientIpAddress ?? r.ClientIpAddress ?? '') as string,
          state: (r.state ?? r.State ?? 'Unknown') as string,
          isRdpSession: (r.isRdpSession ?? r.IsRdpSession ?? false) as boolean,
        }));

        self.postMessage({
          type: 'METRICS_SESSIONS_UPDATED',
          payload: {
            activeUserSessions,
            expiredUserSessions,
            systemUsers,
            rdpSessions,
          }
        });
      }
    } else if (currentActiveSubTab === 'services') {
      const resp = await fetch(`${urlPrefix}/api/metrics/services`, { signal });
      if (resp.ok) {
        const data = await resp.json();
        const rawList = (data.services || data.Services || (Array.isArray(data) ? data : [])) as Array<Record<string, unknown>>;
        const servicesList = rawList.map((s: Record<string, unknown>) => ({
          serviceName: (s.serviceName ?? s.ServiceName ?? '') as string,
          displayName: (s.displayName ?? s.DisplayName ?? '') as string,
          status: (s.status ?? s.Status ?? 'Stopped') as string,
          startType: (s.startType ?? s.StartType ?? 'Manual') as string,
          processId: (s.processId ?? s.ProcessId ?? 0) as number,
        }));
        self.postMessage({ type: 'METRICS_SERVICES_UPDATED', payload: servicesList });
      }
    }
  } catch (err: unknown) {
    if (err instanceof Error && err.name === 'AbortError') {
      // Cleanly ignore aborts
    } else {
      console.warn('[TelemetryWorker] Fetch error for category:', currentActiveSubTab, err);
    }
  } finally {
    clearTimeout(timeoutId);
    currentAbortController = null;
    isFetching = false;
  }
}
