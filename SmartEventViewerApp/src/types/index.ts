export interface EventDto {
  idx: number;
  id: number;
  level: 'Critical' | 'Error' | 'Warning' | 'Information' | 'Verbose';
  risk: 'Critical' | 'High' | 'Medium' | 'Low';
  provider: string;
  time: string;
  desc: string;
  xml?: string;
}

export interface EventLogResponseDto {
  channel: string;
  totalCount: number;
  events: EventDto[];
}

export interface MultiChannelAnomaliesDto {
  securityEvents: EventDto[];
  systemEvents: EventDto[];
  applicationEvents: EventDto[];
  sysmonEvents: EventDto[];
  totalCriticalCount: number;
  totalErrorCount: number;
  totalWarningCount: number;
}

export interface ChannelTreeData {
  windowsLogs: string[];
  appServicesLogs: string[];
}
