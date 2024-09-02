#include "sentiment.hpp" // contains all headers and relevant function definitions.

using json = nlohmann::json;
using namespace std;
using namespace pqxx;

// ------------------------------ DEFINES ---------------------------------\
// we should probably read these from the website eventually.
#define START_DAY 5
#define START_MONTH 6
#define START_YEAR 2024
#define LIMIT_POSTS 5 // max number of posts we can run in a given day 
#define DAYS 1


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    // explanation of this v complicated line of code:
    // we cast userp to a string point.
    // Appending on a string (char * ), with size * nmemb is fixed product.
    //
    ((string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// get the reddit access token for my account
string getRedditAccessToken(const string &clientId, const string &clientSecret, const string &username, const string &password)
{
    CURL *curl;
    CURLcode res; // store the code returned by the result of CURL, denoting whether or not something was successful or not.
    string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.reddit.com/api/v1/access_token");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // print username/password for verification purposes
        cout << "Desired Username: " << username << " Desired Pass: " << password << "\n";
        string postFields = "grant_type=password&username=" + username + "&password=" + password;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

        string userCredentials = clientId + ":" + clientSecret;
        cout << "Client ID/Secret are as follows: " << userCredentials << "\n";
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "User-Agent: StockSentimentCpp-0.1");

        // adjust curl options
        curl_easy_setopt(curl, CURLOPT_USERPWD, userCredentials.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    return readBuffer;
}

// Fetch posts from Reddit WallStreetBets THIS IS A DEPRECATED FUCNTION DO NOT USE
void fetchRedditPosts(const string &subreddit, const string &accessToken, vector<post>& storage)
{
    CURL *curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        cout << "Fetching Reddit posts!\n";
        string url = "https://oauth.reddit.com/r/" + subreddit + "/new?limit=1";
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        headers = curl_slist_append(headers, "User-Agent: StockSentimentCpp-0.1");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    // after reading, can we please select batch
    try
    {

        json jsonData = json::parse(readBuffer); // Parse the JSON data
        // cout << jsonData << "\n";
        // Iterate through each post in the JSON data
        for (const auto &postJson : jsonData["data"]["children"])
        {
            post newPost; // Create a default-constructed post object
            // Assign JSON values to post object fields
            newPost.post_data = postJson["data"]["selftext"];
            cout << newPost.post_data << "\n";
            // newPost.date = postJson["data"]["created_utc"].get<string>(); // Assuming `created_utc` is formatted as a string or converting it accordingly
            cout << newPost.date << "\n";
            newPost.stock_ticker = postJson["data"]["title"]; // Example: extracting the stock ticker from the title
            cout << newPost.stock_ticker << "\n";
            newPost.post_title = postJson["data"]["title"];
            newPost.rating = 0; // Default value, could be set based on other logic
            // Add the constructed post object to the storage vector
            storage.push_back(newPost);
        }
    }
    catch (const json::exception &e)
    {
        cerr << "Error parsing JSON: " << e.what() << endl;
    }
    //return readBuffer;
}

// Parse and print Reddit posts
void parseAndPrintRedditPosts(const string &gptAPI, vector<post>& storage)
{
    try
    {
        // Parse the JSON string
        // json jsonData = json::parse(jsonStr);
        // Initialize OpenAI API
        liboai::OpenAI oai;
        if (!oai.auth.SetKey(gptAPI))
        {
            cerr << "Failed to set the API key." << endl;
            return;
        }
        liboai::Conversation convo;
        // Add a system directive as the first user data
        if (!convo.AddUserData("I would like you to extract the stock symbol from this post/image, along with a numerical score between -10 and 10 for how positively it refers to the stock. If there are multiple stocks, only operate on the most important. Be very concise. Format responses as 'NKE -8' or 'TSLA 3' or 'N/A -101' if there isn't a relevant stock ticker.")){
            cerr << "Failed to add system directive to conversation." << endl;
            return;
        }
        // Iterate over each post
        stringstream ss;
        for (auto &post : storage)
        {
            string title = post.post_title;// post["data"]["title"];
            string data = post.post_data;//post["data"]["selftext"];
            // Add post title and data to the conversation
            if (!convo.AddUserData(title + " " + data))
            {
                cerr << "Failed to add post data to conversation." << endl;
                continue; // Skip to the next post
            }
            sleep(1);
            try
            {
                // Get response from OpenAI
                liboai::Response response = oai.ChatCompletion->create(
                    "gpt-3.5-turbo", convo); // this is cacnerous

                // Update our conversation with the response
                if (!convo.Update(response))
                {
                    // TODO: read all of these strings into a dictionary due to their hopefully consistent formatting. 
                    cerr << "Failed to update conversation with response." << endl;
                }
                else
                {
                    // Print the response
                    ss.clear();
                    ss.str("");
                    string temp;
                    cout << "Bot: " << convo.GetLastResponse() << endl;
                    ss.str(convo.GetLastResponse());
                    ss >> post.stock_ticker;
                    ss >> temp;
                    post.rating = stoi(temp);

                }
            }
            catch (const exception &e)
            {
                cerr << "Error: " << e.what() << endl;
            }

            // Print the original post
            cout << "Post: " << title << endl;
            cout << "Data: " << data << endl;
        }
    }
    catch (const json::parse_error &e)
    {
        cerr << "Failed to parse JSON: " << e.what() << endl;
    }
}

// Function to read secrets from a file
void readSecrets(string &clientId, string &clientSecret, string &username, string &password, string &gptAPI, string &polygonAPI, string &dbpassword, string &dbpassword_new)
{
    ifstream file("./../.env");
    string line;

    if (!file.is_open())
    {
        cerr << "Failed to open secrets.txt" << endl;
        return;
    }

    while (getline(file, line))
    {
        istringstream iss(line);
        string key, value;
        if (getline(iss, key, '=') && getline(iss, value))
        {
            if (key == "CLIENT_ID")
                clientId = value;
            else if (key == "CLIENT_SECRET")
                clientSecret = value;
            else if (key == "REDDIT_USERNAME")
                username = value;
            else if (key == "REDDIT_PASSWORD")
                password = value;
            else if (key == "GPT_API")
                gptAPI = value;
            else if (key == "POLYGON_API")
                polygonAPI = value;
            else if (key == "DB_PASSWORD")
                dbpassword = value;
            else if (key == "NEW_DB_PASSWORD")
                dbpassword_new = value;
        }
    }
}

// Function to convert tm structure to Unix timestamp
time_t convertToUnixTimestamp(const tm &timeStruct)
{
    tm temp = timeStruct;
    temp.tm_isdst = -1; // Not considering daylight saving time
    return mktime(&temp);
}

// The mapping is as follows:
// dates -> posts -> API-processed posts -> prices // holy fweak each one of these is an enormous separate step.
bool isHoliday(const tm& date) {
    static const unordered_set<string> holidays = {
        "2023-01-01", // New Year's Day
        "2023-07-04", // Independence Day
        "2023-12-25", // Christmas Day
        // Other holidays here: 
    };
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &date);
    return holidays.find(buffer) != holidays.end();
}

// Function to get the next date from a tm structure
tm getNextDate(const tm& date) {
    tm nextDate = date;
    time_t nextTime = mktime(&nextDate) + 86400; // Add one day (86400 seconds)
    return *gmtime(&nextTime);
}

// Function to convert tm structure to Unix timestamp as a string
string convertToUnixTimestampString(const tm& timeStruct) {
    return to_string(convertToUnixTimestamp(timeStruct));
}

vector<tm> defineDates(int start_year, int start_month, int start_day, int data_points) {
    vector<tm> dates;
    tm start_date = {};
    start_date.tm_year = start_year - 1900; // Year since 1900
    start_date.tm_mon = start_month - 1;    // Month since January (0-11)
    start_date.tm_mday = start_day;         // Day of the month (1-31)
       for (int i = 0; i < data_points;) {
        // Calculate wday
        time_t temp_time = mktime(&start_date);
        tm* temp_tm = localtime(&temp_time);
        start_date.tm_wday = temp_tm->tm_wday;

        // Skip weekends and holidays
        if (start_date.tm_wday == 0 || start_date.tm_wday == 6 || isHoliday(start_date)) {
            start_date = getNextDate(start_date);
            continue; // Do not increment i
        } else {
            dates.push_back(start_date);
            start_date = getNextDate(start_date);
            i++; // Only increment i when a valid date is added
        }
    }
    cout << "Retrieving data from the following days: \n";
    for (const auto& date : dates) {
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &date);
        cout << buffer << "\n";
    }
    return dates;
}

