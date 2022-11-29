# pr_chatserver

## Architecture
The server contains 2 main processes:
- The key listener process, that waits for a CTRL+D
- The server process, which contains 2 main sockets:
  - The server_fd socket, which receives messages and handles actions.
  - The send_fd socket, which sends messages.
  - This process creates an underlying socket for each ingoing socket

# Tech Stack
Libraries:
- Logger: https://github.com/rxi/log.c

## TODO
- Send images
- Send polls
- Encrypt end to end
- Add a reaction on a message
- Upload files (max 1MB)
- Send url as qr-codes