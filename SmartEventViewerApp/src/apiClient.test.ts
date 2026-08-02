import { fetchApiChannels, fetchApiEvents } from './apiClient';

async function runApiTests() {
  console.log('--- Running React UI API Client Integration Tests ---');
  
  // Test 1: fetchApiChannels with mock fetch
  let test1Passed = false;
  const originalFetch = window.fetch;
  try {
    window.fetch = (async (url: string) => {
      console.log(`[API TEST] Intercepted fetch call to: ${url}`);
      if (url.includes('/api/channels')) {
        return {
          ok: true,
          status: 200,
          json: async () => ({ channels: ['Security', 'System', 'Application'] })
        } as Response;
      }
      return { ok: false, status: 404 } as Response;
    }) as typeof fetch;

    const channelsData = await fetchApiChannels('http://localhost:8080');
    if (channelsData && channelsData.channels && channelsData.channels.length === 3) {
      test1Passed = true;
      console.log('[PASS] fetchApiChannels API access test');
    }
  } catch (err) {
    console.error('[FAIL] fetchApiChannels test:', err);
  }

  // Test 2: fetchApiEvents with mock fetch
  let test2Passed = false;
  try {
    window.fetch = (async (url: string) => {
      console.log(`[API TEST] Intercepted fetch call to: ${url}`);
      if (url.includes('/api/events?channel=Security')) {
        return {
          ok: true,
          status: 200,
          json: async () => ({
            channel: 'Security',
            totalCount: 1,
            events: [{ index: 1, id: 4624, level: 'Information', risk: 'Low', provider: 'Security-Auditing', time: '2026-07-30', message: 'Logon ok' }]
          })
        } as Response;
      }
      return { ok: false, status: 404 } as Response;
    }) as typeof fetch;

    const eventsData = await fetchApiEvents('Security', 'http://localhost:8080');
    if (eventsData && eventsData.events && eventsData.events[0].id === 4624) {
      test2Passed = true;
      console.log('[PASS] fetchApiEvents API access test');
    }
  } catch (err) {
    console.error('[FAIL] fetchApiEvents test:', err);
  } finally {
    window.fetch = originalFetch;
  }

  if (test1Passed && test2Passed) {
    console.log('--- All React UI API Tests Passed Successfully ---');
  } else {
    console.error('--- Some React UI API Tests Failed ---');
  }
}

if (typeof window !== 'undefined') {
  (window as unknown as Record<string, unknown>).__runApiTests = runApiTests;
}
