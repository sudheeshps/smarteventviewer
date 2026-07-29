import React from 'react';

export const Dashboard: React.FC = () => {
  return (
    <div style={{ flex: 1, padding: '20px', display: 'flex', flexDirection: 'column', gap: '16px', overflowY: 'auto' }}>
      <header>
        <h1 style={{ fontSize: '1.4rem', color: '#f8fafc' }}>📊 SIEM System Event Viewer & Analytics Dashboard</h1>
        <p style={{ fontSize: '0.8rem', color: '#94a3b8' }}>Real-time telemetry and risk overview across Windows Kernel EvtQuery Subsystem.</p>
      </header>

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '14px' }}>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '10px', padding: '16px' }}>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8' }}>TOTAL EVENTS INGESTED</div>
          <div style={{ fontSize: '1.6rem', fontWeight: 700, color: '#38bdf8' }}>70,057</div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '10px', padding: '16px' }}>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8' }}>CRITICAL RISKS DETECTED</div>
          <div style={{ fontSize: '1.6rem', fontWeight: 700, color: '#f87171' }}>142</div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '10px', padding: '16px' }}>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8' }}>SYSTEM SOURCES ACTIVE</div>
          <div style={{ fontSize: '1.6rem', fontWeight: 700, color: '#fbbf24' }}>29</div>
        </div>
        <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid rgba(255,255,255,0.1)', borderRadius: '10px', padding: '16px' }}>
          <div style={{ fontSize: '0.75rem', color: '#94a3b8' }}>LOCAL RAG LLM ENGINE</div>
          <div style={{ fontSize: '1.6rem', fontWeight: 700, color: '#4ade80' }}>ACTIVE</div>
        </div>
      </div>
    </div>
  );
};
