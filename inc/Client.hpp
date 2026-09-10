#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <vector>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <sys/socket.h>
# include <algorithm>

class	Channel;

class	Client
{
	private:
		int							_fd;
		struct sockaddr_in			_address;
		std::string					_buffer;
		std::string					_nickname;
		std::string					_username;
		std::string					_realname;
		std::string					_hostname;
		bool						_authenticated;
		bool						_registered;
		bool						_disconnected;
		std::vector<Channel*>		_channels;
		std::vector<std::string>	_invites;

	public:
		Client(int fd, struct sockaddr_in address);
		~Client();

		int							getFd() const;
		std::string					getNickName() const;
		std::string					getUserName() const;
		std::string					getRealName() const;
		std::string					getHostName() const;
		std::string					getPrefix() const;
		bool						isAuthenticated() const;
		bool						isRegistered() const;
		bool						isDisconnected() const;
		std::string&				getBuffer();
		const std::vector<Channel*>	getChannels() const;

		void		setNickName(std::string& nick);
		void		setUserName(const std::string& user);
		void		setRealName(const std::string& real);
		void		setHostName(std::string& host);
		void		setAuthenticated(bool authenticated);
		void		setRegistered(bool registered);
		void		setDisconnected(bool disconnected);

		void		joinChannel(Channel* channel);
		void		leaveChannel(Channel* channel);
		bool		isInChannel(const std::string& name) const;
		void		addInvite(const std::string& channelName);
		bool		isInvitedTo(const std::string& channelName) const;
		void		removeInvite(const std::string& channelName);

		void		appendToBuffer(const std::string& data);
    	void		clearBuffer();
   		bool 		hasCompleteMessage() const;
    	std::string	extractMessage();
		void 		sendMessage(const std::string& message) const;
};

#endif
