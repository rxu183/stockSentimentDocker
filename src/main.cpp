#include "sentiment.hpp" // contains all headers and relevant function definitions.
using json = nlohmann::json;
using namespace std;
using namespace pqxx;

//------------------------------ DEFINES ---------------------------------

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
    ifstream file("/workspace/.env");
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

// vector<tm> defineDates(int start_year, int start_month, int start_day, int data_points) {
//     vector<tm> dates;
//     tm start_date = {};
//     start_date.tm_year = start_year - 1900; // Year since 1900
//     start_date.tm_mon = start_month - 1;    // Month since January (0-11)
//     start_date.tm_mday = start_day;         // Day of the month (1-31)
//        for (int i = 0; i < data_points;) {
//         // Calculate wday
//         time_t temp_time = mktime(&start_date);
//         tm* temp_tm = localtime(&temp_time);
//         start_date.tm_wday = temp_tm->tm_wday;

//         // Skip weekends and holidays
//         if (start_date.tm_wday == 0 || start_date.tm_wday == 6 || isHoliday(start_date)) {
//             start_date = getNextDate(start_date);
//             continue; // Do not increment i
//         } else {
//             dates.push_back(start_date);
//             start_date = getNextDate(start_date);
//             i++; // Only increment i when a valid date is added
//         }
//     }
//     cout << "Retrieving data from the following days: \n";
//     for (const auto& date : dates) {
//         char buffer[32];
//         strftime(buffer, sizeof(buffer), "%Y-%m-%d", &date);
//         cout << buffer << "\n";
//     }
//     return dates;
// }

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
bool establishConnection(const string& conn_str, const string& fallback_conn_str, const string& db_password_new) {
    try {
        // Establish a connection to the PostgreSQL database using the primary connection string
        connection conn(conn_str);

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
            } catch (const exception &e) {
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
        connection C(conn_str);
        if (C.is_open()) {
            cout << "Opened database successfully: " << C.dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
            return false;
        }
        // Create a table with a unique constraint
        try {
            work W(C);  // Start a transaction
            string sql = //"DROP TABLE IF EXISTS posts; "
                "CREATE TABLE IF NOT EXISTS posts ("
                "POST_ID SERIAL PRIMARY KEY, "
                "Title TEXT UNIQUE, "
                "Stock_Ticker TEXT, "
                "Rating INT, "
                "Data TEXT, "
                "Date TIMESTAMP, "
                "P_VW_Two_Days_Out NUMERIC(10, 2) DEFAULT 0, "
                "P_VW_One_Week_Out NUMERIC(10, 2) DEFAULT 0, "
                "P_VW_One_Month_Out NUMERIC(10, 2) DEFAULT 0, "
                "VW_One_Day_Out NUMERIC(10, 2) DEFAULT 0, "
                "VW_Two_Days_Out NUMERIC(10, 2) DEFAULT 0, "
                "VW_One_Week_Out NUMERIC(10, 2) DEFAULT 0, "
                "VW_One_Month_Out NUMERIC(10, 2) DEFAULT 0);";
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

bool insertData(const string& conn_str, const vector<post>& storage) {
    try {
        // Establish a connection to the database
        connection conn(conn_str);
        if (!conn.is_open()) {
            cerr << "Error: Could not connect to the database" << endl;
            return false;
        }

        // Start a transaction block
        work txn(conn);

        // Prepare an SQL statement for inserting data into the posts table
        // Use ON CONFLICT (Date) DO NOTHING to ignore duplicates
        conn.prepare("insert_post", 
            "INSERT INTO posts (Title, Data, Date, Stock_Ticker, Rating, VW_One_Day_Out, VW_Two_Days_Out, VW_One_Week_Out, VW_One_Month_Out) "
            "VALUES ($1, $2, to_timestamp($3), $4, $5, $6, $7, $8, $9) "
            "ON CONFLICT (Title) DO NOTHING");
        // Loop through each post object in the storage vector
         for (const auto& p : storage) {
            // Execute the prepared statement with the values from the post struct
            txn.exec_prepared("insert_post", 
                p.post_title,        // Title column
                p.post_data,         // Data column
                p.date,              // Date column as a timestamp
                p.stock_ticker,      // Stock_Ticker column
                p.rating,            // Rating column
                p.VW_one_day,        // VW_One_Day_Out column
                p.VW_two_days,       // VW_Two_Days_Out column
                p.VW_one_week,       // VW_One_Week_Out column
                p.VW_one_month       // VW_One_Month_Out column
            );
        }
        // Commit the transaction
        txn.commit();
        cout << "Data inserted successfully, ignoring duplicates." << endl;
        // Close the connection
        conn.disconnect();
        return true;
    } catch (const sql_error& e) {
        cerr << "SQL error RIGHT HERE: " << e.what() << endl;
        return false;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return false;
    }
}
// Function to retrieve incomplete data from the database and store it in the provided vector of structs
void retrieveIncompleteData(const string& conn_str, vector<post>& storage) {
    try {
        // Establish a connection to the database
        connection conn(conn_str);
        if (!conn.is_open()) {
            cerr << "Error: Could not connect to the database" << endl;
            return;
        }

        // Start a non-transactional work object
        nontransaction txn(conn);

        // SQL query to retrieve entries with any NULL or 0 VW fields, date as seconds since the epoch,
        // and Stock_Ticker not equal to 'N/A'
        string query = "SELECT POST_ID, Title, Data, Stock_Ticker, Rating, "
                       "EXTRACT(EPOCH FROM Date) AS date_seconds, "
                       "COALESCE(VW_One_Day_Out, 0) AS VW_One_Day_Out, "
                       "COALESCE(VW_One_Week_Out, 0) AS VW_One_Week_Out, "
                       "COALESCE(VW_One_Month_Out, 0) AS VW_One_Month_Out, "
                       "COALESCE(VW_Two_Days_Out, 0) AS VW_Two_Days_Out "  // Added VW_Two_Days_Out column
                       "FROM posts "
                       "WHERE (VW_One_Day_Out IS NULL OR VW_One_Day_Out = 0 OR "
                       "VW_One_Week_Out IS NULL OR VW_One_Week_Out = 0 OR "
                       "VW_One_Month_Out IS NULL OR VW_One_Month_Out = 0 OR "
                       "VW_Two_Days_Out IS NULL OR VW_Two_Days_Out = 0) "  // Added condition for VW_Two_Days_Out
                       "AND Stock_Ticker <> 'N/A'";

        // Execute the query
        result res = txn.exec(query);
        // Loop through each row in the result set
        for (const auto& row : res) {
            post p;  // Create a new post struct
            // Fill in the post struct fields with data from the row
            p.postID = row["post_id"].as<int>();  // Retrieve POST_ID
            p.post_title = row["title"].c_str();
            p.post_data = row["data"].c_str();
            p.stock_ticker = row["stock_ticker"].c_str();
            p.rating = row["rating"].as<int>();
            // Retrieve date as seconds since epoch and round to the nearest integer
            p.date = static_cast<long long>(round(row["date_seconds"].as<double>()));
            // Retrieve VW values, using 0 if they were NULL in the database
            p.VW_one_day = row["vw_one_day_out"].as<double>();
            p.VW_one_week = row["vw_one_week_out"].as<double>();
            p.VW_one_month = row["vw_one_month_out"].as<double>();
            p.VW_two_days = row["vw_two_days_out"].as<double>();  
            // Add the filled post struct to the storage vector
            storage.push_back(p);
        }
        // Close the connection
        conn.disconnect();
        cout << "Retrieved " << storage.size() << " incomplete entries from the database." << endl;
    } catch (const sql_error& e) {
        cerr << "SQL error: " << e.what() << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

set<string> holidays = {"2024-01-01", "2024-12-25"};  // EXPAND HOLIDY DAYTES 

// Function to check if a date is a holiday
bool isHoliday(const tm& date) {
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &date);
    return holidays.find(buffer) != holidays.end();
}

// Modify roundToNextTradingDay to account for holidays
tm* roundToNextTradingDay(time_t& time) {
    tm* tm_time = gmtime(&time);

    // Skip weekends and holidays
    while (tm_time->tm_wday == 0 || tm_time->tm_wday == 6 || isHoliday(*tm_time)) {
        time += 86400;  // Add one day
        tm_time = gmtime(&time);
    }
    return tm_time;
}

void searchUpdatePrices(vector<post>& storage, const string& polygonAPI) {
    CURL* curl;
    CURLcode res;
    string readBuffer;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (!curl) {
        cerr << "Failed to initialize CURL." << endl;
        return;
    }

    for (auto& elem : storage) {
        elem.date -= 10000000;
        // Properly adjust dates for production (remove fudging)
        // long long original_date_ms = elem.date * 1000;  // Convert seconds to milliseconds

        // Calculate trading day-adjusted timestamps in milliseconds
        // long long date_plus_one_day_sec = elem.date + 86400;  // +1 day in seconds
        // long long date_plus_two_days_sec = elem.date + 2 * 86400;  // +2 days in seconds
        // long long date_plus_one_week_sec = elem.date + 7 * 86400;  // +1 week in seconds
        long long date_plus_one_month_sec = elem.date + 35 * 86400;  // Approx. +1 month in seconds

        // Round to the next trading day if necessary
        // time_t one_day_time = date_plus_one_day_sec;
        // tm* tm_one_day = roundToNextTradingDay(one_day_time);
        // date_plus_one_day_sec = mktime(tm_one_day);

        // time_t two_days_time = date_plus_two_days_sec;
        // tm* tm_two_days = roundToNextTradingDay(two_days_time);
        // date_plus_two_days_sec = mktime(tm_two_days);

        // time_t one_week_time = date_plus_one_week_sec;
        // tm* tm_one_week = roundToNextTradingDay(one_week_time);
        // date_plus_one_week_sec = mktime(tm_one_week);

        // time_t one_month_time = date_plus_one_month_sec;
        // tm* tm_one_month = roundToNextTradingDay(one_month_time);
        // date_plus_one_month_sec = mktime(tm_one_month);

        // Convert these to milliseconds for the API request
        // long long date_plus_one_day_ms = date_plus_one_day_sec * 1000;
        // long long date_plus_two_days_ms = date_plus_two_days_sec * 1000;
        // long long date_plus_one_week_ms = date_plus_one_week_sec * 1000;
        // long long date_plus_one_month_ms = date_plus_one_month_sec * 1000;

        // Build URI for each range using the formatted date
        stringstream build_uri;
        time_t original_time = static_cast<time_t>(elem.date);
        time_t end_time_month = static_cast<time_t>(date_plus_one_month_sec);

        build_uri << "https://api.polygon.io/v2/aggs/ticker/" << elem.stock_ticker << "/range/1/day/"
                  << put_time(gmtime(&original_time), "%Y-%m-%d") << "/"
                  << put_time(gmtime(&end_time_month), "%Y-%m-%d")
                  << "?adjusted=true&sort=asc&apiKey=" << polygonAPI;

        cout << "Built URI in the form of: " << build_uri.str() << "\n";
        
        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, build_uri.str().c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            continue;  // Skip to the next post if there was an error
        }

        // Parse JSON response
        try {
            auto jsonResponse = nlohmann::json::parse(readBuffer);
            cout << "Attempting to parse result from Polygon" << "\n";
            if (jsonResponse.contains("results")) {
                cout << "Entering results object from Polygon" << "\n";
                for (const auto& result : jsonResponse["results"]) {
                    long long timestamp_ms = result["t"];  // Time in milliseconds
                    double volume_weighted_price = result["vw"];  // Volume-weighted price
                    cout << "Retrieved ms: " << timestamp_ms << " and VW_Price: " << volume_weighted_price <<  " from Polygon " << "\n";
                    // Ensure the correct day is selected by checking exact range conditions
                    // elem.VW_one_day = 5; // GIGA TEMP PELASE EDIT
                    if (elem.VW_one_day == 0) {
                        cout << "Saved ONEDAY: " << volume_weighted_price << " at " << timestamp_ms << "\n";
                        elem.VW_one_day = volume_weighted_price;  // Update for two days out
                    } else if (elem.VW_two_days == 0){
                        cout << "Saved TWODAYS: " << volume_weighted_price << " at " << timestamp_ms << "\n";
                        elem.VW_two_days = volume_weighted_price;
                    } else if (timestamp_ms <=(elem.date + 8 * 86400) * 1000) {
                        cout << "Saved WEEK: " << volume_weighted_price << " at " << timestamp_ms << "\n";
                        elem.VW_one_week = volume_weighted_price;
                    } else if (timestamp_ms >=  (elem.date + 30 * 86400) * 1000) { // hardcode the month 
                        cout << "Saved MONTH: " << volume_weighted_price << " at " << timestamp_ms << "\n";
                        elem.VW_one_month = volume_weighted_price;
                    }
                }
            }
            cout << "Supposedly processed element : " << elem << "\n";
        } catch (nlohmann::json::parse_error& e) {
            cerr << "JSON parsing error: " << e.what() << endl;
        }

        // Clear the buffer for the next iteration
        readBuffer.clear();
    }

    // Clean up CURL
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}


void mergeEntries(const vector<post>& storage, const string& connectionString) {
    try {
        // Establish a connection to the PostgreSQL database
        pqxx::connection conn(connectionString);

        if (!conn.is_open()) {
            cerr << "Failed to open database connection." << endl;
            return;
        }

        // Prepare statements for insertion and updating
        // Insert command with corrected column names including VW_Two_Days_Out
        conn.prepare("update_post", 
                    "UPDATE posts SET "
                    "VW_One_Day_Out = $2, "
                    "VW_Two_Days_Out = $3, "  // Corrected column name
                    "VW_One_Week_Out = $4, "
                    "VW_One_Month_Out = $5 "
                    "WHERE post_id = $1");

        // Start a transaction block
        pqxx::work txn(conn);
        for (const auto& elem : storage) {
            // Execute prepared statements to insert or update the entry
            txn.exec_prepared("update_post", 
                              elem.postID, elem.VW_one_day, elem.VW_two_days, elem.VW_one_week, elem.VW_one_month);  // Added elem.VW_two_days
            cout << "Processed post ID " << elem.postID << " in the database." << endl;
        }

        // Commit the transaction
        txn.commit();
        // Close the connection
        conn.disconnect();
        cout << "Database merge operation completed successfully." << endl;
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}
/**
 * @brief Main function that calls all of the various helper functions above.
 * 
 * @return int
 */
int main()
{
    // This is used for storing the various posts before they get transferred to the database
    cout << "Attempting to read/create secrets from AWS" << "\n";
    system("python3 ../src/read_secrets_aws.py");
    // some kind of pre-check work for non-local running: 
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
    establishConnection(conn_str, fallback_conn_str, dbpassword_new);
    createTables(conn_str);
    insertData(conn_str, storage); 
    storage.clear();
    // ------------------DATA UPDATES --------------------
    retrieveIncompleteData(conn_str, storage); //TODO: THIS SEEMS SUSPICIOUS ALL 3 ? maybe because it excludes N/A entries hmm . maybe this works
    searchUpdatePrices(storage, polygonAPI);
    mergeEntries(storage, conn_str);
    // merge entries that already exist with the new and relevant data
     for (const auto& elem : storage){
        cout << elem << "\n";
    }
    // ------------------DATA MODELING --------------------
    // after the updates are written (potentially very buggy) XD, run the python file
    int result = system("python3 ../src/model.py");
        // Check the result
    if (result == 0) {
        std::cout << "Python script executed successfully." << std::endl;
    } else {
        std::cerr << "Python script execution failed with error code: " << result << std::endl;
    }
    // ------------------DATA VISUALIZATIONS --------------------
    // holy goat
    return 0;
}