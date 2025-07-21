# diy-message-broker
This is a minimal pub/sub message broker which is inspired by [`NATS`](https://docs.nats.io/reference/reference-protocols). It uses a zero-allocation byte parser like the original NATS and also uses a Trie-based subscription store for efficient wildcard matching.
<br>

## Functionality

### Commands
You can execute these commands after connecting to the nat-server that you spin up on localhost 4222 (using: telnet localhost 4222).
<br> This is based on NATS, so you can refer to the [`Nats Demo`](https://docs.nats.io/reference/reference-protocols/nats-protocol-demo) to get an idea of how to use this. 
<br><br>The available commands are listed below:
| Functionality | Command | Description |
| ------------ | ------------ | ------------ |
| CONNECT | `CONNECT {}` | Confirms connection to the server and provides connection parameters. This is the first command expected. Currently, the only check that is made is that the args provided are a valid json, the connection params are actually not used. Ther server responds with a PING after this and expects a PONG in return.
| PING | `PING` | You can ping the server, it acts like a health check. If the server is up and running it will respond back with a PONG.
| PONG | `PONG` | This is expected from the client if the server responds with a PING. In this implementation, the server only pings the client right after CONNECT is sent and acknowledged.
| Publish Message | `PUB <subject/topic> <payloadSize>\r\n<payloadMessage>\r\n` | This is how you publish a message to a topic. "." is used to create heirarchies in topics. Topics are case sensitive.<br> Examples of valid topics for publish : "foo.bar", "Organization.TechTeam.Leads", "severance.season3.updates", etc. <br>Example of publish command : "PUB foo.bar 5\r\nHello\r\n".
| Subscribe to a subject/topic | `SUB <subject/topic> <intSubscriptionId>\r\n` | This is how you subscribe to a topic. "." is used to create heirarchies in topics. Topics are case sensitive.<br>In the case of subscribe, wildcard characters can also be used. "\*" is for matching a single token and ">" is used for matching multiple tokens (and hence has to be the last character if used). <br>So, for example if you subscribe to "foo.\*" using "SUB foo.\* 10", if someone publishes to "foo.bar" you will get the message but if someone publishes to "foo.bar.test" you won't get the message. Now, if you subscribe to "foo.>", you will get the same message. For more info on subjects refer to the [`official NATS Documentation on subjects`](https://docs.nats.io/nats-concepts/subjects)
| Unsubscribe to a topic | `UNSUB <intSubscriptionId>\r\n` | This is used to unsubscribe to a topic that your previously have subscribed to. Let's say you subscirbed to "foo.bar" with subscription ID "10", then you would use "UNSUB 10\r\n" to unsubscribe to that topic. This only unsubscribes to the particular subscription ID, you could be subscribed to the same topic using a different subscription ID, that subscription would still remain untouched.

### Example Flow

This is an example of how the flow could be like when using the nats-server:

## Technical Architecture  
<br>
 
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

## Issues or bugs in the tool? Want to add a new functionality?
Contributions are always welcome. You could open up an issue if you feel like something is wrong with the tool or a PR if you just want to improve it.
