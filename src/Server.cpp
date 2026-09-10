#include "../inc/Server.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Client.hpp"
#include "../inc/Utils.hpp"

static bool	g_running = true;

void	signalHandler(int signum)
{
	(void)signum;
	g_running = false;
}

void	Server::setupSocket()
{
	//	Create a socket for comunication TCP using IPv.4,
	//		the system selects the appropriate protocol.
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0)
	{
		std::cerr << "Error: Server socket creation failed" << std::endl;
		exit(1);
	}

	//	Modify a socket's setting's option allowing using a local address
	//		that may still be temporarily associated with the previous socket.
	int	opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "Error: setsockopt failed" << std::endl;
		close(_serverSocket);
		exit(1);
	}

	//	Set up socket as non-blocking to manage many clients from a single process.
	if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) < 0)
	{
		std::cerr << "Error: fcntl failed" << std::endl;
		close(_serverSocket);
		exit(1);
	}

	//	This structure contains the server's IPv4 address.
	struct sockaddr_in	serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(_port);

	//	Associate the socket, IP, and port to obtain the server's local address.
	if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		std::cerr << "Error: bind failed on port " << _port << "." << std::endl;
		close(_serverSocket);
		exit(1);
	}

	//	Turn socket into a listening socket.
	if (listen(_serverSocket, SOMAXCONN) < 0)
	{
		std::cerr << "Error: listen failed." << std::endl;
		close(_serverSocket);
		exit(1);
	}
	std::cout << "Server listening on port: " << _port << "." << std::endl;
	
	//	This structure contains the fds and events for their respective sockets.
	//		Initially from the server socket.
	struct pollfd	serverpfd;
	serverpfd.fd = _serverSocket;
	serverpfd.events = POLLIN;
	serverpfd.revents = 0;
	_pollFds.push_back(serverpfd);
}

Server::Server(int port, const std::string& password)
	: _port(port), _serverSocket(-1), _password(password), _running(false)
{
	signal(SIGINT, signalHandler);
	setupSocket();
}

Server::~Server()
{
	for (std::map<int, Client*>::iterator it =
			_clients.begin(); it != _clients.end(); ++it)
		delete it->second;
	_clients.clear();

	for (std::map<std::string, Channel*>::iterator it =
			_channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	_channels.clear();

	if (_serverSocket >= 0)
		close(_serverSocket);
}

void	Server::run()
{
	_running = true;
	g_running = true;

	std::cout << "Server running: press Ctrl+C to stop." << std::endl;
	while (_running && g_running)
	{
		//	Wait for events and return the number of FDs with events.
		int	tmp = poll(&_pollFds[0], _pollFds.size(), -1);
		if (tmp < 0)
		{
			// if errno matches signal interruption, ignore it
			if (errno == EINTR)
				continue;
			// Other wise, error break loop
			std::cout << "Error: poll failed" << std::endl;
			break;
		}
		for (size_t i = 0; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].revents == 0)
				continue;
			// Incoming events pending
			if (_pollFds[i].revents & POLLIN)
			{
				if (_pollFds[i].fd == _serverSocket)
					handleNewConnection();
				else
					handleClientData(_pollFds[i].fd);
			}
			if (_pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				if (_pollFds[i].fd != _serverSocket)
					removeClient(_pollFds[i].fd);
			}
		}
	}
	std::cout << "Server stopped." << std::endl;
}

void	Server::handleNewConnection()
{
	struct	sockaddr_in	clientAddr;
	socklen_t	addrLen = sizeof(clientAddr);

	int	clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientFd < 0)
	{
		std::cerr << "Error: accept failed" << std::endl;
		return;
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
	{
		std::cerr << "Error: fcntl failed";
		close(clientFd);
		return;
	}
	Client* client = new Client(clientFd, clientAddr);
	_clients[clientFd] = client;

	struct pollfd	pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	
	// std::cout << "New client connected: fd= " << clientFd << ", ip=" <<
	// inet_ntoa(clientAddr.sin_addr) << std::endl;
	std::cout << "New client connected: fd= " << clientFd << ", ip=" <<
	client->getHostName() << std::endl;
}

void	Server::handleClientData(int fd)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	Client* client = it->second;
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));

	ssize_t bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		if (bytesRead == 0)
			std::cerr << "Client fd: " << fd << " disconnected" << std::endl;
		else
			std::cerr << "Error: recv failed of fd= " << fd << "." << std::endl;
		removeClient(fd);
		return; 
	}

	client->appendToBuffer(std::string(buffer, bytesRead));
	while (client->hasCompleteMessage())
	{
		std::string line = client->extractMessage();
		if (!line.empty())
			processCommand(client, line);
		if (_clients.find(fd) == _clients.end())
			return;
	}
}

