# pr_chatserver

## Architecture
The server contains 2 main processes:
- The key listener process, that waits for a CTRL+D
- The server process, which contains 2 main sockets:
  - The server_fd socket, which receives messages and handles actions.
  - The send_fd socket, which sends messages.
  - This process creates an underlying socket for each ingoing socket

# Tech Stack
Librairies:
- Logger: https://github.com/rxi/log.c

## To-do list
- Envoi de messages par sockets X
- Envoi d'images (png, gif, stickers)
- Envoi de sondages
- Encryption de bout en bout
- Créer des groupes d'utilisateur
- Mettre des réactions
- Upload de fichiers (max 1MB)
- Quand on se connecte, afficher le nom des personnes connectées X
- transformer les url en qr-code