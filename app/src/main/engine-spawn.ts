/*
 * engine-spawn.ts - the real engine process: Electron's utilityProcess running
 * dist/engine/engine.cjs, as the supervisor's EngineProcess. Not unit-tested (it needs
 * Electron); the end-to-end test (stage 124's second phase) launches it for real.
 */
import { utilityProcess, type MessagePortMain } from "electron";
import type { EngineToMain, MainToEngine } from "../shared/process-messages";
import type { Logger } from "./logger";
import type { EngineProcess } from "./supervisor";

export function spawnEngineProcess(enginePath: string, logger: Logger): EngineProcess<MessagePortMain> {
  const child = utilityProcess.fork(enginePath, [], {
    serviceName: "Rev-Bench engine",
    stdio: "pipe",
    env: { ...process.env },
  });
  const forward = (level: "info" | "error") => (chunk: Buffer) => {
    const text = chunk.toString("utf8").trimEnd();
    if (text) logger.log(level, "engine-stdio", text);
  };
  child.stdout?.on("data", forward("info"));
  child.stderr?.on("data", forward("error"));
  return {
    get pid() {
      return child.pid;
    },
    postMessage: (message: MainToEngine, ports?: MessagePortMain[]) => child.postMessage(message, ports),
    kill: () => void child.kill(),
    onMessage: (listener) => void child.on("message", (message: EngineToMain) => listener(message)),
    onExit: (listener) => void child.on("exit", (code) => listener(code)),
  };
}