void	Server::removeClient(int fd)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it == _clients.end())
		return;
	Client* client = it->second;

	std::vector<Channel*> channels = client->getChannels();
	for (std::vector<Channel*>::iterator channelIt = channels.begin();
		channelIt != channels.end(); ++channelIt)
	{
		Channel* channel = *channelIt;
		bool	wasOperator = channel->isOperator(client);
		channel->removeClient(client);
		client->leaveChannel(channel);
		if (channel->isEmpty())
			deleteChannel(channel->getName());
		else if (wasOperator)
		{
			Client* newOperator = channel->getClients()[0];
			channel->addOperator(newOperator);
			std::string modeMsg = newOperator->getPrefix() + " MODE " + channel->getName()
				+ " +o " + newOperator->getNickName() + "\r\n";
			channel->broadcast(modeMsg, NULL);
		}
	}
	close(fd);
	delete client;
	_clients.erase(it);

	for (std::vector<struct pollfd>::iterator pollIt = _pollFds.begin();
		pollIt != _pollFds.end(); ++pollIt)
	{
		if (pollIt->fd == fd)
		{
			_pollFds.erase(pollIt);
			break;
		}
	}
	std::cout << "Client fd: " << fd << " removed." << std::endl;
}

void	Server::processCommand(Client* client, std::string& line)
{
	line = trim(line);
	if (line.empty())
		return;
	std::cout << "Processing from fd " << client->getFd() << ": " <<
		line << std::endl;
	
	std::string	command;
	std::string	rest;
	size_t	spacePos = line.find(' ');
	if (spacePos == std::string::npos)
	{
		command = toUpper(line);
		rest = "";
	}
	else
	{
		command = toUpper(line.substr(0, spacePos));
		rest = trim(line.substr(spacePos + 1));
	}
	std::vector<std::string> params;
	std::string message = "";
	size_t	colonPos = rest.find(':');
	if (colonPos != std::string::npos)
	{
		std::string	paramsStr = trim(rest.substr(0, colonPos));
		message = rest.substr(colonPos + 1);
		if (!paramsStr.empty())
			params = splitParams(paramsStr);
	}
	else
	{
		if (!rest.empty())
			params = splitParams(rest);
	}
	bool	isAuthCommand = (command == "PASS" || command == "NICK" ||
		command == "USER" || command == "QUIT" || command == "PING");
	if (!client->isRegistered() && !isAuthCommand)
	{
		sendError(client, "451", "You have not registered");
		return;
	}
	if (command == "PASS")
		cmdPass(client, params, message);
	else if (command == "NICK")
		cmdNick(client, params, message);
	else if (command == "USER")
		cmdUser(client, params, message);
	else if (command == "QUIT")
        cmdQuit(client, params, message);
    else if (command == "JOIN")
        cmdJoin(client, params, message);
    else if (command == "PART")
        cmdPart(client, params, message);
    else if (command == "TOPIC")
        cmdTopic(client, params, message);
    else if (command == "NAMES")
        cmdNames(client, params, message);
    else if (command == "INVITE")
        cmdInvite(client, params, message);
    else if (command == "KICK")
        cmdKick(client, params, message);
    else if (command == "PRIVMSG")
        cmdPrivmsg(client, params, message);
    else if (command == "MODE")
        cmdMode(client, params, message);
    else if (command == "PING")
        cmdPing(client, params, message);
    else
    {
        sendError(client, "421", command + " :Unknown command");
    }
}

void		Server::sendReply(Client* client, const std::string& code, const std::string& msg) const
{
	std::string reply = ":ircserv " + code + " " + client->getNickName() + " :" +
		msg + "\r\n";
	client->sendMessage(reply); 
}

void		Server::sendError(Client* client, const std::string& code, const std::string& msg) const
{
	sendReply(client, code, msg);
}

void		Server::sendRaw(Client* client, const std::string& msg) const
{
	client->sendMessage(msg + "\r\n");
}

Client*		Server::findClientByNick(const std::string& nick) const
{
	for (std::map<int, Client*>::const_iterator it = _clients.begin(); it
		!= _clients.end(); ++it)
	{
		if (it->second->getNickName() == nick)
			return it->second;
	}
	return NULL;
}

