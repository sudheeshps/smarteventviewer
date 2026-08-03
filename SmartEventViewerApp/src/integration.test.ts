import { describe, it, expect } from 'vitest';
import { fetchApiChannels, fetchApiEvents } from './apiClient';

describe('Real SmartEventViewerServer WebAPI Integration', () => {
  const SERVER_BASE_URL = 'http://127.0.0.1:8080';

  it('should fetch live channels from hosted native SmartEventViewerServer', async () => {
    console.log(`[React Integration Test] Sending GET ${SERVER_BASE_URL}/api/channels...`);
    const data = await fetchApiChannels(SERVER_BASE_URL);
    expect(data).toBeDefined();
    expect(data.channels).toBeDefined();
    if (data && data.channels) {
      expect(Array.isArray(data.channels)).toBe(true);
      expect(data.channels.length).toBeGreaterThan(0);
      console.log(`[React Integration Test] Successfully retrieved ${data.channels.length} channels from native server.`);
    }
  });

  it('should fetch live Application log events from hosted native SmartEventViewerServer', async () => {
    console.log(`[React Integration Test] Sending GET ${SERVER_BASE_URL}/api/events?channel=Application...`);
    const data = await fetchApiEvents('Application', SERVER_BASE_URL);
    expect(data).toBeDefined();
    expect(data.channel).toBe('Application');
    expect(data.totalCount).toBeGreaterThanOrEqual(0);
    expect(Array.isArray(data.events)).toBe(true);
    console.log(`[React Integration Test] Successfully retrieved Application events. Total count: ${data.totalCount}`);
  });

  it('should process threat analysis request and allow immediate status polling without deadlock', async () => {
    console.log(`[React Integration Test] Enqueueing async analysis request to ${SERVER_BASE_URL}/api/analyze...`);
    const postRes = await fetch(`${SERVER_BASE_URL}/api/analyze`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ query: 'Check for failed logon attempts' })
    });
    expect(postRes.status).toBe(200);
    const postData = await postRes.json();
    expect(postData.taskId).toBeDefined();
    const taskId = postData.taskId;

    console.log(`[React Integration Test] Polling status for taskId=${taskId} concurrently...`);
    const statusRes = await fetch(`${SERVER_BASE_URL}/api/analyze/status?taskId=${taskId}`);
    expect(statusRes.status).toBe(200);
    const statusData = await statusRes.json();
    expect(statusData.taskId).toBe(taskId);
    expect(['pending', 'processing', 'completed']).toContain(statusData.status);
    console.log(`[React Integration Test] Status returned immediately with status='${statusData.status}'! Zero deadlock detected.`);
  });
});
