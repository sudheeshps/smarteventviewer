export interface ChannelData {
  channels?: string[];
}

export interface EventsData {
  channel?: string;
  totalCount?: number;
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
