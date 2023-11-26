#ifndef USEHANDLER_H
#define USEHANDLER_H

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>

using Poco::DateTimeFormat;
using Poco::DateTimeFormatter;
using Poco::ThreadPool;
using Poco::Timestamp;
using Poco::Net::HTMLForm;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::NameValueCollection;
using Poco::Net::ServerSocket;
using Poco::Util::Application;
using Poco::Util::HelpFormatter;
using Poco::Util::Option;
using Poco::Util::OptionCallback;
using Poco::Util::OptionSet;
using Poco::Util::ServerApplication;

#include "../../database/message.h"


static bool hasSubstr(const std::string &str, const std::string &substr)
{
    if (str.size() < substr.size())
        return false;
    for (size_t i = 0; i <= str.size() - substr.size(); ++i)
    {
        bool ok{true};
        for (size_t j = 0; ok && (j < substr.size()); ++j)
            ok = (str[i + j] == substr[j]);
        if (ok)
            return true;
    }
    return false;
}

class ServiceHandler : public HTTPRequestHandler
{
private:

    void badRequestError(HTTPServerResponse &response,  std::string instance)
    {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
        root->set("content", "/errors/bad_request");
        root->set("title", "Internal exception");
        root->set("text", "400");
        root->set("detail", "Недостаточно параметров в теле запроса");
        root->set("instance", instance);
        std::ostream &ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }

    void notFoundError(HTTPServerResponse &response, std::string instance, std::string message)
    {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
        root->set("content", "/errors/not_found");
        root->set("title", "Internal exception");
        root->set("text", "404");
        root->set("detail", message);
        root->set("instance", instance);
        std::ostream &ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }
    

public:
    ServiceHandler(const std::string &format) : _format(format)
    {
    }

    void handleRequest(HTTPServerRequest &request,
                       HTTPServerResponse &response)
    {

        HTMLForm form(request, request.stream());
        try
        {
            response.set("Access-Control-Allow-Origin", "*"); //для работы swagger

            if (hasSubstr(request.getURI(), "/message"))
            {
                if(request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET)
                {
                    if(form.has("order_id"))
                    {
                        std::string order_id_str = form.get("order_id");
                        long order_id = atol(order_id_str.c_str());
                        auto results = database::Message::read_by_order_id(order_id);
                        if (!results.empty())
                        {
                            Poco::JSON::Array arr;
                            for (auto s : results)
                                arr.add(s.toJSON());

                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            std::ostream &ostr = response.send();
                            Poco::JSON::Stringifier::stringify(arr, ostr);   

                            return;
                        }
                        else
                        {
                            notFoundError(response, request.getURI(), "Order id " + order_id_str + " not found");
                            return;
                        }
                    }
                    else
                    {
                        badRequestError(response, request.getURI());
                        return;
                    }
                    
                }
                else if(request.getMethod() == Poco::Net::HTTPRequest::HTTP_POST)
                {
                    if (form.has("text") && form.has("sender_id") && form.has("order_id"))
                    {
                        std::string error_msg = "Adding an message failed. ";
                        bool flag = true;
                        database::Message message;
                        message.text() = form.get("text");
                        //add check foreign key
                        message.sender_id() = atol(form.get("sender_id").c_str());
                        message.order_id() = atol(form.get("order_id").c_str());

                        if(!(flag = message.is_user_exist()))
                        {
                            error_msg += "Пользователь с таким id не существует!";
                        }

                        if (flag && message.save_to_mysql())
                        {                   
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                            root->set("inserted_id", message.id());
                            std::ostream &ostr = response.send();
                            Poco::JSON::Stringifier::stringify(root, ostr);

                            return;
                        }
                        else
                        {
                            notFoundError(response, request.getURI(), error_msg);
                            return;
                        }
                    }
                    else
                    {
                        badRequestError(response, request.getURI());
                        return;
                    }
                }
                else if(request.getMethod() == Poco::Net::HTTPRequest::HTTP_PUT)
                {
                    if(form.has("id") && form.has("text"))
                    {
                        std::string id_str = form.get("id");
                        long id = atol(id_str.c_str());
                        
                        std::optional<database::Message> message = database::Message::read_by_id(id);
                        if(message)
                        {
                            message->text() = form.get("text");
                            if(message->update_in_mysql())
                            {
                                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                                response.setChunkedTransferEncoding(true);
                                response.setContentType("application/json");
                                Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                                root->set("updated", id_str);
                                std::ostream &ostr = response.send();
                                Poco::JSON::Stringifier::stringify(root, ostr);

                                return;
                            }
                            else
                            {
                                badRequestError(response, request.getURI());
                                return;
                            }               
                        }
                        else
                        {
                            notFoundError(response, request.getURI(), "Message id " + id_str + " not found");
                            return;
                        }
                    }
                    else
                    {
                        badRequestError(response, request.getURI());
                        return;
                    }
                }
                
            }
            
        }
        catch (...)//(const Poco::Exception& e)
        {
            //std::cout<<e.displayText()<<std::endl;
        }
        notFoundError(response, request.getURI(), "Request not found");
    }

private:
    std::string _format;
};
#endif