import requests
from urllib.parse import quote

def archive(url):
    try:
        url = quote(url, safe=':/')
        wayback_url = f"https://web.archive.org/save/{url}"
        response = requests.get(wayback_url, timeout=10)
        response.raise_for_status()
        if "Saving page now" in response.text:
            return f"URL {url} successfully archived."
        raise Exception("Failed to archive the URL.")

    except requests.exceptions.RequestException as req_err:
        return f"Request failed: {req_err}"

    except Exception as err:
        return f"An unexpected error occurred: {err}"

# Example usage:
url_to_archive = "https://example.com"
result = archive(url_to_archive)
print(result)
