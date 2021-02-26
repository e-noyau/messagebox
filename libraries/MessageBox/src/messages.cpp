
#include <ESP_Mail_Client.h>

#include "messages.h"

// This IMAP API is a pain in the but.
static IMAPFetcher *fetcher = nullptr;
static void imapCallbackKludge(IMAP_Status status) {
  if (fetcher) fetcher->imapCallback(status);
}

void IMAPFetcher::invokeCallback(const std::string &message,
                                 const std::string &author, 
                                 MessageError error) {
  _callback(message, author, error);
  _callback = nullptr;
  /* Close the session in case the session is still open */
  _imap.closeSession();

  /* Clear all stored data in IMAPSession object */
  _imap.empty(); 
}


void IMAPFetcher::imapCallback(IMAP_Status status) {
  if (!status.success()) {
    Serial.println(status.info());
    return;
  }

  fetcher = nullptr;
  /* Get the message list from the message list data */
  IMAP_MSG_List msgList = _imap.data();

  if (msgList.msgItems.size() == 0) {
    invokeCallback("Sorry, all the messages have been consumed. Ask your loved ones for more!",
                   "", MessageError::MESSAGES_OK);
  } else {
    for (size_t i = 0; i < msgList.msgItems.size(); i++) {
      IMAP_MSG_Item msg = msgList.msgItems[i];
      std::string from(msg.from);
      size_t index = from.find(' ', 3);
      if (index != std::string::npos) {
        from.erase(index);
      }

      if (!MailClient.setFlag(&_imap, atoi(msg.UID), "\\Seen", false)) {
        invokeCallback("Flag setting failure", "", MessageError::MESSAGES_FLAG_SET_FAIL);
      } else {
        invokeCallback(msg.subject, from, MessageError::MESSAGES_OK);        
      }
      break;
    }
  }
}


bool IMAPFetcher::initilializeIMAP() {
  if (WiFi.status() != WL_CONNECTED) {
    invokeCallback("WiFi not connected", "", MessageError::MESSAGES_NO_NETWORK);
    return false;
  }

  // Set the session config
  _session.server.host_name = _connection_data.host.c_str();
  _session.server.port = _connection_data.port;
  _session.login.email = _connection_data.email.c_str();
  _session.login.password = _connection_data.password.c_str();

  // Search criteria
  _config.search.criteria = "UID SEARCH UNSEEN";

  // Also search the unseen message
  _config.search.unseen_msg = true;

  // Download only headers.
  _config.download.header = true;
  _config.download.text = false;
  _config.download.html = false;
  _config.download.attachment = false;
  _config.download.inlineImg = false;

  /** Set to enable the results i.e. html and text messaeges
   * which the content stored in the IMAPSession object is limited
   * by the option config.limit.msg_size.
   * The whole message can be download through config.download.text
   * or config.download.html which not depends on these enable options.
  */
  _config.enable.html = false;
  _config.enable.text = false;

  // Report the result by message UID in the ascending order
  _config.enable.recent_sort = true;

  // Report the download progress via the default serial port
  _config.enable.download_status = false;

  // Set the limit of number of messages in the search results
  _config.limit.search = 1;

  /** Set the maximum size of message stored in
   * IMAPSession object in byte
  */
  _config.limit.msg_size = 512;

  /** Set the maximum attachments and inline images files size
   * that can be downloaded in byte.
   * The file which its size is larger than this limit may be saved
   * as truncated file.
  */
  _config.limit.attachment_size = 1024 * 1024 * 5;

  /* Connect to server with the session and config */
  bool connected = _imap.connect(&_session, &_config);
  if (!connected) {
    invokeCallback("IMAP not connected", "", MessageError::MESSAGES_IMAP_CONNECTION_FAILED);
    return false;
  }

  /* Open or select the mailbox folder to read or search the message */
  bool selected = _imap.selectFolder(_connection_data.folder.c_str(), /*readonly=*/false);
  if (!selected) {
    invokeCallback("Folder not found", "", MessageError::MESSAGES_FOLDER_NOT_FOUND);
    return false;
  }
}


bool IMAPFetcher::getFirstUnreadMessage(messageCallback messageCallback) {

  if (_callback != nullptr) {
    messageCallback("previous callback not finished", "",
                    MessageError::MESSAGES_CONNECTION_IN_PROGRESS);
  }

  _callback = messageCallback;
  fetcher = this;
  _imap.callback(&imapCallbackKludge);

  if (!initilializeIMAP()) {
    return false;
  }

  /** Read or search the Email and close the TCP session whne done
   * The second parameter is for close the session.
  */
  bool read = MailClient.readMail(&_imap, false);
  if (!read) {
    _callback("read Mail failed", "", MessageError::MESSAGES_NO_MESSAGE_FOUND);
    return false;
  }
}