Channel*	Server::findChannel(const std::string& name) const
{
	std::map<std::string, Channel*>::const_iterator it = _channels.find(name);
	if (it != _channels.end())
		return it->second;
	return NULL;
}

Channel*	Server::createChannel(const std::string& name, Client* creator)
{
	Channel*	channel = new Channel(name);
	_channels[name] = channel;

	channel->addClient(creator);
	channel->addOperator(creator);
	creator->joinChannel(channel);

	return channel;
}

void	Server::deleteChannel(const std::string& name)
{
	std::map<std::string, Channel*>::iterator it = _channels.find(name);
	if (it != _channels.end())
	{
		delete it->second;
		_channels.erase(it);
	}
}

bool	Server::isNickNameTaken(const std::string& name) const
{
	for (std::map<int, Client*>::const_iterator it = _clients.begin();
		it != _clients.end(); ++it)
	{
		if (it->second->getNickName() == name)
			return true;
	}
	return false;
}

void	Server::broadcastToChannel(Channel* channel, const std::string& msg, Client* exclude) const
{
	channel->broadcast(msg, exclude);
}

void	Server::cmdPass(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	(void)message;
	if (params.empty())
	{
		sendError(client, "461", "PASS: Not enough parameters");
		return;
	}
	if (client->isAuthenticated())
	{
		sendError(client, "462", " You may not reregister");
		return;
	}
	if (params[0] == _password)
	{
		client->setAuthenticated(true);
		std::cout << "Client fd: " << client->getFd() << " authenticated." << std::endl;
	}
	else
		sendError(client, "464", " Password incorrect");
}

void	Server::cmdNick(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	(void)message;
	if (params.empty())
	{
		sendError(client, "431", " No nickname given");
		return;
	}
	std::string	newNick = params[0];
	if (newNick.empty() || newNick.length() > 9 || newNick[0] == '#' ||
		newNick[0] == '&')
	{
		sendError(client, "432", newNick + ": Erroneous nickname");
		return;
	}
	if (isNickNameTaken(newNick) && client->getNickName() != newNick)
	{
		sendError(client, "433", newNick + " Nickname is already in use");
		return;
	}
	if (client->getNickName() != "*" && client->getNickName() != newNick)
	{
		std::string	nickChange = ":" + client->getNickName() + "!" + client->getUserName()
			+ "@" + client->getHostName() + " NICK : " + newNick + "\r\n"; 
		
		std::vector<Channel*>	channels = client->getChannels();
		for (std::vector<Channel*>::iterator it = channels.begin();
				it != channels.end(); ++it)
		{
			(*it)->broadcast(nickChange, NULL);
		}
		client->sendMessage(nickChange);
	}
	client->setNickName(newNick);
	if (client->isAuthenticated() && !client->getUserName().empty() && !client->isRegistered())
	{
		client->setRegistered(true);
		sendReply(client, "001", "Welcome to the Internet Relay Network "
        	+ client->getPrefix());
	}
}

void	Server::cmdUser(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	if (params.size() < 3)
	{
		sendError(client, "461", "USER: Not enough parameters");
		return;
	}
	if (client->isRegistered())
	{
		sendError(client, "462", "You may not reregister");
		return;
	}
	client->setUserName(params[0]);
	if (!message.empty())
		client->setRealName(message);
	else if (params.size() >= 4)
		client->setRealName(params[3]);
	else
		client->setRealName(params[0]);
	
	if (client->isAuthenticated() && client->getNickName() != "*" && !client->isRegistered())
	{	
		client->setRegistered(true);
		sendReply(client, "001", "Welcome to the Internet Relay Network "
        	+ client->getPrefix());
	}
}

void	Server::cmdQuit(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	(void)params;

	std::string		quitMsg = "Client has quit";
	if (!message.empty())
		quitMsg = message;

	std::vector<Channel*>	channels = client->getChannels();
	for (std::vector<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		std::string	msg = client->getPrefix() + " QUIT :" + quitMsg + "\r\n";
		(*it)->broadcast(msg, client);
	}
	removeClient(client->getFd());
}

