const cac = require('cac');
const silk = require('./index');
const { version } = require('./package');

const cli = cac();

function intValidator(name, value, config) {
  if (typeof value !== 'number' || Math.floor(value) !== value) {
    throw new Error(`--${name} should be provided as an integer.`);
  }
  if (typeof config.max === 'number' && value > config.max) {
    throw new Error(`--${name} should be less than ${config.max}.`);
  }
  return;
}

function rangeValidator(name, value, config) {
  if (typeof value !== 'number' || value < config.min || value > config.max) {
    throw new Error(`--${name} should be provided as a number from ${config.min} to ${config.max}.`);
  }
  return;
}

function oneOfValidator(name, value, config) {
  if (config.oneOf.every(possible => possible !== value)) {
    throw new Error(`--${name} should be provided as one of ${JSON.stringify(config.oneOf)}`);
  }
  return;
}

const validators = {
  int: intValidator,
  range: rangeValidator,
  oneOf: oneOfValidator,
};

function validateOptions(options, config) {
  Object.keys(config).forEach((key) => {
    const option = options[key];
    let type = config[key];
    if (typeof type !== 'string') type = config[key].type;
    validators[type](key, option, config[key]);
  });
}

cli.command('decode [input] [output]', 'decode silk format audio')
  .option('-q, --quiet', 'Whether there is any output into console', { default: false })
  .option('--lossProb [lossProb]', 'Simulated packet loss percentage (0-100)', { default: 0 })
  .option('--fsHz [fs]', 'Sampling rate of output signal in Hz', { default: 24000 })
  .action((input, output, options) => {
    try {
      if (typeof input !== 'string') {
        throw new Error(`Input of filepath should be provided, but received ${input}.`);
      }
      if (typeof output !== 'string') {
        throw new Error(`Output of filepath should be provided, but received ${output}.`);
      }
      validateOptions(options, {
        lossProb: { type: 'range', min: 0, max: 100 },
        fsHz: 'int',
      });
      silk.decode(input, output, options);
    } catch (e) {
      console.error(`[ERROR]: ${e.message}`);
    }
  });

cli.command('encode [input] [output]', 'encode speech input to silk format')
  .option('-q, --quiet', 'Whether there is any output into console', { default: false })
  .option('--fsHz [fs]', 'API sampling rate in Hz', { default: 24000 })
  .option('--fxMaxInternal [fxMaxInternal]', 'Maximum internal sampling rate in Hz', { default: 0 })
  .option('--packetLength [packageLength]', 'Packet interval in ms', { default: 20 })
  .option('--rate', 'Target bitrate', { default: 25000 })
  .option('--loss', 'Uplink loss estimate, in percent (0-100)', { default: 0 })
  .option('--complexity', 'Set complexity, 0: low, 1: medium, 2: high', { default: 2 })
  .option('--inbandFEC', 'Enable inband FEC usage', { default: false })
  .option('--dtx', 'Enable DTX', { default: false })
  .option('--tencent', 'Add Tencent (Wechat, QQ) header in exported file', { default: false })
  .option('--tencentAmr', 'Add Tencent AMR header in exported file', { default: false })
  .action((input, output, options) => {
    try {
      if (typeof input !== 'string') {
        throw new Error(`Input of filepath should be provided, but received ${input}.`);
      }
      if (typeof output !== 'string') {
        throw new Error(`Output of filepath should be provided, but received ${output}.`);
      }
      validateOptions(options, {
        fsHz: 'int',
        fxMaxInternal: 'int',
        packetLength: 'int',
        rate: 'int',
        loss: { type: 'range', min: 0, max: 100 },
        complexity: { type: 'oneOf', oneOf: [0, 1, 2] },
      });
      silk.encode(input, output, options);
    } catch (e) {
      console.error(`[ERROR]: ${e.message}`);
    }
  });

cli.command('compare [ref] [test]', 'comapre two audio files')
  .option('-q, --quiet', 'Whether there is any output into console', { default: false })
  .option('--diff', 'Only determine bit-exactness', { default: false })
  .option('--fs', 'Sampling rate in Hz, max is 48000', { default: 24000 })
  .action((ref, test, options) => {
    try {
      if (typeof ref !== 'string') {
        throw new Error(`First input of filepath should be provided, but received ${ref}.`);
      }
      if (typeof test !== 'string') {
        throw new Error(`Second input of filepath should be provided, but received ${test}.`);
      }
      validateOptions(options, {
        fs: { type: 'int', max: 48000 },
      });
      silk.compare(ref, test, options);
    } catch (e) {
      console.error(`[ERROR]: ${e.message}`);
    }
  });

cli.help();

cli.version(version);

cli.parse();
