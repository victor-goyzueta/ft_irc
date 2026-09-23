*This project has been created as part of the 42 curriculum by vgoyzuet, jocalder.*

# ft_irc

## Description

**ft_irc** is a C++98 implementation of an IRC (Internet Relay Chat) server developed as part of the 42 curriculum.

The goal of the project is to build a functional IRC server capable of handling multiple clients simultaneously and communicating through the IRC protocol.

The server implements a non-blocking, event-driven architecture based on TCP sockets and `poll()`. It manages connected clients, channels, user registration, channel operators, permissions, invitations, topics, private messages, and channel modes.

The project was developed collaboratively by:

* [Jonathan Calderón](https://github.com/jocalder)
* [Víctor Goyzueta](https://github.com/victor-goyzueta)

### Main concepts

The project focuses on:

* TCP/IP socket programming
* Non-blocking I/O
* Event-driven programming with `poll()`
* IRC protocol parsing
* Client and connection management
* Channel management
* User authentication and registration
* Operator permissions
* IRC numeric replies and error handling
* C++98 object-oriented programming
* Dynamic memory management

## Features

### Server

* TCP server using IPv4 sockets
* Configurable port and server password
* Non-blocking sockets
* Multiple simultaneous client connections
* Event management using `poll()`
* Graceful shutdown with `SIGINT`

### Client registration

Clients can register using:

```text
PASS <password>
NICK <nickname>
USER <username> <mode> <unused> :realname
```

The server keeps track of authentication and registration state separately.

### IRC commands

The server currently handles the following commands:

* `PASS`
* `NICK`
* `USER`
* `JOIN`
* `PART`
* `QUIT`
* `TOPIC`
* `NAMES`
* `INVITE`
* `KICK`
* `PRIVMSG`
* `MODE`

### Channel management

Channels support:

* Channel creation through `JOIN`
* Multiple users
* Channel operators
* Topics
* Invitations
* Channel passwords
* User limits
* Operator-only operations
* Channel modes

The implemented channel modes include:

| Mode | Description             |
| ---- | ----------------------- |
| `i`  | Invite-only channel     |
| `t`  | Restrict topic changes  |
| `k`  | Channel password        |
| `o`  | Channel operator status |
| `l`  | User limit              |

## Architecture

The server is divided into three main entities:

```text
Server
├── manages sockets and the event loop
├── manages connected clients
├── manages channels
└── processes IRC commands

Client
├── stores connection information
├── stores registration information
├── maintains the incoming data buffer
└── tracks joined channels and invitations

Channel
├── stores connected clients
├── stores channel operators
├── manages channel modes
├── manages topic information
└── manages invitations
```

Incoming data is handled as a TCP byte stream. Since a single `recv()` call does not necessarily correspond to a complete IRC message, each client maintains an input buffer.

The general flow is:

```text
Client
  │
  │ TCP
  ▼
poll()
  │
  ▼
recv()
  │
  ▼
Client buffer
  │
  ├── incomplete message → wait for more data
  │
  └── complete message
          │
          ▼
    command parsing
          │
          ▼
    command handler
          │
          ▼
      IRC response
```

## Instructions

### Requirements

A Unix-like environment with:

* C++ compiler
* `make`
* POSIX socket APIs

The project is compiled using **C++98**.

### Compilation

Clone the repository:

```bash
git clone https://github.com/victor-goyzueta/ft_irc.git
cd ft_irc
```

Compile the server:

```bash
make
```

This generates the executable:

```text
ircserv
```

### Execution

Run the server with:

```bash
./ircserv <port> <password>
```

For example:

```bash
./ircserv 6667 mypassword
```

The port must be between `1024` and `65535`.

### Cleaning

Remove object files:

```bash
make clean
```

Remove object files and the executable:

```bash
make fclean
```

Rebuild the project:

```bash
make re
```

## Usage

The server can be tested using a standard IRC client.

For low-level testing and debugging, `nc` (Netcat) can also be used:

```bash
nc localhost 6667
```

After connecting, register the client:

```text
PASS mypassword
NICK victor
USER victor 0 * :Victor Goyzueta
```

Once registered, channels and IRC commands can be tested directly.

For example:

```text
JOIN #42
PRIVMSG #42 :Hello from ft_irc!
```

An IRC message uses `CRLF` (`\r\n`) as its line terminator.

## Project Structure

```text
ft_irc/
├── inc/
│   ├── Channel.hpp
│   ├── Client.hpp
│   ├── Server.hpp
│   └── Utils.hpp
├── src/
│   ├── Channel.cpp
│   ├── Client.cpp
│   ├── Server.cpp
│   └── Utils.cpp
├── main.cpp
├── Makefile
└── README.md
```

### `Server`

Responsible for:

* Creating and configuring the listening socket
* Accepting new connections
* Managing the `poll()` event loop
* Receiving client data
* Processing IRC commands
* Managing clients and channels
* Sending IRC responses

### `Client`

Responsible for storing:

* File descriptor
* Network address
* Nickname
* Username
* Real name
* Hostname
* Authentication state
* Registration state
* Joined channels
* Invitations
* Incoming message buffer

### `Channel`

Responsible for:

* Channel members
* Channel operators
* Channel modes
* Topic information
* Passwords
* User limits
* Invitations
* Broadcasting messages

### `Utils`

Contains common utilities used by the server, including:

* String trimming
* Uppercase conversion
* Parameter parsing
* Whitespace validation

## Resources

### IRC Protocol

* [RFC 2812 — Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)
* [RFC 2811 — Internet Relay Chat: Channel Management](https://www.rfc-editor.org/rfc/rfc2811)
* [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)

These RFCs were used as references for IRC message formatting, client registration, commands, numeric replies, channels, and channel modes.

## IRC Numeric Replies

The server uses IRC numeric replies to report successful operations and protocol errors to clients.

The following numeric replies are currently used by the server:

| Code  | Name                   | Description                                                         |
| ----- | ---------------------- | ------------------------------------------------------------------- |
| `001` | `RPL_WELCOME`          | Sent after successful client registration.                          |
| `002` | `RPL_YOURHOST`         | Provides information about the server host.                         |
| `003` | `RPL_CREATED`          | Indicates when the server was created.                              |
| `004` | `RPL_MYINFO`           | Provides server and supported protocol information.                 |
| `341` | `RPL_INVITING`         | Confirms that a user has been invited to a channel.                 |
| `353` | `RPL_NAMREPLY`         | Returns the list of users in a channel.                             |
| `366` | `RPL_ENDOFNAMES`       | Indicates the end of a `NAMES` response.                            |
| `324` | `RPL_CHANNELMODEIS`    | Returns the current modes of a channel.                             |
| `401` | `ERR_NOSUCHNICK`       | The specified nickname does not exist.                              |
| `403` | `ERR_NOSUCHCHANNEL`    | The specified channel does not exist.                               |
| `404` | `ERR_CANNOTSENDTOCHAN` | The client cannot send a message to the specified channel.          |
| `411` | `ERR_NORECIPIENT`      | No recipient was specified for a message.                           |
| `412` | `ERR_NOTEXTTOSEND`     | No message text was provided.                                       |
| `421` | `ERR_UNKNOWNCOMMAND`   | The server does not recognize the specified command.                |
| `431` | `ERR_NONICKNAMEGIVEN`  | No nickname was provided.                                           |
| `432` | `ERR_ERRONEUSNICKNAME` | The specified nickname is invalid.                                  |
| `433` | `ERR_NICKNAMEINUSE`    | The specified nickname is already in use.                           |
| `441` | `ERR_USERNOTINCHANNEL` | The specified user is not a member of the channel.                  |
| `442` | `ERR_NOTONCHANNEL`     | The client is not a member of the specified channel.                |
| `443` | `ERR_USERONCHANNEL`    | The specified user is already a member of the channel.              |
| `451` | `ERR_NOTREGISTERED`    | The client has not completed registration.                          |
| `461` | `ERR_NEEDMOREPARAMS`   | The command does not contain enough parameters.                     |
| `462` | `ERR_ALREADYREGISTRED` | The client is already registered and cannot register again.         |
| `464` | `ERR_PASSWDMISMATCH`   | The supplied server password is incorrect.                          |
| `471` | `ERR_CHANNELISFULL`    | The channel has reached its user limit.                             |
| `472` | `ERR_UNKNOWNMODE`      | The specified channel mode is not supported.                        |
| `473` | `ERR_INVITEONLYCHAN`   | The channel is invite-only and the client has not been invited.     |
| `475` | `ERR_BADCHANNELKEY`    | The supplied channel password is incorrect.                         |
| `482` | `ERR_CHANOPRIVSNEEDED` | The client does not have the required channel operator privileges.  |
| `502` | `ERR_USERSDONTMATCH`   | The requested operation cannot be performed on another user's mode. |

Some error messages also provide the expected command syntax to make manual testing and debugging easier. For example:

```text
461 KICK :Not enough parameters -> KICK <channel> <target> [:message]
461 MODE :Not enough parameters -> MODE <channel> [...]
```

The numeric replies follow the IRC protocol conventions defined by the RFC specifications listed in the [Resources](#resources) section.

### Networking

* [Linux `socket()` documentation](https://man7.org/linux/man-pages/man2/socket.2.html)
* [Linux `poll()` documentation](https://man7.org/linux/man-pages/man2/poll.2.html)
* [Linux `accept()` documentation](https://man7.org/linux/man-pages/man2/accept.2.html)
* [Linux `recv()` documentation](https://man7.org/linux/man-pages/man2/recv.2.html)
* [Linux `send()` documentation](https://man7.org/linux/man-pages/man2/send.2.html)
* [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

These resources were used to understand TCP sockets, non-blocking communication, `poll()`, connection handling, and network I/O.

### C++98

* [cppreference — C++](https://en.cppreference.com/)
* [C++ Reference](https://cplusplus.com/reference/)

These references were used for C++98 language features, STL containers, iterators, strings, and standard library functionality.

### AI usage

AI tools, primarily ChatGPT, were used as a learning and development support tool during the project.

They were used for:

* Understanding the IRC protocol and RFC terminology.
* Understanding TCP socket communication and the `poll()` event loop.
* Reviewing the interaction between `socket()`, `bind()`, `listen()`, `accept()`, `recv()` and `send()`.
* Understanding IRC message parsing, CRLF termination, numeric replies, and command parameters.
* Investigating edge cases and potential bugs.
* Reviewing implementation decisions and identifying areas requiring further testing.
* Clarifying C++98 concepts and standard library behaviour.
* Reviewing parts of the existing implementation to improve understanding of the code.

AI was used primarily for **technical explanations, debugging guidance, code review, and learning**, while the implementation and final code decisions were made by the project authors.