void	Server::cmdJoin(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	(void)message;

	if (params.empty())
	{
		sendError(client, "461", "JOIN: Not enough parameters");
		return;
	}
	std::string	channelName = params[0];
	if (channelName.empty() || channelName.length() < 2 || (channelName[0] != '#' && channelName[0] != '&'))
	{
		sendError(client, "476", " bad channel mask");
		return;
	}
	Channel*	channel = findChannel(channelName);
	if (!channel)
	{
		createChannel(channelName, client);
		std::string	joinMsg = client->getPrefix() + " JOIN " + channelName + "\r\n";
		client->sendMessage(joinMsg);
		sendReply(client, "332", channelName + " :");
		std::string	namesList = "@" + client->getNickName();
		sendReply(client, "353", "= " + channelName + " :" + namesList);
		sendReply(client, "366", channelName + " : End of /NAMES list.");
		std::cout << "Channel " << channelName << " created by " << client->getNickName()
			<< std::endl;
		return;
	}
	if (channel->hasClient(client))
		return;
	if (channel->hasMode('l') && channel->getClientCount() >= channel->getUserLimit())
	{
		sendError(client, "471", channelName + " :Cannot join channel (+l)");
		return;
	}
	if (channel->hasMode('i') && !channel->isInvited(client->getNickName()) &&
		!client->isInvitedTo(channelName))
	{
		sendError(client, "473", channelName + " :Cannot join channel (+i)");
		return;
	}
	if (channel->hasMode('k'))
	{
		std::string	providedKey = (params.size() > 1) ? params[1] : "";
		{
			if (providedKey != channel->getPassword())
			{
				sendError(client, "475", channelName + " :Cannot join channel (+k).");
				return;
			}
		}
	}
	channel->addClient(client);
	client->joinChannel(channel);

	channel->removeInvitedUser(client->getNickName());
	client->removeInvite(channelName);

	std::string	joinMsg = client->getPrefix() + " JOIN " + channelName + "\r\n";
	channel->broadcast(joinMsg, NULL);
	if (!channel->getTopic().empty())
		sendReply(client, "332", channelName + " :" + channel->getTopic());
	else
		sendReply(client, "332", channelName + " :");

	std::string namesList;
    const std::vector<Client*>& clients = channel->getClients();
    for (std::vector<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (!namesList.empty())
            namesList += " ";
        if (channel->isOperator(*it))
            namesList += "@";
        namesList += (*it)->getNickName();
    }
    sendReply(client, "353", "= " + channelName + " :" + namesList);
    sendReply(client, "366", channelName + " :End of /NAMES list");
}

void	Server::cmdPart(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	if (params.empty())
	{
		sendError(client, "461", "PART :Not enough parameters");
		return;
	}
	std::string	channelName = params[0];
	Channel*	channel = findChannel(channelName);
	if (!channel)
	{
		sendError(client, "403", channelName + " :No such channel");
		return;
	}
	if (!channel->hasClient(client))
	{
		sendError(client, "442", channelName + " :You are not on that channel");
		return;
	}

	std::string	partMsg = client->getPrefix() + " PART " + channelName;
	if (!message.empty())
		partMsg += " :" + message;
	partMsg += "\r\n";
	channel->broadcast(partMsg, NULL);

	channel->removeClient(client);
	client->leaveChannel(channel);
	if (channel->isEmpty())
		deleteChannel(channelName);
}

void	Server::cmdTopic(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	if (params.empty())
	{
		sendError(client, "461", "TOPIC: Not enough parameters");
		return;
	}
	std::string	channelName = params[0];
	Channel*	channel = findChannel(channelName);
	if (!channel)
	{
		sendError(client, "403", channelName + " :No such channel");
		return;
	}
    if (!channel->hasClient(client))
	{
		sendError(client, "442", channelName + " :You're not on that channel");
		return;
	}
	if (message.empty() && (params.size() == 1 || (params.size() > 1 && params[1][0] != ':')))
    {
		if (channel->getTopic().empty())
			sendReply(client, "331", channelName + " :No topic is set");
		else
		{
			sendReply(client, "332", channelName + " :" + channel->getTopic());
			std::stringstream ss;
			ss << channel->getTopicTime();
			sendReply(client, "333", channelName + " " + channel->getTopicSetter() + " " + ss.str());
		}
		return;
	}
	if (channel->hasMode('t') && !channel->isOperator(client))
	{
		sendError(client, "482", channelName + " :You're not channel operator");
		return;
	}
	channel->setTopic(message, client->getNickName());
	std::string topicMsg = client->getPrefix() + " TOPIC " +
		channelName + " :" + message + "\r\n";
    channel->broadcast(topicMsg, NULL);
}

