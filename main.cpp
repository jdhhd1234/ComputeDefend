#include "main.hpp"
#include "curl/curl.h"

void MainCurl::CurlHandle(const char inputURL)
{
    /* easy handle 생성 */
    curl = curl_easy_init();
    if(curl) 
    {
        /* 요청할 URL 설정 */
        curl_easy_setopt(curl, CURLOPT_URL, inputURL);

        /* HTTP 리디렉션(3xx) 자동 추적 설정 */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /* 요청 수행 */
        res = curl_easy_perform(curl);

        /* 에러 발생 시 처리 */
        if(res != CURLE_OK)
        {
            std::cout << stderr << "curl_easy_perform() failed: " << curl_easy_strerror(res);
            return;
        }

        /* handle 정리 */
        curl_easy_cleanup(curl);
    }
}

int main()
{
    MainCurl mainCurl;
    return 0;
}
