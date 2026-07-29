import React from 'react';

export const RiskCenter: React.FC = () => {
  return (
    <div style={{ flex: 1, padding: '20px', display: 'flex', flexDirection: 'column', gap: '16px', overflowY: 'auto' }}>
      <header>
        <h1 style={{ fontSize: '1.4rem', color: '#f87171' }}>🚨 SIEM Security Risk Center & Threat Operations</h1>
        <p style={{ fontSize: '0.8rem', color: '#94a3b8' }}>Real-time threat detection, suspicious RDP sessions, and active compromise indicators.</p>
      </header>

      <div style={{ background: 'rgba(30, 41, 59, 0.7)', border: '1px solid #f87171', borderRadius: '10px', padding: '16px' }}>
        <h3 style={{ color: '#f87171', marginBottom: '8px' }}>High Severity Security Breaches</h3>
        <p style={{ fontSize: '0.82rem', color: '#94a3b8' }}>Event ID 4625 (Failed Logon Spike) detected from IP 192.168.1.105 targeting Administrator account.</p>
      </div>
    </div>
  );
};
