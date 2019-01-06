const fs = require('fs');
const Stream = require('stream');
const silk = require('bindings')('silk.node');

function getStreamTransform(api, options) {
  let buffer = Buffer.alloc(0);
  return new Stream.Transform({
    transform(chunk, encoding, callback) {
      buffer = Buffer.concat([buffer, chunk]);
      callback();
    },
    flush(callback) {
      try {
        const output = api(buffer, options);
        callback(null, output);
      } catch (e) {
        callback(e);
      }
    },
  });
}

function getBuffer(input) {
  if (typeof input === 'string') return fs.readFileSync(input);
  if (input instanceof Buffer) return input;
  throw new Error('should be either string of filename or buffer of file content');
}

function getWrappedApi(api) {
  return function (input, output, options) {
    if (arguments.length === 0) {
      return getStreamTransform(api, { });
    } else if (arguments.length === 1) {
      if (typeof input === 'object' && !(input instanceof Buffer)) {
        return getStreamTransform(api, input || { });
      }
      return api(getBuffer(input));
    }
    const inBuffer = getBuffer(input);
    if (typeof output === 'string') {
      const outBuffer = api(inBuffer, options || { });
      fs.writeFileSync(output, outBuffer);
    } else {
      return api(inBuffer, output || { });
    }
  };
}

const decode = getWrappedApi(silk.decode);
const encode = getWrappedApi(silk.encode);

function compare(ref, test, options = { }) {
  const refBuffer = getBuffer(ref);
  const testBuffer = getBuffer(test);
  return silk.compare(refBuffer, testBuffer, options);
}

module.exports = {
  decode,
  encode,
  compare,
};
