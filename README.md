<a href="https://www.bigclown.com/"><img src="https://bigclown.sirv.com/logo.png" width="200" height="59" alt="BigClown Logo" align="right"></a>

# Firmware Skeleton for BigClown wireless Denkovi 1 Wire 8 Relay Module

[![Travis](https://img.shields.io/travis/blavka/bcf-denkovi-1wire-relay/master.svg)](https://travis-ci.org/blavka/bcf-denkovi-1wire-relay)
[![Release](https://img.shields.io/github/release/blavka/bcf-denkovi-1wire-relay.svg)](https://github.com/blavka/bcf-denkovi-1wire-relay/releases)
[![License](https://img.shields.io/github/license/blavka/bcf-denkovi-1wire-relay.svg)](https://github.com/blavka/bcf-denkovi-1wire-relay/blob/master/LICENSE)
[![Twitter](https://img.shields.io/twitter/follow/BigClownLabs.svg?style=social&label=Follow)](https://twitter.com/BigClownLabs)

This repository contains firmware skeleton for remote control Denkovi 1 Wire 8 Relay Module.


## MQTT

q1 can be replace from range q1 .. q8

### Change state
```
bch pub node/{alias or id}/denkovi-relay/q1/state/set true
```
or
```
mosquitto_pub -t "node/aaaa/denkovi-relay/q1/state/set" -m true
```

response

```
node/{alias or id}/denkovi-relay/q1/state true
```

### Request state

```
bch pub node/{alias or id}/denkovi-relay/q1/state/get
```
or
```
mosquitto_pub -t "node/aaaa/denkovi-relay/q1/state/get" -n
```

response

```
node/{alias or id}/denkovi-relay/q1/state true
```



## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO s.r.o.**](https://www.hardwario.com/) in the heart of Europe.
