# Silk SDK

This is node-gyp version of Silk SDK library (see <https://developer.skype.com/silk> for details.)

## Installation

To install this package, please run:

```bash
npm install silk-sdk
```

or

```bash
yarn add silk-sdk
```

If you wish to use cli tool, try install globally using:

```bash
npm install silk-sdk --global
```

or

```bash
yarn global add silk-sdk
```

## Command Usage

### decode

To decode silk format audio, run:

```bash
silk-sdk decode [input] [output]
```

where `input` is the name of input file and `output` is the name of output file.
You can pass further options after, availables are shown below:

  + -q, --quiet            Whether there is any output into console (default: false)
  + --lossProb [lossProb]  Simulated packet loss percentage (0-100) (default: 0)
  + --fsHz [fs]            Sampling rate of output signal in Hz (default: 24000)

### encode

To encode audio to silk format, run:

```bash
silk-sdk encode [input] [output]
```

where `input` is the name of input file and `output` is the name of output file.
You can pass further options after, availables are shown below:

  + -q, --quiet                      Whether there is any output into console (default: false)
  + --fsHz [fs]                      API sampling rate in Hz (default: 24000)
  + --fxMaxInternal [fxMaxInternal]  Maximum internal sampling rate in Hz (default: 0)
  + --packetLength [packageLength]   Packet interval in ms (default: 20)
  + --rate                           Target bitrate (default: 25000)
  + --loss                           Uplink loss estimate, in percent (0-100) (default: 0)
  + --complexity                     Set complexity, 0: low, 1: medium, 2: high (default: 2)
  + --inbandFEC                      Enable inband FEC usage (default: false)
  + --dtx                            Enable DTX (default: false)
  + --tencent                        Add Tencent (Wechat, QQ) header in exported file (default: false)
  + --tencentAmr                     Add Tencent AMR header in exported file (default: false)

### compare

To compare to silk format audio files, run:

```bash
silk-sdk compare [ref] [test]
```

where `ref` is the name of first file and `test` is the name of second file.
You can pass further options after, availables are shown below:

  + -q, --quiet    Whether there is any output into console (default: false)
  + --diff         Only determine bit-exactness (default: false)
  + --fs           Sampling rate in Hz, max is 48000 (default: 24000)

## API Usage

### decode

This API will decode silk format audio. It's return value and behavior will change depends on the
parameters provided.

#### Stream.Transform decode(options)

| Parameter | Type   | Required | Description                             |
| --------- | ------ | -------- | --------------------------------------- |
| options   | object | false    | Optional options, see below for details |

This API will return a transform stream, which can be used to consume input stream and generate
decode stream.

Exmaple usage:

```javascript
fs.createReadStream('input-path-here')
  .pipe(silk.decode({ quiet: true }))
  .pipe(fs.createWriteStream('output-path-here'));
```

#### buffer decode(input, options)

| Parameter | Type          | Required | Description                             |
| --------- | ------------- | -------- | --------------------------------------- |
| input     | string/buffer | true     | Bitstream input to decoder              |
| options   | object        | false    | Optional options, see below for details |

For first parameter `input`, you can pass in either `string` or `buffer`. If `string` is provided,
it will be used as path of file; if `buffer` is provided, it should be the buffer of input file.

This API will return a decoded buffer that can be either saved in file or used by other APIs.

Example usage:

```javascript
fs.writeFileSync(silk.decode('input-path-here', { quiet: true }));
```

#### void decode(input, output, options)

| Parameter | Type          | Required | Description                             |
| --------- | ------------- | -------- | --------------------------------------- |
| input     | string/buffer | true     | Bitstream input to decoder              |
| output    | string        | true     | Speech output (file path) from decoder  |
| options   | object        | false    | Optional options, see below for details |

For first parameter `input`, you can pass in either `string` or `buffer`. If `string` is provided,
it will be used as path of file; if `buffer` is provided, it should be the buffer of input file.

No return for this API, decode result will be saved to output path directly.

Example usage:

```javascript
silk.decode('./input.silk', './output.pcm', { quiet: true });
```

#### Options

| Attribute | Type    | Default | Description                              |
| --------- | ------- | --------| ---------------------------------------- |
| quiet     | boolean | true    | Whether there is any output into console |
| lossProb  | float   | 0       | Simulated packet loss percentage (0-100) |
| fsHz      | int     | 24000   | Sampling rate of output signal in Hz     |


### encode

This API will encode speech input to silk format. It's return value and behavior will change depends
on the parameters provided.

#### Stream.Transform encode(options)

| Parameter | Type   | Required | Description                             |
| --------- | ------ | -------- | --------------------------------------- |
| options   | object | false    | Optional options, see below for details |

This API will return a transform stream, which can be used to consume input stream and generate
decode stream.

Exmaple usage:

```javascript
fs.createReadStream('input-path-here')
  .pipe(silk.encode({ quiet: true }))
  .pipe(fs.createWriteStream('output-path-here'));
```

#### buffer encode(input, options)

| Parameter | Type          | Required | Description                             |
| --------- | ------------- | -------- | --------------------------------------- |
| input     | string/buffer | true     | Speech input to encoder                 |
| options   | object        | false    | Optional options, see below for details |

For first parameter `input`, you can pass in either `string` or `buffer`. If `string` is provided,
it will be used as path of file; if `buffer` is provided, it should be the buffer of input file.

This API will return a encoded buffer that can be either saved in file or used by other APIs.

Example usage:

```javascript
fs.writeFileSync(silk.encode('input-path-here', { quiet: true }));
```

#### void decode(input, output, options)

| Parameter | Type          | Required | Description                             |
| --------- | ------------- | -------- | --------------------------------------- |
| input     | string/buffer | true     | Speech input to encoder                 |
| output    | string        | true     | Bitstream output from encoder           |
| options   | object        | false    | Optional options, see below for details |

For first parameter `input`, you can pass in either `string` or `buffer`. If `string` is provided,
it will be used as path of file; if `buffer` is provided, it should be the buffer of input file.

No return for this API, encode result will be saved to output path directly.

Example usage:

```javascript
silk.encode('./input.silk', './output.pcm', { quiet: true });
```

#### Options

| Attribute     | Type    | Default | Description                                      |
| ------------- | ------- | --------| ------------------------------------------------ |
| quiet         | boolean | true    | Whether there is any output into console         |
| fsHz          | int     | 24000   | API sampling rate in Hz                          |
| fxMaxInternal | int     | 0       | Maximum internal sampling rate in Hz             |
| packetLength  | int     | 20      | Packet interval in ms                            |
| rate          | int     | 25000   | Target bitrate                                   |
| loss          | int     | 0       | Uplink loss estimate, in percent (0-100)         |
| complexity    | int     | 2       | Set complexity, 0: low, 1: medium, 2: high       |
| inbandFEC     | boolean | false   | Enable inband FEC usage                          |
| dtx           | boolean | false   | Enable DTX                                       |
| tencent       | boolean | false   | Add Tencent (Wechat, QQ) header in exported file |
| tencentAmr    | boolean | false   | Add Tencent AMR header in exported file          |

### compare

#### bool compare(input, output, stream)

Compare two audio files.

| Parameter | Type          | Required | Description                                                 |
| --------- | ------------- | -------- | ----------------------------------------------------------- |
| inputA    | string/buffer | true     | Reference file                                              |
| inputB    | string/buffer | true     | File to be tested, should be of same length as inputA (pcm) |
| options   | object        | false    | Optional options, see below for details                     |

For both `inputA` and `inputB`, you can pass in either `string` or `buffer`. If `string` is provided,
it will be used as path of file; if `buffer` is provided, it should be the buffer of input file.

Below are possible options for `compare`.

| Attribute     | Type    | Default | Description                              |
| ------------- | ------- | --------| ---------------------------------------- |
| quiet         | boolean | true    | Whether there is any output into console |
| diff          | boolean | false   | Only determine bit-exactness             |
| fs            | int     | 24000   | Sampling rate in Hz, max is 48000        |

Example usage:

```javascript
const silk = require('silk-sdk');
silk.compare('./ref.pcm', './test.pcm', { quiet: true });
```

## License

This wrap of original Silk SDK code is under MIT License.
For Silk SDK itself, please check their license for details.
