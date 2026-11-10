/*
 * logger.ts - the app's log file in the user-data folder (logs/rev-bench.log): the main
 * process's lines, the engine process's own (forwarded by the main process) and whatever
 * the engine writes to its stdout and stderr. One file, kept to a size: past `maxBytes` it
 * moves to rev-bench.log.1 (replacing the older one) and starts again.
 */
import { appendFileSync, mkdirSync, renameSync, rmSync, statSync } from "node:fs";
import { dirname } from "node:path";

export type LogLevel = "info" | "warn" | "error";

export interface Logger {
  log(level: LogLevel, source: string, message: string): void;
}

export interface FileLoggerOptions {
  maxBytes?: number;
  now?: () => Date;
}

export function createFileLogger(file: string, options: FileLoggerOptions = {}): Logger {
  const maxBytes = options.maxBytes ?? 1_000_000;
  const now = options.now ?? (() => new Date());
  let ready = false;
  return {
    log(level, source, message) {
      try {
        if (!ready) {
          mkdirSync(dirname(file), { recursive: true });
          ready = true;
        }
        try {
          if (statSync(file).size >= maxBytes) {
            rmSync(`${file}.1`, { force: true });
            renameSync(file, `${file}.1`);
          }
        } catch {
          // no file yet
        }
        const line = `${now().toISOString()} [${source}] ${level}: ${message.replace(/\r?\n/g, "\n    ")}\n`;
        appendFileSync(file, line, "utf8");
      } catch {
        // a log that can't be written must never stop the app
      }
    },
  };
}
