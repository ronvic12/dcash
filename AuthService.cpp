#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

void PrintResponse(string &auth_token, string &user_id, HTTPResponse *response); // prototype
AuthService::AuthService() : HttpService("/auth-tokens")
{
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response)
{

    WwwFormEncodedDict account_info = request->formEncodedBody();

    string username, password;
    username = account_info.get("username");
    password = account_info.get("password");
    bool hasUpper = false;
    for (size_t i = 0; i < username.length(); i++)
    {
        if (isupper(username[i]) > 0)
        {
            hasUpper = true;
            break;
        }
    }

    if (hasUpper == true)
    {
        throw ClientError::unauthorized();
    }
    // if account doesnt exist, create an account

    if (m_db->users.find(username) == m_db->users.end())
    {

        // create  user account

        User *User_account = new User();
        // make an edge case here. username must be lower case

        User_account->username = username;
        User_account->password = password;
        User_account->balance = 0;
        User_account->email = "";
        // store it into database
        m_db->users.insert(pair<string, User *>(User_account->username, User_account));
        // making user id
        StringUtils *create_ID_AND_Tokens = new StringUtils();
        User_account->user_id = create_ID_AND_Tokens->createUserId();
        // authorization for the user
        string Auth_Token = create_ID_AND_Tokens->createAuthToken();
        m_db->auth_tokens.insert(pair<string, User *>(Auth_Token, User_account));       
        PrintResponse(Auth_Token, User_account->user_id, response);
        response->setStatus(201);
    }

    // if there is a user account or user account exists, user_id is stable.
    else if (m_db->users.find(username) != m_db->users.end())
    {
        // if password doesnt match, then its an error
        if (m_db->users.at(username)->password != password)
        {
            throw ClientError::unauthorized(); 
        }
        // if password match, create a new Auth token.

        else
        {
            User *User_account = m_db->users.at(username);
            StringUtils *create_ID_AND_Tokens = new StringUtils();
            string Auth_Token = create_ID_AND_Tokens->createAuthToken();
            m_db->auth_tokens.insert(pair<string, User *>(Auth_Token, User_account));
            delete create_ID_AND_Tokens;
            // if u have an existing user, use the same auth_token that is given to you. not sure
            PrintResponse(Auth_Token, User_account->user_id, response);
            response->setStatus(200);
        }
    }
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response)
{
    //check this back later.
    string auth_token = request->getAuthToken();
    if (m_db->auth_tokens.at(auth_token)==m_db->auth_tokens.at(request->getPathComponents()[1]))
    {
        string Auth_Token = request->getPathComponents()[1]; // delete from the url one
        m_db->auth_tokens.erase(Auth_Token);
    }
}

// json file that returns an object
void PrintResponse(string &auth_token, string &user_id, HTTPResponse *response)
{
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();
    o.AddMember("auth_token", auth_token, a);
    o.AddMember("user_id", user_id, a);
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}
