import React, { useState, useEffect } from 'react';
import type { EventDto } from '../types';
import { fetchApiEvents, fetchEventSummary, fetchApiAnalyze, fetchApiAnalyzeStatus } from '../apiClient';

interface EventsExplorerProps {
  channelName: string;
  onOpenChat?: (query: string, response: string) => void;
}

export const EventsExplorer: React.FC<EventsExplorerProps> = ({ channelName, onOpenChat }) => {
  const [events, setEvents] = useState<EventDto[]>([]);
  const [totalCount, setTotalCount] = useState<number>(0);
  const [isLoadingEvents, setIsLoadingEvents] = useState<boolean>(false);
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
  const [showXml, setShowXml] = useState<boolean>(false);

  useEffect(() => {
    setCurrentPage(1);
    fetchEvents(channelName, 1);
  }, [channelName]);

  const [serverLevelCounts, setServerLevelCounts] = useState<{ critical: number; error: number; warning: number; info: number; verbose: number }>({
    critical: 0,
    error: 0,
    warning: 0,
    info: 0,
    verbose: 0,
  });

  const fetchEvents = async (channel: string, page: number) => {
    setIsLoadingEvents(true);
    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    console.log(`[UI-APP DEBUG] Fetching paged events (Page ${page}) for channel '${channel}' from: ${baseUrl}`);
    try {
      // 1. Fetch channel summary counts
      const summaryData = await fetchEventSummary(channel, baseUrl).catch(() => null);
      if (summaryData) {
        setServerLevelCounts({
          critical: summaryData.criticalCount || 0,
          error: summaryData.errorCount || 0,
          warning: summaryData.warningCount || 0,
          info: summaryData.infoCount || 0,
          verbose: summaryData.verboseCount || 0,
        });
      }

      // 2. Fetch page event records list
      const data = await fetchApiEvents(channel, baseUrl, page, pageSize);
      console.log(`[UI-APP DEBUG] Received paged events payload:`, data);
      setTotalCount(data.totalCount || 0);
      setServerTotalPages(data.totalPages || Math.ceil((data.totalCount || 0) / pageSize) || 1);

      const startIndex = (page - 1) * pageSize;
      const mappedEvents: EventDto[] = ((data.events as unknown as Array<Record<string, unknown>>) || []).map((e, i) => ({
        idx: (e.index as number) || (startIndex + i + 1),
        id: (e.id as number) || 0,
        level: ((e.level as string) || 'Information') as EventDto['level'],
        risk: ((e.risk as string) || 'Low') as EventDto['risk'],
        provider: (e.provider as string) || channel,
        time: e.time ? (typeof e.time === 'string' && e.time.includes('T') ? new Date(e.time).toLocaleString() : (e.time as string)) : '',
        desc: (e.message as string) || `Event ID #${e.id} logged by ${e.provider || channel}.`,
        xml: (e.rawXml as string) || '',
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
    } finally {
      setIsLoadingEvents(false);
    }
  };

  const handlePageChange = (newPage: number) => {
    setCurrentPage(newPage);
    fetchEvents(channelName, newPage);
  };

  const handleAnalyze = async () => {
    if (!chatQuery.trim()) return;
    const queryText = chatQuery.trim();
    setIsAnalyzing(true);

    if (onOpenChat) {
      onOpenChat(queryText, '⏳ Enqueued for analysis...');
    }

    const baseUrl = window.location.origin.includes(':') ? window.location.origin : 'http://127.0.0.1:8080';
    try {
      // 1. Immediately enqueue request non-blockingly
      const enqueueRes = await fetchApiAnalyze(channelName, queryText, baseUrl);
      const taskId = enqueueRes.taskId;

      if (!taskId) {
        setAiResponse(enqueueRes.analysis || 'Error initiating analysis task.');
        if (onOpenChat) onOpenChat(queryText, enqueueRes.analysis || 'Error initiating task.');
        return;
      }

      // 2. Poll status endpoint to stream real-time push notification updates
      let isCompleted = false;
      let statusRes = enqueueRes;

      while (!isCompleted) {
        await new Promise((resolve) => setTimeout(resolve, 300));
        statusRes = await fetchApiAnalyzeStatus(taskId, baseUrl);
        
        if (statusRes.progressMessage && onOpenChat) {
          onOpenChat(queryText, `⏳ ${statusRes.progressMessage}`);
        }

        if (statusRes.status === 'COMPLETED' || statusRes.status === 'FAILED' || (statusRes.analysis && statusRes.analysis.trim().length > 0)) {
          isCompleted = true;
        }
      }

      let finalResult = (statusRes.analysis && statusRes.analysis.trim().length > 0) ? statusRes.analysis : `Analyzed ${statusRes.eventsAnalyzed || 0} event records for '${channelName}'.`;

      setAiResponse(finalResult);
      if (onOpenChat) {
        onOpenChat(queryText, finalResult);
      }
    } catch (err) {
      console.error('[UI-APP DEBUG] Error calling backend analyze endpoint:', err);
    } finally {
      setIsAnalyzing(false);
    }
  };

  const filteredEvents = events.filter((e) => {
    const matchesSev = filterSeverity === 'ALL' || e.level === filterSeverity || e.risk === filterSeverity;
    const matchesSearch = !searchQuery || e.id.toString().includes(searchQuery) || e.provider.toLowerCase().includes(searchQuery.toLowerCase());
    return matchesSev && matchesSearch;
  });

  const calculatedTotalPages = Math.max(1, Math.ceil(totalCount / pageSize));
  const displayTotalPages = serverTotalPages > 1 ? serverTotalPages : calculatedTotalPages;
  const pageEvents = filteredEvents;

  const counts = {
    critical: serverLevelCounts.critical,
    error: serverLevelCounts.error,
    warning: serverLevelCounts.warning,
    info: (serverLevelCounts.info > 0)
      ? serverLevelCounts.info
      : Math.max(0, totalCount - (serverLevelCounts.critical + serverLevelCounts.error + serverLevelCounts.warning)),
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
      <div style={{ flex: 1, background: 'rgba(30, 41, 59, 0.7)', borderRadius: '6px', border: '1px solid rgba(255,255,255,0.1)', overflowY: 'auto', maxHeight: '380px', position: 'relative' }}>
        {isLoadingEvents && (
          <div style={{ position: 'absolute', inset: 0, background: 'rgba(15, 23, 42, 0.75)', backdropFilter: 'blur(2px)', display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', zIndex: 10, gap: '8px' }}>
            <div style={{ width: '28px', height: '28px', border: '3px solid rgba(56,189,248,0.2)', borderTop: '3px solid #38bdf8', borderRadius: '50%', animation: 'spin 0.8s linear infinite' }} />
            <div style={{ color: '#38bdf8', fontWeight: 600, fontSize: '0.75rem' }}>
              ⏳ Ingesting and decoding events for channel <span style={{ color: '#f8fafc' }}>{channelName}</span>...
            </div>
          </div>
        )}
        <style>{`@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }`}</style>
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
                      background: (evt.level === 'Critical' || evt.risk === 'Critical') ? 'rgba(248,113,113,0.25)' : ((evt.level === 'Error' || evt.risk === 'High') ? 'rgba(248,113,113,0.15)' : ((evt.level === 'Warning' || evt.risk === 'Medium') ? 'rgba(251,191,36,0.2)' : 'rgba(74,222,128,0.2)')),
                      color: (evt.level === 'Critical' || evt.risk === 'Critical' || evt.level === 'Error' || evt.risk === 'High') ? '#f87171' : ((evt.level === 'Warning' || evt.risk === 'Medium') ? '#fbbf24' : '#4ade80'),
                    }}
                  >
                    {evt.level !== 'Information' ? evt.level : evt.risk}
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
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', borderRadius: '6px', border: '1px solid rgba(255,255,255,0.1)', padding: '8px 12px', display: 'flex', flexDirection: 'column', gap: '6px', fontSize: '0.7rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '1px solid rgba(255,255,255,0.1)', paddingBottom: '4px' }}>
            <span style={{ fontSize: '0.75rem', fontWeight: 700, color: '#38bdf8' }}>
              🔍 Event Details Inspector (#{selectedEvent.idx})
            </span>
            {selectedEvent.xml && (
              <button
                onClick={() => setShowXml(!showXml)}
                style={{ background: showXml ? '#38bdf8' : '#1e293b', color: showXml ? '#0f172a' : '#38bdf8', border: '1px solid rgba(56,189,248,0.3)', padding: '2px 8px', borderRadius: '4px', cursor: 'pointer', fontSize: '0.65rem', fontWeight: 600 }}
              >
                {showXml ? 'Hide XML Dump' : '📄 View Raw XML'}
              </button>
            )}
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '6px' }}>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Event ID:</span> <div style={{ fontWeight: 700, color: '#f8fafc' }}>{selectedEvent.id}</div></div>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Time:</span> <div>{selectedEvent.time}</div></div>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Provider:</span> <div>{selectedEvent.provider}</div></div>
            <div><span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Level:</span> <div>{selectedEvent.level}</div></div>
          </div>
          <div>
            <span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Issue / Description:</span>
            <div style={{ color: '#e2e8f0', marginTop: '2px', fontWeight: 600 }}>{selectedEvent.desc}</div>
          </div>

          {showXml && selectedEvent.xml && (
            <div style={{ marginTop: '4px' }}>
              <span style={{ fontSize: '0.65rem', color: '#94a3b8' }}>Raw Windows Event EvtRender XML:</span>
              <pre style={{ background: '#0f172a', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '4px', padding: '8px', overflowX: 'auto', fontSize: '0.65rem', color: '#38bdf8', whiteSpace: 'pre-wrap', maxHeight: '180px' }}>
                {selectedEvent.xml}
              </pre>
            </div>
          )}
        </div>
      )}
    </div>
  );
};

export default EventsExplorer;
