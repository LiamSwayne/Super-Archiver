# archive several degrees of outlinks from internet archive

import re
from urllib.parse import urlparse

# SETTINGS
starterLink = '' # initial link to search
degrees = 3 # number of levels of outlinks to archive. ex: 2 means outlinks of outlinks
linkLimit = 1000 # max number of links

def getURL(linkStr,ipaddressClassA=109,shutdown=False):
    import requests, sys, bs4, re
    
    successful = False
    headers = {
            'User-Agent': ''
            }
    headers.update({'User-Agent':'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/'+str(ipaddressClassA)+'.0.0.0 Safari/537.36'})
    blankTexts = [
        'meta content="Error - IMDb"',
        '<html><body><b>Http/1.1 Service Unavailable</b></body> </html>',
        '<title>404 Error - IMDb</title>'
        ]
    
    rerouteCount = 0
    while not successful:
        if rerouteCount >= 10:
            print("Reroute limit reached while getting: "+linkStr)
            if shutdown:
                print("Shutting down.")
                sys.exit()
            else:
                break
        try:
            response = requests.get(linkStr, headers=headers)
            successful = True
        except:
            ConnectionError
            if ipaddressClassA <= 255:
                ipaddressClassA+=1
            else:
                ipaddressClassA = 1
            headers.update({'User-Agent':'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/'+str(ipaddressClassA)+'.0.0.0 Safari/537.36'})
            print("Connection error encountered. Rerouting...")
            rerouteCount +=1
        if successful:
            text = str(bs4.BeautifulSoup(response.content, 'html.parser'))
            for i in range(len(blankTexts)):
                if re.search(blankTexts[i],text):
                    successful = False
    return text

from urllib.parse import urlparse

def cut_off_url(link):
    parsed_link = urlparse(link)
    cut_off_link = parsed_link.scheme + "://" + parsed_link.netloc
    return cut_off_link

def get_links_from_html(html):
    # Use regular expressions to find all the links in the HTML code
    link_pattern = re.compile(r'<a\s+(?:[^>]*?\s+)?href=(["\'])(.*?)\1', re.IGNORECASE)
    links = link_pattern.findall(html)
    # Extract the links from the regex matches
    extracted_links = [link[1] for link in links]
    return extracted_links

def find_all_links(start_url, degrees):
    global linkLimit
    global linkList
    
    if degrees < 1 or len(linkList) > linkLimit:
        return linkList
    
    domain = cut_off_url(start_url)

    # Get the HTML code from the start URL
    try:
        html = getURL(start_url)
        # Extract all the links from the HTML code
        links = get_links_from_html(html)

        for i in range(len(links)):
            if links[i][0] == "/":
                links[i] = domain + links[i]
        
        # Print the extracted links
        for link in links:
            linkList.append(link)
            if len(linkList) % 100 == 0:
                print(str(len(linkList))+" links found.")

            if len(linkList) > linkLimit:
                return linkList

        # Recursively find links in each extracted link
        for link in links:
            find_all_links(link, degrees - 1)
        else:
            return linkList
    except Exception:
        print("An exception occured.")

    return linkList

def remove_duplicates(lst):
    result = []
    for item in lst:
        if item not in result:
            result.append(item)
    return result

linkList = [starterLink]

find_all_links(starterLink, degrees)

# removing duplicates
linkList = remove_duplicates(linkList)

print("Archiving links...")

for i in range(len(linkList)):
    getURL('https://web.archive.org/save/'+linkList[i])
    print(linkList[i])
    if i % 100 == 0 and i > 0:
        print(str(i)+" links archived.")

print("Archival complete.")
