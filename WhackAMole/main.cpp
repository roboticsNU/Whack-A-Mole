#include <iostream>
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <thread>
#include <mutex>

using namespace std;
unsigned char dat[100];
unsigned char res[100];
std::size_t received;
sf::UdpSocket socket;
sf::UdpSocket ResSocket;
sf::CircleShape buttons[5];
std::mutex mut;

void serverThread()
{
	sf::IpAddress sender;
	unsigned short port;
	// bind the socket to a port
	if (ResSocket.bind(8888) != sf::Socket::Done)
	{
		cout << "PIZDA";
	}
	
	while (true)
	{
		if (ResSocket.receive(res, 100, received, sender, port) != sf::Socket::Done)
		{
			cout << "sadsd";
		}
		
		if(res[0] == 229)
		{
			int currentActive = res[1];
			for (int i = 0; i < 5; i++)
			{
				buttons[i].setFillColor(sf::Color((0x6b0101FF)));
			}
			buttons[currentActive].setFillColor(sf::Color::Red);
		}
	}
}

int main()
{
	/*for (;;)
	{
		dat[0] = 230;
		dat[1] = 0;
		sf::IpAddress recipient = "192.168.11.17";
		socket.send(dat, 100, recipient, 8888);
	}*/
	for (int i = 0; i < 5; i++)
	{
		buttons[i].setFillColor(sf::Color(0x6b0101FF));
		buttons[i].setRadius(50.f);
	}
	buttons[1].setPosition(200.f, 60.f);
	buttons[3].setPosition(320.f, 60.f);
	buttons[0].setPosition(115.f, 220.f);
	buttons[2].setPosition(265.f, 220.f);
	buttons[4].setPosition(415.f, 220.f);

	sf::IpAddress recipient = "192.168.11.17";

	thread th(serverThread);
	//th.join();
	
	sf::RenderWindow window(sf::VideoMode(640, 480), "Whack a mole", sf::Style::Fullscreen);
	

	// run the program as long as the window is open
	while (window.isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
			if(event.type == sf::Event::MouseButtonPressed)
			{
				for (int i = 0; i < 5; i++)
					if(buttons[i].getGlobalBounds().contains(event.mouseButton.x, event.mouseButton.y))
					{
						dat[0] = 230;
						dat[1] = i;
						if (socket.send(dat, 100, recipient, 8888) != sf::Socket::Done)
						{
							cout << "GG WP" << endl;
						}
					}
			}
		}

		// clear the window with black color
		window.clear(sf::Color::Black);

		// draw everything here...
		for (int i = 0; i < 5; i++)
			window.draw(buttons[i]);

		// end the current frame
		window.display();
		sf::sleep(sf::milliseconds(20));
	}

	while(true)
	{
		string in;
		cout << "Enter a command: ";
		cin >> in;
		cout << endl;

		if(in == "reset")
		{
			dat[0] = 228;
			if (socket.send(dat, 100, recipient, 8888) != sf::Socket::Done)
			{
				cout << "GG WP" << endl;
			}
		}
	}
}