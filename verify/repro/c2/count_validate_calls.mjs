// Preload hook: counts how the committed JavaScript tests call the public
// `Blaze.prototype.validate` (callback / format string / callbackless), bucketed
// by the calling *.test.mjs file. Prints a summary on exit.
//
// Run from the root of a Blaze checkout that has build/contrib/sourcemeta_blaze_contrib_compile:
//   node --import ./verify/repro/c2/count_validate_calls.mjs --test ports/javascript/trace.test.mjs
import { resolve } from 'node:path';

const { Blaze } = await import(resolve(process.cwd(), 'ports/javascript/index.mjs'));
const counts = {};
const original = Blaze.prototype.validate;
Blaze.prototype.validate = function (instance, callbackOrFormat) {
  const stack = new Error().stack.split('\n').map((line) => line.trim());
  const caller = (stack.find((line) => line.includes('.test.mjs')) || 'unknown')
    .replace(/.*\/(ports\/javascript\/[^:]+).*/, '$1');
  const kind = typeof callbackOrFormat === 'function' ? 'callback'
    : typeof callbackOrFormat === 'string' ? 'format-string'
      : 'callbackless';
  counts[caller] ??= {};
  counts[caller][kind] = (counts[caller][kind] || 0) + 1;
  return original.call(this, instance, callbackOrFormat);
};
process.on('exit', () => {
  console.error('VALIDATE CALL SUMMARY ' + JSON.stringify(counts));
});
