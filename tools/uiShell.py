#coding: utf-8

import os
import re
import sys
import urllib2
import md5

def usage():
    print >> sys.stderr, "usage: python %s root_path url_pattern" % (sys.argv[0])

class FIFOQueue(object):
    def __init__(self):
        self.__queue = []

    def push(self, item):
        self.__queue.append(item)

    def pop(self):
        if not len(self.__queue):
            return None
        item = self.__queue[0]
        self.__queue = self.__queue[1:]
        return item

def fetch_file_content(url):
    retry = 2
    for i in xrange(retry):
        try:
            fp = urllib2.urlopen(url, timeout=1)
            return fp.read()
        except Exception as ex:
            pass
    print >> sys.stderr, "fetch_file_content error, url=%s, msg=%s" %(url, ex)
    return None

def upload_file(file_content):
    if not file_content:
        return
    fn = md5.new(file_content).hexdigest()
    fp = open("webdata/" + fn, "w")
    fp.write(file_content)
    fp.close()
    return
    retry = 2
    for i in xrange(retry):
        try:
            fp = urllib2.urlopen(url="http://127.0.0.1:8000/put", data=file_content, timeout=1)
            return fp.read()
        except Exception as ex:
            pass
    print >> sys.stderr, "upload_file error, url=%s, msg=%s" %(url, ex)
    return None

def successors(url, file_content):
    if not url or not file_content:
        return []

    head, tail = os.path.split(url)

    root = re.findall(r'.*?://[^/]*', url)[0]
    urls = []
    for url in re.findall(r'href="(.*?)"', file_content):
        if url[:4] == "http":
            urls.append(url)
        elif url[:1] == "/":
            urls.append(root + url)
        else:
            if tail.find(".") >= 0:
                urls.append(os.path.join(head, url))
            else:
                urls.append(os.path.join(os.path.join(head, tail), url))

    urls = [u.split("#")[0].split("?")[0] for u in urls if os.path.splitext(u)[1] in ['.htm', '.html', ''] and u[-1] != "/"]
    return urls

if __name__ == "__main__":
    if len(sys.argv) != 3:
        usage()
        exit(0)

    root_path = sys.argv[1]
    url_pattern = sys.argv[2]

    print >> sys.stderr, "root path: %s" % (root_path)
    print >> sys.stderr, "url pattern: %s" % (url_pattern)

    tq = FIFOQueue()
    tq.push(root_path)

    visited = set([root_path])
    while True:
        url = tq.pop()
        print >> sys.stderr, "extracting", url

        if not url: break

        file_content = fetch_file_content(url)
        upload_file(file_content)

        for s in successors(url, file_content):
            if not s in visited and re.match(url_pattern, s):
                visited.add(s)
                tq.push(s)
