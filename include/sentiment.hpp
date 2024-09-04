#pragma once // this tells the c++ compiler that things should only be compiled once.
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <map>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <regex>
#include <unordered_map>
#include <sstream>
#include <liboai.h>
#include <unistd.h>
#include <unordered_set>
#include <pqxx/pqxx> 
#include <vector>
#include <ostream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <set>
#include <aws/core/Aws.h>
#include <aws/secretsmanager/SecretsManagerClient.h>
#include <aws/secretsmanager/model/GetSecretValueRequest.h>
using json = nlohmann::json;
using namespace std;

struct post {
    string post_title;
    string post_data;
    long long date;
    string stock_ticker;
    int rating;
    int postID;
    double VW_one_day;
    double VW_two_days;
    double VW_one_week;
    double VW_one_month;
    // int P_VW_one_day; this is the current price we use as a delta relative to everything else.
    double P_VW_two_days;
    double P_VW_one_week;
    double P_VW_one_month;
    // Default constructor
    post() : post_title(""), post_data(""), date(0), stock_ticker(""), rating(0),  postID(0), VW_one_day(0), VW_two_days(0), VW_one_week(0), VW_one_month(0), P_VW_two_days(0), P_VW_one_week(0), P_VW_one_month(0){}
    // Constructor based on JSON string to initialize member variables
    post(const string& jsonStr) {
        try {
            json jsonData = json::parse(jsonStr);
            post_title = jsonData["post_title"];
            post_data = jsonData["post_data"].get<string>();
            date = jsonData["date"];
            stock_ticker = jsonData["stock_ticker"].get<string>();
            rating = jsonData["rating"].get<long long>();
            postID = 0;
        } catch (const json::exception& e) {
            cerr << "Error parsing JSON: " << e.what() << endl;
        }
    }

    friend ostream& operator<<(ostream& os, const post& inp) {
        os << "Post ID: " << inp.postID << endl;
        os << "Title: " << inp.post_title << endl;
        os << "Data: " << inp.post_data << endl;
        os << "Date: " << inp.date << endl;
        os << "Stock Ticker: " << inp.stock_ticker << endl;
        os << "Rating: " << inp.rating << endl;
        os << "Volume-Weighted Price One Day After Post: " << inp.VW_one_day << endl;
        os << "Volume-Weighted Price Two Days After Post: " << inp.VW_two_days << endl;
        os << "Volume-Weighted Price One Week Out: " << inp.VW_one_week << endl;
        os << "Volume-Weighted Price One Month Out: " << inp.VW_one_month << endl;
        os << "Predicted Volume-Weighted Price Two Days Out: " << inp.P_VW_two_days << endl;
        os << "Predicted Volume-Weighted Price One Week Out: " << inp.P_VW_one_week << endl;
        os << "Predicted Volume-Weighted Price One Month Out: " << inp.P_VW_one_month << endl;
        return os; // Return the stream to allow chaining
    }
    // Converts the struct to a formatted JSON string for output or other uses
    string convertToJSON() const {
        json jsonData;
        jsonData["post_data"] = post_data;
        jsonData["date"] = date;
        jsonData["stock_ticker"] = stock_ticker;
        jsonData["rating"] = rating;
        jsonData["post_title"] = post_title;
        return jsonData.dump(4); // 4 spaces for pretty-printing
    }
};

/**
 * @brief Write a callback ?? apparently this is like an event noticer, kind of like Accept in C, except without a "while true" loop.
 * 
 * @param contents 
 * @param size 
 * @param nmemb 
 * @param userp 
 * @return size_t 
 */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

/**
 * @brief This is a string that fetches tweets with the API.
 * @param query 
 * @param bearerToken Token for calling the API. Should we publish to github? Surely not, let's keep it in a gitignore.
 * @return std::string Returns a string of tweets based on the query.
 */
void fetchRedditPosts(const std::string& subreddit, const std::string& accessToken, std::vector<post>& storage);

/**
 * @brief Parses the input jsonString, and then prints it to screen.
 * 
 * @param jsonStr This is a string jsonStr that denotes the input.
 */
void parseAndPrintRedditPosts(const std::string& gptAPI, std::vector<post>& storage);


/**
 * @brief Parses the raw secrets in the .env file, updating all of the corresponding inputs with the appropriate values.
 */
void readSecrets(string &clientId, string &clientSecret, string &username, string &password, string &gptAPI, string &polygonAPI, string &dbpassword, string &dbpassword_new);