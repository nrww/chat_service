#include "message.h"
#include "database.h"
#include "../config/config.h"

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SessionFactory.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>

#include <sstream>
#include <exception>

using namespace Poco::Data::Keywords;
using Poco::Data::Session;
using Poco::Data::Statement;
#define DATE_FORMAT "\'DD.MM.YYYY HH24:MI:SS\'"

namespace database
{

    void Message::init()
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement create_stmt(session);
            create_stmt << "CREATE TABLE IF NOT EXISTS `message` ("
                        << "`message_id` INT NOT NULL AUTO_INCREMENT,"
                        << "`text` VARCHAR(4000) NOT NULL,"
                        << "`sender_id` INT NOT NULL,"
                        << "`date` DATETIME NOT NULL,"
                        << "`order_id` INT NOT NULL,"
                        << "PRIMARY KEY (`message_id`),"
                        << "FOREIGN KEY (`order_id`) REFERENCES `order` (`order_id`),"
                        << "FOREIGN KEY (`sender_id`) REFERENCES `user` (`user_id`)"
                        << ");",
                now;
            
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << "connection:" << e.displayText() << std::endl;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {
            std::cout << "statement:" << e.displayText() << std::endl;
        }
    }

    bool Message::update_in_mysql()
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement update(session);

            update  << "UPDATE `message` "
                    << "SET `text` = ? "
                    << "WHERE `message_id` = ? ;",
                use(_text),
                use(_id),
                now;

            update.execute();

            std::cout << "updated: " << _id << std::endl;
            return true;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        return false;
    }

    bool Message::delete_in_mysql(long id)
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement del(session);
            
            del << "DELETE FROM `message` WHERE `message_id` = ?;",
                use(id),
                range(0, 1); 
            del.execute();
            std::cout << "deleted: " << id << std::endl;
            return true;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        return false;
    }

    Poco::JSON::Object::Ptr Message::toJSON() const
    {
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();

        root->set("id", _id);
        root->set("text", _text);
        root->set("date", _date);
        root->set("order_id", _order_id);
        root->set("sender_id", _sender_id);

        return root;
    }

    Message Message::fromJSON(const std::string &str)
    {
        Message user;
        Poco::JSON::Parser parser;
        Poco::Dynamic::Var result = parser.parse(str);
        Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();

        user.id() = object->getValue<long>("id");
        user.text() = object->getValue<std::string>("text");
        user.date() = object->getValue<std::string>("date");
        user.order_id() = object->getValue<long>("order_id");
        user.sender_id() = object->getValue<long>("sender_id");

        return user;
    }

    std::optional<Message> Message::read_by_id(long id)
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement select(session);
            Message a;
            select  << "SELECT `message_id`, `text`, TO_CHAR(`date`, " << DATE_FORMAT << "), `order_id`, `sender_id`"
                    << "FROM `message`" 
                    << "WHERE `message_id` = ? ;",
                into(a._id),
                into(a._text),
                into(a._date),
                into(a._order_id),
                into(a._sender_id),
                use(id),
                range(0, 1); //  iterate over result set one row at a time

            select.execute();
            Poco::Data::RecordSet rs(select);
            if (rs.moveFirst()) return a;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        return {};
    }

    std::vector<Message> Message::read_by_order_id(long order_id)
    {
        std::vector<Message> result;
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Statement select(session);
            Message a;
            select  << "SELECT `message_id`, `text`, TO_CHAR(`date`, " << DATE_FORMAT << "), `order_id`, `sender_id` " 
                    << "FROM `message` " 
                    << "WHERE `order_id` = ? ;",
                into(a._id),
                into(a._text),
                into(a._date),
                into(a._order_id),
                into(a._sender_id),
                use(order_id),
                range(0, 1); //  iterate over result set one row at a time

            while (!select.done())
            {
                if (select.execute())
                    result.push_back(a);
            }
            return result;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        return std::vector<Message>();
    }

    bool Message::save_to_mysql()
    {
        try
        {
            Poco::Data::Session session = database::Database::get().create_session();
            Poco::Data::Statement insert(session);

            insert  << "INSERT INTO `message` (`text`, `date`, `order_id`, `sender_id`)"
                    << "VALUES(?, NOW(), ?, ?)",
                use(_text),
                use(_order_id),
                use(_sender_id);

            insert.execute();

            Poco::Data::Statement select(session);
            select << "SELECT LAST_INSERT_ID()",
                into(_id),
                range(0, 1); //  iterate over result set one row at a time

            if (!select.done())
            {
                select.execute();
            }
            std::cout << "inserted: " << _id << std::endl;
            return true;
        }
        catch (Poco::Data::MySQL::ConnectionException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        catch (Poco::Data::MySQL::StatementException &e)
        {
            std::cout << e.displayText() << std::endl;
        }
        return false;
    }

    const long &Message::get_id() const
    {
        return _id;
    }

    const std::string &Message::get_text() const
    {
        return _text;
    }

    const std::string &Message::get_date() const
    {
        return _date;
    }

    const long &Message::get_order_id() const
    {
        return _order_id;
    }

    const long &Message::get_sender_id() const
    {
        return _sender_id;
    }


    long &Message::id()
    {
        return _id;
    }

    std::string &Message::text()
    {
        return _text;
    }

    std::string &Message::date()
    {
        return _date;
    }

    long &Message::order_id()
    {
        return _order_id;
    }

    long &Message::sender_id()
    {
        return _sender_id;
    }
}