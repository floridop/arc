#!/usr/bin/env python

"""
Python client for the HED cache service.
"""

import sys
import httplib
import re

try:
    # Available in python >= 2.5
    import xml.etree.ElementTree as ET
except ImportError:
    try:
        # python 2.4 version
        from elementtree import ElementTree as ET
    except ImportError:
        print "Error importing ElementTree. Please install it or use python 2.5 or greater"
        sys.exit(1)

class CacheException(Exception):
    """
    Exceptions generated by this code.
    """
    pass

def splitURL(url):
    """
    Split url into (protocol, host, port, path) and return this tuple.
    """
    match = re.match('(\w*)://([^/?#:]*):?(\d*)/?(.*)', url)
    if match is None:
        raise CacheException('URL '+url+' is malformed')
    
    port_s = match.group(3)
    if (len(port_s) > 0):
        port = int(port_s)
    else:
        port = None
        
    urltuple = (match.group(1), match.group(2), port, '/'+match.group(4))
    return urltuple
    
def addETElement(root, child, text):
    """
    Utility function to add a child element with text to the root element.
    Returns the child element.
    """
    sub = ET.SubElement(root, child)
    sub.text = text
    return sub
    
def checkSOAPFault(element):
    """
    Checks the response from a service given in element for SOAP faults, and
    raises an exception if there is one.
    """
    response_body = element[0]
    if response_body.find('{http://schemas.xmlsoap.org/soap/envelope/}Fault') is not None:
        fault = response_body.find('{http://schemas.xmlsoap.org/soap/envelope/}Fault')
        faultcode = fault.find('{http://schemas.xmlsoap.org/soap/envelope/}faultcode').text
        faultstring = fault.find('{http://schemas.xmlsoap.org/soap/envelope/}faultstring').text
        raise CacheException('SOAP error: '+faultcode+' - '+faultstring)


def cacheCheck(service, proxy, urls):
    """
    Call the cache service at service to query if the URLs given in the
    list urls exist in the cache. Returns a dictionary of each URL mapped
    to true or false.
    """
    
    (protocol, host, port, path) = splitURL(service)
    
    # create request with etree
    soap = ET.Element('soap-env:Envelope', attrib={'xmlns:echo': 'urn:echo', 'xmlns:soap-enc': 'http://schemas.xmlsoap.org/soap/encoding/', 'xmlns:soap-env': 'http://schemas.xmlsoap.org/soap/envelope/', 'xmlns:xsd': 'http://www.w3.org/2001/XMLSchema', 'xmlns:xsi': 'http://www.w3.org/2001/XMLSchema-instance' })
    
    body = ET.SubElement(soap, 'soap-env:Body')
    cachecheck = ET.SubElement(body, 'CacheCheck')
    files = ET.SubElement(cachecheck, 'TheseFilesNeedToCheck')
    for url in urls:
        addETElement(files, 'FileURL', url)
        
    request = ET.tostring(soap)
        
    conn = httplib.HTTPSConnection(host, port, proxy, proxy)
    try:
        conn.request('POST', path, request)
        resp = conn.getresponse()
    except Exception, e:
        raise CacheException('Error connecting to service at ' + host + ': ' + str(e))
    
    if resp.status != 200:
        conn.close()
        raise CacheException('Error code '+resp.status+' returned: '+resp.reason)

    xmldata = resp.read()
    
    conn.close()
    response = ET.XML(xmldata)
    checkSOAPFault(response)
    
    try:
        cache_result = response.find('{http://schemas.xmlsoap.org/soap/envelope/}Body').find('CacheCheckResponse').find('CacheCheckResult')
        results = cache_result.findall('Result')
    except:
        raise CacheException('Error with XML structure received from echo service')
    
    if len(results) == 0:
        raise CacheException('No results returned')
    
    cachefiles = {}
    for result in results:
        url = result.find('FileURL').text
        incache = False
        if result.find('ExistInTheCache').text == 'true':
            incache = True
        cachefiles[url] = incache 

    return cachefiles

