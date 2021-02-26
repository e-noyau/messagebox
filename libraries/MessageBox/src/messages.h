#pragma once
#include <string>
#include <functional>

#include <ESP_Mail_Client.h>

enum MessageError {
	MESSAGES_OK = 0,
	MESSAGES_NO_NETWORK,
	MESSAGES_IMAP_CONNECTION_FAILED,
	MESSAGES_FOLDER_NOT_FOUND,
	MESSAGES_NO_MESSAGE_FOUND,
  MESSAGES_FLAG_SET_FAIL,
  MESSAGES_CONNECTION_IN_PROGRESS
} ;

struct IMAPConnectionData {
  std::string host;
  int port;
  std::string email;
  std::string password;
  std::string folder;
};

typedef std::function<void(const std::string &message,
                           const std::string &author,
                           MessageError error)> messageCallback;

class IMAPFetcher {
public:
  IMAPFetcher(IMAPConnectionData &connection_data) : _connection_data(connection_data){
    _imap.debug(0);
  };
  ~IMAPFetcher() {};
  
  bool getFirstUnreadMessage(messageCallback callback);
  
  // kludge
  void imapCallback(IMAP_Status status);
private:
  bool initilializeIMAP();
  void invokeCallback(const std::string &message,
                      const std::string &author, 
                      MessageError error);
                      
  IMAPConnectionData _connection_data;
  IMAPSession _imap;
  ESP_Mail_Session _session;
  IMAP_Config _config;
  messageCallback _callback = nullptr;
};

