#ifndef AUTHOR_H
#define AUTHOR_H

#include <string>
#include <vector>
#include "Poco/JSON/Object.h"
#include <optional>

namespace database
{
    class Message{
        private:
            long _id;
            long _sender_id;
            long _order_id;
            std::string _text;
            std::string _date;


        public:

            static Message fromJSON(const std::string & str);

            const long        &get_id() const;
            const std::string &get_text() const;
            const std::string &get_date() const;
            const long        &get_order_id() const;
            const long        &get_sender_id() const;


            long        &id();
            std::string &text();
            std::string &date();
            long        &order_id();
            long        &sender_id();

            static void init();
            void update_in_mysql();
            bool save_to_mysql();
            static bool delete_in_mysql(long id);

            static std::optional<Message> read_by_id(long id);
            static std::vector<Message> read_by_order_id(long sender_id);

            Poco::JSON::Object::Ptr toJSON() const;

    };
}

#endif