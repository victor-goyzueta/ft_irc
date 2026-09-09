#include "../inc/Channel.hpp"
#include "../inc/Client.hpp"

Channel::Channel(const std::string& name)
	: _name(name),
	_topic(""),
	_topicSetter(""),
	_topicTime(0),
	_password(""),
	_userLimit(0)
{
	_modes.insert('t');
}

Channel::~Channel() {}

std::string					Channel::getName() const {return _name;}
std::string					Channel::getTopic() const {return _topic;}
std::string					Channel::getTopicSetter() const {return _topicSetter;}
time_t						Channel::getTopicTime() const {return _topicTime;}
std::string					Channel::getPassword() const {return _password;}
size_t						Channel::getUserLimit() const {return _userLimit;}
size_t						Channel::getClientCount() const {return _clients.size();}
const std::vector<Client*>& Channel::getClients() const {return _clients;}

std::string		Channel::getModeString() const
{
	std::string	result = "+";
	for (std::set<char>::const_iterator it = _modes.begin(); it != _modes.end(); ++it)
		result += *it;
	return result;
}

std::string		Channel::getModeParams() const
{
	std::string	result;
	if (hasMode('k'))
		result += " " + _password;
	if (hasMode('l'))
	{
		std::stringstream	ss;
		ss << _userLimit;
		result += " " + ss.str();
	}
	return result;
}

void	Channel::setTopic(const std::string& topic, const std::string& setter)
{
	_topic = topic;
	_topicSetter = setter;
	_topicTime = time(NULL);
}

void	Channel::setPassword(const std::string& pass) {_password = pass;}
void	Channel::setUserLimit(size_t limit) {_userLimit = limit;}
void	Channel::removeUserLimit() {_userLimit = 0;}

bool	Channel::hasMode(char mode) const
{
	return _modes.find(mode) != _modes.end();
}

void	Channel::addMode(char mode) {_modes.insert(mode);}
void	Channel::removeMode(char mode) {_modes.erase(mode);}
void	Channel::addClient(Client* client) {_clients.push_back(client);}

void	Channel::removeClient(Client* client)
{
	std::vector<Client*>::iterator it = std::find(_clients.begin(), _clients.end(), client);
	if (it != _clients.end())
		_clients.erase(it);
	removeOperator(client);
}

bool	Channel::hasClient(Client* client) const
{
	return std::find(_clients.begin(), _clients.end(), client) != _clients.end();
}

bool	Channel::hasClient(const std::string& nick) const
{
	for (std::vector<Client*>::const_iterator it = _clients.begin();
		it != _clients.end(); ++it)
	{
		if ((*it)->getNickName() == nick)
			return true;
	}
	return false;
}

bool	Channel::isEmpty() const {return _clients.empty();}

void	Channel::addOperator(Client* client)
{
	if (!isOperator(client))
		_operators.push_back(client);
}

void	Channel::removeOperator(Client* client)
{
	std::vector<Client*>::iterator it = std::find(_operators.begin(), _operators.end(), client);
	if (it != _operators.end())
		_operators.erase(it);
}

bool	Channel::isOperator(Client* client) const
{
	return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

bool	Channel::isOperator(const std::string& nick) const
{
	for (std::vector<Client*>::const_iterator it = _operators.begin();
		it != _operators.end(); ++it)
	{
		if ((*it)->getNickName() == nick)
			return true;
	}
	return false;
}

void	Channel::addInvitedUser(const std::string& nick)
{
	if (!isInvited(nick))
		_invitedUsers.push_back(nick);
}

void	Channel::removeInvitedUser(const std::string& nick)
{
	std::vector<std::string>::iterator it = std::find(_invitedUsers.begin(),
		_invitedUsers.end(), nick);
	if (it != _invitedUsers.end())
		_invitedUsers.erase(it);
}

bool	Channel::isInvited(const std::string& nick) const
{
	return std::find(_invitedUsers.begin(), _invitedUsers.end(), nick) != _invitedUsers.end();
}

void	Channel::broadcast(const std::string& message, Client* exclude) const
{
	for (std::vector<Client*>::const_iterator it = _clients.begin();
		it != _clients.end(); ++it)
	{
		if (*it != exclude)
			(*it)->sendMessage(message);
	}
}
