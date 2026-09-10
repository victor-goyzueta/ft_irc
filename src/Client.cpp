#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

Client::Client(int fd, struct sockaddr_in address)
	: _fd(fd),
	_address(address),
	_buffer(""),
	_nickname("*"),
	_username(""),
	_realname(""),
	_hostname(""),
	_authenticated(false),
	_registered(false),
	_disconnected(false)
{
	// std::string	message = "Wellcome\n";
	_hostname = inet_ntoa(_address.sin_addr);
	// if (_fd >= 0 && !message.empty())
	// 	send(_fd, message.c_str(), message.length(), 0);
}

Client::~Client()
{
	if (_fd >= 0)
		close(_fd);
}

int				Client::getFd() const {return _fd;}
std::string		Client::getNickName() const {return _nickname;}
std::string		Client::getUserName() const {return _username;}
std::string		Client::getRealName() const {return _realname;}
std::string		Client::getHostName() const {return _hostname;}
std::string		Client::getPrefix() const
{
	return ":" + _nickname + "!" + _username + "@" + _hostname;
}
bool			Client::isAuthenticated() const {return _authenticated;}
bool			Client::isRegistered() const {return _registered;}
bool			Client::isDisconnected() const {return _disconnected;}
std::string&	Client::getBuffer() {return _buffer;}
const std::vector<Channel*>	Client::getChannels() const {return _channels;}

void	Client::setNickName(std::string& nick) {_nickname = nick;}
void	Client::setUserName(const std::string& user) {_username = user;}
void	Client::setRealName(const std::string& real) {_realname = real;}
void	Client::setHostName(std::string& host) {_hostname = host;}
void	Client::setAuthenticated(bool authenticated) {_authenticated = authenticated;}
void	Client::setRegistered(bool registered) {_registered = registered;}
void	Client::setDisconnected(bool disconnected) {_disconnected = disconnected;}

std::string	Client::extractMessage()
{
	size_t	pos = _buffer.find('\n');
	if (pos == std::string::npos)
		return "";
	
	std::string	message = _buffer.substr(0, pos + 1);
	_buffer = _buffer.substr(pos + 1);
	size_t	len = message.length();

	if (!message.empty() && message[len - 1] == '\n')
	{
		if (len > 1 && message[len - 2] == '\r')
			message = message.substr(0, len - 2);
		else
			message = message.substr(0, len - 1);
	}
	return message;
}

void	Client::joinChannel(Channel* channel)
{
	_channels.push_back(channel);
}

void	Client::leaveChannel(Channel* channel)
{
	std::vector<Channel*>::iterator it = std::find(_channels.begin(),
		_channels.end(), channel);
	if (it != _channels.end())
		_channels.erase(it);
}

bool	Client::isInChannel(const std::string& name) const
{
	for (std::vector<Channel*>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if ((*it)->getName() == name)
			return true;
	}
	return false;
}

void	Client::addInvite(const std::string& channelName)
{
	if (!isInvitedTo(channelName))
		_invites.push_back(channelName);
}

bool	Client::isInvitedTo(const std::string& channelName) const
{
	for (std::vector<std::string>::const_iterator it = _invites.begin(); it != _invites.end(); ++it)
	{
		if (*it == channelName)
			return true;
	}
	return false;
}

void	Client::removeInvite(const std::string& channelName)
{
	std::vector<std::string>::iterator it = std::find(_invites.begin(),
		_invites.end(), channelName);
	if (it != _invites.end())
		_invites.erase(it);
}

void	Client::appendToBuffer(const std::string& data) {_buffer += data;}
void	Client::clearBuffer() {_buffer.clear();}
bool	Client::hasCompleteMessage() const
{
	return _buffer.find('\n') != std::string::npos;
}

void	Client::sendMessage(const std::string& message) const
{
	if (_fd >= 0 && !message.empty())
		send(_fd, message.c_str(), message.length(), 0);
}