// This is just fetching raw posts; maybe we'll implement saving training data for later:
string fetchPosts(const string& subreddit, const string& accessToken, int limit, vector<post>& storage) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        string url = "https://oauth.reddit.com/r/" + subreddit + "/new?limit=" + to_string(limit);
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
        headers = curl_slist_append(headers, "User-Agent: StockSentimentCpp-0.1");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    try
    {
        json jsonData = json::parse(readBuffer); // Parse the JSON data

        // Iterate through each post in the JSON data
        for (const auto &postJson : jsonData["data"]["children"])
        {
            post newPost; // Create a default-constructed post object

            // Assign JSON values to post object fields
            newPost.post_data = postJson["data"]["selftext"];
            newPost.date = postJson["data"]["created_utc"]; // Assuming `created_utc` is formatted as a string or converting it accordingly
            // newPost.stock_ticker = postJson["data"]["title"].get<string>(); // Example: extracting the stock ticker from the title
            newPost.post_title = postJson["data"]["title"];
            newPost.rating = 0; // Default value, could be set based on other logic
            // Add the constructed post object to the storage vector
            storage.push_back(newPost);
        }
    }
    catch (const json::exception &e)
    {
        cerr << "Error parsing JSON: " << e.what() << endl;
    }
    return readBuffer;
}

