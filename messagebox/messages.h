
typedef enum {
	MESSAGES_OK = 0,
	MESSAGES_NO_NETWORK,
	MESSAGES_IMAP_CONNECTION_FAILED,
	MESSAGES_FOLDER_NOT_FOUND,
	MESSAGES_NO_MESSAGE_FOUND,
} MessageError;

typedef void (*messageCallback)(String message, MessageError error);

void setupMessages();
bool getFirstUnreadMessage(messageCallback callback);

