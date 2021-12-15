#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

void PrintAccountResponse(string &email, int &balance, HTTPResponse *response);

AccountService::AccountService() : HttpService("/users")
{
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response)
{
    /*Fetches the user object for this account. You can only fetch the user
object for a the account that you authenticated.*/

    //this is for require authentication
    User *Authenticated_user = getAuthenticatedUser(request);
    vector<string>User_Id=request->getPathComponents();

   if(User_Id.back()!=Authenticated_user->user_id)
   {
       throw ClientError::unauthorized();
   }
    
    if (Authenticated_user == NULL) // if auth token doesnt found in database
    {
        throw ClientError::unauthorized();
    }
    string auth_token = request->getAuthToken();
    User *User_account = m_db->auth_tokens.at(auth_token);
    PrintAccountResponse(User_account->email, User_account->balance, response);
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response)
{
    User *Authenticated_user = getAuthenticatedUser(request);
    vector<string>User_Id=request->getPathComponents();

   if(User_Id.back()!=Authenticated_user->user_id)
   {
       throw ClientError::unauthorized();
   }

    if (Authenticated_user == NULL)
    {
        throw ClientError::unauthorized();
    }

    WwwFormEncodedDict account_info = request->formEncodedBody();
    string email;
    string balance;
    balance = account_info.get("amount");
    email = account_info.get("email");

    string auth_token = request->getAuthToken();
    User *User_account = m_db->auth_tokens.at(auth_token);
    User_account->email = email;

    if (User_account->email == "")
    {
        throw ClientError::unauthorized();
    }
    
    PrintAccountResponse(User_account->email, User_account->balance, response);
}

void PrintAccountResponse(string &email, int &balance, HTTPResponse *response)
{
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();
    o.AddMember("balance", balance, a);
    o.AddMember("email", email, a);
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}
