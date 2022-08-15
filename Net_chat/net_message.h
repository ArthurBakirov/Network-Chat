#ifndef NET_MESSAGE_H
#define NET_MESSAGE_H

#include "net_common.h"

namespace olc
{
  namespace net
  {
    template<class T>
    struct message_header
    {
      T id{};
      uint32_t size = 0;
    };

    template<class T>
    struct message
    {
      message_header<T> header{};
      std::vector<uint8_t> body;

      size_t size() const
      {
        return body.size();
      }

      friend std::ostream& operator<< (const message<T>& msg, std::ostream& out)
      {
        out << "ID: " << int(msg.header.id) << " size: " << msg.header.size << std::endl;
        return out;
      }

      template<class DataType>
      friend message<T>& operator<< (message<T>& msg, const DataType& data)
      {
        static_assert(std::is_standard_layout<DataType>::value,"Data is not in standard layout");
        size_t i = msg.body.size();
        msg.body.resize(msg.body.size() + sizeof(DataType));
        std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
        msg.header.size = msg.size();
        return msg;
      }

      template<class DataType>
      friend message<T>& operator>> (message<T>& msg, DataType& data)
      {
        static_assert(std::is_standard_layout<DataType>::value,"Data is not in standard layout");
        size_t i = msg.body.size() - sizeof(DataType);
        std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
        msg.body.resize(i);
        msg.header.size = msg.size();
        return msg;
      }


      friend message<T>& operator<< (message<T>& msg, std::string& data)
      {
        static_assert(std::is_standard_layout<std::string>::value,"Data is not in standard layout");
        size_t i = msg.body.size();
        msg.body.resize(msg.body.size() + data.length());
        std::memcpy(msg.body.data() + i, reinterpret_cast<void *>(data.data()), data.length());
        msg.header.size = msg.size();
        msg << data.length();
        return msg;
      }

      friend message<T>& operator>> (message<T>& msg, std::string& data)
      {
        static_assert(std::is_standard_layout<std::string>::value,"Data is not in standard layout");
        size_t length;
        size_t CurrentLength = data.length();
        msg >> length;
        data.resize(data.length() + length);
        size_t i = msg.body.size() - length;
        std::memcpy(data.data() + CurrentLength, reinterpret_cast<char*>(msg.body.data() + i), length);
        msg.body.resize(i);
        msg.header.size = msg.size();
        return msg;
      }
    }; //message

    template<class T>
    class connection;

    template<class T>
    struct owned_message
    {
      std::shared_ptr<connection<T>> remote = nullptr;
      message<T> msg;
      friend std::ostream& operator<< (const owned_message<T>& msg_, std::ostream& out)
      {
        out << msg_;
        return out;
      }

    };
  } //namespace net
} //namespace olc

#endif /* End of NET_MESSAGE_H */