void	Server::cmdNames(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	(void)message;
	if (params.empty())
		return;
	std::string	channelName = params[0];
	Channel*	channel = findChannel(channelName);
	if (!channel)
	{
		sendReply(client, "366", channelName + " :End of /NAMES list");
		return;
	}
	if (!channel->hasClient(client))
	{
		sendReply(client, "366", channelName + " :End of /NAMES list");
		return;
	}
	std::string	namesList;
	const std::vector<Client*>& clients = channel->getClients();
	for (std::vector<Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (!namesList.empty())
			namesList += " ";
		if (channel->isOperator(*it))
			namesList += "@";
		namesList += (*it)->getNickName();
	}
	sendReply(client, "353", "= " + channelName + " :" + namesList);
	sendReply(client, "366", channelName + " :End of /NAMES list");
}

void	Server::cmdInvite(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	(void)message;
	if (params.size() < 2)
	{
		sendError(client, "461", "INVITE: Not enough parameters");
		return;
	}
	std::string	targetNick = params[0];
	std::string	channelName = params[1];
	Channel*	channel = findChannel(channelName);
	if (!channel)
	{
		sendError(client, "403", channelName + " :No such channel");
		return;
	}
	if (!channel->hasClient(client))
	{
		sendError(client, "442", channelName + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(client))
	{
		sendError(client, "482", channelName + " :You're not channel operator");
		return;
	}
	Client* target = findClientByNick(targetNick);
	if (!target)
	{
		sendError(client, "401", targetNick + " :No such nick/channel");
		return;
	}
	if (channel->hasClient(target))
	{
		sendError(client, "443", targetNick + " " + channelName + " :is already on channel");
		return;
	}

	channel->addInvitedUser(targetNick);
	target->addInvite(channelName);

	sendReply(client, "341", channelName + " " + targetNick);

	std::string inviteMsg = client->getPrefix() + " INVITE " + targetNick + " :" + channelName + "\r\n";
	target->sendMessage(inviteMsg);
}

void	Server::cmdKick(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	if (params.size() < 2)
	{
		sendError(client, "461", "KICK :Not enough parameters");
		return;
	}
	std::string channelName = params[0];
	std::string targetNick = params[1];
	std::string reason = message.empty() ? "Kicked by operator" : message;

	Channel* channel = findChannel(channelName);
	if (!channel)
	{
		sendError(client, "403", channelName + " :No such channel");
		return;
	}
	if (!channel->hasClient(client))
	{
		sendError(client, "442", channelName + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(client))
	{
		sendError(client, "482", channelName + " :You're not channel operator");
		return;
	}

	Client* target = findClientByNick(targetNick);
	if (!target || !channel->hasClient(target))
	{
		sendError(client, "441", targetNick + " " + channelName + " :They aren't on that channel");
		return;
	}
	if (target == client)
	{
		sendError(client, "482", channelName + " :You cannot kick yourself");
		return;
	}

	std::string kickMsg = client->getPrefix() + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";
	channel->broadcast(kickMsg, NULL);
	channel->removeClient(target);
	target->leaveChannel(channel);
	if (channel->isEmpty())
		deleteChannel(channelName);
}

void	Server::cmdPrivmsg(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	if (params.empty())
	{
		sendError(client, "411", "No recipient given (PRIVMSG)");
		return;
	}
	if (message.empty())
	{
		sendError(client, "412", "No text to send");
		return;
	}
    std::string target = params[0];
	if (target[0] == '#' || target[0] == '&')
	{
		Channel* channel = findChannel(target);
		if (!channel)
		{
			sendError(client, "403", target + " :No such channel");
			return;
		}
		if (!channel->hasClient(client))
		{
			sendError(client, "404", target + " :Cannot send to channel");
			return;
		}
		std::string msg = client->getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
		channel->broadcast(msg, client);
	}
	else
	{
		Client* targetClient = findClientByNick(target);
		if (!targetClient)
		{
			sendError(client, "401", target + " :No such nick/channel");
			return;
		}
		std::string msg = client->getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
		targetClient->sendMessage(msg);
	}
}

void	Server::cmdPing(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	std::string token = "ircserv";
	if (!params.empty())
		token = params[0];
	else if (!message.empty())
		token = message;
	std::string pong = ":ircserv PONG ircserv :" + token + "\r\n";
	client->sendMessage(pong);
}

void	Server::cmdMode(Client* client, const std::vector<std::string>& params, const std::string& message)
{
	(void)message;
	if (params.empty())
	{
		sendError(client, "461", "MODE :Not enough parameters");
		return;
	}
	std::string	target = params[0];
	if (params.size() == 1)
	{
		Channel* channel = findChannel(target);
		if (!channel)
		{
			sendError(client, "502", "Can't change mode for other users");
			return;
		}
		std::string	modeReply = channel->getModeString() + channel->getModeParams();
		sendReply(client, "324", target + " " + modeReply);
		return;
	}
	std::string	modes = params[1];
	Channel*	channel = findChannel(target);
	if (!channel)
	{
		sendError(client, "403", target + " :No such channel");
		return;
	}
	if (!channel->hasClient(client))
	{
		sendError(client, "442", target + " :You're not on that channel");
		return;
	}
	if (!channel->isOperator(client))
	{
		sendError(client, "482", target + " :You're not channel operator");
		return;
	}
	bool	adding = true;
	size_t	paramIdx = 2;
	for (size_t i = 0; i < modes.length(); ++i)
	{
		char c = modes[i];
		if (c == '+') {adding = true; continue;}
		if (c == '-') {adding = false; continue;}

		if (c == 'i')
		{
			if (adding) channel->addMode('i');
			else channel->removeMode('i');
			std::string modeMsg = client->getPrefix() + " MODE " + target + " "
				+ (adding ? "+" : "-") + "i\r\n";
			channel->broadcast(modeMsg, NULL);
		}
		else if (c == 't')
		{
			if (adding) channel->addMode('t');
			else channel->removeMode('t');
			std::string modeMsg = client->getPrefix() + " MODE " + target + " "
				+ (adding ? "+" : "-") + "i\r\n";
			channel->broadcast(modeMsg, NULL);
		}
		else if (c == 'k')
		{
			if (adding)
			{
				if (paramIdx >= params.size())
				{
					sendError(client, "461", "MODE :Not enough parameters");
					continue;
				}
				channel->setPassword(params[paramIdx]);
				channel->addMode('k');
				std::string modeMsg = client->getPrefix() + " MODE " + target + " +k "
					+ params[paramIdx] + "\r\n";
				channel->broadcast(modeMsg, NULL);
				paramIdx++;
			}
			else
			{
				channel->setPassword("");
				channel->removeMode('k');
				std::string modeMsg = client->getPrefix() + " MODE " + target + " -k\r\n";
				channel->broadcast(modeMsg, NULL);
			}
		}
		else if (c == 'o')
		{
			if (paramIdx >= params.size())
			{
				sendError(client, "461", "MODE :Not enough parameters");
				continue;
			}
			std::string targetNick = params[paramIdx];
			Client* targetClient = findClientByNick(targetNick);
			if (!targetClient || !channel->hasClient(targetClient))
			{
				sendError(client, "441", targetNick + " " + target + " :They aren't on that channel");
				paramIdx++;
				continue;
			}
			if (adding)
			{
				channel->addOperator(targetClient);
				std::string modeMsg = client->getPrefix() + " MODE " + target + " +o " + targetNick + "\r\n";
				channel->broadcast(modeMsg, NULL);
			}
			else
			{
				channel->removeOperator(targetClient);
				std::string modeMsg = client->getPrefix() + " MODE " + target + " -o " + targetNick + "\r\n";
				channel->broadcast(modeMsg, NULL);
			}
			paramIdx++;
		}
		else if (c == 'l')
		{
			if (adding)
			{
				if (paramIdx >= params.size())
				{
					sendError(client, "461", "MODE :Not enough parameters");
					continue;
				}
				std::string	limitStr = params[paramIdx];
				bool	validNumber = !limitStr.empty();
				for (size_t j = 0; j < limitStr.length(); ++j)
				{
					if (limitStr[j] < '0' || limitStr[j] > '9')
					{
						validNumber = false;
						break;
					}
				}
				if (!validNumber)
				{
					sendError(client, "461", "MODE :Invalid user number");
					paramIdx++;
					continue;
				}
				int	limit = std::atoi(params[paramIdx].c_str());
				if (limit <= 0)
				{
					paramIdx++;
					continue;
				}
				channel->setUserLimit(static_cast<size_t>(limit));
				channel->addMode('l');

				std::stringstream	ss;
				ss << limit;
				std::string	modeMsg = client->getPrefix() + " MODE " + target
					+ " +l " + ss.str() + "\r\n";
				channel->broadcast(modeMsg, NULL);
				paramIdx++; 
			}
			else
			{
				channel->removeUserLimit();
				channel->removeMode('l');
				std::string	modeMsg = client->getPrefix() + " MODE " + target
					+ " -l\r\n";
				channel->broadcast(modeMsg, NULL);
			}
		}
		else
			sendError(client, "472", std::string(1, c) + " :is unknown mode char to me");
	}
}
