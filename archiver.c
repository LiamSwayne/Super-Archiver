#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>

// SETTINGS
char* starterLink = ""; // initial link to search
int degrees = 3; // number of levels of outlinks to archive. ex: 2 means outlinks of outlinks
int linkLimit = 1000; // max number of links

char* getURL(char* linkStr, int ipaddressClassA, int shutdown) {
    CURL* curl;
    CURLcode res;
    char* text = NULL;
    int successful = 0;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "User-Agent: ");
    headers = curl_slist_append(headers, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/0.0.0.0 Safari/537.36");

    const char* blankTexts[] = {
        "meta content=\"Error - IMDb\"",
        "<html><body><b>Http/1.1 Service Unavailable</b></body> </html>",
        "<title>404 Error - IMDb</title>"
    };

    int rerouteCount = 0;
    while (!successful) {
        if (rerouteCount >= 10) {
            printf("Reroute limit reached while getting: %s\n", linkStr);
            if (shutdown) {
                printf("Shutting down.\n");
                exit(1);
            } else {
                break;
            }
        }
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, linkStr);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &text);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                successful = 1;
            } else {
                if (ipaddressClassA <= 255) {
                    ipaddressClassA++;
                } else {
                    ipaddressClassA = 1;
                }
                char userAgent[200];
                sprintf(userAgent, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/%d.0.0.0 Safari/537.36", ipaddressClassA);
                curl_slist_free_all(headers);
                headers = curl_slist_append(headers, userAgent);
                printf("Connection error encountered. Rerouting...\n");
                rerouteCount++;
            }
            curl_easy_cleanup(curl);
        }
        if (successful) {
            for (int i = 0; i < sizeof(blankTexts) / sizeof(blankTexts[0]); i++) {
                if (strstr(text, blankTexts[i]) != NULL) {
                    successful = 0;
                }
            }
        }
    }
    curl_slist_free_all(headers);
    return text;
}

char* cut_off_url(const char* link) {
    char* cut_off_link = (char*)malloc(strlen(link) + 1);
    if (cut_off_link == NULL) {
        perror("Memory allocation error");
        exit(1);
    }

    // Parse the link
    const char* netloc_start = strstr(link, "://");
    if (netloc_start == NULL) {
        free(cut_off_link);
        return NULL;
    }
    netloc_start += 3;  // Move to the beginning of the netloc

    const char* netloc_end = strchr(netloc_start, '/');
    if (netloc_end == NULL) {
        strcpy(cut_off_link, link);
    } else {
        size_t length = netloc_end - link;
        strncpy(cut_off_link, link, length);
        cut_off_link[length] = '\0';
    }

    return cut_off_link;
}

char** get_links_from_html(const char* html, int* numLinks) {
    const char* linkPattern = "<a\\s+(?:[^>]*?\\s+)?href=([\"'])(.*?)\\1";
    regex_t regex;
    int reti;
    regmatch_t matches[3];
    char** extracted_links = NULL;
    *numLinks = 0;

    // Compile the regular expression
    reti = regcomp(&regex, linkPattern, REG_EXTENDED | REG_ICASE);
    if (reti) {
        perror("Regex compilation failed");
        return NULL;
    }

    // Find all the links in the HTML
    while (1) {
        reti = regexec(&regex, html, 3, matches, 0);
        if (reti) {
            break;  // No more matches
        }

        char* link = (char*)malloc(matches[2].rm_eo - matches[2].rm_so + 1);
        if (link == NULL) {
            perror("Memory allocation error");
            break;
        }

        strncpy(link, &html[matches[2].rm_so], matches[2].rm_eo - matches[2].rm_so);
        link[matches[2].rm_eo - matches[2].rm_so] = '\0';

        (*numLinks)++;
        extracted_links = (char**)realloc(extracted_links, (*numLinks) * sizeof(char*));
        extracted_links[(*numLinks) - 1] = link;

        // Move to the end of the match
        html += matches[0].rm_eo;
    }

    regfree(&regex);
    return extracted_links;
}

// Function declarations from the previous C code
char* cut_off_url(const char* link);
char** get_links_from_html(const char* html, int* numLinks);
char* getURL(const char* linkStr, int ipaddressClassA, int shutdown);

#define LINK_LIMIT 1000

static int linkCount = 0;
static char** linkList = NULL;

char** find_all_links(const char* start_url, int degrees, int linkLimit) {
    if (degrees < 1 || linkCount > linkLimit) {
        return linkList;
    }

    const char* domain = cut_off_url(start_url);

    // Get the HTML code from the start URL
    char* html = getURL(start_url, 109, 0);

    if (html == NULL) {
        printf("An exception occurred.\n");
        return linkList;
    }

    // Extract all the links from the HTML code
    int numLinks = 0;
    char** links = get_links_from_html(html, &numLinks);

    for (int i = 0; i < numLinks; i++) {
        if (links[i][0] == '/') {
            char* fullLink = (char*)malloc(strlen(domain) + strlen(links[i]) + 1);
            if (fullLink == NULL) {
                perror("Memory allocation error");
                exit(1);
            }
            strcpy(fullLink, domain);
            strcat(fullLink, links[i]);
            free(links[i]);
            links[i] = fullLink;
        }
    }

    // Print the extracted links
    for (int i = 0; i < numLinks; i++) {
        linkCount++;
        linkList = (char**)realloc(linkList, linkCount * sizeof(char*));
        linkList[linkCount - 1] = links[i];
        free(links[i]);

        if (linkCount % 100 == 0) {
            printf("%d links found.\n", linkCount);
        }

        if (linkCount > linkLimit) {
            return linkList;
        }
    }

    // Recursively find links in each extracted link
    for (int i = 0; i < numLinks; i++) {
        char** recursiveResult = find_all_links(links[i], degrees - 1, linkLimit);
        if (recursiveResult != NULL) {
            linkList = recursiveResult;
        }
    }

    return linkList;
}

// Function declarations from the previous C code
char** find_all_links(const char* start_url, int degrees, int linkLimit);
char* getURL(const char* linkStr, int ipaddressClassA, int shutdown);

char** remove_duplicates(char** lst, int* count) {
    char** result = NULL;
    int resultCount = 0;

    for (int i = 0; i < *count; i++) {
        int isDuplicate = 0;
        for (int j = 0; j < resultCount; j++) {
            if (strcmp(lst[i], result[j]) == 0) {
                isDuplicate = 1;
                break;
            }
        }
        if (!isDuplicate) {
            resultCount++;
            result = (char**)realloc(result, resultCount * sizeof(char*));
            result[resultCount - 1] = lst[i];
        }
    }

    *count = resultCount;
    return result;
}

int main() {
    const char* starterLink = ""; // initial link to search
    int degrees = 3; // number of levels of outlinks to archive. ex: 2 means outlinks of outlinks
    int linkLimit = 1000; // max number of links

    char** linkList = find_all_links(starterLink, degrees, linkLimit);

    if (linkList == NULL) {
        printf("Failed to retrieve links.\n");
        return 1;
    }

    int linkCount = 0;
    for (int i = 0; linkList[i] != NULL; i++) {
        linkCount++;
    }

    // Removing duplicates
    linkList = remove_duplicates(linkList, &linkCount);

    printf("Archiving links...\n");

    for (int i = 0; i < linkCount; i++) {
        getURL(linkList[i], 109, 0);
        printf("%s\n", linkList[i]);

        if (i % 100 == 0 && i > 0) {
            printf("%d links archived.\n", i);
        }
    }

    printf("Archival complete.\n");

    return 0;
}
