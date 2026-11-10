/*
 * config.ts - how the environment chooses what the engine process does. These variables are for
 * tests and developers; a user never sets them.
 *
 *   RB_FAKE_ADDON=1          the test double instead of the real add-on (never a silent fallback)
 *   RB_BACKEND=null          the null audio backend: no sound device is opened (tests)
 *   RB_SAMPLE_RATE=44100     the null backend's rate, 44100 or 48000
 *   RB_DEVICE=n              a device index (default backend)
 *   RB_NATIVE_PATH=file      where rb_native.node is (read by the engine process)
 *   RB_USER_DATA=folder      the user-data folder (tests use a temporary one)
 */
import type { BuildChoice, EngineInit } from "../shared/process-messages";

export function engineInitFromEnv(env: Record<string, string | undefined>, build: BuildChoice): EngineInit {
  const backend = env.RB_BACKEND === "null" ? "null" : "default";
  const init: EngineInit = {
    addon: env.RB_FAKE_ADDON === "1" ? "fake" : "native",
    backend,
    build,
  };
  const rate = Number(env.RB_SAMPLE_RATE);
  if (backend === "null" && (rate === 44100 || rate === 48000)) init.sampleRate = rate;
  const device = env.RB_DEVICE === undefined || env.RB_DEVICE === "" ? NaN : Number(env.RB_DEVICE);
  if (backend === "default" && Number.isInteger(device) && device >= 0) init.device = device;
  return init;
}
