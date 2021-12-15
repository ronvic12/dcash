#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include "HttpService.h"
#include "ClientError.h"

using namespace std;

HttpService::HttpService(string pathPrefix)
{
  this->m_pathPrefix = pathPrefix;
}

User *HttpService::getAuthenticatedUser(HTTPRequest *request)
{
  // TODO: implement this function
  bool has_auth_token = request->hasAuthToken();
  string get_auth_token = request->getAuthToken();
  if (has_auth_token == false)
  {
    throw ClientError::badRequest();
  }

    if (get_auth_token == " ") // if token is empty. returns an error
    {
      return NULL;
    }

    // if you  cant find auth token in data base, returns an error
    if (m_db->auth_tokens.find(get_auth_token) == m_db->auth_tokens.end())
    {
      return NULL;
    }
  

  User *User_account = m_db->auth_tokens.at(get_auth_token);
  return User_account;
}

string HttpService::pathPrefix()
{
  return m_pathPrefix;
}

void HttpService::head(HTTPRequest *request, HTTPResponse *response)
{
  cout << "HEAD " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::get(HTTPRequest *request, HTTPResponse *response)
{
  cout << "GET " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::put(HTTPRequest *request, HTTPResponse *response)
{
  cout << "PUT " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::post(HTTPRequest *request, HTTPResponse *response)
{
  cout << "POST " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::del(HTTPRequest *request, HTTPResponse *response)
{
  cout << "DELETE " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}
