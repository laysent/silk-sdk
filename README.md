# Silk SDK

This is node-gyp version of Silk SDK library (see <https://developer.skype.com/silk> for details.)

## API Usage

### decode(input, output, options)

Decode silk format audio.

| Parameter | Type   | Required | Description                             |
| --------- | ------ | -------- | --------------------------------------- |
| input     | string | true     | Bitstream input to decoder              |
| output    | string | true     | Speech output from decoder              |
| options   | object | false    | Optional options, see below for details |

Below are possible options for `decode`.

| Attribute | Type    | Default | Description                              |
| --------- | ------- | --------| ---------------------------------------- |
| quiet     | boolean | true    | Whether there is any output into console |
| lossProb  | float   | 0       | Simulated packet loss percentage (0-100) |
| fsHz      | int     | 24000   | Sampling rate of output signal in Hz     |

Example usage:

```javascript
const silk = require('silk');
silk.decode('./input.silk', './output.pcm', { quiet: true });
```

### encode(input, output, options)

Encode audio to silk format.

| Parameter | Type   | Required | Description                             |
| --------- | ------ | -------- | --------------------------------------- |
| input     | string | true     | Speech input to encoder                 |
| output    | string | true     | Bitstream output from encoder           |
| options   | object | false    | Optional options, see below for details |

Below are possible options for `encode`.

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

Example usage:

```javascript
const silk = require('silk');
silk.encode('./input.pcm', './output.silk', { quiet: true });
```

### compare(input, output, options)

Compare two audio files.

| Parameter | Type   | Required | Description                                                 |
| --------- | ------ | -------- | ----------------------------------------------------------- |
| inputA    | string | true     | Reference file                                              |
| inputB    | string | true     | File to be tested, should be of same length as inputA (pcm) |
| options   | object | false    | Optional options, see below for details                     |

Below are possible options for `compare`.

| Attribute     | Type    | Default | Description                              |
| ------------- | ------- | --------| ---------------------------------------- |
| quiet         | boolean | true    | Whether there is any output into console |
| diff          | boolean | false   | Only determine bit-exactness             |
| fs            | int     | 24000   | Sampling rate in Hz, max is 48000        |

Example usage:

```javascript
const silk = require('silk');
silk.compare('./ref.pcm', './test.pcm', { quiet: true });
```

## License

This wrap of original Silk SDK code is under MIT License.
For Silk SDK itself, please check their license for details.
