/*
 * bridge-impl.ts - the window's API (RevBenchBridge) over an IPC object, so it can be tested
 * without Electron. preload.ts hands it the real ipcRenderer. The preload exposes only these
 * few functions: no `ipcRenderer`, no `send` with a channel the page chooses.
 */
import { ENGINE_PORT_MESSAGE, IPC, type EnginePortMessage, type EngineStatus, type RevBenchBridge } from "../shared/bridge";

/** The slice of Electron's ipcRenderer the bridge uses. */
export interface IpcLike {
  invoke(channel: string): Promise<unknown>;
  send(channel: string): void;
  on(channel: string, listener: (event: { ports: MessagePort[] }, payload: unknown) => void): void;
  removeListener(channel: string, listener: (event: { ports: MessagePort[] }, payload: unknown) => void): void;
}

export interface BridgeEnv {
  ipc: IpcLike;
  platform: string;
  versions: { electron: string; chrome: string };
  /** window.postMessage: the port reaches the page as a message event */
  postToPage(message: EnginePortMessage, ports: MessagePort[]): void;
}

function isStatus(value: unknown): value is EngineStatus {
  return typeof value === "object" && value !== null && typeof (value as EngineStatus).state === "string" && typeof (value as EngineStatus).restarts === "number";
}

export function makeBridge(env: BridgeEnv): RevBenchBridge {
  // The main process sends a port when the page asks for one, and after every engine restart.
  env.ipc.on(IPC.enginePort, (event, payload) => {
    const restarts = typeof (payload as { restarts?: unknown } | null)?.restarts === "number" ? (payload as { restarts: number }).restarts : 0;
    env.postToPage({ channel: ENGINE_PORT_MESSAGE, restarts }, event.ports);
  });
  return {
    platform: env.platform,
    versions: { electron: env.versions.electron, chrome: env.versions.chrome },
    onEngineStatus(callback) {
      let live = true;
      const listener = (_event: unknown, status: unknown) => {
        if (live && isStatus(status)) callback(status);
      };
      env.ipc.on(IPC.engineStatus, listener);
      void env.ipc.invoke(IPC.getEngineStatus).then((status) => {
        if (live && isStatus(status)) callback(status);
      });
      return () => {
        live = false;
        env.ipc.removeListener(IPC.engineStatus, listener);
      };
    },
    requestEnginePort() {
      env.ipc.send(IPC.requestEnginePort);
    },
    restartEngine() {
      env.ipc.send(IPC.restartEngine);
    },
  };
}
