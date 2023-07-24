#include <stdio.h>
#include <tgbot/tgbot.h>
#include <vector>
#include <string.h>
#include <ctime>
#include <curl/curl.h>
#include "json.hpp"

using namespace TgBot;
using namespace std;
using json = nlohmann::json;

vector<std::string> botCommands = {"start","time","currency"};

string get_time_as_string(){
    time_t now = time(NULL);
    tm* ptr = localtime(&now);
    char buf[32];
    strftime(buf,32,"%c",ptr);
    return buf;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string get_http_data(const string& url, struct curl_slist *headers = NULL) {
    CURL* curl = curl_easy_init();
    string response_string;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        if(headers != NULL) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response_string;
}

int main() {
   Bot bot("TELEGRAM BOT API");

    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
    vector<InlineKeyboardButton::Ptr> buttons;
    InlineKeyboardButton::Ptr usd_btn(new InlineKeyboardButton),crypto_btn(new InlineKeyboardButton);
    usd_btn->text = "USD/EUR";
    crypto_btn->text = "Crypto";
    usd_btn->callbackData = "USD/EUR";
    crypto_btn->callbackData = "Crypto";
    buttons.push_back(usd_btn);
    buttons.push_back(crypto_btn);
    keyboard->inlineKeyboard.push_back(buttons);

    bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi! "+message->chat->firstName);
    });
    bot.getEvents().onCommand("time", [&bot](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Current sever time: "+get_time_as_string());
    });

    bot.getEvents().onCommand("currency", [&bot,&keyboard](Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id,"Select Option",false,0,keyboard);
    });

    bot.getEvents().onCallbackQuery([&bot](CallbackQuery::Ptr query){
        if(StringTools::startsWith(query->data,"USD/EUR")){
            string url_usd = "http://api.nbp.pl/api/exchangerates/rates/A/USD/?format=json";
            string http_data_usd = get_http_data(url_usd);
            json j_usd = json::parse(http_data_usd);
            string usd_to_pln = to_string(j_usd["rates"][0]["mid"]);

            string url_eur = "http://api.nbp.pl/api/exchangerates/rates/A/EUR/?format=json";
            string http_data_eur = get_http_data(url_eur);
            json j_eur = json::parse(http_data_eur);
            string eur_to_pln = to_string(j_eur["rates"][0]["mid"]);

            bot.getApi().sendMessage(query->message->chat->id, "USD to PLN: " + usd_to_pln + "\nEUR to PLN: " + eur_to_pln);
        }else if(StringTools::startsWith(query->data,"Crypto")){
            string url = "https://pro-api.coinmarketcap.com/v1/cryptocurrency/listings/latest?start=1&limit=2&convert=USD";
            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "X-CMC_PRO_API_KEY: YOUR-API-KEY");
            string http_data = get_http_data(url, headers);
            json j = json::parse(http_data);
            string btc_to_usd = to_string(j["data"][0]["quote"]["USD"]["price"]);
            string eth_to_usd = to_string(j["data"][1]["quote"]["USD"]["price"]);
            bot.getApi().sendMessage(query->message->chat->id, "BTC to USD: " + btc_to_usd + "\nETH to USD: " + eth_to_usd);
        }
    });

    bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
        for(const auto& command :botCommands){
            if("/"+command == message->text){
                return;
            }
        }
        bot.getApi().sendMessage(message->chat->id,"Sorry wrong command");
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (TgException& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}

