#include <iostream>
#include <curl/curl.h>
#include <string>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

int main() {
    CURL* curl = curl_easy_init();
    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://trefle.io/api/v1/plants?page=1&token=usr-jVbAaSE1s0aXz2EP729tqHDxNBJ0lm8gaPDNNoRww6Y");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  // подробный вывод
    
    CURLcode res = curl_easy_perform(curl);
    
    std::cout << "Result: " << res << std::endl;
    std::cout << "Response length: " << response.length() << std::endl;
    std::cout << "Response: " << response << std::endl;
    
    curl_easy_cleanup(curl);
    return 0;
}