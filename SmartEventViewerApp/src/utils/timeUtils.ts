function formatComponentsTo12Hour(
  year: number,
  month: number,
  day: number,
  hour: number,
  minute: number,
  second: number
): string {
  const ampm = hour >= 12 ? 'PM' : 'AM';
  const hour12 = hour % 12 || 12;
  const mm = String(minute).padStart(2, '0');
  const ss = String(second).padStart(2, '0');
  const yyyy = String(year);
  const MM = String(month).padStart(2, '0');
  const dd = String(day).padStart(2, '0');
  const hh = String(hour12).padStart(2, '0');

  return `${yyyy}-${MM}-${dd} ${hh}:${mm}:${ss} ${ampm}`;
}

/**
 * Formats a local date-time string or timestamp into 12-hour format (e.g. "2026-08-18 10:18:17 AM").
 * Does NOT perform any timezone shift or UTC conversion, preserving the exact local date and time.
 *
 * @param timeStr The datetime string, number, or Date.
 * @returns 12-hour formatted string (e.g. "YYYY-MM-DD hh:mm:ss AM/PM"), or '-' if empty/invalid.
 */
export function formatTo12Hour(timeStr?: string | number | Date | null): string {
  if (timeStr === undefined || timeStr === null) {
    return '-';
  }

  if (timeStr instanceof Date) {
    if (isNaN(timeStr.getTime())) return '-';
    return formatComponentsTo12Hour(
      timeStr.getFullYear(),
      timeStr.getMonth() + 1,
      timeStr.getDate(),
      timeStr.getHours(),
      timeStr.getMinutes(),
      timeStr.getSeconds()
    );
  }

  if (typeof timeStr === 'number') {
    const d = new Date(timeStr);
    if (isNaN(d.getTime())) return '-';
    return formatComponentsTo12Hour(
      d.getFullYear(),
      d.getMonth() + 1,
      d.getDate(),
      d.getHours(),
      d.getMinutes(),
      d.getSeconds()
    );
  }

  const trimmed = String(timeStr).trim();
  if (!trimmed || trimmed === '' || trimmed === '-') {
    return '-';
  }

  // Check if it matches "YYYY-MM-DD HH:mm:ss" or "YYYY-MM-DDTHH:mm:ss"
  const match = trimmed.match(/^(\d{4})-(\d{2})-(\d{2})[T\s](\d{2}):(\d{2})(?::(\d{2}))?/);
  if (match) {
    const [, year, month, day, hour, minute, second] = match;
    return formatComponentsTo12Hour(
      parseInt(year, 10),
      parseInt(month, 10),
      parseInt(day, 10),
      parseInt(hour, 10),
      parseInt(minute, 10),
      second ? parseInt(second, 10) : 0
    );
  }

  // If it already has AM/PM, return as is
  if (/\b(AM|PM)\b/i.test(trimmed)) {
    return trimmed;
  }

  // Fallback to Date parse in local time if other format
  const d = new Date(trimmed);
  if (!isNaN(d.getTime())) {
    return formatComponentsTo12Hour(
      d.getFullYear(),
      d.getMonth() + 1,
      d.getDate(),
      d.getHours(),
      d.getMinutes(),
      d.getSeconds()
    );
  }

  return trimmed;
}

/**
 * Converts a UTC or ISO-8601 timestamp into the client's local timezone format in 12-hour format.
 * Handles ISO strings with 'Z', timezone offsets, space-separated UTC strings, and timestamps.
 *
 * @param utcStr The UTC timestamp string, number, or Date.
 * @returns Formatted local datetime string, or '-' if empty/invalid.
 */
export function formatUtcToLocal(utcStr?: string | number | Date | null): string {
  if (utcStr === undefined || utcStr === null) {
    return '-';
  }

  if (utcStr instanceof Date) {
    return isNaN(utcStr.getTime()) ? '-' : formatTo12Hour(utcStr);
  }

  if (typeof utcStr === 'number') {
    const d = new Date(utcStr);
    return isNaN(d.getTime()) ? '-' : formatTo12Hour(d);
  }

  const trimmed = String(utcStr).trim();
  if (!trimmed || trimmed === '' || trimmed === '-') {
    return '-';
  }

  let parseable = trimmed;

  // If format is "YYYY-MM-DD HH:mm:ss", convert to ISO "YYYY-MM-DDTHH:mm:ss"
  if (/^\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}/.test(parseable)) {
    parseable = parseable.replace(/\s+/, 'T');
  }

  // If ISO datetime without explicit timezone indicator, assume UTC
  if (/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?$/.test(parseable)) {
    parseable = `${parseable}Z`;
  }

  const d = new Date(parseable);
  if (isNaN(d.getTime())) {
    return formatTo12Hour(trimmed);
  }

  return formatTo12Hour(d);
}
