# diy-message-broker
A minimal message broker based on NATS

  ```mermaid
  
stateDiagram-v2
direction LR
  [*] --> OP_START
  OP_START --> OP_C: C/c
  OP_START --> OP_P: P/p
  OP_START --> OP_S: S/s
  OP_START --> OP_U: U/u

  OP_C --> OP_CO: O/o
  OP_CO --> OP_CON: N/n
  OP_CON --> OP_CONN: N/n
  OP_CONN --> OP_CONNE: E/e
  OP_CONNE --> OP_CONNEC: C/c
  OP_CONNEC --> OP_CONNECT: T/t
  OP_CONNECT --> OP_CONNECT_SPC: SPC/TAB
  OP_CONNECT_SPC --> CONNECT_ARG: {
  CONNECT_ARG --> CONNECT_ARG: else
  CONNECT_ARG --> [*]: \n

  OP_P --> OP_PI: I/i
  OP_PI --> OP_PIN: N/n
  OP_PIN --> OP_PING: G/g
  OP_PING --> OP_PING: \r
  OP_PING --> [*]: \n

  OP_P --> OP_PO: O/o
  OP_PO --> OP_PON: N/n
  OP_PON --> OP_PONG: G/g
  OP_PONG --> OP_PONG: \r
  OP_PONG --> [*]: \n

  OP_P --> OP_PU: U/u
  OP_PU --> OP_PUB: B/b
  OP_PUB --> OP_PUB_SPC: SPC/TAB
  OP_PUB_SPC --> PUB_ARG: [non-space]
  PUB_ARG --> PUB_ARG: else
  PUB_ARG --> MSG_PAYLOAD: \n
  MSG_PAYLOAD --> MSG_PAYLOAD: else
  MSG_PAYLOAD --> MSG_END_R: \r
  MSG_END_R --> MSG_END_N: \n
  MSG_END_N --> [*]

  OP_S --> OP_SU: U/u
  OP_SU --> OP_SUB: B/b
  OP_SUB --> OP_SUB_SPC: SPC/TAB
  OP_SUB_SPC --> SUB_ARG: [non-space]
  SUB_ARG --> SUB_ARG: else
  SUB_ARG --> [*]: \n

  OP_U --> OP_UN: N/n
  OP_UN --> OP_UNS: S/s
  OP_UNS --> OP_UNSU: U/u
  OP_UNSU --> OP_UNSUB: B/b
  OP_UNSUB --> OP_UNSUB_SPC: SPC/TAB
  OP_UNSUB_SPC --> UNSUB_ARG: [non-space]
  UNSUB_ARG --> UNSUB_ARG: else
  UNSUB_ARG --> [*]: \n


  ```
