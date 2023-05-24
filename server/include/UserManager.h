#pragma once

#include "User.h"
#include <iostream>
#include <map>
using namespace std;

#define NOERROR 0

typedef int ErrorCode;

class UserManager
{
public:
    static const int MAX_USER_NUM = 50;
    UserManager();
    ~UserManager();

    bool Login(string username);
    bool Logout();
    User* GetUser();
    User* user;

public:
    map<pthread_t, int> user_addr;
    User* p_users[MAX_USER_NUM];
};