// Function to fetch the posts from a set of given dates on this sub-reddit.
string fetchPostsFromSpecificDates(const string& subreddit, const string& accessToken, const vector<tm>& dates) {
    CURL* curl;
    CURLcode res;
    string allReadBuffer;

    for (const auto& date : dates) {
        string readBuffer;

        // Calculate the start and end timestamps for the desired date
        tm startOfDay = date;
        startOfDay.tm_hour = 0;
        startOfDay.tm_min = 0;
        startOfDay.tm_sec = 0;
        time_t startTimestamp = convertToUnixTimestamp(startOfDay);

        tm endOfDay = date;
        endOfDay.tm_hour = 23;
        endOfDay.tm_min = 59;
        endOfDay.tm_sec = 59;
        time_t endTimestamp = convertToUnixTimestamp(endOfDay);

        curl = curl_easy_init();
        if (curl) {
            string url = "https://oauth.reddit.com/r/" + subreddit + "/search?limit=" + to_string(LIMIT_POSTS) + "&restrict_sr=1&q=timestamp:" 
                          + to_string(startTimestamp) + ".." + to_string(endTimestamp) + "&sort=new";
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
            headers = curl_slist_append(headers, "User-Agent: StockSentimentCpp-0.1");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                cout << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            } else {
                allReadBuffer += readBuffer; // Append the result to the main buffer
            }
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
    }
    return allReadBuffer;
}

// Function to fetch stock price data from Yahoo Finance
string fetch_stock_data(const string &ticker)
{
    string readBuffer;
    string url = "https://query1.finance.yahoo.com/v7/finance/quote?symbols=" + ticker;

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return readBuffer;
}

// Establish a connection to the database and create other database 
bool establish_connection(const string& conn_str, const string& fallback_conn_str, const string& db_password_new) {
    try {
        // Establish a connection to the PostgreSQL database using the primary connection string
        pqxx::connection conn(conn_str);

        if (conn.is_open()) {
            cout << "Connected to database: " << conn.dbname() << endl;
        } else {
            cerr << "Failed to connect to the database with primary connection string." << endl;
            throw runtime_error("Primary connection failed. Trying fallback connection...");
        }

        // Create a transactional object
        work txn(conn);

        // Execute a simple SQL query
        result res = txn.exec("SELECT version()");

        // Print the query results
        cout << "PostgreSQL version: " << res[0][0].c_str() << endl;

        // Commit the transaction
        txn.commit();

        // Close the connection
        conn.disconnect();
        return true;

    } catch (const exception& e) {
        stringstream cmd_builder;
        cerr << "Primary connection attempt failed: " << e.what() << endl;
        try {
            // Establish a connection using the fallback connection string
            connection fallback_conn(fallback_conn_str);

            if (fallback_conn.is_open()) {
                cout << "Connected to database (fallback): " << fallback_conn.dbname() << endl;
            } else {
                cerr << "Failed to connect to the database with fallback connection string." << endl;
                return false;
            }

            // Create a transactional object
           

            // Execute SQL statements to create a new database and a user with privileges
            try {
                // Create a new database
                nontransaction ntxn(fallback_conn);
                ntxn.exec("CREATE DATABASE post_storage;");
                ntxn.commit();
                work txn(fallback_conn);
                cout << "Database 'post_storage' created successfully." << endl;

                // Create a new user (admin) with a login and password
                
                cmd_builder << "CREATE ROLE admin_user WITH LOGIN PASSWORD '" << db_password_new << "';";
                txn.exec(cmd_builder.str());
                cout << "User 'admin_user' created successfully." << endl;

                // Grant all privileges on the 'post_storage' database to the new user
                txn.exec("GRANT ALL PRIVILEGES ON DATABASE post_storage TO admin_user;");
                cout << "Granted all privileges on 'post_storage' to 'admin_user'." << endl;

                // Commit the transaction
                txn.commit();
                cout << "All changes committed successfully." << endl;
            } catch (const std::exception &e) {
                cerr << "Error during SQL execution: " << e.what() << endl;
                return false;
            }

            // Close the connection
            fallback_conn.disconnect();
            return true;

        } catch (const exception& fallback_e) {
            cerr << "Fallback connection attempt failed: " << fallback_e.what() << endl;
            return false;
        }
    }
}

