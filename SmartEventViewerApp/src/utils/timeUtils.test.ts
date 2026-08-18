import { describe, it, expect } from 'vitest';
import { formatUtcToLocal, formatTo12Hour } from './timeUtils';

describe('timeUtils - formatTo12Hour', () => {
  it('should return "-" for empty or null inputs', () => {
    expect(formatTo12Hour(null)).toBe('-');
    expect(formatTo12Hour(undefined)).toBe('-');
    expect(formatTo12Hour('')).toBe('-');
    expect(formatTo12Hour('-')).toBe('-');
  });

  it('should format morning time in 12-hour AM format without timezone shifts', () => {
    expect(formatTo12Hour('2026-08-18 10:18:17')).toBe('2026-08-18 10:18:17 AM');
    expect(formatTo12Hour('2026-08-18 00:05:30')).toBe('2026-08-18 12:05:30 AM');
  });

  it('should format afternoon/evening time in 12-hour PM format without timezone shifts', () => {
    expect(formatTo12Hour('2026-08-18 14:30:00')).toBe('2026-08-18 02:30:00 PM');
    expect(formatTo12Hour('2026-08-18 12:00:00')).toBe('2026-08-18 12:00:00 PM');
    expect(formatTo12Hour('2026-08-18 23:59:59')).toBe('2026-08-18 11:59:59 PM');
  });

  it('should handle ISO format strings', () => {
    expect(formatTo12Hour('2026-08-18T15:45:10')).toBe('2026-08-18 03:45:10 PM');
  });

  it('should preserve already formatted AM/PM strings', () => {
    expect(formatTo12Hour('2026-08-18 10:18:17 AM')).toBe('2026-08-18 10:18:17 AM');
  });
});

describe('timeUtils - formatUtcToLocal', () => {
  it('should return "-" for empty or null inputs', () => {
    expect(formatUtcToLocal(null)).toBe('-');
    expect(formatUtcToLocal(undefined)).toBe('-');
    expect(formatUtcToLocal('')).toBe('-');
    expect(formatUtcToLocal('-')).toBe('-');
  });

  it('should format UTC timestamp in 12-hour local time', () => {
    const utcIso = '2026-08-18T04:22:15.000Z';
    const local = formatUtcToLocal(utcIso);
    expect(local).toMatch(/\b(AM|PM)\b/);
  });
});
