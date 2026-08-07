// Telemetry Web Worker running in a separate background thread
// Connects to WebSocket /ws/telemetry for real-time server push events
// and pulls endpoint data strictly when notified that its category updated.

let socket: WebSocket | null = null;
let currentAbortController: AbortController | null = null;
let currentBaseUrl: string = '';
let currentActiveSubTab: string = 'overview';
let reconnectTimeoutId: ReturnType<typeof setTimeout> | null = null;

self.onmessage = (event: MessageEvent) => {
  const { type, baseUrl, subTab } = event.data;

  if (type === 'START_POLLING') {
    currentBaseUrl = baseUrl || currentBaseUrl;
    currentActiveSubTab = subTab || currentActiveSubTab;
    connectWebSocket();
    fetchActiveSubTab();
  } else if (type === 'STOP_POLLING') {
    disconnectWebSocket();
  } else if (type === 'CHANGE_SUBTAB') {
    currentActiveSubTab = subTab;
    fetchActiveSubTab();
  }
};

function disconnectWebSocket() {
  if (reconnectTimeoutId !== null) {
    clearTimeout(reconnectTimeoutId);
    reconnectTimeoutId = null;
  }
  if (socket) {
    socket.onclose = null;
    socket.close();
    socket = null;
  }
  if (currentAbortController) {
    currentAbortController.abort();
    currentAbortController = null;
  }
}

function connectWebSocket() {
  disconnectWebSocket();

  const urlPrefix = currentBaseUrl ? currentBaseUrl : (self.location.origin.includes(':') ? self.location.origin : 'http://127.0.0.1:8080');
  const wsUrl = urlPrefix.replace(/^http/, 'ws') + '/ws/telemetry';

  try {
    socket = new WebSocket(wsUrl);

    socket.onopen = () => {
      console.log('[TelemetryWorker] WebSocket connected to', wsUrl);
    };

    socket.onmessage = (messageEvent: MessageEvent) => {
      try {
        const msg = JSON.parse(messageEvent.data);
        if (msg.type === 'TELEMETRY_UPDATED') {
          handleServerPush(msg.category);
        }
      } catch (err) {
        console.warn('[TelemetryWorker] Invalid WS message format:', err);
      }
    };

    socket.onerror = (err) => {
      console.warn('[TelemetryWorker] WebSocket error:', err);
    };

    socket.onclose = () => {
      console.log('[TelemetryWorker] WebSocket connection closed. Reconnecting in 3s...');
      socket = null;
      reconnectTimeoutId = setTimeout(() => {
        connectWebSocket();
      }, 3000);
    };
  } catch (err) {
    console.warn('[TelemetryWorker] Failed to create WebSocket connection:', err);
  }
}

function handleServerPush(updatedCategory: string) {
  if (updatedCategory === 'summary' && currentActiveSubTab === 'overview') {
    fetchActiveSubTab();
  } else if (updatedCategory === 'processes' && currentActiveSubTab === 'processes') {
    fetchActiveSubTab();
  } else if (updatedCategory === 'sessions' && (currentActiveSubTab === 'sessions' || currentActiveSubTab === 'users')) {
    fetchActiveSubTab();
  }
}

async function fetchActiveSubTab() {
  if (currentAbortController) {
    currentAbortController.abort();
  }

  currentAbortController = new AbortController();
  const signal = currentAbortController.signal;
  const urlPrefix = currentBaseUrl ? currentBaseUrl : (self.location.origin.includes(':') ? self.location.origin : 'http://127.0.0.1:8080');

  try {
    if (currentActiveSubTab === 'overview') {
      const resp = await fetch(`${urlPrefix}/api/metrics/summary`, { signal });
      if (resp.ok) {
        const data = await resp.json();
        self.postMessage({ type: 'METRICS_SUMMARY_UPDATED', payload: data });
      }
    } else if (currentActiveSubTab === 'processes') {
      const resp = await fetch(`${urlPrefix}/api/metrics/processes`, { signal });
      if (resp.ok) {
        const data = await resp.json();
        const processesList = Array.isArray(data) ? data : (data.topProcesses || data.TopProcesses || []);
        self.postMessage({ type: 'METRICS_PROCESSES_UPDATED', payload: processesList });
      }
    } else if (currentActiveSubTab === 'sessions' || currentActiveSubTab === 'users') {
      const resp = await fetch(`${urlPrefix}/api/metrics/sessions`, { signal });
      if (resp.ok) {
        const data = await resp.json();
        self.postMessage({ type: 'METRICS_SESSIONS_UPDATED', payload: data });
      }
    }
  } catch (err: unknown) {
    if (err instanceof Error && err.name === 'AbortError') {
      // Cleanly ignore aborts
    } else {
      console.warn('[TelemetryWorker] Fetch error for category:', currentActiveSubTab, err);
    }
  } finally {
    currentAbortController = null;
  }
}
