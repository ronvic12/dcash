#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") {}

void DepositResponse(int &balance, Deposit *to, HTTPResponse *response);
void DepositService::post(HTTPRequest *request, HTTPResponse *response)
{

    User *Authenticated_user = getAuthenticatedUser(request);

    if (Authenticated_user == NULL)
    {
        throw ClientError::unauthorized();
    }

    HttpClient client("api.stripe.com", 443, true);

    client.set_basic_auth(m_db->stripe_secret_key, "");

    string amount, stripe_token, token, to, str;

    WwwFormEncodedDict account_info = request->formEncodedBody();
    amount = account_info.get("amount");
    stripe_token = account_info.get("stripe_token");

    // if it is an w/o stripe token then it is an error
    if (amount.empty())
    {
        throw ClientError::badRequest();
    }
    if (stoi(amount) < 50 || stoi(amount) < 0)
    {
        throw ClientError::badRequest();
    }

    if (stripe_token.empty())
    {
        throw ClientError::unauthorized();
    }

    if (stripe_token != "tok_visa")
    {
        throw ClientError::unauthorized();
    }

    WwwFormEncodedDict body;
    body.set("amount", stoi(amount));
    body.set("currency", "usd");
    body.set("source", stripe_token);

    string encoded_body = body.encode(); // encoding the whole body

    HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);
    Document *d = client_response->jsonBody();
    string charge_id = (*d)["id"].GetString();
    delete d;

    if (client_response->success())
    {
        Deposit *Deposit_account = new Deposit();
        Deposit_account->stripe_charge_id = charge_id;
        Deposit_account->amount = stoi(amount);
        Deposit_account->to = getAuthenticatedUser(request);
        m_db->deposits.push_back(Deposit_account);
        Deposit *Deposit_account_array;

        Document document;
        Document::AllocatorType &a = document.GetAllocator();
        Value o;
        o.SetObject();
        o.AddMember("balance", Authenticated_user->balance += stoi(amount), a);
        Value arrays;
        arrays.SetArray();
        for (unsigned int i = 0; i < m_db->deposits.size(); i++)
        {
            Deposit_account_array = m_db->deposits[i];
            if (m_db->deposits[i]->to == Deposit_account->to)
            {
                Value d;
                d.SetObject();
                d.AddMember("to", Deposit_account_array->to->username, a);
                d.AddMember("amount", Deposit_account_array->amount, a);
                d.AddMember("stripe_charge_id", Deposit_account_array->stripe_charge_id, a);
                arrays.PushBack(d, a);
            }
        }
        o.AddMember("deposits", arrays, a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);
        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
    }
    else if (!(client_response->success()))
    {
        throw ClientError::badRequest();
    }
}
