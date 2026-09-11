// Run from the repo root of the audited branch after building the compile CLI:
//   node --import ./verify/repro/c1_instrument_validate.mjs ports/javascript/trace.test.mjs
// Wraps Blaze.prototype.validate and, at exit, prints how many calls in the
// committed JS test run passed a trace callback vs. no callback (public
// callback-less API) vs. a standard-output format string.
import { Blaze } from '../../ports/javascript/index.mjs';

const counts = { callback: 0, noCallback: 0, formatString: 0 };
const original = Blaze.prototype.validate;
Blaze.prototype.validate = function (instance, callbackOrFormat) {
  if (typeof callbackOrFormat === 'function') counts.callback += 1;
  else if (typeof callbackOrFormat === 'string') counts.formatString += 1;
  else counts.noCallback += 1;
  return original.call(this, instance, callbackOrFormat);
};

process.on('exit', () => {
  process.stderr.write(`[c1-instrument] validate() calls: ${JSON.stringify(counts)}\n`);
});
