#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "pk_test_51Iw0MLLhN7zBU7jHN0Ln3iX3fXQPvzfoYvhWk3NpPlJ0Rmpjrx8qhubSsBQFzzq4s7Njnd41uXfH7HMK3zVU30bE00kI0lEGIy";

string auth_token;
string user_id;
/*Disclaimer: always open gunrock_web server before doing anything*/
vector<string> command_lists;
void batchMode(string file);
void dcashcommands(string command);
vector<string> split(string str, string token);
void dcashprompt();
void errormessage();

int main(int argc, char *argv[])
{
  stringstream config;
  int fd = open("config.json", O_RDONLY);
  if (fd < 0)
  {
    cout << "could not open config.json" << endl;
    exit(1);
  }
  int ret;
  char buffer[4096];
  while ((ret = read(fd, buffer, sizeof(buffer))) > 0)
  {
    config << string(buffer, ret);
  }
  Document d;
  d.Parse(config.str());
  API_SERVER_PORT = d["api_server_port"].GetInt();
  API_SERVER_HOST = d["api_server_host"].GetString();
  PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();

  //ifstream dcashfile(argv[1]);

  // interactive and batch error test case
  if (argc > 2)
  {
    errormessage();
  }
  else if (argc == 2)
  {
    if (access(argv[1], F_OK) != -1)
    {
      batchMode(argv[1]);
    }
    else
    {
      errormessage();
    }
  }

  else
  {
    string command;
    while (1)
    {
      dcashprompt();
      getline(cin, command);

      if (command.empty() == 1)
      {
        errormessage();
      }
      else
      {
        dcashcommands(command);
      }
    }
  }
  return 0;
}

void batchMode(string file)
{
  string command;
  ifstream dcashfile(file);
  if (dcashfile.is_open())
  {
    // batch mode
    while (getline(dcashfile, command))
    {
      if (command.empty() == 1)
      {
        errormessage();
      }
      else
      {
        dcashcommands(command);
      }
    }
  }

  else
  {
    errormessage();
  }

  dcashfile.close();
}

void dcashprompt()
{
  cout << "D$> ";
}

void errormessage()
{
  cout << "Error\n";
}
vector<string> split(string str, string token)
{
  vector<string> result;
  while (str.size())
  {
    size_t index = str.find(token);
    if (index != string::npos)
    {
      result.push_back(str.substr(0, index));
      str = str.substr(index + token.size());
      if (str.size() == 0)
        result.push_back(str);
    }
    else
    {
      result.push_back(str);
      str = "";
    }
  }
  return result;
}

void dcashcommands(string command)
{

  if (command == " ")
  {
    errormessage();
  }

  command_lists = split(command, " "); //parsing commands

  if (command_lists[0].compare("auth") == 0)
  {
    string username = command_lists[1];
    string password = command_lists[2];
    string email = command_lists[3];
    // make a case that if one of the auth arguments is empty, return an error
    if (command_lists.size() != 4)
    {
      errormessage();
    }

    else if (command_lists.size() == 4)
    {

      // test case here for 3 and 4

      HttpClient client("localhost", API_SERVER_PORT, false);
      WwwFormEncodedDict body;
      body.set("username", username);
      body.set("password", password);
      body.set("email", email);

      string encoded_body = body.encode();

      HTTPClientResponse *client_response = client.post("/auth-tokens", encoded_body);
      if (!(client_response->success()))
      {
        errormessage();
      }
      else
      {
        Document *d = client_response->jsonBody();
        auth_token = (*d)["auth_token"].GetString();
        user_id = (*d)["user_id"].GetString();
        HttpClient Client_Id("localhost", API_SERVER_PORT, false);
        Client_Id.set_header("x-auth-token", auth_token);
        HTTPClientResponse *client_balance = Client_Id.put("/users/" + user_id, encoded_body); // so that it can update the email as well
        Document *b = client_balance->jsonBody();
        int balance = (*b)["balance"].GetInt();
        // Right format of balance
        printf("Balance: $%4.2f\n", (float)balance);
      }
    }
  }

  else if (command_lists[0].compare("balance") == 0)
  {
    if (command_lists.size() != 1)
    {
      errormessage();
    }
    HttpClient balanceclient("localhost", API_SERVER_PORT, false);

    balanceclient.set_header("x-auth-token", auth_token);

    HTTPClientResponse *client_balance_1 = balanceclient.get("/users/" + user_id);

    Document *b = client_balance_1->jsonBody();
    int balance1 = (*b)["balance"].GetInt();
    printf("Balance: $%4.2f\n", (float)balance1);
  }

  else if (command_lists[0].compare("deposit") == 0)
  {
    // make a case that if one of the auth arguments is empty, return an error
    if (command_lists.size() != 6)
    {
      errormessage();
    }
    else if (command_lists.size() == 6)
    {
      HttpClient depositclient("api.stripe.com", 443, true);
      depositclient.set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);

      int amount = stoi(command_lists[1]);
      string card_number = command_lists[2];
      string card_exp_month = command_lists[4];
      string card_exp_year = command_lists[3];
      string card_cvc = command_lists[5];

      WwwFormEncodedDict deposit_body;
      deposit_body.set("card[number]", card_number);
      deposit_body.set("card[exp_month]", card_exp_month);
      deposit_body.set("card[exp_year]", card_exp_year);
      deposit_body.set("card[cvc]", card_cvc);
      string deposit_encoded_body = deposit_body.encode();

      HTTPClientResponse *deposit_client_response = depositclient.post("/v1/tokens", deposit_encoded_body);

      Document *deposits_response = deposit_client_response->jsonBody();
      string token_id = (*deposits_response)["id"].GetString();
      delete deposits_response;

      HttpClient depositclient1("localhost", API_SERVER_PORT, false);
      depositclient1.set_header("x-auth-token", auth_token);

      WwwFormEncodedDict body2;
      body2.set("amount", amount);
      body2.set("stripe_token", token_id);
      string encoded_body_2 = body2.encode();

      HTTPClientResponse *depositclient1_response = depositclient1.post("/deposits", encoded_body_2);
      if (!(depositclient1_response->success()))
      {
        errormessage();
      }
      else
      {
        Document *deposit_account = depositclient1_response->jsonBody();
        int deposit_balance = (*deposit_account)["balance"].GetInt();
        printf("Balance: $%4.2f\n", (float)deposit_balance);
      }
    }
  }
  else if (command_lists[0].compare("send") == 0)
  {
    if (command_lists.size() != 2)
    {
      errormessage();
    }
    else if (command_lists.size() == 2)
    {
      HttpClient client_transfer("localhost", API_SERVER_PORT, false);
      client_transfer.set_header("x-auth-token", auth_token);
      string receiver = command_lists[1];
      int amount = stoi(command_lists[2]);

      WwwFormEncodedDict transfer_body;
      transfer_body.set("amount",amount);
      transfer_body.set("to",receiver);
      string transfer_body_encode=transfer_body.encode();
      HTTPClientResponse *client_transfer_response=client_transfer.post("/transfers",transfer_body_encode);
      if(!(client_transfer_response->success()))
      {
        errormessage();
      }
      else{
      Document *transfer_response= client_transfer_response->jsonBody();
      int transfer_balance=(*transfer_response)["balance"].GetInt();
      printf("Balance: $%4.2f\n", (float)transfer_balance);
      }
    }
  }
  else if (command_lists[0].compare("logout") == 0)
  {
    HttpClient client_del("localhost", API_SERVER_PORT, false);
    client_del.set_header("x-auth-token", auth_token);
    client_del.del("/auth-tokens/" + auth_token);
    exit(0);
  }
}