bool createTables(const string& conn_str){
    try {
        // Connect to the PostgreSQL database
        pqxx::connection C(conn_str);
        if (C.is_open()) {
            cout << "Opened database successfully: " << C.dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
            return false;
        }

        // Create a table with a unique constraint
        try {
            pqxx::work W(C);  // Start a transaction

            string sql = "DROP TABLE IF EXISTS posts; "
                "CREATE TABLE IF NOT EXISTS posts ("
                "POST_ID SERIAL PRIMARY KEY, "
                "Stock_Ticker TEXT, "
                "Rating INT, "
                "Title TEXT UNIQUE, "
                "Data TEXT UNIQUE, "
                "Date TIMESTAMP, "  // Change this to TIMESTAMP
                "VW_One_Day_Out INT, "
                "VW_One_Week_Out INT, "
                "VW_One_Month_Out INT);";

            W.exec(sql);  // Execute the SQL command
            W.commit();   // Commit the transaction
            cout << "Table created successfully." << endl;
        } catch (const exception &e) {
            cerr << e.what() << endl;
            return false;
        }

        // Close the database connection
        C.disconnect();
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return false;
    }
    return true;


}
/**
 * @brief Main function that calls all of the various helper functions above.
 * 
 * @return int
 */
int main()
{
    // This is used for storing the various posts before they get transferred to the database
    vector<post> storage;  
    // Various authentication information
    string clientId, clientSecret, username, password, gptAPI, polygonAPI, dbpassword, dbpassword_new;
    readSecrets(clientId, clientSecret, username, password, gptAPI, polygonAPI, dbpassword, dbpassword_new);

    // Retrieve access token.
    string accessTokenJson = getRedditAccessToken(clientId, clientSecret, username, password);
    // cout << "Access Token Response: " << accessTokenJson << endl;

    // Parse JSON response to extract the actual access token
    json accessTokenData = json::parse(accessTokenJson);
    string accessToken = accessTokenData["access_token"];

    // ------------------DATA RETRIEVAL ------------------
    // start_year, start_month, start_day, number of days from start date onwards to iterate over. 
    string subreddit = "wallstreetbets";
    // string postsJson = fetchRedditPosts(subreddit, accessToken);
    // string postsJson = fetchPostsFromSpecificDates(subreddit, accessToken, dates);
    string postsJson = fetchPosts(subreddit, accessToken, LIMIT_POSTS, storage);
    //
    parseAndPrintRedditPosts(gptAPI, storage);
    for (const auto& elem : storage){
        cout << elem << "\n";
    }
    
    // ------------------DATA STORAGE --------------------
    stringstream conn_stream;
    conn_stream << "dbname=post_storage "
                << "user=admin_user "
                << "password=" << dbpassword_new << " "
                << "host=my_postgres_db "
                << "port=5432";
    string conn_str = conn_stream.str();
    stringstream fallback_stream;
    fallback_stream << "dbname=postgres "
                    << "user=postgres "
                    << "password=" << dbpassword << " "
                    << "host=my_postgres_db "
                    << "port=5432";
    string fallback_conn_str = fallback_stream.str();
    establish_connection(conn_str, fallback_conn_str, dbpassword_new);
    createTables(conn_str);
    // createTables
    // try {
    //     // Establish a connection to the PostgreSQL database
       
    //     pqxx::connection conn(conn_str);

    //     if (conn.is_open()) {
    //         cout << "Connected to database: " << conn.dbname() << endl;
    //     } else {
    //         cout << "Failed to connect to the database." << endl;
    //         return 1;
    //     }

    //     // Create a transactional object
    //     pqxx::work txn(conn);

    //     // Execute a simple SQL query
    //     pqxx::result res = txn.exec("SELECT version()");
        
    //     // Print the query results
    //     cout << "PostgreSQL version: " << res[0][0].c_str() << endl;

    //     // Commit the transaction
    //     txn.commit();

    //     // Close the connection
    //     conn.disconnect();
    // } catch (const exception &e) {
    //     cerr << e.what() << endl;
    //     return 1;
    // }
    return 0;
}