#include "inc/Server.hpp"

int	main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
	}
	std::string	portStr = argv[1];
	for (size_t i = 0; i < portStr.length(); i++)
	{
		if (portStr < "0" || portStr > "9")
		{
			std::cerr << "Error: port must be a number." << std::endl;
			return 1;
		}
	}
	int	port = std::atoi(argv[1]);
	if (port < 1024 || port > 65535)
	{
		std::cerr << "Error: port must be between 1024 and 65535." << std::endl;
		return 1;
	}
	std::string	password = argv[2];
	
	Server	server(port, password);
	server.run();
	return 0;
}
