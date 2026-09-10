#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <vector>
# include <set>
# include <ctime>
# include <algorithm>
# include <sstream>

class	Client;

class	Channel
{
	private:
		std::string					_name;
		std::string					_topic;
		std::string					_topicSetter;
		time_t						_topicTime;
		std::vector<Client*>		_clients;
		std::vector<Client*>		_operators;
		std::set<char>				_modes;
		std::string					_password;
		size_t						_userLimit;
		std::vector<std::string>	_invitedUsers;

	public:
		Channel(const std::string& name);
		~Channel();

		std::string					getName() const;
		std::string					getTopic() const;
		std::string					getTopicSetter() const;
		time_t						getTopicTime() const;
		std::string					getPassword() const;
		size_t						getUserLimit() const;
		size_t						getClientCount() const;
		const std::vector<Client*>&	getClients() const;
		std::string					getModeString() const;
		std::string					getModeParams() const;

		void setTopic(const std::string& topic, const std::string& setter);
		void setPassword(const std::string& pass);
		void setUserLimit(size_t limit);
    	void removeUserLimit();

	    bool hasMode(char mode) const;
    	void addMode(char mode);
    	void removeMode(char mode);

    	void addClient(Client* client);
    	void removeClient(Client* client);
    	bool hasClient(Client* client) const;
    	bool hasClient(const std::string& nick) const;
    	bool isEmpty() const;

    	void addOperator(Client* client);
    	void removeOperator(Client* client);
    	bool isOperator(Client* client) const;
    	bool isOperator(const std::string& nick) const;

    	void addInvitedUser(const std::string& nick);
    	void removeInvitedUser(const std::string& nick);
    	bool isInvited(const std::string& nick) const;
		void broadcast(const std::string& message, Client* exclude) const;
};

#endif
