/*
 * main.ts - the window's entry: loads the design system's CSS and Inter, makes the engine store
 * and mounts the interface.
 */
import "@fontsource/inter/latin-400.css";
import "@fontsource/inter/latin-500.css";
import "@fontsource/inter/latin-600.css";
import "./design/tokens.css";
import "./design/base.css";
import { mount } from "svelte";
import App from "./App.svelte";
import { EngineStore } from "./lib/engine-store.svelte";

const store = new EngineStore({
  bridge: window.revbench,
  window,
  requestFrame: (callback) => window.requestAnimationFrame(callback),
  cancelFrame: (handle) => window.cancelAnimationFrame(handle),
});
store.start();

const target = document.getElementById("app");
if (!target) throw new Error("rev-bench: index.html has no #app");
mount(App, { target, props: { store } });
