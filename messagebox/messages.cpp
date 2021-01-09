#include <ESP_Mail_Client.h>

#include "config.h"
#include "messages.h"


static messageCallback callback = nullptr;
static IMAPSession imap;

void setupMessages() {
  imap.debug(0);
}


static void invokeCallback(String message, MessageError error) {
  callback(message, error);
  callback = nullptr;
  /* Close the session in case the session is still open */
  imap.closeSession();

  /* Clear all stored data in IMAPSession object */
  imap.empty();
  callback = nullptr;
  
}

static void imapCallback(IMAP_Status status) {
  if (!status.success()) {
    // Serial.println(status.info());
    return;
  }

  /* Get the message list from the message list data */
  IMAP_MSG_List msgList = imap.data();

  for (size_t i = 0; i < msgList.msgItems.size(); i++) {
      /* Iterate to get each message data through the message item data */
      IMAP_MSG_Item msg = msgList.msgItems[i];
      invokeCallback(msg.subject, MessageError::MESSAGES_OK);
      break;
  }
}


bool _initilializeIMAP(messageCallback messageCallback) {

  
  if (WiFi.status() != WL_CONNECTED) {
    messageCallback("WiFi not connected", MessageError::MESSAGES_NO_NETWORK);
    return false;
  }


  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = IMAP_HOST;
  session.server.port = IMAP_PORT;
  session.login.email = EMAIL;
  session.login.password = IMAP_PASSWORD;

  /* Setup the configuration for searching or fetching operation and its result */
  IMAP_Config config;

  /* Search criteria */
  config.search.criteria = "UID SEARCH UNSEEN";

  /* Also search the unseen message */
  config.search.unseen_msg = true;


  /** Set to download heades, text and html messaeges,
   * attachments and inline images respectively.
  */
  config.download.header = true;
  config.download.text = false;
  config.download.html = false;
  config.download.attachment = false;
  config.download.inlineImg = false;

  /** Set to enable the results i.e. html and text messaeges
   * which the content stored in the IMAPSession object is limited
   * by the option config.limit.msg_size.
   * The whole message can be download through config.download.text
   * or config.download.html which not depends on these enable options.
  */
  config.enable.html = false;
  config.enable.text = false;

  /* Set to enable the sort the result by message UID in the ascending order */
  config.enable.recent_sort = true;

  /* Set to report the download progress via the default serial port */
  config.enable.download_status = true;

  /* Set the limit of number of messages in the search results */
  config.limit.search = 1;

  /** Set the maximum size of message stored in
   * IMAPSession object in byte
  */
  config.limit.msg_size = 512;

  /** Set the maximum attachments and inline images files size
   * that can be downloaded in byte.
   * The file which its size is larger than this limit may be saved
   * as truncated file.
  */
  config.limit.attachment_size = 1024 * 1024 * 5;

  /* Connect to server with the session and config */
  bool connected = imap.connect(&session, &config);
  if (!connected) {
    invokeCallback("IMAP not connected", MessageError::MESSAGES_IMAP_CONNECTION_FAILED);
    return false;
  }


  /* Open or select the mailbox folder to read or search the message */
  bool selected = imap.selectFolder("MessageBox");
  if (!selected) {
    invokeCallback("Folder not found", MessageError::MESSAGES_FOLDER_NOT_FOUND);
    return false;
  }
}


bool getFirstUnreadMessage(messageCallback messageCallback) {

  if (callback != nullptr) {
    messageCallback("previous callback not finished", MessageError::MESSAGES_NO_NETWORK);
  }

  callback = messageCallback;
  imap.callback(imapCallback);

  if (!_initilializeIMAP(messageCallback)) {
    return false;
  }

  /** Read or search the Email and close the TCP session whne done
   * The second parameter is for close the session.
  */
  bool read = MailClient.readMail(&imap, true);
  if (!read) {
    callback("read Mail failed", MessageError::MESSAGES_NO_MESSAGE_FOUND);
    return false;
  }
}

