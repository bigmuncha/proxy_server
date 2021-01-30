#include <iostream>
#include <cstdlib>
#include <boost/asio.hpp>
#include <memory>
#include <utility>
#include <cstdio>
#include <random>
#include <ctime>

using  boost::asio::ip::tcp;

class session
    :public std::enable_shared_from_this<session>
{
    public:
        session(boost::asio::io_context& io_context,tcp::socket socket,
                std::pair<std::string, std::string> pair)
            :ioster(io_context),
             client_socket_ (std::move(socket)),
             server_socket_(io_context),
             ip_and_port(pair)
        {

        }


        void start()
        {
            try {

                tcp::resolver resolver(ioster);
                std::cout << ip_and_port.first.c_str()
                          <<ip_and_port.second.c_str() << "Conec\n";
                boost::asio::connect(server_socket_,
                                     resolver.resolve(
                                         ip_and_port.first.c_str(),
                                         ip_and_port.second.c_str()));


                /*server_socket_.write_some(
                    boost::asio::buffer(
                    client_socket_.remote_endpoint().address().to_string()));
*/
                //boost::asio::connect(server_socket_,resolver.resolve(ip_and_port.first.c_str(),ip_and_port.second.c_str()));
                message_from_serv_to_cli();
            } catch (std::exception& e) {
                std::cerr << "Exception v starte: " << e.what() << "\n";

            }

        }

        

    private:
        void message_from_serv_to_cli(){
            auto self(shared_from_this());
            server_socket_.read_some(boost::asio::buffer(data_, max_length));
            client_socket_.async_write_some(
                boost::asio::buffer(data_, max_length),
                [this,self](boost::system::error_code ec, std::size_t)
                {
                    if(!ec)
                    {
                        message_from_cli_to_serv();
                    }
                });

        }
        void message_from_cli_to_serv(){
            auto self(shared_from_this());
            client_socket_.read_some(boost::asio::buffer(data_, max_length));
            server_socket_.async_write_some(
                boost::asio::buffer(data_, max_length),
                [this,self](boost::system::error_code ec, std::size_t)
                {
                    if(!ec)
                    {
                        message_from_serv_to_cli();
                    }
                });

        }

        boost::asio::io_context& ioster;
        tcp::socket client_socket_;
        tcp::socket server_socket_;
        enum {max_length = 1024};
        //char port
        std::pair<std::string, std::string> ip_and_port;
        char data_[max_length];

};


class server {
    public:
        server(boost::asio::io_context& io_context, std::vector<std::string> config)
            :ioster(io_context),
            acceptor_(io_context, tcp::endpoint(tcp::v4(), atoi(config[0].c_str()))),
             configarray(std::move(config))
        {
            do_accept();

        }

    private:
        void do_accept()
        {
            acceptor_.async_accept(
                [this](boost::system::error_code ec, tcp::socket socket)
                {
                    if(!ec)
                    {

                        std::make_shared<session>(ioster,std::move(socket),get_ip_and_port())->start();
                    }

                    do_accept();
                });
        }


        std::pair<std::string, std::string> get_ip_and_port(){
            std::mt19937 gen{std::random_device()()};
            std::uniform_int_distribution<> uid(1,configarray.size()-1);
            std::string curstring = configarray[uid(gen)];
            auto pos = curstring.find(':');
            std::pair<std::string,std::string> retpair(
                curstring.substr(0,pos), curstring.substr(
                    pos+1,curstring.length()));
            return retpair;
        }

        boost::asio::io_context& ioster;
        tcp::acceptor acceptor_;
        std::vector<std::string> configarray;
};


int main(int argc, char *argv[]) {

  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <config file name>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    FILE *pfile;
    pfile = fopen(argv[1],"r");

    std::vector<std::string> omar;

    char *str;
    if(pfile!= NULL){
        while(fgets(str,1024,pfile) != NULL){
            omar.push_back(str);
        }
    }
    for(int i =1;i < omar.size();i++){
        omar[i].pop_back();
    }



    server s(io_context, std::move(omar));

    io_context.run();

  }
  catch (std::exception& e)
  {
    std::cerr << "Exception v maine: " << e.what() << "\n";
  }


    return 0;
}
