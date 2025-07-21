# diy-message-broker
This is a minimal pub/sub message broker which is inspired by [`NATS`](https://docs.nats.io/reference/reference-protocols).<br> It supports multiple concurrent client connections, uses a zero-allocation byte parser like the original NATS and also uses a Trie-based subscription store for efficient wildcard matching.
<br>

## Functionality

### Commands
You can execute these commands after connecting to the nat-server that you spin up on localhost 4222 (using: telnet localhost 4222).
<br> This is based on NATS, so you can refer to the [`Nats Demo`](https://docs.nats.io/reference/reference-protocols/nats-protocol-demo) to get an idea of how to use this. 
<br><br>The available commands are listed below:
| Functionality | Command | Description |
| ------------ | ------------ | ------------ |
| Connect | `CONNECT {}` | Confirms connection to the server and provides connection parameters. This is the first command expected. Currently, the only check that is made is that the args provided are a valid json, the connection params are actually not used. Ther server responds with a PING after this and expects a PONG in return.
| Ping | `PING` | You can ping the server, it acts like a health check. If the server is up and running it will respond back with a PONG.
| Pong | `PONG` | This is expected from the client if the server responds with a PING. In this implementation, the server only pings the client right after CONNECT is sent and acknowledged.
| Publish Message | `PUB <subject/topic> <payloadSize>\r\n<payloadMessage>\r\n` | This is how you publish a message to a topic. "." is used to create heirarchies in topics. Topics are case sensitive.<br> Examples of valid topics for publish : "foo.bar", "Organization.TechTeam.Leads", "severance.season3.updates", etc. <br>Example of publish command : "PUB foo.bar 5\r\nHello\r\n".
| Subscribe to a subject/topic | `SUB <subject/topic> <intSubscriptionId>\r\n` | This is how you subscribe to a topic. "." is used to create heirarchies in topics. Topics are case sensitive.<br>In the case of subscribe, wildcard characters can also be used. "\*" is for matching a single token and ">" is used for matching multiple tokens (and hence has to be the last character if used). <br>So, for example if you subscribe to "foo.\*" using "SUB foo.\* 10", if someone publishes to "foo.bar" you will get the message but if someone publishes to "foo.bar.test" you won't get the message. Now, if you subscribe to "foo.>", you will get the same message. For more info on subjects refer to the [`official NATS Documentation on subjects`](https://docs.nats.io/nats-concepts/subjects)
| Unsubscribe to a topic | `UNSUB <intSubscriptionId>\r\n` | This is used to unsubscribe to a topic that your previously have subscribed to. Let's say you subscirbed to "foo.bar" with subscription ID "10", then you would use "UNSUB 10\r\n" to unsubscribe to that topic. This only unsubscribes to the particular subscription ID, you could be subscribed to the same topic using a different subscription ID, that subscription would still remain untouched.

### Example Flow

This is an example of how the flow could be like when using the nats-server:

```
Trying ::1...
Connected to localhost.
Escape character is '^]'.
INFO {"server_id":4208158381076342694,"server_name":"nats-message-broker","version":"1.0.0","client_id":8890077863449949768,"client_ip":"172.17.0.1","host_ip":"0.0.0.0","host_port":4222}
CONNECT {}
+OK
PING
PONG
SUB foo.*.test 10
+OK
PUB foo.abc.test 12
Hello World!
+OK
MSG foo.abc.test 10 12
Hello World!
UNSUB 10
+OK
```

## Steps to run the server

### Using Docker

The Docker build uses makefile internally but is simpler to run. The follwoing commands need to be executed in the project root directory:

`docker build -t nats-broker .` - This is used to build the image. This also runs the unit and integration test cases and fails the build if they don't pass.
<br>`docker run -p 4222:4222 nats-broker` - This is used to spin up the server on 4222 (you can change the host port to whatever you like).

Once the server is up and running, you can connect to it using `telnet localhost 4222`

### Using make

If you are using make, make sure these are available in your system:
build-essential, nlohmann-json3-dev, GoogleTest and GoogleMock
<br><br>
If not they can be installed using the following command:
```
    sudo apt-get update && 
    sudo apt-get install -y build-essential nlohmann-json3-dev libgtest-dev libgmock-dev cmake make
    rm -rf /var/lib/apt/lists/*
```
<br>

You also need to build the source file for GoogleTest and GoogleMock (they are source-only packages)
```
    cd /usr/src/gtest && cmake . && make && cp lib/*.a /usr/lib/
```

Then you can use the following commands in the project root directory:

`make clean` - To clear the build directory
<br>`make test` - To build the executable for the unit and integration test cases. This will be generated in "build/test_nats". You can then run `./build/test_nats` to run the test cases.
<br>`make all` - To build the actual executable for the nats-broker. This will be generated in "build/nats". You can then execute `./build/nats` to run the server.

Once the server is up and running, you can connect to it using `telnet localhost 4222`

## Technical Architecture  

Once the server is spun up, it uses one thread per client and mutex locks on common objects to make sure multiple clients can connect to the server at once. <br>

### Zero-Allocation Byte Parser using FSM
One of the first things that happen when a user types a command is that command goes through the Parser. The parser is designed to not allocate any extra memory during parsing which reduces the burden on the memory allocator and garbage collector. The orignal NATS parser is also designed in a similar way because performance matters in a large scale message broker. This zero-allocation byte parsing is achieved by using string_views rather than strings, performing copies of data only when necessary and by using a Finate State Machine (FSM) that goes byte by byte and checks for the ParserState and validity of operation.
<br><br> The FSM diagram below shows how the parsing is performed.
<br>
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
<sub><i>Diagram generated using <a href="https://mermaid.js.org/">Mermaid.js</a></i></sub>

### Trie-like subscription store (called Sublist)

To store the subscriptions a Trie-like data structure is used. Each node is a sub-heirarchy in the topic. When there is a subsription, the trie is parsed level by level and if a node doesn't exist it's created. In the last sub-heirarchy, the subscription is stored ( A structure with client_id and subscription_id). We can take the below diagram as an example representation.

```mermaid
%%{init: {'themeVariables': {'fontSize': '10px'}}}%%
flowchart
direction LR
ROOT((ROOT))
ROOT --> FOO[foo]
FOO --> BAR[bar]
BAR --> TEST[["test<br/>(1, 10), (2, 11)"]]
FOO --> STAR["\*"]
STAR --> NEW[["new<br/>(3, 12)"]]
FOO --> GT[["><br/>(4, 13)"]]
ROOT --> WEATHER[weather]
WEATHER --> INDIA[India]
INDIA --> Bangalore[["Bangalore<br/>(5, 20)"]]
ROOT --> STOCK[stock]
STOCK --> NYSE[["NYSE<br/>(6, 30)"]]
``` 
<sub><i>Diagram generated using <a href="https://mermaid.js.org/">Mermaid.js</a></i></sub>

Let's say we get a request from a client to publish a message to foo.bar.new. We will do a Breadth First Search (BFS). We will go level-by-level. 
<br>
First we go to "foo". 
<br>Then, both "\*" as well as ">" match. Since ">" is a terminal character, we consider the subcription in that node. 
<br>Then from "\*", we match with "new", so we consider that subscription as well.
<br>So ultimately, we get the following pairs of (client_id, sub_id) : (4,13) and (3,12). The server then contacts the respective client and sends the message.

## Issues or bugs in the tool? Want to add a new functionality?
Contributions are always welcome. You could open up an issue if you feel like something is wrong with the tool or a PR if you just want to improve it.
