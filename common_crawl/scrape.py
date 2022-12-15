from time import time
# https://rushter.com/blog/python-fast-html-parser/

import ssl
from bs4 import BeautifulSoup
import csv
from urllib.request import urlopen
import urllib.parse
import pandas as pd
# import aiohttp
# import asyncio
from tqdm import tqdm
import time
import concurrent.futures
from multiprocessing.pool import ThreadPool as Pool
NUM_THREADS = 30
def read_row_to_url(row):
    '''
    This will parse through the URL's returned from the xml file
    '''
    url = row[0]
    url = urllib.parse.urlsplit(url)
    url = list(url)
    url[2] = urllib.parse.quote(url[2])
    url = urllib.parse.urlunsplit(url)
    return url

'''
Used for testing purposes
'''
def scrape_single_page(url):
    ssl._create_default_https_context = ssl._create_unverified_context
    
    soup = BeautifulSoup(urlopen(url).read(), 'lxml')
    power_users = [user.text for user in soup.find_all("strong") if user.text != "Economist"]
    # print("power users",  power_users)
    posts = soup.find_all("div", {"class": "post"})
    authors = soup.find_all("small")
    text_posts = [t.text if len(t) != '' else None for t in posts ]
    text_authors = []
    len_authors = len(authors)
    urls = []
    index = 0
    power_users_index = 0
    while index < len_authors:
        author = authors[index]
        # print("index", index)
        # print("author", author.text)
        len_author_text = len(author.text)
        if len_author_text == 4:
            text_authors.append(author.text)
            index += 1
        elif len_author_text > 4:
            text_authors.append(power_users[power_users_index])
            power_users_index += 1
        elif len_author_text == 0:
            if (index + power_users_index) % 2 == 0:
                text_authors.append(None)
            index += 1
        urls.append(url)
        index += 1
    # print("authors", text_authors)
    # print("text_posts", text_posts)
    # print("len(text authors)", len(text_authors))
    # print("len(text_posts", len(text_posts))
    # print("len urls", len(urls))
    return text_authors, text_posts, urls

def return_urls (links_xml):
    # return_urls = []
    # line_count = 0
    with open(links_xml, 'r') as f:
        data = f.read()
        soup = BeautifulSoup(data, 'xml')
        urls = soup.find_all("loc")
        urls = [url.text for url in urls]
    return urls
    # with open(links_csv) as csv_file:
    #     csv_reader = csv.reader(csv_file, delimiter=',')
    #     for row in csv_reader:
    #         if line_count == 0:
    #             line_count += 1
    #         else:
    #             url = read_row_to_url(row)
    #             return_urls.append(url)
    #             line_count += 1
    # return return_urls
        
def scrape_links(links_xml):
    dataframe = pd.DataFrame(columns=['Author', 'Content', 'URL'])
    dataframe_authors = []
    dataframe_posts = []
    dataframe_urls = []
    urls = return_urls(links_xml)
    print("len(urls)", len(urls))
    executor = concurrent.futures.ProcessPoolExecutor(10)
    futures = []
    for url in urls:
        future = executor.submit(scrape_single_page, url)
        futures.append(future)
    for future in tqdm(concurrent.futures.as_completed(futures)):
        try:
            text_authors, text_posts, urls = future.result()
            if len(text_authors) != len(text_posts) and len(text_authors) != len(urls):
                print("faulty url:", urls[0])
                break
            dataframe_authors += text_authors
            dataframe_posts += text_posts
            dataframe_urls += urls
        except:
            pass
    dataframe['Author'] = pd.Series(dataframe_authors)
    dataframe['Content'] = pd.Series(dataframe_posts)
    dataframe['URL'] = pd.Series(dataframe_urls)
    dataframe.to_csv('scrape_output.csv')





if __name__ == "__main__":
    # https://urlsearch.commoncrawl.org/CC-MAIN-2022-40/ 
    # files were retrieved using this link with https://www.econjobrumors.com/
    # result: https://urlsearch.commoncrawl.org/CC-MAIN-2022-40-index?url=https%3A%2F%2Fwww.econjobrumors.com%2F&output=json
    files = ["files/CC-MAIN-20220926211042-20220927001042-00114.warc.gz",
    "files/CC-MAIN-20220926211042-20220927001042-00381.warc.gz",
    "files/CC-MAIN-20220930212504-20221001002504-00381.warc.gz",
    "files/CC-MAIN-20221005073953-20221005103953-00381.warc.gz",
    "files/CC-MAIN-20221006124156-20221006154156-00114.warc.gz",
    "files/CC-MAIN-20221006124156-20221006154156-00230.warc.gz",
    "files/CC-MAIN-20221006124156-20221006154156-00381.warc.gz",
    "files/CC-MAIN-20221006124156-20221006154156-00753.warc.gz",
    "files/CC-MAIN-20221007175237-20221007205237-00381.warc.gz"
    ]
    csv_file_20k = "ejmr_20k_links_scrape.csv"
    csv_file_500 = "internal_html.csv"
    xml_files_60k = ["60k_links/1_60k.xml", "60k_links/2_60k.xml", "60k_links/3_60k.xml"]
    normal_url = "https://www.econjobrumors.com/topic/fake-journal-of-international-economics"
    super_user_url = "https://www.econjobrumors.com/topic/fake-journal-of-international-economics/page/3"
    missing_username_url = "https://www.econjobrumors.com/topic/german-market/page/3"
    faulty_url = "https://www.econjobrumors.com/topic/helping-with-trolls-a-solution-from-kirk"
    # has mixture of both super and missing url 
    start_time = time.time()
    # scrape_single_page(super_user_url)
    scrape_links(xml_files_60k[0])
    total_time = time.time() - start_time
    print("total time:", total_time)
