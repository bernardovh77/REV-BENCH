// A stand-in for the engine process, for the snapshot-path measurement only: a utility process
// that sends messages of a given size through the MessagePortMain it is handed, as the engine
// does its snapshots, so the real cross-process path can be measured at sizes the engine's
// snapshots don't reach yet (runtime-v1 allows 64 KiB). Message { size, hz, seconds }: hz 0 sends
// as fast as the event loop turns.
process.parentPort.on("message", (event) => {
  const port = event.ports[0];
  const { size, hz, seconds } = event.data;
  port.start();
  const stamp = () => performance.timeOrigin + performance.now();
  const end = Date.now() + seconds * 1000;
  let sent = 0;
  const send = () => {
    port.postMessage({ type: "snapshot", seq: sent++, sentAt: stamp(), bytes: new ArrayBuffer(size) });
  };
  if (hz > 0) {
    // timers on Windows fire every 15.6 ms or so, so it sends what is due at each one (now and then
    // two in a row), which keeps the average rate right
    const start = Date.now();
    const timer = setInterval(() => {
      if (Date.now() >= end) {
        clearInterval(timer);
        port.postMessage({ type: "done", sent });
        return;
      }
      const due = Math.floor(((Date.now() - start) * hz) / 1000) - sent;
      for (let i = 0; i < due; i++) send();
    }, 4);
  } else {
    const turn = () => {
      if (Date.now() >= end) {
        port.postMessage({ type: "done", sent });
        return;
      }
      for (let i = 0; i < 4; i++) send();
      setImmediate(turn);
    };
    turn();
  }
});
