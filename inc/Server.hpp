#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <string>
# include <vector>
# include <poll.h>
# include <map>
# include <cstdlib>
# include <cstring>
# include <sstream>
# include <unistd.h>
# include <fcntl.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <algorithm>
# include <signal.h>
# include <cerrno>

class Client;
class Channel;

class Server
{
	private:
		int								_port;
		int								_serverSocket;
		std::string						_password;
		bool							_running;
		std::vector<struct pollfd>		_pollFds;
		std::map<int, Client*>			_clients;
		std::map<std::string, Channel*>	_channels;

		void	setupSocket();
		void	handleNewClient();
		void	handleNewConnection();
		void	handleClientData(int fd);
		void	removeClient(int fd);
		void	processCommand(Client* client, std::string& line);

		void	cmdPass(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdNick(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdUser(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdJoin(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdQuit(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdPart(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdTopic(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdNames(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdInvite(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdKick(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdPrivmsg(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdPing(Client* client, const std::vector<std::string>& params, const std::string& message);
		void	cmdMode(Client* client, const std::vector<std::string>& params, const std::string& message);

		Client* 	findClientByNick(const std::string& nick) const;
		Channel*	findChannel(const std::string& name) const;
		Channel*	createChannel(const std::string& name, Client* creator);

		void	deleteChannel(const std::string& name);
		bool	isNickNameTaken(const std::string& name) const;
		void	broadcastToChannel(Channel* channel, const std::string& msg, Client* exclude) const;
		void	sendReply(Client* client, const std::string& code, const std::string& msg) const;
		void	sendError(Client* client, const std::string& code, const std::string& msg) const;
		void	sendRaw(Client* client, const std::string& msg) const;

	public:
		Server(int port, const std::string& password);
		~Server();

		void	run();
};

#endif
