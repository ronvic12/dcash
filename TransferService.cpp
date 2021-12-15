#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") {}

void TransferService::post(HTTPRequest *request, HTTPResponse *response)
{
    User *Authenticated_user = getAuthenticatedUser(request);

    if (Authenticated_user == NULL)
    {
        throw ClientError::unauthorized();
    }

    WwwFormEncodedDict account_info = request->formEncodedBody();

    string to_username, amount;
    to_username = account_info.get("to");
    amount = account_info.get("amount");

    if (stoi(amount) < 50 || stoi(amount) < 0)
    {
        throw ClientError::badRequest();
    }
    int balance = Authenticated_user->balance -= stoi(amount);
    if (balance < 0)
    {
        throw ClientError::unauthorized();
    }

    // new transaction.
    User *sender = getAuthenticatedUser(request); //sender
    sender->balance -= stoi(amount);
    User *reciever = m_db->users.find(to_username)->second; //reciever
    if (m_db->users.find(to_username) == m_db->users.end())
    {
        throw ClientError::unauthorized();
    }
    reciever->balance += stoi(amount);
    Transfer *Transfer_money = new Transfer();
    Transfer_money->amount = stoi(amount); // need this to print in response
    Transfer_money->from = sender;
    Transfer_money->to = reciever; // get the reciever username
    m_db->transfers.push_back(Transfer_money);

    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();
    o.AddMember("balance", balance, a);
    // arrays here
    Value transfer_array;
    transfer_array.SetArray();
    for (unsigned int i = 0; i < m_db->transfers.size(); i++)
    {
        Transfer *Transfer_account_array = m_db->transfers[i];
        if (getAuthenticatedUser(request)->user_id == Transfer_money->from->user_id || getAuthenticatedUser(request)->user_id == Transfer_money->to->user_id)
        {
            // add an object to our array
            Value transfer;
            transfer.SetObject();
            transfer.AddMember("from", Transfer_account_array->from->username, a);
            transfer.AddMember("to", Transfer_account_array->to->username, a);
            transfer.AddMember("amount", Transfer_account_array->amount, a);
            transfer_array.PushBack(transfer, a);
        }
        else if (getAuthenticatedUser(request)->user_id != Transfer_money->from->user_id || getAuthenticatedUser(request)->user_id != Transfer_money->to->user_id)
        {
            throw ClientError::unauthorized();
        }
    }
    o.AddMember("transfers", transfer_array, a);
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}

