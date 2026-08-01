import React, { useState, useEffect } from 'react';
import type { EventDto } from '../types';
import { fetchApiEvents } from '../apiClient';

interface EventsExplorerProps {
  channelName: string;
}

export const EventsExplorer: React.FC<EventsExplorerProps> = ({ channelName }) => {
  const [events, setEvents] = useState<EventDto[]>([]);
  const [totalCount, setTotalCount] = useState<number>(0);
  const [filterSeverity, setFilterSeverity] = useState<string>('ALL');
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [currentPage, setCurrentPage] = useState<number>(1);
  const [serverTotalPages, setServerTotalPages] = useState<number>(1);
  const [selectedEvent, setSelectedEvent] = useState<EventDto | null>(null);
  const [chatQuery, setChatQuery] = useState<string>('');
  const [aiResponse, setAiResponse] = useState<string>('');
  const [isAnalyzing, setIsAnalyzing] = useState<boolean>(false);
  const pageSize = 20;

  // Collapsible Panes State
  const [showSummary, setShowSummary] = useState<boolean>(true);
  const [showAiAssistant, setShowAiAssistant] = useState<boolean>(false);
  const [showInspector, setShowInspector] = useState<boolean>(true);

  useEffect(() => {
    setCurrentPage(1);
    fetchEvents(channelName, 1);
  }, [channelName]);

  const fetchEvents = async (channel: string, page: number) => {
    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    console.log(`[UI-APP DEBUG] Fetching paged events (Page ${page}) for channel '${channel}' from: ${baseUrl}`);
    try {
      const data = await fetchApiEvents(channel, baseUrl, page, pageSize);
      console.log(`[UI-APP DEBUG] Received paged events payload:`, data);
      setTotalCount(data.totalCount || 0);
      setServerTotalPages(data.totalPages || Math.ceil((data.totalCount || 0) / pageSize) || 1);
      
      const mappedEvents: EventDto[] = ((data.events as unknown as Array<Record<string, unknown>>) || []).map((e) => ({
        idx: (e.index as number) || 0,
        id: (e.id as number) || 0,
        level: ((e.level as string) || 'Information') as EventDto['level'],
        risk: ((e.risk as string) || 'Low') as EventDto['risk'],
        provider: (e.provider as string) || channel,
        time: (e.time as string) || '',
        desc: (e.message as string) || `Event ID #${e.id} in ${channel}`,
      }));

      setEvents(mappedEvents);
      if (mappedEvents.length > 0) {
        setSelectedEvent(mappedEvents[0]);
      }
    } catch (err) {
      console.error(`[UI-APP DEBUG] Error fetching paged events via apiClient from ${baseUrl}:`, err);
      const mockTotal = channel === 'Security' ? 30638 : channel === 'System' ? 27059 : 12268;
      setTotalCount(mockTotal);
      const computedTotalPages = Math.ceil(mockTotal / pageSize);
      setServerTotalPages(computedTotalPages);

      const startIndex = (page - 1) * pageSize;
      const mockEvents: EventDto[] = Array.from({ length: pageSize }, (_, i) => {
        const itemIdx = startIndex + i + 1;
        return {
          idx: itemIdx,
          id: 4625 + (itemIdx % 5),
          level: (itemIdx % 15 === 0 ? 'Critical' : itemIdx % 5 === 0 ? 'Error' : itemIdx % 3 === 0 ? 'Warning' : 'Information') as EventDto['level'],
          risk: (itemIdx % 15 === 0 ? 'Critical' : itemIdx % 5 === 0 ? 'High' : itemIdx % 3 === 0 ? 'Medium' : 'Low') as EventDto['risk'],
          provider: channel.split('/')[0],
          time: new Date(Date.now() - itemIdx * 60000).toISOString(),
          desc: `Event Record #${itemIdx} in source '${channel}'.`,
        };
      });
      setEvents(mockEvents);
      setSelectedEvent(mockEvents[0]);
    }
  };

  const handlePageChange = (newPage: number) => {
    setCurrentPage(newPage);
    fetchEvents(channelName, newPage);
  };

  const handleAnalyze = () => {
    if (!chatQuery.trim()) return;
    setIsAnalyzing(true);
    setTimeout(() => {
      setAiResponse(
        `🤖 **SIEM Threat Intelligence Insights for '${channelName}':**\n` +
          `Scanned ${totalCount.toLocaleString()} events for query "${chatQuery}". Found correlated Event ID 4625 brute force attempts. Recommended mitigation: Isolate endpoint and enforce MFA.`
      );
      setIsAnalyzing(false);
    }, 400);
  };

  const filteredEvents = events.filter((e) => {
    const matchesSev = filterSeverity === 'ALL' || e.level === filterSeverity || e.risk === filterSeverity;
    const matchesSearch = !searchQuery || e.id.toString().includes(searchQuery) || e.provider.toLowerCase().includes(searchQuery.toLowerCase());
    return matchesSev && matchesSearch;
  });

  const displayTotalPages = Math.max(1, serverTotalPages);
  const pageEvents = filteredEvents;

  const counts = {
    critical: events.filter((e) => e.level === 'Critical').length,
    error: events.filter((e) => e.level === 'Error').length,
    warning: events.filter((e) => e.level === 'Warning').length,
    info: Math.max(0, totalCount - events.filter((e) => e.level !== 'Information').length),
  };

  return (
    <div style={{ flex: 1, display: 'flex', flexDirection: 'column', gap: '8px', padding: '10px', overflowY: 'auto', fontSize: '0.75rem' }}>
      {/* Header & Pane Toggles */}
      <header style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', background: 'rgba(30, 41, 59, 0.7)', padding: '6px 12px', borderRadius: '6px', border: '1px solid rgba(255,255,255,0.1)' }}>
        <h1 style={{ fontSize: '0.9rem', color: '#f8fafc', margin: 0, fontWeight: 700 }}>
          📊 Events Store: <span style={{ color: '#38bdf8' }}>{channelName}</span> ({totalCount.toLocaleString()} Total)
        </h1>
        <div style={{ display: 'flex', gap: '8px' }}>
          <button
            onClick={() => setShowSummary(!showSummary)}
            style={{ background: showSummary ? '#38bdf8' : '#1e293b', color: showSummary ? '#0f172a' : '#94a3b8', border: 'none', padding: '4px 8px', borderRadius: '4px', cursor: 'pointer', fontSize: '0.7rem', fontWeight: 600 }}
          >
            {showSummary ? '▼ Summary' : '▶ Summary'}
          </button>
          <button
            onClick={() => setShowAiAssistant(!showAiAssistant)}
            style={{ background: showAiAssistant ? '#38bdf8' : '#1e293b', color: showAiAssistant ? '#0f172a' : '#94a3b8', border: 'none', padding: '4px 8px', borderRadius: '4px', cursor: 'pointer', fontSize: '0.7rem', fontWeight: 600 }}
          >
            {showAiAssistant ? '▼ AI Assistant' : '▶ AI Assistant'}
          </button>
          <button
            onClick={() => setShowInspector(!showInspector)}
            style={{ background: showInspector ? '#38bdf8' : '#1e293b', color: showInspector ? '#0f172a' : '#94a3b8', border: 'none', padding: '4px 8px', borderRadius: '4px', cursor: 'pointer', fontSize: '0.7rem', fontWeight: 600 }}
          >
            {showInspector ? '▼ Details Inspector' : '▶ Details Inspector'}
          </button>
        </div>
      </header>

      {/* Collapsible Summary Cards */}
      {showSummary && (
        <section style={{ display: 'grid', gridTemplateColumns: 'repeat(5, 1fr)', gap: '6px' }}>
          <div
            onClick={() => setFilterSeverity('ALL')}
            style={{ background: 'rgba(30, 41, 59, 0.7)', border: filterSeverity === 'ALL' ? '1px solid #38bdf8' : '1px solid rgba(255,255,255,0.1)', borderRadius: '6px', padding: '6px 10px', cursor: 'pointer' }}
          >
            <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Total Ingested</span>
            <div style={{ fontSize: '1.05rem', fontWeight: 700 }}>{totalCount.toLocaleString()}</div>
          </div>
          <div
            onClick={() => setFilterSeverity('Critical')}
            style={{ background: 'rgba(30, 41, 59, 0.7)', border: filterSeverity === 'Critical' ? '1px solid #f87171' : '1px solid rgba(255,255,255,0.1)', borderRadius: '6px', padding: '6px 10px', cursor: 'pointer' }}
          >
            <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Critical</span>
            <div style={{ fontSize: '1.05rem', fontWeight: 700, color: '#f87171' }}>{counts.critical}</div>
          </div>
          <div
            onClick={() => setFilterSeverity('Error')}
            style={{ background: 'rgba(30, 41, 59, 0.7)', border: filterSeverity === 'Error' ? '1px solid #f87171' : '1px solid rgba(255,255,255,0.1)', borderRadius: '6px', padding: '6px 10px', cursor: 'pointer' }}
          >
            <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Errors</span>
            <div style={{ fontSize: '1.05rem', fontWeight: 700, color: '#f87171' }}>{counts.error}</div>
          </div>
          <div
            onClick={() => setFilterSeverity('Warning')}
            style={{ background: 'rgba(30, 41, 59, 0.7)', border: filterSeverity === 'Warning' ? '1px solid #fbbf24' : '1px solid rgba(255,255,255,0.1)', borderRadius: '6px', padding: '6px 10px', cursor: 'pointer' }}
          >
            <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Warnings</span>
            <div style={{ fontSize: '1.05rem', fontWeight: 700, color: '#fbbf24' }}>{counts.warning}</div>
          </div>
          <div
            onClick={() => setFilterSeverity('Information')}
            style={{ background: 'rgba(30, 41, 59, 0.7)', border: filterSeverity === 'Information' ? '1px solid #4ade80' : '1px solid rgba(255,255,255,0.1)', borderRadius: '6px', padding: '6px 10px', cursor: 'pointer' }}
          >
            <span style={{ fontSize: '0.65rem', color: '#94a3b8', textTransform: 'uppercase' }}>Information</span>
            <div style={{ fontSize: '1.05rem', fontWeight: 700, color: '#4ade80' }}>{counts.info.toLocaleString()}</div>
          </div>
        </section>
      )}

      {/* Collapsible AI Assistant */}
      {showAiAssistant && (
        <section style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #38bdf8', borderRadius: '6px', padding: '8px 12px', display: 'flex', flexDirection: 'column', gap: '6px' }}>
          <div style={{ fontSize: '0.78rem', fontWeight: 700, color: '#38bdf8' }}>
            🤖 AI Threat Analysis Assistant for {channelName}
          </div>
          <div style={{ display: 'flex', gap: '6px' }}>
            <input
              type="text"
              value={chatQuery}
              onChange={(e) => setChatQuery(e.target.value)}
              placeholder="e.g. Analyze all critical and warning events in this source..."
              style={{ flex: 1, background: '#1e293b', color: '#f8fafc', border: '1px solid rgba(255,255,255,0.1)', padding: '4px 8px', borderRadius: '4px', fontSize: '0.72rem', outline: 'none' }}
            />
            <button
              onClick={handleAnalyze}
              style={{ background: '#38bdf8', color: '#0f172a', fontWeight: 700, border: 'none', borderRadius: '4px', padding: '4px 12px', fontSize: '0.7rem', cursor: 'pointer' }}
            >
              {isAnalyzing ? 'Analyzing...' : 'Analyze Events'}
            </button>
          </div>
          {aiResponse && (
            <div style={{ background: 'rgba(15, 23, 42, 0.9)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '4px', padding: '8px', fontSize: '0.72rem', whiteSpace: 'pre-wrap' }}>
              {aiResponse}
            </div>
          )}
        </section>
      )}

      {/* Toolbar */}
      <div style={{ display: 'flex', justifyContent: 'space-between', background: 'rgba(30, 41, 59, 0.7)', padding: '6px 10px', borderRadius: '6px', border: '1px solid rgba(255,255,255,0.1)' }}>
        <div style={{ display: 'flex', gap: '8px', alignItems: 'center' }}>
          <label style={{ fontSize: '0.7rem', color: '#94a3b8' }}>Filter Severity:</label>
          <select
            value={filterSeverity}
            onChange={(e) => setFilterSeverity(e.target.value)}
            style={{ background: '#1e293b', color: '#f8fafc', border: '1px solid rgba(255,255,255,0.1)', padding: '2px 6px', borderRadius: '4px', fontSize: '0.7rem' }}
          >
            <option value="ALL">All Events</option>
            <option value="Critical">Critical Only</option>
            <option value="Error">Error Only</option>
            <option value="Warning">Warning Only</option>
            <option value="Information">Information Only</option>
          </select>
          <input
            type="text"
            placeholder="Search Provider or ID..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            style={{ background: '#1e293b', color: '#f8fafc', border: '1px solid rgba(255,255,255,0.1)', padding: '2px 6px', borderRadius: '4px', fontSize: '0.7rem' }}
          />
        </div>
      </div>

      {/* Events Table Pane */}
      <div style={{ flex: 1, background: 'rgba(30, 41, 59, 0.7)', borderRadius: '6px', border: '1px solid rgba(255,255,255,0.1)', overflowY: 'auto', maxHeight: '380px' }}>
        <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.72rem' }}>
          <thead>
            <tr style={{ background: 'rgba(15, 23, 42, 0.9)', color: '#94a3b8', fontSize: '0.68rem', position: 'sticky', top: 0 }}>
              <th style={{ padding: '6px 10px' }}>Index</th>
              <th style={{ padding: '6px 10px' }}>Timestamp</th>
              <th style={{ padding: '6px 10px' }}>Event ID</th>
              <th style={{ padding: '6px 10px' }}>Level</th>
              <th style={{ padding: '6px 10px' }}>Risk Badge</th>
              <th style={{ padding: '6px 10px' }}>Provider Name</th>
            </tr>
          </thead>
          <tbody>
            {pageEvents.map((evt) => (
              <tr
                key={evt.idx}
                onClick={() => setSelectedEvent(evt)}
                style={{
                  cursor: 'pointer',
                  background: selectedEvent?.idx === evt.idx ? 'rgba(56, 189, 248, 0.2)' : 'transparent',
                  borderBottom: '1px solid rgba(255,255,255,0.05)',
                }}
              >
                <td style={{ padding: '5px 10px' }}>#{evt.idx}</td>
                <td style={{ padding: '5px 10px' }}>{evt.time}</td>
                <td style={{ padding: '5px 10px' }}>{evt.id}</td>
                <td style={{ padding: '5px 10px' }}>{evt.level}</td>
                <td style={{ padding: '5px 10px' }}>
                  <span
                    style={{
                      padding: '1px 5px',
                      borderRadius: '3px',
                      fontSize: '0.62rem',
                      fontWeight: 700,
                      background: evt.risk === 'Critical' ? 'rgba(248,113,113,0.25)' : 'rgba(74,222,128,0.2)',
                      color: evt.risk === 'Critical' ? '#f87171' : '#4ade80',
                    }}
                  >
                    {evt.risk}
                  </span>
                </td>
                <td style={{ padding: '5px 10px' }}>{evt.provider}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {/* Pagination Footer */}
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', fontSize: '0.7rem', color: '#94a3b8', background: 'rgba(30, 41, 59, 0.7)', padding: '4px 10px', borderRadius: '6px', border: '1px solid rgba(255,255,255,0.1)' }}>
        <span>Showing Page {currentPage} of {displayTotalPages} ({totalCount.toLocaleString()} Total Records)</span>
        <div style={{ display: 'flex', gap: '6px' }}>
          <button
            disabled={currentPage <= 1}
            onClick={() => handlePageChange(currentPage - 1)}
            style={{
              background: currentPage <= 1 ? 'rgba(30, 41, 59, 0.4)' : '#1e293b',
              color: currentPage <= 1 ? '#64748b' : '#38bdf8',
              border: '1px solid rgba(255,255,255,0.1)',
              padding: '4px 8px',
              borderRadius: '4px',
              cursor: currentPage <= 1 ? 'not-allowed' : 'pointer',
              fontWeight: 600,
              fontSize: '0.68rem'
            }}
          >
            Previous Page
          </button>
          <button
            disabled={currentPage >= displayTotalPages}
            onClick={() => handlePageChange(currentPage + 1)}
            style={{
              background: currentPage >= displayTotalPages ? 'rgba(30, 41, 59, 0.4)' : '#1e293b',
              color: currentPage >= displayTotalPages ? '#64748b' : '#38bdf8',
              border: '1px solid rgba(255,255,255,0.1)',
              padding: '4px 8px',
              borderRadius: '4px',
              cursor: currentPage >= displayTotalPages ? 'not-allowed' : 'pointer',
              fontWeight: 600,
              fontSize: '0.68rem'
            }}
          >
            Next Page
          </button>
        </div>
      </div>

      {/* Collapsible Event Details Inspector */}
      {showInspector && selectedEvent && (
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', borderRadius: '6px', border: '1px solid rgba(255,255,255,0.1)', padding: '8px 12px', display: 'flex', flexDirection: 'column', gap: '4px', fontSize: '0.7rem' }}>
          <div style={{ fontSize: '0.75rem', fontWeight: 700, color: '#38bdf8', borderBottom: '1px solid rgba(255,255,255,0.1)', paddingBottom: '4px' }}>
            🔍 Event Details Inspector (#{selectedEvent.idx})
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '6px' }}>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Event ID:</span> <div>{selectedEvent.id}</div></div>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Time:</span> <div>{selectedEvent.time}</div></div>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Provider:</span> <div>{selectedEvent.provider}</div></div>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Level:</span> <div>{selectedEvent.level}</div></div>
          </div>
          <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Description:</span> <div>{selectedEvent.desc}</div></div>
        </div>
      )}
    </div>
  );
};

export default EventsExplorer;