def cacheLink(service, proxy, user, jobid, urls, dostage):
    """
    Call the cache service at service to link the given dictionary of urls
    to the session directory corresponding to jobid. The url dictionary
    maps remote URLs corresponding to the original source files to local
    filenames on the session directory. If dostage is true then any files not
    in the cache will be downloaded from source. The best way to use this
    method may be to call once with dostage=False, and then if there are
    missing files, make a decision whether to call again with doStage=True.
    Alternatively cacheCheck can be called first.
    
    Returns a tuple of (return code, urls) where urls is a dictionary
    representing the state of each requested url.
    
    Possible return codes:
    all successful
    one or more cache files is locked
    permission denied on one or more cache files
    download of one or more cache files failed
    one or more cache files is not present and dostage is false
    cache service errors (failed to connect, not authorised, bad config etc)
    too many downloads already in progress according to configured limit on server
    """
        
    (protocol, host, port, path) = splitURL(service)
    
    # create request with etree
    soap = ET.Element('soap-env:Envelope', attrib={'xmlns:echo': 'urn:echo', 'xmlns:soap-enc': 'http://schemas.xmlsoap.org/soap/encoding/', 'xmlns:soap-env': 'http://schemas.xmlsoap.org/soap/envelope/', 'xmlns:xsd': 'http://www.w3.org/2001/XMLSchema', 'xmlns:xsi': 'http://www.w3.org/2001/XMLSchema-instance' })
    
    body = ET.SubElement(soap, 'soap-env:Body')
    cachecheck = ET.SubElement(body, 'CacheLink')
    files = ET.SubElement(cachecheck, 'TheseFilesNeedToLink')
    for url in urls:
        file = ET.SubElement(files, 'File')
        addETElement(file, 'FileURL', url)
        addETElement(file, 'FileName', urls[url])
    addETElement(cachecheck, 'Username', user)
    addETElement(cachecheck, 'JobID', jobid)
    stage = 'false'
    if dostage:
        stage = 'true'
    addETElement(cachecheck, 'Stage', stage)
        
    request = ET.tostring(soap)
        
    conn = httplib.HTTPSConnection(host, port, proxy, proxy)
    try:
        conn.request('POST', path, request)
        resp = conn.getresponse()
    except Exception, e:
        raise CacheException('Error connecting to service at ' + host + ': ' + str(e))
    
    if resp.status != 200:
        conn.close()
        raise CacheException('Error code '+resp.status+' returned: '+resp.reason)

    xmldata = resp.read()
    
    conn.close()
    response = ET.XML(xmldata)
    checkSOAPFault(response)
    
    try:
        cache_result = response.find('{http://schemas.xmlsoap.org/soap/envelope/}Body').find('CacheLinkResponse').find('CacheLinkResult')
        results = cache_result.findall('Result')
    except:
        raise CacheException('Error with XML structure received from echo service')
    
    if len(results) == 0:
        raise CacheException('No results returned')
    
    cachefiles = {}
    for result in results:
        url = result.find('FileURL').text
        link_result_code = result.find('ReturnCode').text
        link_result_text = result.find('ReturnCodeExplanation').text
        cachefiles[url] = (link_result_code, link_result_text)

    return cachefiles


def echo(service, proxy, say):
    """
    Call the echo service at the given location and print the response to
    say. Useful as a test.
    """
    
    (protocol, host, port, path) = splitURL(service)
    
    # create request with etree
    soap = ET.Element('soap-env:Envelope', attrib={'xmlns:echo': 'urn:echo', 'xmlns:soap-enc': 'http://schemas.xmlsoap.org/soap/encoding/', 'xmlns:soap-env': 'http://schemas.xmlsoap.org/soap/envelope/', 'xmlns:xsd': 'http://www.w3.org/2001/XMLSchema', 'xmlns:xsi': 'http://www.w3.org/2001/XMLSchema-instance' })
    
    body = ET.SubElement(soap, 'soap-env:Body')
    echo = ET.SubElement(body, 'echo:echo')
    addETElement(echo, 'echo:say', 'hello')
    
    request = ET.tostring(soap)
    
    conn = httplib.HTTPSConnection(host, port, proxy, proxy)
    try:
        conn.request('POST', path, request)
    except Exception, e:
        raise CacheException('Error connecting to service at ' + host + ': ' + str(e))
    resp = conn.getresponse()
   
    print(resp.status, resp.reason)
    if resp.status != 200:
        conn.close()
        raise CacheException('Error code '+resp.status+' returned: '+resp.reason)
    
    xmldata = resp.read()
    
    conn.close()
    response = ET.XML(xmldata)
    checkSOAPFault(response)
    
    try:
        answer = response[0][0][0].text
    except:
        raise CacheException('Error with XML structure received from echo service')
    
    print('Echo says ' + answer)

if __name__ == '__main__':
    
    proxy = '/tmp/x509up_u1000'
    #proxy = '/home/cameron/dev/userCerts/userproxy-demo5'
    endpoint = 'https://localhost:60001/cacheservice'
    
    urls_to_check = ['srm://srm.ndgf.org/ops/jens1', 'lfc://lfc1.ndgf.org/:guid=8471134f-494e-41cb-b81e-b341f6a18caf']
    
    try:
        cacheurls = cacheCheck(endpoint, proxy, urls_to_check)
    except CacheException, e:
        print('Error calling cacheCheck: ' + str(e))
        sys.exit(1)
        
    print(cacheurls)
    
    urls_to_link = {'srm://srm.ndgf.org/ops/jens1': 'file1',
                    'lfc://lfc1.ndgf.org/:guid=8471134f-494e-41cb-b81e-b341f6a18caf': 'file3'}
    try:
        cacheurls = cacheLink(endpoint, proxy, 'cameron', '1', urls_to_link, False)
    except CacheException, e:
        print('Error calling cacheLink: ' + str(e))
        sys.exit(1)
    
    print(cacheurls)
