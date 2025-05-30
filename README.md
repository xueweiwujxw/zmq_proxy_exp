## Zmq Proxy Example

### How to build

- Prepare the build system

  - [Download waf](https://waf.io/)
  - Install git-python

    ```bash
    pip install git-python
    ```

    > [!TIP]
    > Learn more about waf from the official documentation:
    > [`The waf Book`](https://waf.io/book/)

  - Install libzmq

    ```bash
    sudo apt install libzmq3-dev
    ```

- Build

  ```bash
   waf configure build
  ```

### How to run

After a successful build, you'll find three executables in the `build/dist/app` directory.

#### Directory Structure Preview

```log
$ dl --tree build
 build
├──  .lock-waf_linux_build
├──  .wafpickle-linux-50858736-20
├──  app
│   ├──  zmq_proxy.cc.2.o
│   ├──  zmq_recv.cc.4.o
│   └──  zmq_send.cc.3.o
├──  c4che
│   ├──  _cache.py
│   └──  build.config.py
├──  config.log
├──  dist
│   └──  app
│       ├──  zmq_proxy
│       ├──  zmq_recv
│       └──  zmq_send
├──  lib
│   └──  log
│       └──  log.c.1.o
├──  liblog.a
└── 󱧼 src
```

#### Execution Instructions:

- Run `./build/dist/app/zmq_proxy`

  This will creates a XSUB/XPUB proxy for ipc messages. It will subscribe all messages that send to `ipc:///tmp/zmq-proxy-exp_xsub` and publish them via `ipc:///tmp/zmq-proxy-exp_xpub`

- Run `./build/dist/app/zmq_recv`

  This will connect to `ipc:///tmp/zmq-proxy-exp_xpub` and receive messages. It will print filtered messages.

- Run `./build/dist/app/zmq_send <thread_num>`

  `<thread_num` is a value between 1 - 16. This will create several threads that bind to `ipc:///tmp/zmq-proxy-exp_pub_${thread_number_id}` and publish messages to `ipc:///tmp/zmq-proxy-exp_xsub`.

  At runtime:

  - Enter a positive number to send that many messages.
  - Enter a negative number or 0 to exit.

Messages will appear in the `zmq_recv` terminal.
