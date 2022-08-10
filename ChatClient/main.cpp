#include "Client.h"

int main()
{
  Custom_Client c;
  c.Connect("127.0.0.1", 60000);
  if (c.IsConnected())
  {
    std::cout << "Connected to the host. Type Help for info";
  }
  std::future<void> futureString = std::async(std::launch::async, &Custom_Client::ExecuteMessages, std::ref(c));
  std::string inc_data;
  while(1)
  {
    inc_data = "";
    std::getline(std::cin, inc_data);
    if (inc_data != "")
      c << StrToCommand(inc_data);
    else
      std::cout << ">";
  }
  return 0;
}
