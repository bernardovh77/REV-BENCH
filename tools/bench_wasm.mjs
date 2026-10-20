// bench_wasm.mjs - runs the core's benchmark as WebAssembly, under Node's WASI.
//
//   node tools/bench_wasm.mjs build/rb_bench.wasm [PRESET...] [--rate HZ] [--seconds S]
//
// make bench-wasm builds the .wasm (native/rb_bench.c with the core, for
// wasm32-wasi) and runs this. The numbers compare with make bench's: the same
// code, natively and as WebAssembly, on the same machine.
import { readFile } from 'node:fs/promises';
import { argv, exit } from 'node:process';
import { WASI } from 'node:wasi';

if (argv.length < 3) {
  console.error('usage: node tools/bench_wasm.mjs FILE.wasm [rb_bench options]');
  exit(2);
}
const wasi = new WASI({ version: 'preview1', args: ['rb_bench', ...argv.slice(3)], returnOnExit: true });
const module = await WebAssembly.compile(await readFile(argv[2]));
const instance = await WebAssembly.instantiate(module, wasi.getImportObject());
exit(wasi.start(instance));
