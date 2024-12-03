#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

// 메모리 구조체 정의
struct MemoryStruct {
    char *memory;
    size_t size;
};

// curl 콜백 함수 - API 응답 데이터를 메모리에 저장
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("Not enough memory\n");  
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// 주소를 입력받아 좌표를 가져오는 함수
char* get_coordinates(const char* address) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        char url[256];
        char *encoded_address = curl_easy_escape(curl, address, 0);
        snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-geocode/v2/geocode?query=%s", encoded_address);
        curl_free(encoded_address);

        const char* api_key_id = getenv("NAVER_API_KEY_ID");
        const char* api_key_secret = getenv("NAVER_API_KEY_SECRET");

        if (api_key_id == NULL || api_key_secret == NULL) {
            fprintf(stderr, "API 키를 찾을 수 없습니다. .env 파일을 로드했는지 확인하세요.\n");
            return NULL;
        }

        struct curl_slist *headers = NULL;
        char api_key_id_header[100];
        char api_key_secret_header[100];
        snprintf(api_key_id_header, sizeof(api_key_id_header), "X-NCP-APIGW-API-KEY-ID: %s", api_key_id);
        snprintf(api_key_secret_header, sizeof(api_key_secret_header), "X-NCP-APIGW-API-KEY: %s", api_key_secret);
        headers = curl_slist_append(headers, api_key_id_header);
        headers = curl_slist_append(headers, api_key_secret_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            cJSON *json = cJSON_Parse(chunk.memory);
            if (json == NULL) {
                printf("Error parsing JSON\n");
                free(chunk.memory);
                return NULL;
            }

            cJSON *addresses = cJSON_GetObjectItem(json, "addresses");
            if (addresses == NULL || cJSON_GetArraySize(addresses) == 0) {
                printf("No addresses found.\n");
                cJSON_Delete(json);
                free(chunk.memory);
                return NULL;
            }

            cJSON *first = cJSON_GetArrayItem(addresses, 0);
            if (first == NULL) {
                printf("No address data found.\n");
                cJSON_Delete(json);
                free(chunk.memory);
                return NULL;
            }

            const char *x = cJSON_GetObjectItem(first, "x")->valuestring;
            const char *y = cJSON_GetObjectItem(first, "y")->valuestring;

            char *coordinates = (char *)malloc(50);
            snprintf(coordinates, 50, "%s,%s", x, y);

            cJSON_Delete(json);
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return coordinates;
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    curl_global_cleanup();
    return NULL;
}

// 출발지와 도착지 좌표로부터 최단 소요 시간을 가져오는 함수
void get_duration(const char* start_coords, const char* end_coords) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        char url[512];
        snprintf(url, sizeof(url), "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving?start=%s&goal=%s", start_coords, end_coords);

        const char* api_key_id = getenv("NAVER_API_KEY_ID");
        const char* api_key_secret = getenv("NAVER_API_KEY_SECRET");

        if (api_key_id == NULL || api_key_secret == NULL) {
            fprintf(stderr, "API 키를 찾을 수 없습니다. .env 파일을 로드했는지 확인하세요.\n");
            return;
        }

        struct curl_slist *headers = NULL;
        char api_key_id_header[100];
        char api_key_secret_header[100];
        snprintf(api_key_id_header, sizeof(api_key_id_header), "X-NCP-APIGW-API-KEY-ID: %s", api_key_id);
        snprintf(api_key_secret_header, sizeof(api_key_secret_header), "X-NCP-APIGW-API-KEY: %s", api_key_secret);
        headers = curl_slist_append(headers, api_key_id_header);
        headers = curl_slist_append(headers, api_key_secret_header);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            cJSON *json = cJSON_Parse(chunk.memory);
            if (json == NULL) {
                printf("Error parsing JSON\n");
                free(chunk.memory);
                return;
            }

            cJSON *route = cJSON_GetObjectItem(json, "route");
            if (route == NULL) {
                printf("No route found.\n");
                cJSON_Delete(json);
                free(chunk.memory);
                return;
            }

            cJSON *traoptimal = cJSON_GetObjectItem(route, "traoptimal");
            if (traoptimal == NULL || cJSON_GetArraySize(traoptimal) == 0) {
                printf("No optimal route found.\n");
                cJSON_Delete(json);
                free(chunk.memory);
                return;
            }

            cJSON *first_route = cJSON_GetArrayItem(traoptimal, 0);
            if (first_route == NULL) {
                printf("No route data found.\n");
                cJSON_Delete(json);
                free(chunk.memory);
                return;
            }

            cJSON *summary = cJSON_GetObjectItem(first_route, "summary");
            if (summary == NULL) {
                printf("No summary found.\n");
                cJSON_Delete(json);
                free(chunk.memory);
                return;
            }

            const cJSON *duration = cJSON_GetObjectItem(summary, "duration");
            if (duration == NULL) {
                printf("No duration found.\n");
            } else {
                int duration_seconds = duration->valueint / 1000;
                int hours = duration_seconds / 3600;
                int minutes = (duration_seconds % 3600) / 60;

                if (hours > 0) {
                    printf("최단 시간 (자동차): %d시간 %d분\n", hours, minutes);
                } else {
                    printf("최단 시간 (자동차): %d분\n", minutes);
                }
            }

            cJSON_Delete(json);
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    curl_global_cleanup();
}

int main(int argc, char* argv[]) {
    /* char start_address[100], end_address[100];
    char input_hour[10], input_min[10];

    printf("출발지를 입력하세요: ");
    fgets(start_address, 100, stdin);
    start_address[strcspn(start_address, "\n")] = 0;

    printf("도착지를 입력하세요: ");
    fgets(end_address, 100, stdin);
    end_address[strcspn(end_address, "\n")] = 0;

    printf("출발 시간을 입력하세요 (시): ");
    fgets(input_hour, 10, stdin);
    input_hour[strcspn(input_hour, "\n")] = 0;

    printf("출발 시간을 입력하세요 (분): ");
    fgets(input_min, 10, stdin);
    input_min[strcspn(input_min, "\n")] = 0; */

    char *start_coords = get_coordinates(argv[1]);
    if (start_coords == NULL) {
        printf("Failed to get start coordinates.\n");
        exit(1);
    }
    char *end_coords = get_coordinates(argv[2]);
    if (end_coords == NULL) {
        printf("Failed to get end coordinates.\n");
        exit(1);
    }

    if (start_coords != NULL && end_coords != NULL) {
        printf("출발지 좌표: %s\n", start_coords);
        printf("도착지 좌표: %s\n", end_coords);

        get_duration(start_coords, end_coords);

        char command[256];
        snprintf(command, sizeof(command), "python3 get_transit_time.py %s %s %s %s", start_coords, end_coords, argv[3], argv[4]);
        int result = system(command);

        if (result != 0) {
            printf("Python script execution failed\n");
        }

        free(start_coords);
        free(end_coords);
    } else {
        printf("Failed to retrieve coordinates.\n");
    }

    return 0;
}