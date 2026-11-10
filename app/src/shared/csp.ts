/*
 * csp.ts - the window's content security policy, the one place it is written.
 * The renderer's build puts it into index.html as a <meta> (vite.config.ts), and the
 * security test (tests/security.test.ts) reads it from here.
 *
 * Everything the window loads is its own build, from a file: scripts, styles, fonts and
 * images from 'self', nothing inline, no eval, no network (the window talks to the engine
 * through a MessagePort, which no policy governs), no frames, plugins or forms.
 */
export const CSP_DIRECTIVES: Readonly<Record<string, readonly string[]>> = {
  "default-src": ["'none'"],
  "script-src": ["'self'"],
  "style-src": ["'self'"],
  "img-src": ["'self'"],
  "font-src": ["'self'"],
  "connect-src": ["'none'"],
  "object-src": ["'none'"],
  "frame-src": ["'none'"],
  "worker-src": ["'none'"],
  "base-uri": ["'none'"],
  "form-action": ["'none'"],
};

export const CONTENT_SECURITY_POLICY: string = Object.entries(CSP_DIRECTIVES)
  .map(([name, sources]) => `${name} ${sources.join(" ")}`)
  .join("; ");
