CREATE TABLE "conversations" (
	"id"	INTEGER,
	"room_id"	TEXT NOT NULL,
	"sender_id"	TEXT,
    "rtm_type" INTEGER,
	"message"	TEXT NOT NULL,
	CONSTRAINT "fk_msg_from" FOREIGN KEY("sender_id") REFERENCES "user"("username"),
	CONSTRAINT "fk_msg_group" FOREIGN KEY("room_id") REFERENCES "room"("name"),
	CONSTRAINT "pk_msg_id" PRIMARY KEY("id" AUTOINCREMENT)
);

CREATE TABLE "room" (
	"name"	TEXT,
	CONSTRAINT "pk_name" PRIMARY KEY("name")
);

CREATE TABLE "user" (
	"username"	TEXT,
	"password"	TEXT,
	CONSTRAINT "pk_username" PRIMARY KEY("username")
);

CREATE TABLE "room_users" (
	"room_id"	TEXT,
	"username"	TEXT,
	FOREIGN KEY("username") REFERENCES "user"("username"),
	FOREIGN KEY("room_id") REFERENCES "room"("name")
);