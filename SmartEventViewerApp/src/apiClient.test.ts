import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { fetchApiChannels, fetchApiEvents } from './apiClient';

describe('apiClient unit tests', () => {
  const originalFetch = globalThis.fetch;

  afterEach(() => {
    globalThis.fetch = originalFetch;
  });

  it('fetchApiChannels should fetch and return channel list', async () => {
    globalThis.fetch = vi.fn().mockImplementation(async (url: string) => {
      if (url.includes('/api/channels')) {
        return {
          ok: true,
          status: 200,
          json: async () => ({ channels: ['Security', 'System', 'Application'] })
        } as Response;
      }
      return { ok: false, status: 404 } as Response;
    });

    const channelsData = await fetchApiChannels('http://localhost:8080');
    expect(channelsData).toBeDefined();
    expect(channelsData.channels).toEqual(['Security', 'System', 'Application']);
  });

  it('fetchApiEvents should fetch and map channel event records', async () => {
    globalThis.fetch = vi.fn().mockImplementation(async (url: string) => {
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
    });

    const eventsData = await fetchApiEvents('Security', 'http://localhost:8080');
    expect(eventsData).toBeDefined();
    expect(eventsData.events).toHaveLength(1);
    expect(eventsData.events![0].id).toBe(4624);
  });
});

